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
    ثابت VNUser1_وچ_Use m_inuser1;
    منطقي m_any_Func_وچ_Begin = خطأ;

عام:
    BeginState() = افتراضي;
    ~BeginState() = افتراضي;
    فراغ user_Mark_بدلیا(Ast_گرہ* nodep) {
        nodep->user1(سچ);
        m_any_Func_وچ_Begin = سچ;
    }
    منطقي any_Func_وچ_Begin() ثابت { أرجع m_any_Func_وچ_Begin; }
};

//######################################################################

صنف BeginVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ثابت m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    AstCellInline* m_inlinesp = خالی_سوچک;  // AstCellInlines to add to m_modp
    Ast_گرہ_FTask* m_ftaskp = خالی_سوچک;  // Current function/task
    Ast_گرہ_Procedure* m_procedurep = خالی_سوچک;  // Current procedure
    Ast_گرہ* m_liftedp = خالی_سوچک;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    صحيح m_if_ڈونگھائی = 0;  // Current if depth
    منطقي m_keepBegins = خطأ;  // True if begins should not be inlined

    // METHODS

    string dot(ثابت string& a, ثابت string& b) { أرجع VString::dot(a, "__DOT__", b); }

    فراغ dot_ناں(ثابت std::string& name, File_سطر* ثابت flp, ثابت حرف* ثابت بلاک_Name) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        إذا (name == "") أرجع;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::ناپ_type pos;
        طالما ((pos = dottedname.لبھو("__DOT__")) != std::string::npos) {
            ثابت std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::لڑی_لمبائی("__DOT__"));
            إذا (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            إذا (!m_ftaskp) {
                AstCellInline* ثابت inlinep = جديد AstCellInline{flp, m_unnamedScope, بلاک_Name};
                m_inlinesp = Ast_گرہ::جوڑو_اگلا_Null(m_inlinesp, inlinep);
            }
        }
    }

    فراغ lift_گرہ(Ast_گرہ* nodep) {
        nodep->unlink_Fr_پٹھ();
        إذا (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            إذا (!m_liftedp) {
                m_liftedp = nodep;
            } وإلا {
                m_liftedp->جوڑو_اگلا(nodep);
            }
        } وإلا {
            // Move to module
            m_modp->جوڑو_Stmtsp(nodep);
        }
    }

    // VISITORS
    فراغ ویکھو(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convert_منزل_While(nodep), nodep);
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = سچ;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        إذا (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_بدلیا(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = خالی_سوچک;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    فراغ ویکھو(Ast_گرہ_Procedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        إذا (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->user_Mark_بدلیا(nodep);
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
        m_liftedp = خالی_سوچک;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ثابت initp) {
            initp->unlink_Fr_پٹھ();
            m_ftaskp->جوڑو_ایتھے_This_As_اگلا(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    فراغ ویکھو(Ast_Gen_بلاک* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dot_ناں(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterate_And_اگلا_Null(nodep->itemsp());
        // Repalce node with body then delete
        إذا (Ast_گرہ* ثابت itemsp = nodep->itemsp()) {
            nodep->جوڑو_اگلا_ایتھے(itemsp->unlink_Fr_پٹھ_With_اگلا());
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
    }
    فراغ ویکھو(AstFork* nodep) override {
        dot_ناں(nodep->name(), nodep->fileline(), "__FORK__");
        iterate_And_اگلا_Null(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = سچ;
            iterate_And_اگلا_Null(nodep->forksp());
        }
        Ast_گرہ* addsp = خالی_سوچک;
        إذا (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_پٹھ_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, declsp);
        }
        إذا (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_پٹھ_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, stmtsp);
        }
        إذا (addsp) nodep->جوڑو_ایتھے_This_As_اگلا(addsp);
        nodep->name("");
    }
    فراغ ویکھو(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = خطأ;
            dot_ناں(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        إذا (m_keepBegins) {
            nodep->name("");
            أرجع;
        }
        Ast_گرہ* addsp = خالی_سوچک;
        إذا (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_پٹھ_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, declsp);
        }
        إذا (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_پٹھ_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, stmtsp);
        }
        إذا (addsp) {
            nodep->replaceWith(addsp);
        } وإلا {
            nodep->unlink_Fr_پٹھ();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(Ast_گرہ_بلاک* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dot_ناں(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        إذا (m_keepBegins) {
            nodep->name("");
            أرجع;
        }
        Ast_گرہ* addsp = خالی_سوچک;
        إذا (Ast_گرہ* ثابت declsp = nodep->declsp()) {
            declsp->unlink_Fr_پٹھ_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, declsp);
        }
        إذا (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_پٹھ_With_اگلا();
            addsp = Ast_گرہ::جوڑو_اگلا(addsp, stmtsp);
        }
        إذا (addsp) {
            nodep->replaceWith(addsp);
        } وإلا {
            nodep->unlink_Fr_پٹھ();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstVar* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) { أرجع; }
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
                AstVar* ثابت portp = nodep->clone_درخت_خالص(خطأ);
                nodep->replaceWith(portp);

                إذا (nodep->is_داخلہ() || nodep->isInout()) {
                    Ast_سونپو* ثابت initAssignp = جديد Ast_سونپو{
                        nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        جديد AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->جوڑو_اگلا_ایتھے(initAssignp);
                }

                إذا (nodep->isWritable()) {
                    Ast_سونپو* ثابت سرا_Assignp = جديد Ast_سونپو{
                        nodep->fileline(), جديد AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        جديد AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->جوڑو_Stmtsp(سرا_Assignp);
                }
            } وإلا {
                nodep->unlink_Fr_پٹھ();
            }
            nodep->name(newName);
            m_ftaskp->جوڑو_ایتھے_This_As_اگلا(nodep);
            nodep->funcLocal(خطأ);
        } وإلا إذا (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_بدلیا(nodep);
            // Move it under enclosing tree
            lift_گرہ(nodep);
        }
    }
    فراغ ویکھو(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlink_Fr_پٹھ_With_اگلا());
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Don't double-add text's
        Ast_گرہ* wasUnderp = m_ftaskp;
        إذا (!m_ftaskp) wasUnderp = m_procedurep;
        إذا (wasUnderp) {
            إذا (nodep->stmtsp()) {
                Ast_گرہ* ثابت newp = جديد AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlink_Fr_پٹھ_With_اگلا()};
                wasUnderp->جوڑو_ایتھے_This_As_اگلا(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
        } وإلا {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    فراغ ویکھو(AstTypedef* nodep) override {
        إذا (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->user_Mark_بدلیا(nodep);
            // Move it under enclosing tree
            lift_گرہ(nodep);
        }
    }
    فراغ ویکھو(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        إذا (m_namedScope != "") {
            m_statep->user_Mark_بدلیا(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlink_Fr_پٹھ();
            m_modp->جوڑو_Stmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        إذا (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        إذا (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ثابت std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        إذا (!scname.خالی_تھاواں()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Cover_Decl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    فراغ ویکھو(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = خطأ;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_if_ڈونگھائی);
        إذا (m_if_ڈونگھائی == -1 || v3Global.opt.if_ڈونگھائی() < 1) {  // Turned off
        } وإلا إذا (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->ترجیح_Pragma()) {
            m_if_ڈونگھائی = -1;
        } وإلا إذا (++m_if_ڈونگھائی > v3Global.opt.if_ڈونگھائی()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3_Error_کوڈ::IFDEPTH, سچ);  // Warn only once
            m_if_ڈونگھائی = -1;
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override {
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
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        إذا (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        إذا (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        إذا (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

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
        إذا (state.any_Func_وچ_Begin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("begin", 0, dump_درخت_Either_Level() >= 3);
}

ساكن Ast_گرہ* createForeachLoop(Ast_گرہ_Foreach* /*nodep*/, Ast_گرہ* bodysp, منطقي arrayMayResize,
                                  Ast_گرہ_اظہاریہ* subfromp, AstVar* varp, Ast_گرہ_اظہاریہ* leftp,
                                  Ast_گرہ_اظہاریہ* rightp, VNType گرہ_Type) {
    File_سطر* ثابت fl = varp->fileline();
    Ast_گرہ_اظہاریہ* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
    Ast_گرہ_اظہاریہ* condp;
    منطقي inc = سچ;
    بدل (گرہ_Type) {
    حالة VNType::LteS: condp = جديد AstLteS{fl, varRefp, rightp}; اكسر;
    حالة VNType::Lt: condp = جديد AstLt{fl, varRefp, rightp}; اكسر;
    حالة VNType::GteS:
        condp = جديد AstGteS{fl, varRefp, rightp};
        inc = خطأ;
        اكسر;
    افتراضي: varp->v3fatalSrc("Missing comparison handling"); اكسر;
    }
    Ast_گرہ_اظہاریہ* incp;
    إذا (inc)
        incp = جديد Ast_جوڑو{fl, varRefp->clone_درخت(خطأ), جديد AstConst{fl, 1}};
    وإلا
        incp = جديد AstSub{fl, varRefp->clone_درخت(خطأ), جديد AstConst{fl, 1}};

    AstLoop* ثابت loopp = جديد AstLoop{fl};
    loopp->جوڑو_Stmtsp(جديد AstLoopTest{fl, loopp, condp});
    AstVar* ناپ_Varp = خالی_سوچک;
    Ast_گرہ_اظہاریہ* ناپ_Getp = خالی_سوچک;
    إذا (arrayMayResize) {
        ناپ_Varp = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->لبھو_UInt32_DType()};
        ناپ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ناپ_Varp->ورتیا_Loop_Idx(سچ);  // Not technically an index, but used only inside loop
        varp->جوڑو_اگلا(ناپ_Varp);
        ناپ_Getp
            = جديد AstCMethodHard{fl, subfromp->clone_درخت_خالص(خطأ), VCMethod::DYN_ناپ, خالی_سوچک};
        ناپ_Getp->dtype_سیٹ_UInt32();
        loopp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, جديد AstVarRef{fl, ناپ_Varp, VAccess::WRITE}, ناپ_Getp});
    }
    loopp->جوڑو_Stmtsp(bodysp);
    Ast_گرہ_Stmt* incStmtp = جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, incp};
    إذا (arrayMayResize) {
        incStmtp = جديد AstIf{fl,
                             جديد AstLte{fl, جديد AstVarRef{fl, ناپ_Varp, VAccess::READ},
                                        ناپ_Getp->clone_درخت_خالص(خطأ)},
                             incStmtp};
    }
    loopp->جوڑو_Stmtsp(incStmtp);
    Ast_گرہ* ثابت stmtsp = varp;  // New statements for outer loop
    stmtsp->جوڑو_اگلا(جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->جوڑو_اگلا(loopp);
    أرجع stmtsp;
}
ساكن Ast_گرہ* createForeachLoopRanged(Ast_گرہ_Foreach* nodep, Ast_گرہ* bodysp,
                                        Ast_گرہ_اظہاریہ* subfromp, AstVar* varp,
                                        ثابت VNumRange& declRange) {
    File_سطر* ثابت fl = varp->fileline();
    V3_نمبر کھبے{nodep, 32};
    V3_نمبر سجے{nodep, 32};
    کھبے.isSigned(سچ);
    سجے.isSigned(سچ);
    کھبے.سیٹ_Long_S(declRange.کھبے());
    سجے.سیٹ_Long_S(declRange.سجے());
    Ast_گرہ_اظہاریہ* ثابت leftp = جديد AstConst{fl, کھبے};
    Ast_گرہ_اظہاریہ* ثابت rightp = جديد AstConst{fl, سجے};
    أرجع createForeachLoop(nodep, bodysp, خطأ /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.کھبے() <= declRange.سجے() ? VNType::LteS : VNType::GteS);
}
ساكن Ast_گرہ* createForeachAssoc(File_سطر* fl, AstVar* varp, Ast_گرہ_اظہاریہ* subfromp,
                                   Ast_گرہ_DType* fromDtp, Ast_گرہ* bodyPointp) {
    Ast_گرہ* loopp = varp;
    AstVar* ثابت اگلا_varp  // Iterator containing next element (to handle mid-array delete)
        = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    اگلا_varp->ورتیا_Loop_Idx(سچ);
    اگلا_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->جوڑو_اگلا(اگلا_varp);

    AstVar* ثابت ہور_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlag_ثنائی_ہندسہ_Packed{}, 1};
    ہور_varp->ورتیا_Loop_Idx(سچ);  // Not technically an index, but used only inside loop
    ہور_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->جوڑو_اگلا(ہور_varp);

    Ast_گرہ_اظہاریہ* ثابت firstp
        = جديد AstCMethodHard{fl, subfromp->clone_درخت_خالص(خطأ), VCMethod::ASSOC_پہلا,
                             جديد AstVarRef{fl, اگلا_varp, VAccess::READWRITE}};
    firstp->dtype_سیٹ_Int();
    Ast_گرہ_اظہاریہ* ثابت nextp
        = جديد AstCMethodHard{fl, subfromp->clone_درخت_خالص(خطأ), VCMethod::ASSOC_اگلا,
                             جديد AstVarRef{fl, اگلا_varp, VAccess::READWRITE}};
    nextp->dtype_سیٹ_Int();

    // _Vmore = array.first(__Vnext)
    loopp->جوڑو_اگلا(جديد Ast_سونپو{fl, جديد AstVarRef{fl, ہور_varp, VAccess::WRITE},
                                 جديد AstNeq{fl, جديد AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ثابت lp = جديد AstLoop{fl};
    loopp->جوڑو_اگلا(lp);
    lp->جوڑو_Stmtsp(جديد AstLoopTest{fl, lp, جديد AstVarRef{fl, ہور_varp, VAccess::READ}});

    // index = __Vnext
    lp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                جديد AstVarRef{fl, اگلا_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, جديد AstVarRef{fl, ہور_varp, VAccess::WRITE},
                                جديد AstNeq{fl, جديد AstConst{fl, 0}, nextp}});
    lp->جوڑو_Stmtsp(bodyPointp);
    أرجع loopp;
}

ساكن منطقي arrayMayResizeCheck(Ast_گرہ* nodesp, AstVar* fromVarp) {
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
    أرجع nodesp->exists_And_اگلا([&](ثابت Ast_گرہ* nodep) -> منطقي {
        إذا (ثابت Ast_گرہ_Var_Ref* varrefp = VN_CAST(nodep, گرہ_Var_Ref)) {
            // Any variable written, as might have indirect ref to array
            إذا (varrefp->access().isWriteOrRW()) {
                إذا (!fromVarp) أرجع سچ;
                إذا (varrefp->varp() == fromVarp) أرجع سچ;
            }
        }
        // No idea what a function might be doing
        إذا (VN_IS(nodep, طریقہ_Call) || VN_IS(nodep, گرہ_FTask_Ref)) أرجع سچ;
        أرجع خطأ;
    });
}

Ast_گرہ* V3Begin::convert_منزل_While(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ثابت Ast_Foreach_سرخی* ثابت headerp = nodep->headerp();
    Ast_گرہ_اظہاریہ* ثابت fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ثابت fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : خالی_سوچک;
    Ast_گرہ_DType* fromDtp = fromp->dtypep()->skipRefp();
    ثابت منطقي arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    Ast_گرہ* bodyPointp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", خالی_سوچک, خطأ};
    Ast_گرہ* newp = خالی_سوچک;
    Ast_گرہ* lastp = nodep;
    AstVar* nestedIndexp = خالی_سوچک;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    Ast_گرہ_اظہاریہ* subfromp = fromp->clone_درخت_خالص(خطأ);
    // Major dimension first
    لكل (Ast_گرہ *argsp = headerp->elementsp(), *اگلا_argsp; argsp; argsp = اگلا_argsp) {
        اگلا_argsp = argsp->nextp();
        ثابت منطقي خالی_تھاواں = VN_IS(argsp, خالی_تھاواں_2);
        AstVar* ثابت varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || خالی_تھاواں, argsp, "Missing foreach loop variable");
        إذا (varp) varp->unlink_Fr_پٹھ()->ورتیا_Loop_Idx(سچ);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        File_سطر* ثابت fl = argsp->fileline();
        إذا (varp) {
            Ast_گرہ* loopp = خالی_سوچک;
            VNRelinker handle;
            lastp->unlink_Fr_پٹھ(&handle);
            إذا (ثابت Ast_گرہ_Array_DType* ثابت adtypep = VN_CAST(fromDtp, گرہ_Array_DType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                إذا (adtypep->isString()) {
                    AstConst* ثابت leftp = جديد AstConst{fl, 0};
                    Ast_گرہ_اظہاریہ* ثابت rightp = جديد AstLenN{fl, fromp->clone_درخت_خالص(خطأ)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } وإلا {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } وإلا إذا (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, قطار_DType)) {
                AstConst* ثابت leftp = جديد AstConst{fl, 0};
                Ast_گرہ_اظہاریہ* ثابت rightp = جديد AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), گرہ_Array_DType)
                        ? جديد AstArraySel{fl, subfromp->clone_درخت_خالص(خطأ),
                                          جديد AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->clone_درخت_خالص(خطأ),
                    VCMethod::DYN_ناپ};
                AstVarRef* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                rightp->dtype_سیٹ_Int();
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
    VL_DO_DANGLING(subfromp->delete_درخت(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    Ast_گرہ* ثابت bodyp = nodep->bodyp();
    إذا (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
        إذا (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->delete_درخت(), bodyPointp);
        أرجع خالی_سوچک;
    }
    إذا (bodyp) {
        bodyPointp->replaceWith(bodyp->unlink_Fr_پٹھ_With_اگلا());
    } وإلا {
        bodyPointp->unlink_Fr_پٹھ();
    }
    VL_DO_DANGLING(bodyPointp->delete_درخت(), bodyPointp);
    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    أرجع newp;
}
