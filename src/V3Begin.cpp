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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Begin.h"

#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3String.h"
#অন্তর্ভাব "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্রেণী BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ধ্রুবক VNUser1InUse m_inuser1;
    যৌক্তিক m_anyFuncInBegin = মিথ্যা;

সর্বজনীন:
    BeginState() = পূর্বনির্ধারিত;
    ~BeginState() = পূর্বনির্ধারিত;
    শূন্য userMarkChanged(Ast_গ্রন্থি* nodep) {
        nodep->user1(সত্য);
        m_anyFuncInBegin = সত্য;
    }
    যৌক্তিক anyFuncInBegin() ধ্রুবক { ফেরত m_anyFuncInBegin; }
};

//######################################################################

শ্রেণী BeginVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ধ্রুবক m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্রন্থি_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_গ্রন্থি* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    পূর্ণসংখ্যা m_ifDepth = 0;  // Current if depth
    যৌক্তিক m_keepBegins = মিথ্যা;  // True if begins should not be inlined

    // METHODS

    string dot(ধ্রুবক string& a, ধ্রুবক string& b) { ফেরত VString::dot(a, "__DOT__", b); }

    শূন্য dotNames(ধ্রুবক std::string& name, File_সারি* ধ্রুবক flp, ধ্রুবক অক্ষর* ধ্রুবক blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        যদি (name == "") ফেরত;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::আকার_type pos;
        যখন ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            ধ্রুবক std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            যদি (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            যদি (!m_ftaskp) {
                AstCellInline* ধ্রুবক inlinep = নতুন AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = Ast_গ্রন্থি::add_পরবর্তী_Null(m_inlinesp, inlinep);
            }
        }
    }

    শূন্য lift_গ্রন্থি(Ast_গ্রন্থি* nodep) {
        nodep->unlinkFrBack();
        যদি (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            যদি (!m_liftedp) {
                m_liftedp = nodep;
            } অন্যথা {
                m_liftedp->add_পরবর্তী(nodep);
            }
        } অন্যথা {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    শূন্য visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        যদি (m_unnamedScope != "") {
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
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        যদি (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* ধ্রুবক initp) {
            initp->unlinkFrBack();
            m_ftaskp->add_Here_This_As_পরবর্তী(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    শূন্য visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_পরবর্তী_Null(nodep->itemsp());
        // Repalce node with body then delete
        যদি (Ast_গ্রন্থি* ধ্রুবক itemsp = nodep->itemsp()) {
            nodep->add_পরবর্তী_Here(itemsp->unlink_Fr_Back_With_পরবর্তী());
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_পরবর্তী_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = সত্য;
            iterate_And_পরবর্তী_Null(nodep->forksp());
        }
        Ast_গ্রন্থি* addsp = nullptr;
        যদি (Ast_গ্রন্থি* ধ্রুবক declsp = nodep->declsp()) {
            declsp->unlink_Fr_Back_With_পরবর্তী();
            addsp = Ast_গ্রন্থি::add_পরবর্তী(addsp, declsp);
        }
        যদি (Ast_গ্রন্থি* ধ্রুবক stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_Back_With_পরবর্তী();
            addsp = Ast_গ্রন্থি::add_পরবর্তী(addsp, stmtsp);
        }
        যদি (addsp) nodep->add_Here_This_As_পরবর্তী(addsp);
        nodep->name("");
    }
    শূন্য visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = মিথ্যা;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        যদি (m_keepBegins) {
            nodep->name("");
            ফেরত;
        }
        Ast_গ্রন্থি* addsp = nullptr;
        যদি (Ast_গ্রন্থি* ধ্রুবক declsp = nodep->declsp()) {
            declsp->unlink_Fr_Back_With_পরবর্তী();
            addsp = Ast_গ্রন্থি::add_পরবর্তী(addsp, declsp);
        }
        যদি (Ast_গ্রন্থি* ধ্রুবক stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_Back_With_পরবর্তী();
            addsp = Ast_গ্রন্থি::add_পরবর্তী(addsp, stmtsp);
        }
        যদি (addsp) {
            nodep->replaceWith(addsp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Block* nodep) override {
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
        যদি (m_keepBegins) {
            nodep->name("");
            ফেরত;
        }
        Ast_গ্রন্থি* addsp = nullptr;
        যদি (Ast_গ্রন্থি* ধ্রুবক declsp = nodep->declsp()) {
            declsp->unlink_Fr_Back_With_পরবর্তী();
            addsp = Ast_গ্রন্থি::add_পরবর্তী(addsp, declsp);
        }
        যদি (Ast_গ্রন্থি* ধ্রুবক stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_Back_With_পরবর্তী();
            addsp = Ast_গ্রন্থি::add_পরবর্তী(addsp, stmtsp);
        }
        যদি (addsp) {
            nodep->replaceWith(addsp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstVar* nodep) override {
        যদি (nodep->user1SetOnce()) { ফেরত; }
        // If static variable, move it outside a function.
        যদি (nodep->lifetime().isStatic() && m_ftaskp) {
            ধ্রুবক std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            যদি (nodep->isIO()) {
                যদি (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ধ্রুবক portp = nodep->clone_বৃক্ষ_Pure(মিথ্যা);
                nodep->replaceWith(portp);

                যদি (nodep->is_প্রবেশ() || nodep->isInout()) {
                    AstAssign* ধ্রুবক initAssignp = নতুন AstAssign{
                        nodep->fileline(), নতুন AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        নতুন AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->add_পরবর্তী_Here(initAssignp);
                }

                যদি (nodep->isWritable()) {
                    AstAssign* ধ্রুবক endAssignp = নতুন AstAssign{
                        nodep->fileline(), নতুন AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        নতুন AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } অন্যথা {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->add_Here_This_As_পরবর্তী(nodep);
            nodep->funcLocal(মিথ্যা);
        } অন্যথা যদি (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_গ্রন্থি(nodep);
        }
    }
    শূন্য visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        যদি (nodep->user1SetOnce()) ফেরত;  // Don't double-add text's
        Ast_গ্রন্থি* wasUnderp = m_ftaskp;
        যদি (!m_ftaskp) wasUnderp = m_procedurep;
        যদি (wasUnderp) {
            যদি (nodep->stmtsp()) {
                Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী()};
                wasUnderp->add_Here_This_As_পরবর্তী(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        } অন্যথা {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    শূন্য visit(AstTypedef* nodep) override {
        যদি (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            lift_গ্রন্থি(nodep);
        }
    }
    শূন্য visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        যদি (m_namedScope != "") {
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
    শূন্য visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        যদি (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        যদি (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        যদি (nodep->user1SetOnce()) ফেরত;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ধ্রুবক std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        যদি (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    শূন্য visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    শূন্য visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = মিথ্যা;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        যদি (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } অন্যথা যদি (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } অন্যথা যদি (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, সত্য);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = মিথ্যা;
        iterateChildren(nodep);
    }

সর্বজনীন:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################

শ্রেণী BeginRelinkVisitor final : সর্বজনীন VNVisitorConst {
    // Replace tasks with new pointer
ব্যক্তিগত:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        যদি (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstVarRef* nodep) override {
        যদি (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        যদি (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Task class functions

শূন্য V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        যদি (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("begin", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

স্থির Ast_গ্রন্থি* createForeachLoop(Ast_গ্রন্থি_Foreach* /*nodep*/, Ast_গ্রন্থি* bodysp, যৌক্তিক arrayMayResize,
                                  Ast_গ্রন্থি_রাশিমালা* subfromp, AstVar* varp, Ast_গ্রন্থি_রাশিমালা* leftp,
                                  Ast_গ্রন্থি_রাশিমালা* rightp, VNType গ্রন্থি_Type) {
    File_সারি* ধ্রুবক fl = varp->fileline();
    Ast_গ্রন্থি_রাশিমালা* varRefp = নতুন AstVarRef{fl, varp, VAccess::READ};
    Ast_গ্রন্থি_রাশিমালা* condp;
    যৌক্তিক inc = সত্য;
    নির্বাচন (গ্রন্থি_Type) {
    ক্ষেত্র VNType::LteS: condp = নতুন AstLteS{fl, varRefp, rightp}; বিরতি;
    ক্ষেত্র VNType::Lt: condp = নতুন AstLt{fl, varRefp, rightp}; বিরতি;
    ক্ষেত্র VNType::GteS:
        condp = নতুন AstGteS{fl, varRefp, rightp};
        inc = মিথ্যা;
        বিরতি;
    পূর্বনির্ধারিত: varp->v3fatalSrc("Missing comparison handling"); বিরতি;
    }
    Ast_গ্রন্থি_রাশিমালা* incp;
    যদি (inc)
        incp = নতুন AstAdd{fl, varRefp->clone_বৃক্ষ(মিথ্যা), নতুন AstConst{fl, 1}};
    অন্যথা
        incp = নতুন AstSub{fl, varRefp->clone_বৃক্ষ(মিথ্যা), নতুন AstConst{fl, 1}};

    AstLoop* ধ্রুবক loopp = নতুন AstLoop{fl};
    loopp->addStmtsp(নতুন AstLoopTest{fl, loopp, condp});
    AstVar* আকার_Varp = nullptr;
    Ast_গ্রন্থি_রাশিমালা* আকার_Getp = nullptr;
    যদি (arrayMayResize) {
        আকার_Varp = নতুন AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        আকার_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        আকার_Varp->usedLoopIdx(সত্য);  // Not technically an index, but used only inside loop
        varp->add_পরবর্তী(আকার_Varp);
        আকার_Getp
            = নতুন AstCMethodHard{fl, subfromp->clone_বৃক্ষ_Pure(মিথ্যা), VCMethod::DYN_আকার, nullptr};
        আকার_Getp->dtypeSetUInt32();
        loopp->addStmtsp(নতুন AstAssign{fl, নতুন AstVarRef{fl, আকার_Varp, VAccess::WRITE}, আকার_Getp});
    }
    loopp->addStmtsp(bodysp);
    Ast_গ্রন্থি_Stmt* incStmtp = নতুন AstAssign{fl, নতুন AstVarRef{fl, varp, VAccess::WRITE}, incp};
    যদি (arrayMayResize) {
        incStmtp = নতুন AstIf{fl,
                             নতুন AstLte{fl, নতুন AstVarRef{fl, আকার_Varp, VAccess::READ},
                                        আকার_Getp->clone_বৃক্ষ_Pure(মিথ্যা)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    Ast_গ্রন্থি* ধ্রুবক stmtsp = varp;  // New statements for outer loop
    stmtsp->add_পরবর্তী(নতুন AstAssign{fl, নতুন AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->add_পরবর্তী(loopp);
    ফেরত stmtsp;
}
স্থির Ast_গ্রন্থি* createForeachLoopRanged(Ast_গ্রন্থি_Foreach* nodep, Ast_গ্রন্থি* bodysp,
                                        Ast_গ্রন্থি_রাশিমালা* subfromp, AstVar* varp,
                                        ধ্রুবক VNumRange& declRange) {
    File_সারি* ধ্রুবক fl = varp->fileline();
    V3_সংখ্যা বাম{nodep, 32};
    V3_সংখ্যা ডান{nodep, 32};
    বাম.isSigned(সত্য);
    ডান.isSigned(সত্য);
    বাম.setLongS(declRange.বাম());
    ডান.setLongS(declRange.ডান());
    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক leftp = নতুন AstConst{fl, বাম};
    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rightp = নতুন AstConst{fl, ডান};
    ফেরত createForeachLoop(nodep, bodysp, মিথ্যা /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.বাম() <= declRange.ডান() ? VNType::LteS : VNType::GteS);
}
স্থির Ast_গ্রন্থি* createForeachAssoc(File_সারি* fl, AstVar* varp, Ast_গ্রন্থি_রাশিমালা* subfromp,
                                   Ast_গ্রন্থি_DType* fromDtp, Ast_গ্রন্থি* bodyPointp) {
    Ast_গ্রন্থি* loopp = varp;
    AstVar* ধ্রুবক পরবর্তী_varp  // Iterator containing next element (to handle mid-array delete)
        = নতুন AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    পরবর্তী_varp->usedLoopIdx(সত্য);
    পরবর্তী_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->add_পরবর্তী(পরবর্তী_varp);

    AstVar* ধ্রুবক more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = নতুন AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(সত্য);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->add_পরবর্তী(more_varp);

    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক firstp
        = নতুন AstCMethodHard{fl, subfromp->clone_বৃক্ষ_Pure(মিথ্যা), VCMethod::ASSOC_FIRST,
                             নতুন AstVarRef{fl, পরবর্তী_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nextp
        = নতুন AstCMethodHard{fl, subfromp->clone_বৃক্ষ_Pure(মিথ্যা), VCMethod::ASSOC_পরবর্তী,
                             নতুন AstVarRef{fl, পরবর্তী_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->add_পরবর্তী(নতুন AstAssign{fl, নতুন AstVarRef{fl, more_varp, VAccess::WRITE},
                                 নতুন AstNeq{fl, নতুন AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ধ্রুবক lp = নতুন AstLoop{fl};
    loopp->add_পরবর্তী(lp);
    lp->addStmtsp(নতুন AstLoopTest{fl, lp, নতুন AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(নতুন AstAssign{fl, নতুন AstVarRef{fl, varp, VAccess::WRITE},
                                নতুন AstVarRef{fl, পরবর্তী_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(নতুন AstAssign{fl, নতুন AstVarRef{fl, more_varp, VAccess::WRITE},
                                নতুন AstNeq{fl, নতুন AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    ফেরত loopp;
}

স্থির যৌক্তিক arrayMayResizeCheck(Ast_গ্রন্থি* nodesp, AstVar* fromVarp) {
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
    যদি (!nodesp) ফেরত মিথ্যা;
    ফেরত nodesp->exists_And_পরবর্তী([&](ধ্রুবক Ast_গ্রন্থি* nodep) -> যৌক্তিক {
        যদি (ধ্রুবক Ast_গ্রন্থি_Var_Ref* varrefp = VN_CAST(nodep, গ্রন্থি_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            যদি (varrefp->access().isWriteOrRW()) {
                যদি (!fromVarp) ফেরত সত্য;
                যদি (varrefp->varp() == fromVarp) ফেরত সত্য;
            }
        }
        // No idea what a function might be doing
        যদি (VN_IS(nodep, MethodCall) || VN_IS(nodep, গ্রন্থি_FTask_Ref)) ফেরত সত্য;
        ফেরত মিথ্যা;
    });
}

Ast_গ্রন্থি* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ধ্রুবক AstForeachHeader* ধ্রুবক headerp = nodep->headerp();
    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ধ্রুবক fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    Ast_গ্রন্থি_DType* fromDtp = fromp->dtypep()->skipRefp();
    ধ্রুবক যৌক্তিক arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_গ্রন্থি* bodyPointp = নতুন AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, মিথ্যা};
    Ast_গ্রন্থি* newp = nullptr;
    Ast_গ্রন্থি* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_গ্রন্থি_রাশিমালা* subfromp = fromp->clone_বৃক্ষ_Pure(মিথ্যা);
    // Major dimension first
    জন্য (Ast_গ্রন্থি *argsp = headerp->elementsp(), *পরবর্তী_argsp; argsp; argsp = পরবর্তী_argsp) {
        পরবর্তী_argsp = argsp->nextp();
        ধ্রুবক যৌক্তিক empty = VN_IS(argsp, Empty);
        AstVar* ধ্রুবক varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        যদি (varp) varp->unlinkFrBack()->usedLoopIdx(সত্য);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_সারি* ধ্রুবক fl = argsp->fileline();
        যদি (varp) {
            Ast_গ্রন্থি* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            যদি (ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক adtypep = VN_CAST(fromDtp, গ্রন্থি_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } অন্যথা যদি (ধ্রুবক AstBasicDType* ধ্রুবক adtypep = VN_CAST(fromDtp, BasicDType)) {
                যদি (adtypep->isString()) {
                    AstConst* ধ্রুবক leftp = নতুন AstConst{fl, 0};
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rightp = নতুন AstLenN{fl, fromp->clone_বৃক্ষ_Pure(মিথ্যা)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } অন্যথা {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } অন্যথা যদি (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* ধ্রুবক leftp = নতুন AstConst{fl, 0};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rightp = নতুন AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), গ্রন্থি_Array_DType)
                        ? নতুন AstArraySel{fl, subfromp->clone_বৃক্ষ_Pure(মিথ্যা),
                                          নতুন AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_বৃক্ষ_Pure(মিথ্যা),
                    VCMethod::DYN_আকার};
                AstVarRef* varRefp = নতুন AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(মিথ্যা);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = নতুন AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } অন্যথা যদি (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = নতুন AstVarRef{fl, varp, VAccess::READ};
                subfromp = নতুন AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            যদি (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_বৃক্ষ(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_গ্রন্থি* ধ্রুবক bodyp = nodep->bodyp();
    যদি (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        যদি (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_বৃক্ষ(), bodyPointp);
        ফেরত nullptr;
    }
    যদি (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_Back_With_পরবর্তী());
    } অন্যথা {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->delete_বৃক্ষ(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    ফেরত newp;
}
