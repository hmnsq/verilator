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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Begin.h"

#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3String.h"
#அந்தர்பாவ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    மாறிலி VNUser1InUse m_inuser1;
    தருக்க m_anyFuncInBegin = பொய்;

பொது:
    BeginState() = இயல்பு;
    ~BeginState() = இயல்பு;
    வெறுமை userMarkChanged(Ast_முனை* nodep) {
        nodep->user1(உண்மை);
        m_anyFuncInBegin = உண்மை;
    }
    தருக்க anyFuncInBegin() மாறிலி { திரும்பு m_anyFuncInBegin; }
};

//######################################################################

வகுப்பு BeginVisitor final : பொது VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* மாறிலி m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_முனை_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_முனை* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    முழு m_ifDepth = 0;  // Current if depth
    தருக்க m_keepBegins = பொய்;  // True if begins should not be inlined

    // METHODS

    string dot(மாறிலி string& a, மாறிலி string& b) { திரும்பு VString::dot(a, "__DOT__", b); }

    வெறுமை dotNames(மாறிலி std::string& name, File_வரி* மாறிலி flp, மாறிலி எழுத்து* மாறிலி blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        எனில் (name == "") திரும்பு;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        போது ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            மாறிலி std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            எனில் (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            எனில் (!m_ftaskp) {
                AstCellInline* மாறிலி inlinep = புதிய AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_முனை::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    வெறுமை lift_முனை(Ast_முனை* nodep) {
        nodep->unlinkFrBack();
        எனில் (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            எனில் (!m_liftedp) {
                m_liftedp = nodep;
            } இல்லையெனில் {
                m_liftedp->addNext(nodep);
            }
        } இல்லையெனில் {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    வெறுமை visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        எனில் (m_unnamedScope != "") {
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
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        எனில் (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* மாறிலி initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    வெறுமை visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        எனில் (Ast_முனை* மாறிலி itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = உண்மை;
            iterateAndNextNull(nodep->forksp());
        }
        Ast_முனை* addsp = nullptr;
        எனில் (Ast_முனை* மாறிலி declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_முனை::addNext(addsp, declsp);
        }
        எனில் (Ast_முனை* மாறிலி stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_முனை::addNext(addsp, stmtsp);
        }
        எனில் (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    வெறுமை visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = பொய்;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        எனில் (m_keepBegins) {
            nodep->name("");
            திரும்பு;
        }
        Ast_முனை* addsp = nullptr;
        எனில் (Ast_முனை* மாறிலி declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_முனை::addNext(addsp, declsp);
        }
        எனில் (Ast_முனை* மாறிலி stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_முனை::addNext(addsp, stmtsp);
        }
        எனில் (addsp) {
            nodep->replaceWith(addsp);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(Ast_முனை_Block* nodep) override {
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
        எனில் (m_keepBegins) {
            nodep->name("");
            திரும்பு;
        }
        Ast_முனை* addsp = nullptr;
        எனில் (Ast_முனை* மாறிலி declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_முனை::addNext(addsp, declsp);
        }
        எனில் (Ast_முனை* மாறிலி stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_முனை::addNext(addsp, stmtsp);
        }
        எனில் (addsp) {
            nodep->replaceWith(addsp);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstVar* nodep) override {
        எனில் (nodep->user1SetOnce()) { திரும்பு; }
        // If static variable, move it outside a function.
        எனில் (nodep->lifetime().isStatic() && m_ftaskp) {
            மாறிலி std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            எனில் (nodep->isIO()) {
                எனில் (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* மாறிலி portp = nodep->clone_மரம்_Pure(பொய்);
                nodep->replaceWith(portp);

                எனில் (nodep->is_உள்ளீடு() || nodep->isInout()) {
                    AstAssign* மாறிலி initAssignp = புதிய AstAssign{
                        nodep->fileline(), புதிய AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        புதிய AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                எனில் (nodep->isWritable()) {
                    AstAssign* மாறிலி endAssignp = புதிய AstAssign{
                        nodep->fileline(), புதிய AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        புதிய AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } இல்லையெனில் {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(பொய்);
        } இல்லையெனில் எனில் (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_முனை(nodep);
        }
    }
    வெறுமை visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Don't double-add text's
        Ast_முனை* wasUnderp = m_ftaskp;
        எனில் (!m_ftaskp) wasUnderp = m_procedurep;
        எனில் (wasUnderp) {
            எனில் (nodep->stmtsp()) {
                Ast_முனை* மாறிலி newp = புதிய AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        } இல்லையெனில் {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    வெறுமை visit(AstTypedef* nodep) override {
        எனில் (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_முனை(nodep);
        }
    }
    வெறுமை visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        எனில் (m_namedScope != "") {
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
    வெறுமை visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        எனில் (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        எனில் (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        மாறிலி std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        எனில் (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    வெறுமை visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = பொய்;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        எனில் (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } இல்லையெனில் எனில் (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } இல்லையெனில் எனில் (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, உண்மை);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = பொய்;
        iterateChildren(nodep);
    }

பொது:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = இயல்பு;
};

//######################################################################

வகுப்பு BeginRelinkVisitor final : பொது VNVisitorConst {
    // Replace tasks with new pointer
தனிப்பட்ட:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        எனில் (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        எனில் (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = இயல்பு;
};

//######################################################################
// Task class functions

வெறுமை V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        எனில் (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("begin", 0, dump_மரம்_Either_Level() >= 3);
}

நிலையான Ast_முனை* createForeachLoop(Ast_முனை_Foreach* /*nodep*/, Ast_முனை* bodysp, தருக்க arrayMayResize,
                                  Ast_முனை_கோவை* subfromp, AstVar* varp, Ast_முனை_கோவை* leftp,
                                  Ast_முனை_கோவை* rightp, VNType முனை_Type) {
    File_வரி* மாறிலி fl = varp->fileline();
    Ast_முனை_கோவை* varRefp = புதிய AstVarRef{fl, varp, VAccess::READ};
    Ast_முனை_கோவை* condp;
    தருக்க inc = உண்மை;
    தேர்வு (முனை_Type) {
    நிலை VNType::LteS: condp = புதிய AstLteS{fl, varRefp, rightp}; நிறுத்து;
    நிலை VNType::Lt: condp = புதிய AstLt{fl, varRefp, rightp}; நிறுத்து;
    நிலை VNType::GteS:
        condp = புதிய AstGteS{fl, varRefp, rightp};
        inc = பொய்;
        நிறுத்து;
    இயல்பு: varp->v3fatalSrc("Missing comparison handling"); நிறுத்து;
    }
    Ast_முனை_கோவை* incp;
    எனில் (inc)
        incp = புதிய AstAdd{fl, varRefp->clone_மரம்(பொய்), புதிய AstConst{fl, 1}};
    இல்லையெனில்
        incp = புதிய AstSub{fl, varRefp->clone_மரம்(பொய்), புதிய AstConst{fl, 1}};

    AstLoop* மாறிலி loopp = புதிய AstLoop{fl};
    loopp->addStmtsp(புதிய AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_முனை_கோவை* sizeGetp = nullptr;
    எனில் (arrayMayResize) {
        sizeVarp = புதிய AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(உண்மை);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = புதிய AstCMethodHard{fl, subfromp->clone_மரம்_Pure(பொய்), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(புதிய AstAssign{fl, புதிய AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_முனை_Stmt* incStmtp = புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE}, incp};
    எனில் (arrayMayResize) {
        incStmtp = புதிய AstIf{fl,
                             புதிய AstLte{fl, புதிய AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_மரம்_Pure(பொய்)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_முனை* மாறிலி stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    திரும்பு stmtsp;
}
நிலையான Ast_முனை* createForeachLoopRanged(Ast_முனை_Foreach* nodep, Ast_முனை* bodysp,
                                        Ast_முனை_கோவை* subfromp, AstVar* varp,
                                        மாறிலி VNumRange& declRange) {
    File_வரி* மாறிலி fl = varp->fileline();
    V3_எண் இடது{nodep, 32};
    V3_எண் வலது{nodep, 32};
    இடது.isSigned(உண்மை);
    வலது.isSigned(உண்மை);
    இடது.setLongS(declRange.இடது());
    வலது.setLongS(declRange.வலது());
    Ast_முனை_கோவை* மாறிலி leftp = புதிய AstConst{fl, இடது};
    Ast_முனை_கோவை* மாறிலி rightp = புதிய AstConst{fl, வலது};
    திரும்பு createForeachLoop(nodep, bodysp, பொய் /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.இடது() <= declRange.வலது() ? VNType::LteS : VNType::GteS);
}
நிலையான Ast_முனை* createForeachAssoc(File_வரி* fl, AstVar* varp, Ast_முனை_கோவை* subfromp,
                                   Ast_முனை_DType* fromDtp, Ast_முனை* bodyPointp) {
    Ast_முனை* loopp = varp;
    AstVar* மாறிலி next_varp  // Iterator containing next element (to handle mid-array delete)
        = புதிய AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(உண்மை);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* மாறிலி more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = புதிய AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(உண்மை);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_முனை_கோவை* மாறிலி firstp
        = புதிய AstCMethodHard{fl, subfromp->clone_மரம்_Pure(பொய்), VCMethod::ASSOC_FIRST,
                             புதிய AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_முனை_கோவை* மாறிலி nextp
        = புதிய AstCMethodHard{fl, subfromp->clone_மரம்_Pure(பொய்), VCMethod::ASSOC_NEXT,
                             புதிய AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(புதிய AstAssign{fl, புதிய AstVarRef{fl, more_varp, VAccess::WRITE},
                                 புதிய AstNeq{fl, புதிய AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* மாறிலி lp = புதிய AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(புதிய AstLoopTest{fl, lp, புதிய AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                                புதிய AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(புதிய AstAssign{fl, புதிய AstVarRef{fl, more_varp, VAccess::WRITE},
                                புதிய AstNeq{fl, புதிய AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    திரும்பு loopp;
}

நிலையான தருக்க arrayMayResizeCheck(Ast_முனை* nodesp, AstVar* fromVarp) {
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
    எனில் (!nodesp) திரும்பு பொய்;
    திரும்பு nodesp->existsAndNext([&](மாறிலி Ast_முனை* nodep) -> தருக்க {
        எனில் (மாறிலி Ast_முனை_Var_Ref* varrefp = VN_CAST(nodep, முனை_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            எனில் (varrefp->access().isWriteOrRW()) {
                எனில் (!fromVarp) திரும்பு உண்மை;
                எனில் (varrefp->varp() == fromVarp) திரும்பு உண்மை;
            }
        }
        // No idea what a function might be doing
        எனில் (VN_IS(nodep, MethodCall) || VN_IS(nodep, முனை_FTask_Ref)) திரும்பு உண்மை;
        திரும்பு பொய்;
    });
}

Ast_முனை* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    மாறிலி AstForeachHeader* மாறிலி headerp = nodep->headerp();
    Ast_முனை_கோவை* மாறிலி fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* மாறிலி fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_முனை_DType* fromDtp = fromp->dtypep()->skipRefp();
    மாறிலி தருக்க arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_முனை* bodyPointp = புதிய AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, பொய்};
    Ast_முனை* newp = nullptr;
    Ast_முனை* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_முனை_கோவை* subfromp = fromp->clone_மரம்_Pure(பொய்);
    // Major dimension first
    க்கு (Ast_முனை *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        மாறிலி தருக்க empty = VN_IS(argsp, Empty);
        AstVar* மாறிலி varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        எனில் (varp) varp->unlinkFrBack()->usedLoopIdx(உண்மை);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_வரி* மாறிலி fl = argsp->fileline();
        எனில் (varp) {
            Ast_முனை* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            எனில் (மாறிலி Ast_முனை_Array_DType* மாறிலி adtypep = VN_CAST(fromDtp, முனை_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } இல்லையெனில் எனில் (மாறிலி AstBasicDType* மாறிலி adtypep = VN_CAST(fromDtp, BasicDType)) {
                எனில் (adtypep->isString()) {
                    AstConst* மாறிலி leftp = புதிய AstConst{fl, 0};
                    Ast_முனை_கோவை* மாறிலி rightp = புதிய AstLenN{fl, fromp->clone_மரம்_Pure(பொய்)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } இல்லையெனில் {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } இல்லையெனில் எனில் (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* மாறிலி leftp = புதிய AstConst{fl, 0};
                Ast_முனை_கோவை* மாறிலி rightp = புதிய AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), முனை_Array_DType)
                        ? புதிய AstArraySel{fl, subfromp->clone_மரம்_Pure(பொய்),
                                          புதிய AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_மரம்_Pure(பொய்),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = புதிய AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(பொய்);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = புதிய AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } இல்லையெனில் எனில் (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = புதிய AstVarRef{fl, varp, VAccess::READ};
                subfromp = புதிய AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            எனில் (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_மரம்(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_முனை* மாறிலி bodyp = nodep->bodyp();
    எனில் (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        எனில் (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_மரம்(), bodyPointp);
        திரும்பு nullptr;
    }
    எனில் (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } இல்லையெனில் {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_மரம்(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    திரும்பு newp;
}
