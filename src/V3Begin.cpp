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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Begin.h"

#شامل "V3Stats.h"
#شامل "V3String.h"
#شامل "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

رده BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ثابت VNUser1InUse m_inuser1;
    منطقی m_anyFuncInBegin = نادرست;

عمومی:
    BeginState() = پیشفرض;
    ~BeginState() = پیشفرض;
    تهی userMarkChanged(Ast_گره* nodep) {
        nodep->user1(درست);
        m_anyFuncInBegin = درست;
    }
    منطقی anyFuncInBegin() ثابت { بازگشت m_anyFuncInBegin; }
};

//######################################################################

رده BeginVisitor final : عمومی VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ثابت m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گره_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گره_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_گره* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    صحیح m_ifDepth = 0;  // Current if depth
    منطقی m_keepBegins = نادرست;  // True if begins should not be inlined

    // METHODS

    string dot(ثابت string& a, ثابت string& b) { بازگشت VString::dot(a, "__DOT__", b); }

    تهی dotNames(ثابت std::string& name, File_سطر* ثابت flp, ثابت نویسه* ثابت blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        اگر (name == "") بازگشت;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        وقتی ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            ثابت std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            اگر (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            اگر (!m_ftaskp) {
                AstCellInline* ثابت inlinep = جدید AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_گره::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    تهی lift_گره(Ast_گره* nodep) {
        nodep->unlinkFrBack();
        اگر (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            اگر (!m_liftedp) {
                m_liftedp = nodep;
            } وگرنه {
                m_liftedp->addNext(nodep);
            }
        } وگرنه {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    تهی visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = درست;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        اگر (m_unnamedScope != "") {
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
    تهی visit(Ast_گره_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        اگر (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* ثابت initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    تهی visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        اگر (Ast_گره* ثابت itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = درست;
            iterateAndNextNull(nodep->forksp());
        }
        Ast_گره* addsp = nullptr;
        اگر (Ast_گره* ثابت declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_گره::addNext(addsp, declsp);
        }
        اگر (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_گره::addNext(addsp, stmtsp);
        }
        اگر (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    تهی visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = نادرست;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        اگر (m_keepBegins) {
            nodep->name("");
            بازگشت;
        }
        Ast_گره* addsp = nullptr;
        اگر (Ast_گره* ثابت declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_گره::addNext(addsp, declsp);
        }
        اگر (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_گره::addNext(addsp, stmtsp);
        }
        اگر (addsp) {
            nodep->replaceWith(addsp);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(Ast_گره_Block* nodep) override {
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
        اگر (m_keepBegins) {
            nodep->name("");
            بازگشت;
        }
        Ast_گره* addsp = nullptr;
        اگر (Ast_گره* ثابت declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = Ast_گره::addNext(addsp, declsp);
        }
        اگر (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = Ast_گره::addNext(addsp, stmtsp);
        }
        اگر (addsp) {
            nodep->replaceWith(addsp);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstVar* nodep) override {
        اگر (nodep->user1SetOnce()) { بازگشت; }
        // If static variable, move it outside a function.
        اگر (nodep->lifetime().isStatic() && m_ftaskp) {
            ثابت std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            اگر (nodep->isIO()) {
                اگر (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ثابت portp = nodep->clone_درخت_Pure(نادرست);
                nodep->replaceWith(portp);

                اگر (nodep->is_درون_داد() || nodep->isInout()) {
                    AstAssign* ثابت initAssignp = جدید AstAssign{
                        nodep->fileline(), جدید AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        جدید AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                اگر (nodep->isWritable()) {
                    AstAssign* ثابت endAssignp = جدید AstAssign{
                        nodep->fileline(), جدید AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        جدید AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } وگرنه {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(نادرست);
        } وگرنه اگر (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_گره(nodep);
        }
    }
    تهی visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        اگر (nodep->user1SetOnce()) بازگشت;  // Don't double-add text's
        Ast_گره* wasUnderp = m_ftaskp;
        اگر (!m_ftaskp) wasUnderp = m_procedurep;
        اگر (wasUnderp) {
            اگر (nodep->stmtsp()) {
                Ast_گره* ثابت newp = جدید AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        } وگرنه {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    تهی visit(AstTypedef* nodep) override {
        اگر (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_گره(nodep);
        }
    }
    تهی visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        اگر (m_namedScope != "") {
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
    تهی visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        اگر (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        اگر (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    تهی visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        اگر (nodep->user1SetOnce()) بازگشت;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ثابت std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        اگر (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    تهی visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    تهی visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = نادرست;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        اگر (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } وگرنه اگر (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } وگرنه اگر (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, درست);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = نادرست;
        iterateChildren(nodep);
    }

عمومی:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = پیشفرض;
};

//######################################################################

رده BeginRelinkVisitor final : عمومی VNVisitorConst {
    // Replace tasks with new pointer
خصوصی:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        اگر (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    تهی visit(AstVarRef* nodep) override {
        اگر (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    تهی visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        اگر (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = پیشفرض;
};

//######################################################################
// Task class functions

تهی V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        اگر (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("begin", 0, dump_درخت_Either_Level() >= 3);
}

ایستا Ast_گره* createForeachLoop(Ast_گره_Foreach* /*nodep*/, Ast_گره* bodysp, منطقی arrayMayResize,
                                  Ast_گره_عبارت* subfromp, AstVar* varp, Ast_گره_عبارت* leftp,
                                  Ast_گره_عبارت* rightp, VNType گره_Type) {
    File_سطر* ثابت fl = varp->fileline();
    Ast_گره_عبارت* varRefp = جدید AstVarRef{fl, varp, VAccess::READ};
    Ast_گره_عبارت* condp;
    منطقی inc = درست;
    انتخاب (گره_Type) {
    حالت VNType::LteS: condp = جدید AstLteS{fl, varRefp, rightp}; بشکن;
    حالت VNType::Lt: condp = جدید AstLt{fl, varRefp, rightp}; بشکن;
    حالت VNType::GteS:
        condp = جدید AstGteS{fl, varRefp, rightp};
        inc = نادرست;
        بشکن;
    پیشفرض: varp->v3fatalSrc("Missing comparison handling"); بشکن;
    }
    Ast_گره_عبارت* incp;
    اگر (inc)
        incp = جدید AstAdd{fl, varRefp->clone_درخت(نادرست), جدید AstConst{fl, 1}};
    وگرنه
        incp = جدید AstSub{fl, varRefp->clone_درخت(نادرست), جدید AstConst{fl, 1}};

    AstLoop* ثابت loopp = جدید AstLoop{fl};
    loopp->addStmtsp(جدید AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    Ast_گره_عبارت* sizeGetp = nullptr;
    اگر (arrayMayResize) {
        sizeVarp = جدید AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(درست);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = جدید AstCMethodHard{fl, subfromp->clone_درخت_Pure(نادرست), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(جدید AstAssign{fl, جدید AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    Ast_گره_Stmt* incStmtp = جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE}, incp};
    اگر (arrayMayResize) {
        incStmtp = جدید AstIf{fl,
                             جدید AstLte{fl, جدید AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->clone_درخت_Pure(نادرست)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_گره* ثابت stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    بازگشت stmtsp;
}
ایستا Ast_گره* createForeachLoopRanged(Ast_گره_Foreach* nodep, Ast_گره* bodysp,
                                        Ast_گره_عبارت* subfromp, AstVar* varp,
                                        ثابت VNumRange& declRange) {
    File_سطر* ثابت fl = varp->fileline();
    V3_عدد چپ{nodep, 32};
    V3_عدد راست{nodep, 32};
    چپ.isSigned(درست);
    راست.isSigned(درست);
    چپ.setLongS(declRange.چپ());
    راست.setLongS(declRange.راست());
    Ast_گره_عبارت* ثابت leftp = جدید AstConst{fl, چپ};
    Ast_گره_عبارت* ثابت rightp = جدید AstConst{fl, راست};
    بازگشت createForeachLoop(nodep, bodysp, نادرست /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.چپ() <= declRange.راست() ? VNType::LteS : VNType::GteS);
}
ایستا Ast_گره* createForeachAssoc(File_سطر* fl, AstVar* varp, Ast_گره_عبارت* subfromp,
                                   Ast_گره_DType* fromDtp, Ast_گره* bodyPointp) {
    Ast_گره* loopp = varp;
    AstVar* ثابت next_varp  // Iterator containing next element (to handle mid-array delete)
        = جدید AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(درست);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* ثابت more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = جدید AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(درست);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    Ast_گره_عبارت* ثابت firstp
        = جدید AstCMethodHard{fl, subfromp->clone_درخت_Pure(نادرست), VCMethod::ASSOC_FIRST,
                             جدید AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_گره_عبارت* ثابت nextp
        = جدید AstCMethodHard{fl, subfromp->clone_درخت_Pure(نادرست), VCMethod::ASSOC_NEXT,
                             جدید AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(جدید AstAssign{fl, جدید AstVarRef{fl, more_varp, VAccess::WRITE},
                                 جدید AstNeq{fl, جدید AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ثابت lp = جدید AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(جدید AstLoopTest{fl, lp, جدید AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                                جدید AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(جدید AstAssign{fl, جدید AstVarRef{fl, more_varp, VAccess::WRITE},
                                جدید AstNeq{fl, جدید AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    بازگشت loopp;
}

ایستا منطقی arrayMayResizeCheck(Ast_گره* nodesp, AstVar* fromVarp) {
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
    اگر (!nodesp) بازگشت نادرست;
    بازگشت nodesp->existsAndNext([&](ثابت Ast_گره* nodep) -> منطقی {
        اگر (ثابت Ast_گره_Var_Ref* varrefp = VN_CAST(nodep, گره_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            اگر (varrefp->access().isWriteOrRW()) {
                اگر (!fromVarp) بازگشت درست;
                اگر (varrefp->varp() == fromVarp) بازگشت درست;
            }
        }
        // No idea what a function might be doing
        اگر (VN_IS(nodep, MethodCall) || VN_IS(nodep, گره_FTask_Ref)) بازگشت درست;
        بازگشت نادرست;
    });
}

Ast_گره* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ثابت AstForeachHeader* ثابت headerp = nodep->headerp();
    Ast_گره_عبارت* ثابت fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ثابت fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_گره_DType* fromDtp = fromp->dtypep()->skipRefp();
    ثابت منطقی arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_گره* bodyPointp = جدید AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, نادرست};
    Ast_گره* newp = nullptr;
    Ast_گره* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_گره_عبارت* subfromp = fromp->clone_درخت_Pure(نادرست);
    // Major dimension first
    برای (Ast_گره *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        ثابت منطقی empty = VN_IS(argsp, Empty);
        AstVar* ثابت varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        اگر (varp) varp->unlinkFrBack()->usedLoopIdx(درست);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_سطر* ثابت fl = argsp->fileline();
        اگر (varp) {
            Ast_گره* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            اگر (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(fromDtp, گره_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } وگرنه اگر (ثابت AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                اگر (adtypep->isString()) {
                    AstConst* ثابت leftp = جدید AstConst{fl, 0};
                    Ast_گره_عبارت* ثابت rightp = جدید AstLenN{fl, fromp->clone_درخت_Pure(نادرست)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } وگرنه {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } وگرنه اگر (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* ثابت leftp = جدید AstConst{fl, 0};
                Ast_گره_عبارت* ثابت rightp = جدید AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), گره_Array_DType)
                        ? جدید AstArraySel{fl, subfromp->clone_درخت_Pure(نادرست),
                                          جدید AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_درخت_Pure(نادرست),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = جدید AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(نادرست);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = جدید AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } وگرنه اگر (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = جدید AstVarRef{fl, varp, VAccess::READ};
                subfromp = جدید AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            اگر (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_درخت(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_گره* ثابت bodyp = nodep->bodyp();
    اگر (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        اگر (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_درخت(), bodyPointp);
        بازگشت nullptr;
    }
    اگر (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } وگرنه {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_درخت(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    بازگشت newp;
}
