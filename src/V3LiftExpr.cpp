// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Lift expressions out of statements
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
//
// V3LiftExpr's Transformations:
//
// Lift impure sub-expressions and function calls out of expressions,
// turning them into additional statements. This has several benefits
// that enable further downstream optimizations:
// - Impure expressions always appear on the RHS of a simple assignment,
//   reducing the size of impure expressions. This also eliminates
//   later needs for cloning impure expressions, preserving side effects
// - Lifted function calls can be inlined without the use of AstExprStmt,
//   which is poorly handled by optimizations, especially Dfg
// - Reduces complexity of downstream lowering passes as they need to deal
//   with fewer special cases.
//
// The generic transformation applies for all AstNodeStmt. Using AstAssign
// as an example:
//    x[impure_x] = impure_y + func(impure_z);
// is transformed into:
//    __VleImpure_0 = impure_y;
//    __VleImpure_1 = impure_z;
//    __VleCall_0 = func(__VleImpure_1);
//    __VleImpure_2 = impure_x;
//    x[__VleImpure_2] = __VleImpure_0 + __VleCall_0;
// All parts of the assignment is now pure, and the function call can be
// inlined by V3Task without the use of AstExprStmt.
//
// Care must be taken for the 4 short-circuiting operators: && || -> ?:
// For example AstLogAnd:
//    z = x && func(y)
// is transformed into:
//    __VleLogAnd_0 = x;
//    if (__VleLogAnd_0) {
//      __VleCall_0 = func(y);
//      __VleLogAnd_0 = __VleCall_0;
//    }
//    z = __VleLogAnd_1;
// Similar patterns are used for AstLogOr, AstLogIf and AstCond to preserve
// the short-circuiting semantics and side effects.
//
// Care must be taken for impure LValues as well. While, all LValue expressions
// permitted by IEEE-1800 are themselves pure (except possibly for the non-lvalue
// sub-expressions like array indices, which are not a problem), we support a
// non-standard but common extension where a function call returning a class
// handle, under a member select can be an LValue, e.g.:
//    'getInstance().member = foo;'
// Fortunately we can discover these via MemberSel, and we can transform them
// to the IEEE-1800 compliant form:
//    __VleLvalCall_0 = getInstance();
//    __VleLvalCall_0.member = foo;
// There are also some other internal LValues represented via CMethodHard that
// return references, and are marked as impure. For this reason only we still
// need to special case their handling via AstNodeExpr::isLValue().
//
//*************************************************************************

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3LiftExpr.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3Inst.h"
#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം Lift_വ്യഞ്ജകം_Visitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    സ്ഥിരം VNUser1InUse m_user1InUse;

    // STATE
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    തർക്കം m_lift = അസത്യം;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_കണ്ണി* m_newStmtps = nullptr;
    // Expressions in some special locations need not, or should not be lifted
    Ast_കണ്ണി_വ്യഞ്ജകം* m_doNotLiftp = nullptr;
    size_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_വ്യഞ്ജകം_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(സ്ഥിരം അക്ഷരം* baseName, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                   സ്ഥിരം std::string& suffix = "") {
        // Reuse existing temporary if available
        എങ്കിൽ (exprp->user1p()) {
            ++m_statTemporariesReused;
            തിരികെ VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += baseName;
        name += "_" + std::to_string(m_nTmps++);
        എങ്കിൽ (!suffix.empty()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->isInternal(സത്യം);
        varp->noReset(സത്യം);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        എങ്കിൽ (m_ftaskp) {
            varp->funcLocal(സത്യം);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } അല്ലെങ്കിൽ {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        തിരികെ varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
            എങ്കിൽ (refp->varp()->user1()) തിരികെ refp->varp();
        }
        തിരികെ nullptr;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    AstAssign* assignIfDifferent(File_വരി* flp, AstVar* varp, Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
            എങ്കിൽ (refp->varp() == varp) തിരികെ nullptr;
        }
        തിരികെ പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlinkFrBack()};
    }

    ശൂന്യം addStmtps(Ast_കണ്ണി* stmtp) {
        എങ്കിൽ (!stmtp) തിരികെ;
        // No need to process again, so mark
        വേണ്ടി (Ast_കണ്ണി* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_കണ്ണി::addNext(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_കണ്ണി* lift(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (!nodep) തിരികെ nullptr;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = സത്യം;
        m_newStmtps = nullptr;
        iterate(nodep);
        തിരികെ m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_കണ്ണി* liftChildren(Ast_കണ്ണി_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = സത്യം;
        m_newStmtps = nullptr;
        iterateChildren(nodep);
        തിരികെ m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = അസത്യം;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        എങ്കിൽ (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCaseItem* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterateAndNextNull(nodep->stmtsp());
    }
    ശൂന്യം visit(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    ശൂന്യം visit(AstAlias* nodep) override {}
    ശൂന്യം visit(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    ശൂന്യം visit(AstWait* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        // Keep lifted statements inside the repeatedly evaluated condition.
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം newStmtps = lift(nodep->condp())) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = nodep->condp()->unlinkFrBack();
            nodep->condp(പുതിയ Ast_വ്യഞ്ജകം_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterateAndNextNull(nodep->stmtsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), കണ്ണി_Var_Ref) ? nodep->rhsp() : nullptr;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം newStmtps = lift(nodep->rhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }

        എങ്കിൽ (nodep->timingControlp()) തിരികെ;

        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം newStmtps = lift(nodep->lhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }
    }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        // Ignore super class constructor calls - can't insert statements before them
        എങ്കിൽ (VN_IS(nodep->exprp(), New)) തിരികെ;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    ശൂന്യം visit(AstCStmt* nodep) override {}
    ശൂന്യം visit(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (!m_lift) തിരികെ;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        എങ്കിൽ (m_doNotLiftp == nodep) തിരികെ;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        എങ്കിൽ (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) തിരികെ;
        // Do not lift if pure
        എങ്കിൽ (nodep->isPure()) തിരികെ;
        // Do not lift if LValue
        എങ്കിൽ (nodep->isLValue()) തിരികെ;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstVar* സ്ഥിരം varp = newVar("Expr", nodep);
        nodep->replaceWith(പുതിയ AstVarRef{flp, varp, VAccess::READ});
        addStmtps(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (!m_lift) തിരികെ;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        എങ്കിൽ (m_doNotLiftp == nodep) തിരികെ;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        എങ്കിൽ (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) തിരികെ;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        എങ്കിൽ (VN_IS(nodep, TaskRef)) തിരികെ;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstVar* സ്ഥിരം varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(പുതിയ AstVarRef{flp, varp, VAccess::READ});
        addStmtps(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        എങ്കിൽ (!m_lift) തിരികെ;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), കണ്ണി_FTask_Ref)
                           ? nodep->fromp()
                           : nullptr;
        iterateChildren(nodep);
        // Done if not a special case
        എങ്കിൽ (!m_doNotLiftp) തിരികെ;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstVar* സ്ഥിരം varp = newVar("LvalCall", nodep->fromp());
        addStmtps(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlinkFrBack()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(പുതിയ AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    ശൂന്യം visit(AstCond* nodep) override {
        എങ്കിൽ (!m_lift) തിരികെ;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = nullptr;

        സ്ഥിരം തർക്കം sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_വൃക്ഷം(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_കണ്ണി* സ്ഥിരം thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        എങ്കിൽ (sameType) varp = getExistingVar(nodep->thenp());
        എങ്കിൽ (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_കണ്ണി* സ്ഥിരം elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        എങ്കിൽ (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        എങ്കിൽ (!thenStmtps && !elseStmtps) തിരികെ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        എങ്കിൽ (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, nodep->condp()->unlinkFrBack()};
        addStmtps(ifp);
        ifp->addThensp(thenStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->thenp()));
        ifp->addElsesp(elseStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(പുതിയ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstLogAnd* nodep) override {
        എങ്കിൽ (!m_lift) തിരികെ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        എങ്കിൽ (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_കണ്ണി* സ്ഥിരം rhsStmtps = lift(nodep->rhsp());
        എങ്കിൽ (!rhsStmtps) തിരികെ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        എങ്കിൽ (!varp) varp = getExistingVar(nodep->rhsp());
        എങ്കിൽ (!varp) varp = newVar("LogAnd", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addThensp(rhsStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(പുതിയ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstLogOr* nodep) override {
        എങ്കിൽ (!m_lift) തിരികെ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        എങ്കിൽ (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_കണ്ണി* സ്ഥിരം rhsStmtps = lift(nodep->rhsp());
        എങ്കിൽ (!rhsStmtps) തിരികെ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        എങ്കിൽ (!varp) varp = getExistingVar(nodep->rhsp());
        എങ്കിൽ (!varp) varp = newVar("LogOr", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(പുതിയ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstLogIf* nodep) override {
        എങ്കിൽ (!m_lift) തിരികെ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_കണ്ണി* സ്ഥിരം rhsStmtps = lift(nodep->rhsp());
        എങ്കിൽ (!rhsStmtps) തിരികെ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        എങ്കിൽ (!varp) varp = newVar("LogIf", nodep);
        addStmtps(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varp, VAccess::WRITE},
                                പുതിയ AstLogNot{flp, nodep->lhsp()->unlinkFrBack()}});
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, പുതിയ AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(പുതിയ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        എങ്കിൽ (!m_lift) തിരികെ;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_വ്യഞ്ജകം_Stmts;
        iterate(nodep->stmtsp());
        addStmtps(nodep->stmtsp()->unlinkFrBackWithNext());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    ശൂന്യം visit(AstConst*) override {}
    ശൂന്യം visit(AstVarRef*) override {}
    ശൂന്യം visit(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    ശൂന്യം visit(AstAssocSel* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    ശൂന്യം visit(AstCExpr* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstCExprUser* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit Lift_വ്യഞ്ജകം_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_വൃക്ഷം();
        iterate(nodep);
        VIsCached::clear_Cache_വൃക്ഷം();
        എങ്കിൽ (m_newStmtps) m_newStmtps->dump_വൃക്ഷം_And_Next(std::cout, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_വ്യഞ്ജകം_Visitor() override {
        V3Stats::addStat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::addStat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::addStat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::addStat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::addStat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::addStat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::addStat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::addStat("LiftExpr, lifted ExprStmt", m_stat_Lifted_വ്യഞ്ജകം_Stmts);
        V3Stats::addStat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::addStat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

ശൂന്യം V3_Lift_വ്യഞ്ജകം::lift_വ്യഞ്ജകം_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_വ്യഞ്ജകം_Visitor{nodep}; }
    V3Global::dump_Check_Global_വൃക്ഷം("lift_expr", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
