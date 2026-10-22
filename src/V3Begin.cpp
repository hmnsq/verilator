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
    ثابت VNUser1_اندريون_Use m_inuser1;
    منطقي m_any_Func_اندريون_Begin = خطأ;

عام:
    BeginState() = افتراضي;
    ~BeginState() = افتراضي;
    فراغ user_Mark_بدليو(Ast_ڳنڍ* nodep) {
        nodep->user1(سچ);
        m_any_Func_اندريون_Begin = سچ;
    }
    منطقي any_Func_اندريون_Begin() ثابت { أرجع m_any_Func_اندريون_Begin; }
};

//######################################################################

صنف BeginVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ثابت m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    AstCellInline* m_inlinesp = خالي_اشارو;  // AstCellInlines to add to m_modp
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Current function/task
    Ast_ڳنڍ_Procedure* m_procedurep = خالي_اشارو;  // Current procedure
    Ast_ڳنڍ* m_liftedp = خالي_اشارو;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    صحيح m_if_اونهائي = 0;  // Current if depth
    منطقي m_keepBegins = خطأ;  // True if begins should not be inlined

    // METHODS

    string dot(ثابت string& a, ثابت string& b) { أرجع VString::dot(a, "__DOT__", b); }

    فراغ dot_نالا(ثابت std::string& name, File_سٽ* ثابت flp, ثابت حرف* ثابت بلاڪ_Name) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        إذا (name == "") أرجع;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::سائيز_type pos;
        طالما ((pos = dottedname.ڳوليو("__DOT__")) != std::string::npos) {
            ثابت std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::لڙي_ڊيگهه("__DOT__"));
            إذا (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            إذا (!m_ftaskp) {
                AstCellInline* ثابت inlinep = جديد AstCellInline{flp, m_unnamedScope, بلاڪ_Name};
                m_inlinesp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(m_inlinesp, inlinep);
            }
        }
    }

    فراغ lift_ڳنڍ(Ast_ڳنڍ* nodep) {
        nodep->unlink_Fr_پٺ();
        إذا (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            إذا (!m_liftedp) {
                m_liftedp = nodep;
            } وإلا {
                m_liftedp->جوڙيو_ايندڙ(nodep);
            }
        } وإلا {
            // Move to module
            m_modp->جوڙيو_Stmtsp(nodep);
        }
    }

    // VISITORS
    فراغ گهمو(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convert_منزل_While(nodep), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        إذا (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_بدليو(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = خالي_اشارو;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        إذا (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_بدليو(nodep);
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
        m_liftedp = خالي_اشارو;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ثابت initp) {
            initp->unlink_Fr_پٺ();
            m_ftaskp->جوڙيو_هتي_This_As_ايندڙ(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dot_نالا(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_ايندڙ_Null(nodep->itemsp());
        // Repalce node with body then delete
        إذا (Ast_ڳنڍ* ثابت itemsp = nodep->itemsp()) {
            nodep->جوڙيو_ايندڙ_هتي(itemsp->unlink_Fr_پٺ_With_ايندڙ());
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(AstFork* nodep) override {
        dot_نالا(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_ايندڙ_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = سچ;
            iterate_And_ايندڙ_Null(nodep->forksp());
        }
        Ast_ڳنڍ* addsp = خالي_اشارو;
        إذا (Ast_ڳنڍ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_پٺ_With_ايندڙ();
            addsp = Ast_ڳنڍ::جوڙيو_ايندڙ(addsp, declsp);
        }
        إذا (Ast_ڳنڍ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_پٺ_With_ايندڙ();
            addsp = Ast_ڳنڍ::جوڙيو_ايندڙ(addsp, stmtsp);
        }
        إذا (addsp) nodep->جوڙيو_هتي_This_As_ايندڙ(addsp);
        nodep->name("");
    }
    فراغ گهمو(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = خطأ;
            dot_نالا(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        إذا (m_keepBegins) {
            nodep->name("");
            أرجع;
        }
        Ast_ڳنڍ* addsp = خالي_اشارو;
        إذا (Ast_ڳنڍ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_پٺ_With_ايندڙ();
            addsp = Ast_ڳنڍ::جوڙيو_ايندڙ(addsp, declsp);
        }
        إذا (Ast_ڳنڍ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_پٺ_With_ايندڙ();
            addsp = Ast_ڳنڍ::جوڙيو_ايندڙ(addsp, stmtsp);
        }
        إذا (addsp) {
            nodep->replaceWith(addsp);
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_بلاڪ* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dot_نالا(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        إذا (m_keepBegins) {
            nodep->name("");
            أرجع;
        }
        Ast_ڳنڍ* addsp = خالي_اشارو;
        إذا (Ast_ڳنڍ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_پٺ_With_ايندڙ();
            addsp = Ast_ڳنڍ::جوڙيو_ايندڙ(addsp, declsp);
        }
        إذا (Ast_ڳنڍ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_پٺ_With_ايندڙ();
            addsp = Ast_ڳنڍ::جوڙيو_ايندڙ(addsp, stmtsp);
        }
        إذا (addsp) {
            nodep->replaceWith(addsp);
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstVar* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) { أرجع; }
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
                AstVar* ثابت portp = nodep->clone_وڻ_خالص(خطأ);
                nodep->replaceWith(portp);

                إذا (nodep->is_داخلا() || nodep->isInout()) {
                    Ast_سونپيو* ثابت initAssignp = جديد Ast_سونپيو{
                        nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        جديد AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->جوڙيو_ايندڙ_هتي(initAssignp);
                }

                إذا (nodep->isWritable()) {
                    Ast_سونپيو* ثابت پڇاڙي_Assignp = جديد Ast_سونپيو{
                        nodep->fileline(), جديد AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        جديد AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->جوڙيو_Stmtsp(پڇاڙي_Assignp);
                }
            } وإلا {
                nodep->unlink_Fr_پٺ();
            }
            nodep->name(newName);
            m_ftaskp->جوڙيو_هتي_This_As_ايندڙ(nodep);
            nodep->funcLocal(خطأ);
        } وإلا إذا (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_بدليو(nodep);
            // Move it under enclosing tree
            lift_ڳنڍ(nodep);
        }
    }
    فراغ گهمو(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        إذا (nodep->user1_سيٽ_Once()) أرجع;  // Don't double-add text's
        Ast_ڳنڍ* wasUnderp = m_ftaskp;
        إذا (!m_ftaskp) wasUnderp = m_procedurep;
        إذا (wasUnderp) {
            إذا (nodep->stmtsp()) {
                Ast_ڳنڍ* ثابت newp = جديد AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ()};
                wasUnderp->جوڙيو_هتي_This_As_ايندڙ(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
        } وإلا {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    فراغ گهمو(AstTypedef* nodep) override {
        إذا (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_بدليو(nodep);
            // Move it under enclosing tree
            lift_ڳنڍ(nodep);
        }
    }
    فراغ گهمو(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        إذا (m_namedScope != "") {
            m_statep->user_Mark_بدليو(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlink_Fr_پٺ();
            m_modp->جوڙيو_Stmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        إذا (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        إذا (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        إذا (nodep->user1_سيٽ_Once()) أرجع;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ثابت std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        إذا (!scname.خالي_جايون()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    فراغ گهمو(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = خطأ;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_if_اونهائي);
        إذا (m_if_اونهائي == -1 || v3Global.opt.if_اونهائي() < 1) {  // Turned off
        } وإلا إذا (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->ترجيح_Pragma()) {
            m_if_اونهائي = -1;
        } وإلا إذا (++m_if_اونهائي > v3Global.opt.if_اونهائي()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3_Error_ڪوڊ::IFDEPTH, سچ);  // Warn only once
            m_if_اونهائي = -1;
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
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
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        إذا (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstVarRef* nodep) override {
        إذا (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        إذا (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

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
        إذا (state.any_Func_اندريون_Begin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("begin", 0, dump_وڻ_Either_Level() >= 3);
}

ساكن Ast_ڳنڍ* createForeachLoop(Ast_ڳنڍ_Foreach* /*nodep*/, Ast_ڳنڍ* bodysp, منطقي arrayMayResize,
                                  Ast_ڳنڍ_اظهار* subfromp, AstVar* varp, Ast_ڳنڍ_اظهار* leftp,
                                  Ast_ڳنڍ_اظهار* rightp, VNType ڳنڍ_Type) {
    File_سٽ* ثابت fl = varp->fileline();
    Ast_ڳنڍ_اظهار* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
    Ast_ڳنڍ_اظهار* condp;
    منطقي inc = سچ;
    بدل (ڳنڍ_Type) {
    حالة VNType::LteS: condp = جديد AstLteS{fl, varRefp, rightp}; اكسر;
    حالة VNType::Lt: condp = جديد AstLt{fl, varRefp, rightp}; اكسر;
    حالة VNType::GteS:
        condp = جديد AstGteS{fl, varRefp, rightp};
        inc = خطأ;
        اكسر;
    افتراضي: varp->v3fatalSrc("Missing comparison handling"); اكسر;
    }
    Ast_ڳنڍ_اظهار* incp;
    إذا (inc)
        incp = جديد Ast_جوڙيو{fl, varRefp->clone_وڻ(خطأ), جديد AstConst{fl, 1}};
    وإلا
        incp = جديد AstSub{fl, varRefp->clone_وڻ(خطأ), جديد AstConst{fl, 1}};

    AstLoop* ثابت loopp = جديد AstLoop{fl};
    loopp->جوڙيو_Stmtsp(جديد AstLoopTest{fl, loopp, condp});
    AstVar* سائيز_Varp = خالي_اشارو;
    Ast_ڳنڍ_اظهار* سائيز_Getp = خالي_اشارو;
    إذا (arrayMayResize) {
        سائيز_Varp = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->ڳوليو_UInt32_DType()};
        سائيز_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        سائيز_Varp->استعمال_ٿيل_Loop_Idx(سچ);  // Not technically an index, but used only inside loop
        varp->جوڙيو_ايندڙ(سائيز_Varp);
        سائيز_Getp
            = جديد AstCMethodHard{fl, subfromp->clone_وڻ_خالص(خطأ), VCMethod::DYN_سائيز, خالي_اشارو};
        سائيز_Getp->dtype_سيٽ_UInt32();
        loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, سائيز_Varp, VAccess::WRITE}, سائيز_Getp});
    }
    loopp->جوڙيو_Stmtsp(bodysp);
    Ast_ڳنڍ_Stmt* incStmtp = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, incp};
    إذا (arrayMayResize) {
        incStmtp = جديد AstIf{fl,
                             جديد AstLte{fl, جديد AstVarRef{fl, سائيز_Varp, VAccess::READ},
                                        سائيز_Getp->clone_وڻ_خالص(خطأ)},
                             incStmtp};
    }
    loopp->جوڙيو_Stmtsp(incStmtp);
    Ast_ڳنڍ* ثابت stmtsp = varp;  // New statements for outer loop
    stmtsp->جوڙيو_ايندڙ(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->جوڙيو_ايندڙ(loopp);
    أرجع stmtsp;
}
ساكن Ast_ڳنڍ* createForeachLoopRanged(Ast_ڳنڍ_Foreach* nodep, Ast_ڳنڍ* bodysp,
                                        Ast_ڳنڍ_اظهار* subfromp, AstVar* varp,
                                        ثابت VNumRange& declRange) {
    File_سٽ* ثابت fl = varp->fileline();
    V3_عدد کاٻي{nodep, 32};
    V3_عدد ساڄي{nodep, 32};
    کاٻي.isSigned(سچ);
    ساڄي.isSigned(سچ);
    کاٻي.سيٽ_Long_S(declRange.کاٻي());
    ساڄي.سيٽ_Long_S(declRange.ساڄي());
    Ast_ڳنڍ_اظهار* ثابت leftp = جديد AstConst{fl, کاٻي};
    Ast_ڳنڍ_اظهار* ثابت rightp = جديد AstConst{fl, ساڄي};
    أرجع createForeachLoop(nodep, bodysp, خطأ /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.کاٻي() <= declRange.ساڄي() ? VNType::LteS : VNType::GteS);
}
ساكن Ast_ڳنڍ* createForeachAssoc(File_سٽ* fl, AstVar* varp, Ast_ڳنڍ_اظهار* subfromp,
                                   Ast_ڳنڍ_DType* fromDtp, Ast_ڳنڍ* bodyPointp) {
    Ast_ڳنڍ* loopp = varp;
    AstVar* ثابت ايندڙ_varp  // Iterator containing next element (to handle mid-array delete)
        = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    ايندڙ_varp->استعمال_ٿيل_Loop_Idx(سچ);
    ايندڙ_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->جوڙيو_ايندڙ(ايندڙ_varp);

    AstVar* ثابت وڌيڪ_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlag_ٻٽو_انگ_Packed{}, 1};
    وڌيڪ_varp->استعمال_ٿيل_Loop_Idx(سچ);  // Not technically an index, but used only inside loop
    وڌيڪ_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->جوڙيو_ايندڙ(وڌيڪ_varp);

    Ast_ڳنڍ_اظهار* ثابت firstp
        = جديد AstCMethodHard{fl, subfromp->clone_وڻ_خالص(خطأ), VCMethod::ASSOC_پهريون,
                             جديد AstVarRef{fl, ايندڙ_varp, VAccess::READWRITE}};
    firstp->dtype_سيٽ_Int();
    Ast_ڳنڍ_اظهار* ثابت nextp
        = جديد AstCMethodHard{fl, subfromp->clone_وڻ_خالص(خطأ), VCMethod::ASSOC_ايندڙ,
                             جديد AstVarRef{fl, ايندڙ_varp, VAccess::READWRITE}};
    nextp->dtype_سيٽ_Int();

    // _Vmore = array.first(__Vnext)
    loopp->جوڙيو_ايندڙ(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, وڌيڪ_varp, VAccess::WRITE},
                                 جديد AstNeq{fl, جديد AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ثابت lp = جديد AstLoop{fl};
    loopp->جوڙيو_ايندڙ(lp);
    lp->جوڙيو_Stmtsp(جديد AstLoopTest{fl, lp, جديد AstVarRef{fl, وڌيڪ_varp, VAccess::READ}});

    // index = __Vnext
    lp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                جديد AstVarRef{fl, ايندڙ_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, وڌيڪ_varp, VAccess::WRITE},
                                جديد AstNeq{fl, جديد AstConst{fl, 0}, nextp}});
    lp->جوڙيو_Stmtsp(bodyPointp);
    أرجع loopp;
}

ساكن منطقي arrayMayResizeCheck(Ast_ڳنڍ* nodesp, AstVar* fromVarp) {
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
    أرجع nodesp->exists_And_ايندڙ([&](ثابت Ast_ڳنڍ* nodep) -> منطقي {
        إذا (ثابت Ast_ڳنڍ_Var_Ref* varrefp = VN_CAST(nodep, ڳنڍ_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            إذا (varrefp->access().isWriteOrRW()) {
                إذا (!fromVarp) أرجع سچ;
                إذا (varrefp->varp() == fromVarp) أرجع سچ;
            }
        }
        // No idea what a function might be doing
        إذا (VN_IS(nodep, طريقو_Call) || VN_IS(nodep, ڳنڍ_FTask_Ref)) أرجع سچ;
        أرجع خطأ;
    });
}

Ast_ڳنڍ* V3Begin::convert_منزل_While(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ثابت Ast_Foreach_سرو* ثابت headerp = nodep->headerp();
    Ast_ڳنڍ_اظهار* ثابت fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ثابت fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : خالي_اشارو;
    Ast_ڳنڍ_DType* fromDtp = fromp->dtypep()->skipRefp();
    ثابت منطقي arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_ڳنڍ* bodyPointp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", خالي_اشارو, خطأ};
    Ast_ڳنڍ* newp = خالي_اشارو;
    Ast_ڳنڍ* lastp = nodep;
    AstVar* nestedIndexp = خالي_اشارو;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_ڳنڍ_اظهار* subfromp = fromp->clone_وڻ_خالص(خطأ);
    // Major dimension first
    لكل (Ast_ڳنڍ *argsp = headerp->elementsp(), *ايندڙ_argsp; argsp; argsp = ايندڙ_argsp) {
        ايندڙ_argsp = argsp->nextp();
        ثابت منطقي خالي_جايون = VN_IS(argsp, خالي_جايون_2);
        AstVar* ثابت varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || خالي_جايون, argsp, "Missing foreach loop variable");
        إذا (varp) varp->unlink_Fr_پٺ()->استعمال_ٿيل_Loop_Idx(سچ);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_سٽ* ثابت fl = argsp->fileline();
        إذا (varp) {
            Ast_ڳنڍ* loopp = خالي_اشارو;
            VNRelinker handle;
            lastp->unlink_Fr_پٺ(&handle);
            إذا (ثابت Ast_ڳنڍ_Array_DType* ثابت adtypep = VN_CAST(fromDtp, ڳنڍ_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                إذا (adtypep->isString()) {
                    AstConst* ثابت leftp = جديد AstConst{fl, 0};
                    Ast_ڳنڍ_اظهار* ثابت rightp = جديد AstLenN{fl, fromp->clone_وڻ_خالص(خطأ)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } وإلا {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } وإلا إذا (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, قطار_DType)) {
                AstConst* ثابت leftp = جديد AstConst{fl, 0};
                Ast_ڳنڍ_اظهار* ثابت rightp = جديد AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), ڳنڍ_Array_DType)
                        ? جديد AstArraySel{fl, subfromp->clone_وڻ_خالص(خطأ),
                                          جديد AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_وڻ_خالص(خطأ),
                    VCMethod::DYN_سائيز};
                AstVarRef* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                rightp->dtype_سيٽ_Int();
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
    VL_DO_DANGLING(subfromp->delete_وڻ(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_ڳنڍ* ثابت bodyp = nodep->bodyp();
    إذا (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
        إذا (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_وڻ(), bodyPointp);
        أرجع خالي_اشارو;
    }
    إذا (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_پٺ_With_ايندڙ());
    } وإلا {
        bodyPointp->unlink_Fr_پٺ();
    }
    VL_DO_DANGLING(bodyPointp->delete_وڻ(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    أرجع newp;
}
