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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "verilatedos.h"

#ضمن "V3LiftExpr.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Inst.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف Lift_اظهار_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;

    // STATE
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Current function/task
    منطقي m_lift = خطأ;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_ڳنڍ* m_newStmtps = خالي_اشارو;
    // Expressions in some special locations need not, or should not be lifted
    Ast_ڳنڍ_اظهار* m_doNotLiftp = خالي_اشارو;
    سائيز_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_اظهار_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(ثابت حرف* بنياد_Name, ثابت Ast_ڳنڍ_اظهار* exprp,
                   ثابت std::string& suffix = "") {
        // Reuse existing temporary if available
        إذا (exprp->user1p()) {
            ++m_statTemporariesReused;
            أرجع VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += بنياد_Name;
        name += "_" + std::منزل_string(m_nTmps++);
        إذا (!suffix.خالي_جايون()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* ثابت varp = جديد AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->is_اندروني(سچ);
        varp->noReset(سچ);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        إذا (m_ftaskp) {
            varp->funcLocal(سچ);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } وإلا {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        أرجع varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_ڳنڍ_اظهار* nodep) {
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            إذا (refp->varp()->user1()) أرجع refp->varp();
        }
        أرجع خالي_اشارو;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    Ast_سونپيو* سونپيو_If_Different(File_سٽ* flp, AstVar* varp, Ast_ڳنڍ_اظهار* nodep) {
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            إذا (refp->varp() == varp) أرجع خالي_اشارو;
        }
        أرجع جديد Ast_سونپيو{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlink_Fr_پٺ()};
    }

    فراغ جوڙيو_Stmtps(Ast_ڳنڍ* stmtp) {
        إذا (!stmtp) أرجع;
        // No need to process again, so mark
        لكل (Ast_ڳنڍ* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_ڳنڍ::جوڙيو_ايندڙ(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_ڳنڍ* lift(Ast_ڳنڍ_اظهار* nodep) {
        إذا (!nodep) أرجع خالي_اشارو;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = سچ;
        m_newStmtps = خالي_اشارو;
        iterate(nodep);
        أرجع m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_ڳنڍ* liftChildren(Ast_ڳنڍ_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = سچ;
        m_newStmtps = خالي_اشارو;
        iterateChildren(nodep);
        أرجع m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = خطأ;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        إذا (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Case_شي* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterate_And_ايندڙ_Null(nodep->stmtsp());
    }
    فراغ گهمو(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    فراغ گهمو(AstAlias* nodep) override {}
    فراغ گهمو(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    فراغ گهمو(AstWait* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = خالي_اشارو;
        // Keep lifted statements inside the repeatedly evaluated condition.
        إذا (Ast_ڳنڍ* ثابت newStmtps = lift(nodep->condp())) {
            Ast_ڳنڍ_اظهار* ثابت condp = nodep->condp()->unlink_Fr_پٺ();
            nodep->condp(جديد Ast_اظهار_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterate_And_ايندڙ_Null(nodep->stmtsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = خالي_اشارو;
        إذا (Ast_ڳنڍ* ثابت newStmtps = liftChildren(nodep)) nodep->جوڙيو_هتي_This_As_ايندڙ(newStmtps);
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), ڳنڍ_Var_Ref) ? nodep->rhsp() : خالي_اشارو;
        إذا (Ast_ڳنڍ* ثابت newStmtps = lift(nodep->rhsp())) {
            nodep->جوڙيو_هتي_This_As_ايندڙ(newStmtps);
        }

        إذا (nodep->timingControlp()) أرجع;

        إذا (Ast_ڳنڍ* ثابت newStmtps = lift(nodep->lhsp())) {
            nodep->جوڙيو_هتي_This_As_ايندڙ(newStmtps);
        }
    }
    فراغ گهمو(Ast_Stmt_اظهار* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        // Ignore super class constructor calls - can't insert statements before them
        إذا (VN_IS(nodep->exprp(), New)) أرجع;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        إذا (Ast_ڳنڍ* ثابت newStmtps = liftChildren(nodep)) nodep->جوڙيو_هتي_This_As_ايندڙ(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    فراغ گهمو(AstCStmt* nodep) override {}
    فراغ گهمو(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        إذا (!m_lift) أرجع;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        إذا (m_doNotLiftp == nodep) أرجع;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        إذا (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) أرجع;
        // Do not lift if pure
        إذا (nodep->is_خالص()) أرجع;
        // Do not lift if LValue
        إذا (nodep->isLValue()) أرجع;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_سٽ* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("Expr", nodep);
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        جوڙيو_Stmtps(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        إذا (!m_lift) أرجع;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        إذا (m_doNotLiftp == nodep) أرجع;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        إذا (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) أرجع;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        إذا (VN_IS(nodep, TaskRef)) أرجع;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_سٽ* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        جوڙيو_Stmtps(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        إذا (!m_lift) أرجع;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), ڳنڍ_FTask_Ref)
                           ? nodep->fromp()
                           : خالي_اشارو;
        iterateChildren(nodep);
        // Done if not a special case
        إذا (!m_doNotLiftp) أرجع;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_سٽ* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("LvalCall", nodep->fromp());
        جوڙيو_Stmtps(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlink_Fr_پٺ()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(جديد AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    فراغ گهمو(AstCond* nodep) override {
        إذا (!m_lift) أرجع;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = خالي_اشارو;

        ثابت منطقي sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_وڻ(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_ڳنڍ* ثابت thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        إذا (sameType) varp = getExistingVar(nodep->thenp());
        إذا (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_ڳنڍ* ثابت elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        إذا (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        إذا (!thenStmtps && !elseStmtps) أرجع;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_سٽ* ثابت flp = nodep->fileline();
        إذا (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* ثابت ifp = جديد AstIf{flp, nodep->condp()->unlink_Fr_پٺ()};
        جوڙيو_Stmtps(ifp);
        ifp->جوڙيو_Thensp(thenStmtps);
        ifp->جوڙيو_Thensp(سونپيو_If_Different(flp, varp, nodep->thenp()));
        ifp->جوڙيو_Elsesp(elseStmtps);
        ifp->جوڙيو_Elsesp(سونپيو_If_Different(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstLogAnd* nodep) override {
        إذا (!m_lift) أرجع;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        إذا (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ڳنڍ* ثابت rhsStmtps = lift(nodep->rhsp());
        إذا (!rhsStmtps) أرجع;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_سٽ* ثابت flp = nodep->fileline();
        إذا (!varp) varp = getExistingVar(nodep->rhsp());
        إذا (!varp) varp = newVar("LogAnd", nodep);
        جوڙيو_Stmtps(سونپيو_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, varp, VAccess::READ}};
        جوڙيو_Stmtps(ifp);
        ifp->جوڙيو_Thensp(rhsStmtps);
        ifp->جوڙيو_Thensp(سونپيو_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstLogOr* nodep) override {
        إذا (!m_lift) أرجع;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        إذا (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ڳنڍ* ثابت rhsStmtps = lift(nodep->rhsp());
        إذا (!rhsStmtps) أرجع;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_سٽ* ثابت flp = nodep->fileline();
        إذا (!varp) varp = getExistingVar(nodep->rhsp());
        إذا (!varp) varp = newVar("LogOr", nodep);
        جوڙيو_Stmtps(سونپيو_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, varp, VAccess::READ}};
        جوڙيو_Stmtps(ifp);
        ifp->جوڙيو_Elsesp(rhsStmtps);
        ifp->جوڙيو_Elsesp(سونپيو_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstLogIf* nodep) override {
        إذا (!m_lift) أرجع;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ڳنڍ* ثابت rhsStmtps = lift(nodep->rhsp());
        إذا (!rhsStmtps) أرجع;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_سٽ* ثابت flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        إذا (!varp) varp = newVar("LogIf", nodep);
        جوڙيو_Stmtps(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, varp, VAccess::WRITE},
                                جديد AstLogNot{flp, nodep->lhsp()->unlink_Fr_پٺ()}});
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, varp, VAccess::READ}};
        جوڙيو_Stmtps(ifp);
        ifp->جوڙيو_Elsesp(rhsStmtps);
        ifp->جوڙيو_Elsesp(سونپيو_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(Ast_اظهار_Stmt* nodep) override {
        إذا (!m_lift) أرجع;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_اظهار_Stmts;
        iterate(nodep->stmtsp());
        جوڙيو_Stmtps(nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlink_Fr_پٺ());
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    فراغ گهمو(AstConst*) override {}
    فراغ گهمو(AstVarRef*) override {}
    فراغ گهمو(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    فراغ گهمو(AstAssocSel* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    فراغ گهمو(AstCExpr* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(AstCExprUser* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح Lift_اظهار_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_وڻ();
        iterate(nodep);
        VIsCached::clear_Cache_وڻ();
        إذا (m_newStmtps) m_newStmtps->dump_وڻ_And_ايندڙ(std::ٻاهر, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_اظهار_Visitor() override {
        V3Stats::جوڙيو_Stat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::جوڙيو_Stat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::جوڙيو_Stat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::جوڙيو_Stat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::جوڙيو_Stat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::جوڙيو_Stat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::جوڙيو_Stat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::جوڙيو_Stat("LiftExpr, lifted ExprStmt", m_stat_Lifted_اظهار_Stmts);
        V3Stats::جوڙيو_Stat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::جوڙيو_Stat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

فراغ V3_Lift_اظهار::lift_اظهار_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_اظهار_Visitor{nodep}; }
    V3Global::dump_Check_Global_وڻ("lift_expr", 0, dump_وڻ_Either_Level() >= 3);
}
