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

جماعت BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ثابت VNUser1_اندر_Use m_inuser1;
    منطقی m_any_Func_اندر_Begin = جھوٹ;

عوامی:
    BeginState() = طےشدہ;
    ~BeginState() = طےشدہ;
    خالی user_Mark_بدلا(Ast_گرہ* nodep) {
        nodep->user1(سچ);
        m_any_Func_اندر_Begin = سچ;
    }
    منطقی any_Func_اندر_Begin() ثابت { واپس m_any_Func_اندر_Begin; }
};

//######################################################################

جماعت BeginVisitor final : عوامی VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ثابت m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    AstCellInline* m_inlinesp = خالی_اشاریہ;  // AstCellInlines to add to m_modp
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    Ast_گرہ_Procedure* m_procedurep = خالی_اشاریہ;  // Current procedure
    Ast_گرہ* m_liftedp = خالی_اشاریہ;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    صحیح m_if_گہرائی = 0;  // Current if depth
    منطقی m_keepBegins = جھوٹ;  // True if begins should not be inlined

    // METHODS

    string dot(ثابت string& a, ثابت string& b) { واپس VString::dot(a, "__DOT__", b); }

    خالی dot_نام(ثابت std::string& name, File_سطر* ثابت flp, ثابت حرف* ثابت بلاک_Name) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        اگر (name == "") واپس;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::سائز_type pos;
        جب ((pos = dottedname.ڈھونڈو("__DOT__")) != std::string::npos) {
            ثابت std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::لڑی_لمبائی("__DOT__"));
            اگر (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            اگر (!m_ftaskp) {
                AstCellInline* ثابت inlinep = نیا AstCellInline{flp, m_unnamedScope, بلاک_Name};
                m_inlinesp = Ast_گرہ::جوڑو_اگلا_Null(m_inlinesp, inlinep);
            }
        }
    }

    خالی lift_گرہ(Ast_گرہ* nodep) {
        nodep->unlink_Fr_عقب();
        اگر (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            اگر (!m_liftedp) {
                m_liftedp = nodep;
            } ورنہ {
                m_liftedp->جوڑو_اگلا(nodep);
            }
        } ورنہ {
            // Move to module
            m_modp->جوڑو_Stmtsp(nodep);
        }
    }

    // VISITORS
    خالی دورہ(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convert_مقصد_While(nodep), nodep);
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        اگر (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_بدلا(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = خالی_اشاریہ;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        اگر (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_بدلا(nodep);
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
        m_liftedp = خالی_اشاریہ;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ثابت initp) {
            initp->unlink_Fr_عقب();
            m_ftaskp->جوڑو_یہاں_This_As_اگلا(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    خالی دورہ(Ast_Gen_بلاک* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dot_نام(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_اگلا_Null(nodep->itemsp());
        // Repalce node with body then delete
        اگر (Ast_گرہ* ثابت itemsp = nodep->itemsp()) {
            nodep->جوڑو_اگلا_یہاں(itemsp->unlink_Fr_عقب_With_اگلا());
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstFork* nodep) override {
        dot_نام(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_اگلا_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = سچ;
            iterate_And_اگلا_Null(nodep->forksp());
        }
        Ast_گرہ* addsp = خالی_اشاریہ;
        اگر (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_عقب_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, declsp);
        }
        اگر (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_عقب_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, stmtsp);
        }
        اگر (addsp) nodep->جوڑو_یہاں_This_As_اگلا(addsp);
        nodep->name("");
    }
    خالی دورہ(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = جھوٹ;
            dot_نام(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        اگر (m_keepBegins) {
            nodep->name("");
            واپس;
        }
        Ast_گرہ* addsp = خالی_اشاریہ;
        اگر (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_عقب_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, declsp);
        }
        اگر (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_عقب_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, stmtsp);
        }
        اگر (addsp) {
            nodep->replaceWith(addsp);
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(Ast_گرہ_بلاک* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dot_نام(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        اگر (m_keepBegins) {
            nodep->name("");
            واپس;
        }
        Ast_گرہ* addsp = خالی_اشاریہ;
        اگر (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_عقب_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, declsp);
        }
        اگر (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_عقب_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, stmtsp);
        }
        اگر (addsp) {
            nodep->replaceWith(addsp);
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstVar* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) { واپس; }
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
                AstVar* ثابت portp = nodep->clone_درخت_خالص(جھوٹ);
                nodep->replaceWith(portp);

                اگر (nodep->is_داخلہ() || nodep->isInout()) {
                    Ast_تفویض* ثابت initAssignp = نیا Ast_تفویض{
                        nodep->fileline(), نیا AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        نیا AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->جوڑو_اگلا_یہاں(initAssignp);
                }

                اگر (nodep->isWritable()) {
                    Ast_تفویض* ثابت انتہا_Assignp = نیا Ast_تفویض{
                        nodep->fileline(), نیا AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        نیا AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->جوڑو_Stmtsp(انتہا_Assignp);
                }
            } ورنہ {
                nodep->unlink_Fr_عقب();
            }
            nodep->name(newName);
            m_ftaskp->جوڑو_یہاں_This_As_اگلا(nodep);
            nodep->funcLocal(جھوٹ);
        } ورنہ اگر (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_بدلا(nodep);
            // Move it under enclosing tree
            lift_گرہ(nodep);
        }
    }
    خالی دورہ(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_عقب_With_اگلا());
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Don't double-add text's
        Ast_گرہ* wasUnderp = m_ftaskp;
        اگر (!m_ftaskp) wasUnderp = m_procedurep;
        اگر (wasUnderp) {
            اگر (nodep->stmtsp()) {
                Ast_گرہ* ثابت newp = نیا AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_عقب_With_اگلا()};
                wasUnderp->جوڑو_یہاں_This_As_اگلا(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
        } ورنہ {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    خالی دورہ(AstTypedef* nodep) override {
        اگر (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_بدلا(nodep);
            // Move it under enclosing tree
            lift_گرہ(nodep);
        }
    }
    خالی دورہ(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        اگر (m_namedScope != "") {
            m_statep->user_Mark_بدلا(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlink_Fr_عقب();
            m_modp->جوڑو_Stmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        اگر (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        اگر (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ثابت std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        اگر (!scname.خالی_خانے()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    خالی دورہ(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    خالی دورہ(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = جھوٹ;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_if_گہرائی);
        اگر (m_if_گہرائی == -1 || v3Global.opt.if_گہرائی() < 1) {  // Turned off
        } ورنہ اگر (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->ترجیح_Pragma()) {
            m_if_گہرائی = -1;
        } ورنہ اگر (++m_if_گہرائی > v3Global.opt.if_گہرائی()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3_Error_رمز::IFDEPTH, سچ);  // Warn only once
            m_if_گہرائی = -1;
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = جھوٹ;
        iterateChildren(nodep);
    }

عوامی:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = طےشدہ;
};

//######################################################################

جماعت BeginRelinkVisitor final : عوامی VNVisitorConst {
    // Replace tasks with new pointer
نجی:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        اگر (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstVarRef* nodep) override {
        اگر (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        اگر (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = طےشدہ;
};

//######################################################################
// Task class functions

خالی V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        اگر (state.any_Func_اندر_Begin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("begin", 0, dump_درخت_Either_Level() >= 3);
}

ساکن Ast_گرہ* createForeachLoop(Ast_گرہ_Foreach* /*nodep*/, Ast_گرہ* bodysp, منطقی arrayMayResize,
                                  Ast_گرہ_اظہاریہ* subfromp, AstVar* varp, Ast_گرہ_اظہاریہ* leftp,
                                  Ast_گرہ_اظہاریہ* rightp, VNType گرہ_Type) {
    File_سطر* ثابت fl = varp->fileline();
    Ast_گرہ_اظہاریہ* varRefp = نیا AstVarRef{fl, varp, VAccess::READ};
    Ast_گرہ_اظہاریہ* condp;
    منطقی inc = سچ;
    انتخاب (گرہ_Type) {
    حالت VNType::LteS: condp = نیا AstLteS{fl, varRefp, rightp}; توڑو;
    حالت VNType::Lt: condp = نیا AstLt{fl, varRefp, rightp}; توڑو;
    حالت VNType::GteS:
        condp = نیا AstGteS{fl, varRefp, rightp};
        inc = جھوٹ;
        توڑو;
    طےشدہ: varp->v3fatalSrc("Missing comparison handling"); توڑو;
    }
    Ast_گرہ_اظہاریہ* incp;
    اگر (inc)
        incp = نیا Ast_جوڑو{fl, varRefp->clone_درخت(جھوٹ), نیا AstConst{fl, 1}};
    ورنہ
        incp = نیا AstSub{fl, varRefp->clone_درخت(جھوٹ), نیا AstConst{fl, 1}};

    AstLoop* ثابت loopp = نیا AstLoop{fl};
    loopp->جوڑو_Stmtsp(نیا AstLoopTest{fl, loopp, condp});
    AstVar* سائز_Varp = خالی_اشاریہ;
    Ast_گرہ_اظہاریہ* سائز_Getp = خالی_اشاریہ;
    اگر (arrayMayResize) {
        سائز_Varp = نیا AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->ڈھونڈو_UInt32_DType()};
        سائز_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        سائز_Varp->مستعمل_Loop_Idx(سچ);  // Not technically an index, but used only inside loop
        varp->جوڑو_اگلا(سائز_Varp);
        سائز_Getp
            = نیا AstCMethodHard{fl, subfromp->clone_درخت_خالص(جھوٹ), VCMethod::DYN_سائز, خالی_اشاریہ};
        سائز_Getp->dtype_سیٹ_UInt32();
        loopp->جوڑو_Stmtsp(نیا Ast_تفویض{fl, نیا AstVarRef{fl, سائز_Varp, VAccess::WRITE}, سائز_Getp});
    }
    loopp->جوڑو_Stmtsp(bodysp);
    Ast_گرہ_Stmt* incStmtp = نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, incp};
    اگر (arrayMayResize) {
        incStmtp = نیا AstIf{fl,
                             نیا AstLte{fl, نیا AstVarRef{fl, سائز_Varp, VAccess::READ},
                                        سائز_Getp->clone_درخت_خالص(جھوٹ)},
                             incStmtp};
    }
    loopp->جوڑو_Stmtsp(incStmtp);
    Ast_گرہ* ثابت stmtsp = varp;  // New statements for outer loop
    stmtsp->جوڑو_اگلا(نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->جوڑو_اگلا(loopp);
    واپس stmtsp;
}
ساکن Ast_گرہ* createForeachLoopRanged(Ast_گرہ_Foreach* nodep, Ast_گرہ* bodysp,
                                        Ast_گرہ_اظہاریہ* subfromp, AstVar* varp,
                                        ثابت VNumRange& declRange) {
    File_سطر* ثابت fl = varp->fileline();
    V3_نمبر بائیں{nodep, 32};
    V3_نمبر دائیں{nodep, 32};
    بائیں.isSigned(سچ);
    دائیں.isSigned(سچ);
    بائیں.سیٹ_Long_S(declRange.بائیں());
    دائیں.سیٹ_Long_S(declRange.دائیں());
    Ast_گرہ_اظہاریہ* ثابت leftp = نیا AstConst{fl, بائیں};
    Ast_گرہ_اظہاریہ* ثابت rightp = نیا AstConst{fl, دائیں};
    واپس createForeachLoop(nodep, bodysp, جھوٹ /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.بائیں() <= declRange.دائیں() ? VNType::LteS : VNType::GteS);
}
ساکن Ast_گرہ* createForeachAssoc(File_سطر* fl, AstVar* varp, Ast_گرہ_اظہاریہ* subfromp,
                                   Ast_گرہ_DType* fromDtp, Ast_گرہ* bodyPointp) {
    Ast_گرہ* loopp = varp;
    AstVar* ثابت اگلا_varp  // Iterator containing next element (to handle mid-array delete)
        = نیا AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    اگلا_varp->مستعمل_Loop_Idx(سچ);
    اگلا_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->جوڑو_اگلا(اگلا_varp);

    AstVar* ثابت مزید_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = نیا AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlag_ثنائی_ہندسہ_Packed{}, 1};
    مزید_varp->مستعمل_Loop_Idx(سچ);  // Not technically an index, but used only inside loop
    مزید_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->جوڑو_اگلا(مزید_varp);

    Ast_گرہ_اظہاریہ* ثابت firstp
        = نیا AstCMethodHard{fl, subfromp->clone_درخت_خالص(جھوٹ), VCMethod::ASSOC_پہلا,
                             نیا AstVarRef{fl, اگلا_varp, VAccess::READWRITE}};
    firstp->dtype_سیٹ_Int();
    Ast_گرہ_اظہاریہ* ثابت nextp
        = نیا AstCMethodHard{fl, subfromp->clone_درخت_خالص(جھوٹ), VCMethod::ASSOC_اگلا,
                             نیا AstVarRef{fl, اگلا_varp, VAccess::READWRITE}};
    nextp->dtype_سیٹ_Int();

    // _Vmore = array.first(__Vnext)
    loopp->جوڑو_اگلا(نیا Ast_تفویض{fl, نیا AstVarRef{fl, مزید_varp, VAccess::WRITE},
                                 نیا AstNeq{fl, نیا AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ثابت lp = نیا AstLoop{fl};
    loopp->جوڑو_اگلا(lp);
    lp->جوڑو_Stmtsp(نیا AstLoopTest{fl, lp, نیا AstVarRef{fl, مزید_varp, VAccess::READ}});

    // index = __Vnext
    lp->جوڑو_Stmtsp(نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE},
                                نیا AstVarRef{fl, اگلا_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->جوڑو_Stmtsp(نیا Ast_تفویض{fl, نیا AstVarRef{fl, مزید_varp, VAccess::WRITE},
                                نیا AstNeq{fl, نیا AstConst{fl, 0}, nextp}});
    lp->جوڑو_Stmtsp(bodyPointp);
    واپس loopp;
}

ساکن منطقی arrayMayResizeCheck(Ast_گرہ* nodesp, AstVar* fromVarp) {
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
    اگر (!nodesp) واپس جھوٹ;
    واپس nodesp->exists_And_اگلا([&](ثابت Ast_گرہ* nodep) -> منطقی {
        اگر (ثابت Ast_گرہ_Var_Ref* varrefp = VN_CAST(nodep, گرہ_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            اگر (varrefp->access().isWriteOrRW()) {
                اگر (!fromVarp) واپس سچ;
                اگر (varrefp->varp() == fromVarp) واپس سچ;
            }
        }
        // No idea what a function might be doing
        اگر (VN_IS(nodep, طریقہ_Call) || VN_IS(nodep, گرہ_FTask_Ref)) واپس سچ;
        واپس جھوٹ;
    });
}

Ast_گرہ* V3Begin::convert_مقصد_While(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ثابت Ast_Foreach_سرخی* ثابت headerp = nodep->headerp();
    Ast_گرہ_اظہاریہ* ثابت fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ثابت fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : خالی_اشاریہ;
    Ast_گرہ_DType* fromDtp = fromp->dtypep()->skipRefp();
    ثابت منطقی arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_گرہ* bodyPointp = نیا AstBegin{nodep->fileline(), "[EditWrapper]", خالی_اشاریہ, جھوٹ};
    Ast_گرہ* newp = خالی_اشاریہ;
    Ast_گرہ* lastp = nodep;
    AstVar* nestedIndexp = خالی_اشاریہ;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_گرہ_اظہاریہ* subfromp = fromp->clone_درخت_خالص(جھوٹ);
    // Major dimension first
    برائے (Ast_گرہ *argsp = headerp->elementsp(), *اگلا_argsp; argsp; argsp = اگلا_argsp) {
        اگلا_argsp = argsp->nextp();
        ثابت منطقی خالی_خانے = VN_IS(argsp, خالی_خانے_2);
        AstVar* ثابت varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || خالی_خانے, argsp, "Missing foreach loop variable");
        اگر (varp) varp->unlink_Fr_عقب()->مستعمل_Loop_Idx(سچ);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_سطر* ثابت fl = argsp->fileline();
        اگر (varp) {
            Ast_گرہ* loopp = خالی_اشاریہ;
            VNRelinker handle;
            lastp->unlink_Fr_عقب(&handle);
            اگر (ثابت Ast_گرہ_Array_DType* ثابت adtypep = VN_CAST(fromDtp, گرہ_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } ورنہ اگر (ثابت AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                اگر (adtypep->isString()) {
                    AstConst* ثابت leftp = نیا AstConst{fl, 0};
                    Ast_گرہ_اظہاریہ* ثابت rightp = نیا AstLenN{fl, fromp->clone_درخت_خالص(جھوٹ)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } ورنہ {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } ورنہ اگر (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, قطار_DType)) {
                AstConst* ثابت leftp = نیا AstConst{fl, 0};
                Ast_گرہ_اظہاریہ* ثابت rightp = نیا AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), گرہ_Array_DType)
                        ? نیا AstArraySel{fl, subfromp->clone_درخت_خالص(جھوٹ),
                                          نیا AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_درخت_خالص(جھوٹ),
                    VCMethod::DYN_سائز};
                AstVarRef* varRefp = نیا AstVarRef{fl, varp, VAccess::READ};
                rightp->dtype_سیٹ_Int();
                rightp->protect(جھوٹ);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = نیا AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } ورنہ اگر (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = نیا AstVarRef{fl, varp, VAccess::READ};
                subfromp = نیا AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
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
    Ast_گرہ* ثابت bodyp = nodep->bodyp();
    اگر (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        اگر (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_درخت(), bodyPointp);
        واپس خالی_اشاریہ;
    }
    اگر (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_عقب_With_اگلا());
    } ورنہ {
        bodyPointp->unlink_Fr_عقب();
    }
    VL_DO_DANGLING(bodyPointp->delete_درخت(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    واپس newp;
}
