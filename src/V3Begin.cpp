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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Begin.h"

#ضمن "V3Stats.h"
#ضمن "V3String.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ثابت VNUser1_دننه_Use m_inuser1;
    منطقي m_any_Func_دننه_Begin = خطأ;

عام:
    BeginState() = افتراضي;
    ~BeginState() = افتراضي;
    فراغ user_Mark_بدل_شوی(Ast_غوټه* nodep) {
        nodep->user1(ریښتیا);
        m_any_Func_دننه_Begin = ریښتیا;
    }
    منطقي any_Func_دننه_Begin() ثابت { أرجع m_any_Func_دننه_Begin; }
};

//######################################################################

صنف BeginVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ثابت m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    AstCellInline* m_inlinesp = تش_اشاره;  // AstCellInlines to add to m_modp
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    Ast_غوټه_Procedure* m_procedurep = تش_اشاره;  // Current procedure
    Ast_غوټه* m_liftedp = تش_اشاره;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    صحيح m_if_ژوروالی = 0;  // Current if depth
    منطقي m_keepBegins = خطأ;  // True if begins should not be inlined

    // METHODS

    string dot(ثابت string& a, ثابت string& b) { أرجع VString::dot(a, "__DOT__", b); }

    فراغ dot_نومونه(ثابت std::string& name, File_کرښه* ثابت flp, ثابت حرف* ثابت بلاک_Name) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        إذا (name == "") أرجع;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::اندازه_type pos;
        طالما ((pos = dottedname.پیدا_کړه("__DOT__")) != std::string::npos) {
            ثابت std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::مزي_اوږدوالی("__DOT__"));
            إذا (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            إذا (!m_ftaskp) {
                AstCellInline* ثابت inlinep = جديد AstCellInline{flp, m_unnamedScope, بلاک_Name};
                m_inlinesp = Ast_غوټه::جمع_کړه_بل_Null(m_inlinesp, inlinep);
            }
        }
    }

    فراغ lift_غوټه(Ast_غوټه* nodep) {
        nodep->unlink_Fr_شا();
        إذا (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            إذا (!m_liftedp) {
                m_liftedp = nodep;
            } وإلا {
                m_liftedp->جمع_کړه_بل(nodep);
            }
        } وإلا {
            // Move to module
            m_modp->جمع_کړه_Stmtsp(nodep);
        }
    }

    // VISITORS
    فراغ لیدنه(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convert_منزل_While(nodep), nodep);
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        إذا (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_بدل_شوی(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = تش_اشاره;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        إذا (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_بدل_شوی(nodep);
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
        m_liftedp = تش_اشاره;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ثابت initp) {
            initp->unlink_Fr_شا();
            m_ftaskp->جمع_کړه_دلته_This_As_بل(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dot_نومونه(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_بل_Null(nodep->itemsp());
        // Repalce node with body then delete
        إذا (Ast_غوټه* ثابت itemsp = nodep->itemsp()) {
            nodep->جمع_کړه_بل_دلته(itemsp->unlink_Fr_شا_With_بل());
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstFork* nodep) override {
        dot_نومونه(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_بل_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = ریښتیا;
            iterate_And_بل_Null(nodep->forksp());
        }
        Ast_غوټه* addsp = تش_اشاره;
        إذا (Ast_غوټه* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_شا_With_بل();
            addsp = Ast_غوټه::جمع_کړه_بل(addsp, declsp);
        }
        إذا (Ast_غوټه* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_شا_With_بل();
            addsp = Ast_غوټه::جمع_کړه_بل(addsp, stmtsp);
        }
        إذا (addsp) nodep->جمع_کړه_دلته_This_As_بل(addsp);
        nodep->name("");
    }
    فراغ لیدنه(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = خطأ;
            dot_نومونه(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        إذا (m_keepBegins) {
            nodep->name("");
            أرجع;
        }
        Ast_غوټه* addsp = تش_اشاره;
        إذا (Ast_غوټه* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_شا_With_بل();
            addsp = Ast_غوټه::جمع_کړه_بل(addsp, declsp);
        }
        إذا (Ast_غوټه* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_شا_With_بل();
            addsp = Ast_غوټه::جمع_کړه_بل(addsp, stmtsp);
        }
        إذا (addsp) {
            nodep->replaceWith(addsp);
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_غوټه_بلاک* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dot_نومونه(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        إذا (m_keepBegins) {
            nodep->name("");
            أرجع;
        }
        Ast_غوټه* addsp = تش_اشاره;
        إذا (Ast_غوټه* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_شا_With_بل();
            addsp = Ast_غوټه::جمع_کړه_بل(addsp, declsp);
        }
        إذا (Ast_غوټه* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_شا_With_بل();
            addsp = Ast_غوټه::جمع_کړه_بل(addsp, stmtsp);
        }
        إذا (addsp) {
            nodep->replaceWith(addsp);
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstVar* nodep) override {
        إذا (nodep->user1_سېټ_Once()) { أرجع; }
        // If static variable, move it outside a function.
        إذا (nodep->lifetime().isStatic() && m_ftaskp) {
            ثابت std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            إذا (nodep->isIO()) {
                إذا (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ثابت portp = nodep->clone_ونه_خالص(خطأ);
                nodep->replaceWith(portp);

                إذا (nodep->is_ننوت() || nodep->isInout()) {
                    Ast_ټاکل* ثابت initAssignp = جديد Ast_ټاکل{
                        nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        جديد AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->جمع_کړه_بل_دلته(initAssignp);
                }

                إذا (nodep->isWritable()) {
                    Ast_ټاکل* ثابت پای_ټکی_Assignp = جديد Ast_ټاکل{
                        nodep->fileline(), جديد AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        جديد AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->جمع_کړه_Stmtsp(پای_ټکی_Assignp);
                }
            } وإلا {
                nodep->unlink_Fr_شا();
            }
            nodep->name(newName);
            m_ftaskp->جمع_کړه_دلته_This_As_بل(nodep);
            nodep->funcLocal(خطأ);
        } وإلا إذا (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_بدل_شوی(nodep);
            // Move it under enclosing tree
            lift_غوټه(nodep);
        }
    }
    فراغ لیدنه(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_شا_With_بل());
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Don't double-add text's
        Ast_غوټه* wasUnderp = m_ftaskp;
        إذا (!m_ftaskp) wasUnderp = m_procedurep;
        إذا (wasUnderp) {
            إذا (nodep->stmtsp()) {
                Ast_غوټه* ثابت newp = جديد AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_شا_With_بل()};
                wasUnderp->جمع_کړه_دلته_This_As_بل(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        } وإلا {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    فراغ لیدنه(AstTypedef* nodep) override {
        إذا (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_بدل_شوی(nodep);
            // Move it under enclosing tree
            lift_غوټه(nodep);
        }
    }
    فراغ لیدنه(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        إذا (m_namedScope != "") {
            m_statep->user_Mark_بدل_شوی(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlink_Fr_شا();
            m_modp->جمع_کړه_Stmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        إذا (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        إذا (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ثابت std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        إذا (!scname.تشه()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    فراغ لیدنه(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = خطأ;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_if_ژوروالی);
        إذا (m_if_ژوروالی == -1 || v3Global.opt.if_ژوروالی() < 1) {  // Turned off
        } وإلا إذا (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->لومړیتوب_Pragma()) {
            m_if_ژوروالی = -1;
        } وإلا إذا (++m_if_ژوروالی > v3Global.opt.if_ژوروالی()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3_Error_کوډ::IFDEPTH, ریښتیا);  // Warn only once
            m_if_ژوروالی = -1;
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = خطأ;
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = افتراضي;
};

//######################################################################

صنف BeginRelinkVisitor final : عام VNVisitorConst {
    // Replace tasks with new pointer
خاص:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        إذا (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        إذا (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        إذا (state.any_Func_دننه_Begin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("begin", 0, dump_ونه_Either_Level() >= 3);
}

ساكن Ast_غوټه* createForeachLoop(Ast_غوټه_Foreach* /*nodep*/, Ast_غوټه* bodysp, منطقي arrayMayResize,
                                  Ast_غوټه_بیان* subfromp, AstVar* varp, Ast_غوټه_بیان* leftp,
                                  Ast_غوټه_بیان* rightp, VNType غوټه_Type) {
    File_کرښه* ثابت fl = varp->fileline();
    Ast_غوټه_بیان* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
    Ast_غوټه_بیان* condp;
    منطقي inc = ریښتیا;
    بدل (غوټه_Type) {
    حالة VNType::LteS: condp = جديد AstLteS{fl, varRefp, rightp}; اكسر;
    حالة VNType::Lt: condp = جديد AstLt{fl, varRefp, rightp}; اكسر;
    حالة VNType::GteS:
        condp = جديد AstGteS{fl, varRefp, rightp};
        inc = خطأ;
        اكسر;
    افتراضي: varp->v3fatalSrc("Missing comparison handling"); اكسر;
    }
    Ast_غوټه_بیان* incp;
    إذا (inc)
        incp = جديد Ast_جمع_کړه{fl, varRefp->clone_ونه(خطأ), جديد AstConst{fl, 1}};
    وإلا
        incp = جديد AstSub{fl, varRefp->clone_ونه(خطأ), جديد AstConst{fl, 1}};

    AstLoop* ثابت loopp = جديد AstLoop{fl};
    loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{fl, loopp, condp});
    AstVar* اندازه_Varp = تش_اشاره;
    Ast_غوټه_بیان* اندازه_Getp = تش_اشاره;
    إذا (arrayMayResize) {
        اندازه_Varp = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->پیدا_کړه_UInt32_DType()};
        اندازه_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        اندازه_Varp->کارول_شوی_Loop_Idx(ریښتیا);  // Not technically an index, but used only inside loop
        varp->جمع_کړه_بل(اندازه_Varp);
        اندازه_Getp
            = جديد AstCMethodHard{fl, subfromp->clone_ونه_خالص(خطأ), VCMethod::DYN_اندازه, تش_اشاره};
        اندازه_Getp->dtype_سېټ_UInt32();
        loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, اندازه_Varp, VAccess::WRITE}, اندازه_Getp});
    }
    loopp->جمع_کړه_Stmtsp(bodysp);
    Ast_غوټه_Stmt* incStmtp = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, incp};
    إذا (arrayMayResize) {
        incStmtp = جديد AstIf{fl,
                             جديد AstLte{fl, جديد AstVarRef{fl, اندازه_Varp, VAccess::READ},
                                        اندازه_Getp->clone_ونه_خالص(خطأ)},
                             incStmtp};
    }
    loopp->جمع_کړه_Stmtsp(incStmtp);
    Ast_غوټه* ثابت stmtsp = varp;  // New statements for outer loop
    stmtsp->جمع_کړه_بل(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->جمع_کړه_بل(loopp);
    أرجع stmtsp;
}
ساكن Ast_غوټه* createForeachLoopRanged(Ast_غوټه_Foreach* nodep, Ast_غوټه* bodysp,
                                        Ast_غوټه_بیان* subfromp, AstVar* varp,
                                        ثابت VNumRange& declRange) {
    File_کرښه* ثابت fl = varp->fileline();
    V3_شمېره کیڼ{nodep, 32};
    V3_شمېره ښی{nodep, 32};
    کیڼ.isSigned(ریښتیا);
    ښی.isSigned(ریښتیا);
    کیڼ.سېټ_Long_S(declRange.کیڼ());
    ښی.سېټ_Long_S(declRange.ښی());
    Ast_غوټه_بیان* ثابت leftp = جديد AstConst{fl, کیڼ};
    Ast_غوټه_بیان* ثابت rightp = جديد AstConst{fl, ښی};
    أرجع createForeachLoop(nodep, bodysp, خطأ /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.کیڼ() <= declRange.ښی() ? VNType::LteS : VNType::GteS);
}
ساكن Ast_غوټه* createForeachAssoc(File_کرښه* fl, AstVar* varp, Ast_غوټه_بیان* subfromp,
                                   Ast_غوټه_DType* fromDtp, Ast_غوټه* bodyPointp) {
    Ast_غوټه* loopp = varp;
    AstVar* ثابت بل_varp  // Iterator containing next element (to handle mid-array delete)
        = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    بل_varp->کارول_شوی_Loop_Idx(ریښتیا);
    بل_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->جمع_کړه_بل(بل_varp);

    AstVar* ثابت نور_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlag_بټ_Packed{}, 1};
    نور_varp->کارول_شوی_Loop_Idx(ریښتیا);  // Not technically an index, but used only inside loop
    نور_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->جمع_کړه_بل(نور_varp);

    Ast_غوټه_بیان* ثابت firstp
        = جديد AstCMethodHard{fl, subfromp->clone_ونه_خالص(خطأ), VCMethod::ASSOC_لومړی,
                             جديد AstVarRef{fl, بل_varp, VAccess::READWRITE}};
    firstp->dtype_سېټ_Int();
    Ast_غوټه_بیان* ثابت nextp
        = جديد AstCMethodHard{fl, subfromp->clone_ونه_خالص(خطأ), VCMethod::ASSOC_بل,
                             جديد AstVarRef{fl, بل_varp, VAccess::READWRITE}};
    nextp->dtype_سېټ_Int();

    // _Vmore = array.first(__Vnext)
    loopp->جمع_کړه_بل(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, نور_varp, VAccess::WRITE},
                                 جديد AstNeq{fl, جديد AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ثابت lp = جديد AstLoop{fl};
    loopp->جمع_کړه_بل(lp);
    lp->جمع_کړه_Stmtsp(جديد AstLoopTest{fl, lp, جديد AstVarRef{fl, نور_varp, VAccess::READ}});

    // index = __Vnext
    lp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                جديد AstVarRef{fl, بل_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, نور_varp, VAccess::WRITE},
                                جديد AstNeq{fl, جديد AstConst{fl, 0}, nextp}});
    lp->جمع_کړه_Stmtsp(bodyPointp);
    أرجع loopp;
}

ساكن منطقي arrayMayResizeCheck(Ast_غوټه* nodesp, AstVar* fromVarp) {
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
    إذا (!nodesp) أرجع خطأ;
    أرجع nodesp->exists_And_بل([&](ثابت Ast_غوټه* nodep) -> منطقي {
        إذا (ثابت Ast_غوټه_Var_Ref* varrefp = VN_CAST(nodep, غوټه_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            إذا (varrefp->access().isWriteOrRW()) {
                إذا (!fromVarp) أرجع ریښتیا;
                إذا (varrefp->varp() == fromVarp) أرجع ریښتیا;
            }
        }
        // No idea what a function might be doing
        إذا (VN_IS(nodep, لاره_Call) || VN_IS(nodep, غوټه_FTask_Ref)) أرجع ریښتیا;
        أرجع خطأ;
    });
}

Ast_غوټه* V3Begin::convert_منزل_While(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ثابت Ast_Foreach_سرلیک* ثابت headerp = nodep->headerp();
    Ast_غوټه_بیان* ثابت fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ثابت fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : تش_اشاره;
    Ast_غوټه_DType* fromDtp = fromp->dtypep()->skipRefp();
    ثابت منطقي arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_غوټه* bodyPointp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", تش_اشاره, خطأ};
    Ast_غوټه* newp = تش_اشاره;
    Ast_غوټه* lastp = nodep;
    AstVar* nestedIndexp = تش_اشاره;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_غوټه_بیان* subfromp = fromp->clone_ونه_خالص(خطأ);
    // Major dimension first
    لكل (Ast_غوټه *argsp = headerp->elementsp(), *بل_argsp; argsp; argsp = بل_argsp) {
        بل_argsp = argsp->nextp();
        ثابت منطقي تشه = VN_IS(argsp, تشه_2);
        AstVar* ثابت varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || تشه, argsp, "Missing foreach loop variable");
        إذا (varp) varp->unlink_Fr_شا()->کارول_شوی_Loop_Idx(ریښتیا);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_کرښه* ثابت fl = argsp->fileline();
        إذا (varp) {
            Ast_غوټه* loopp = تش_اشاره;
            VNRelinker handle;
            lastp->unlink_Fr_شا(&handle);
            إذا (ثابت Ast_غوټه_Array_DType* ثابت adtypep = VN_CAST(fromDtp, غوټه_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                إذا (adtypep->isString()) {
                    AstConst* ثابت leftp = جديد AstConst{fl, 0};
                    Ast_غوټه_بیان* ثابت rightp = جديد AstLenN{fl, fromp->clone_ونه_خالص(خطأ)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } وإلا {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } وإلا إذا (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, کتار_DType)) {
                AstConst* ثابت leftp = جديد AstConst{fl, 0};
                Ast_غوټه_بیان* ثابت rightp = جديد AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), غوټه_Array_DType)
                        ? جديد AstArraySel{fl, subfromp->clone_ونه_خالص(خطأ),
                                          جديد AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_ونه_خالص(خطأ),
                    VCMethod::DYN_اندازه};
                AstVarRef* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                rightp->dtype_سېټ_Int();
                rightp->protect(خطأ);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = جديد AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } وإلا إذا (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                subfromp = جديد AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            إذا (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->delete_ونه(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_غوټه* ثابت bodyp = nodep->bodyp();
    إذا (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        إذا (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_ونه(), bodyPointp);
        أرجع تش_اشاره;
    }
    إذا (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_شا_With_بل());
    } وإلا {
        bodyPointp->unlink_Fr_شا();
    }
    VL_DO_DANGLING(bodyPointp->delete_ونه(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    أرجع newp;
}
