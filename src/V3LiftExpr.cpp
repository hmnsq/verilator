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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "verilatedos.h"

#شامل "V3LiftExpr.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Inst.h"
#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت Lift_اظہاریہ_Visitor final : عوامی VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // STATE
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    منطقی m_lift = جھوٹ;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_گرہ* m_newStmtps = خالی_اشاریہ;
    // Expressions in some special locations need not, or should not be lifted
    Ast_گرہ_اظہاریہ* m_doNotLiftp = خالی_اشاریہ;
    سائز_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_اظہاریہ_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(ثابت حرف* بنیاد_Name, ثابت Ast_گرہ_اظہاریہ* exprp,
                   ثابت std::string& suffix = "") {
        // Reuse existing temporary if available
        اگر (exprp->user1p()) {
            ++m_statTemporariesReused;
            واپس VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += بنیاد_Name;
        name += "_" + std::مقصد_string(m_nTmps++);
        اگر (!suffix.خالی_خانے()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* ثابت varp = نیا AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->is_اندرونی(سچ);
        varp->noReset(سچ);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        اگر (m_ftaskp) {
            varp->funcLocal(سچ);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } ورنہ {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        واپس varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_گرہ_اظہاریہ* nodep) {
        اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            اگر (refp->varp()->user1()) واپس refp->varp();
        }
        واپس خالی_اشاریہ;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    Ast_تفویض* تفویض_If_Different(File_سطر* flp, AstVar* varp, Ast_گرہ_اظہاریہ* nodep) {
        اگر (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            اگر (refp->varp() == varp) واپس خالی_اشاریہ;
        }
        واپس نیا Ast_تفویض{flp, نیا AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlink_Fr_عقب()};
    }

    خالی جوڑو_Stmtps(Ast_گرہ* stmtp) {
        اگر (!stmtp) واپس;
        // No need to process again, so mark
        برائے (Ast_گرہ* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_گرہ::جوڑو_اگلا(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_گرہ* lift(Ast_گرہ_اظہاریہ* nodep) {
        اگر (!nodep) واپس خالی_اشاریہ;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = سچ;
        m_newStmtps = خالی_اشاریہ;
        iterate(nodep);
        واپس m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_گرہ* liftChildren(Ast_گرہ_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = سچ;
        m_newStmtps = خالی_اشاریہ;
        iterateChildren(nodep);
        واپس m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    خالی دورہ(Ast_گرہ* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = جھوٹ;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        اگر (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Case_شے* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterate_And_اگلا_Null(nodep->stmtsp());
    }
    خالی دورہ(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    خالی دورہ(AstAlias* nodep) override {}
    خالی دورہ(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    خالی دورہ(AstWait* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = خالی_اشاریہ;
        // Keep lifted statements inside the repeatedly evaluated condition.
        اگر (Ast_گرہ* ثابت newStmtps = lift(nodep->condp())) {
            Ast_گرہ_اظہاریہ* ثابت condp = nodep->condp()->unlink_Fr_عقب();
            nodep->condp(نیا Ast_اظہاریہ_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterate_And_اگلا_Null(nodep->stmtsp());
    }
    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = خالی_اشاریہ;
        اگر (Ast_گرہ* ثابت newStmtps = liftChildren(nodep)) nodep->جوڑو_یہاں_This_As_اگلا(newStmtps);
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), گرہ_Var_Ref) ? nodep->rhsp() : خالی_اشاریہ;
        اگر (Ast_گرہ* ثابت newStmtps = lift(nodep->rhsp())) {
            nodep->جوڑو_یہاں_This_As_اگلا(newStmtps);
        }

        اگر (nodep->timingControlp()) واپس;

        اگر (Ast_گرہ* ثابت newStmtps = lift(nodep->lhsp())) {
            nodep->جوڑو_یہاں_This_As_اگلا(newStmtps);
        }
    }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;
        // Ignore super class constructor calls - can't insert statements before them
        اگر (VN_IS(nodep->exprp(), New)) واپس;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        اگر (Ast_گرہ* ثابت newStmtps = liftChildren(nodep)) nodep->جوڑو_یہاں_This_As_اگلا(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    خالی دورہ(AstCStmt* nodep) override {}
    خالی دورہ(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {
        اگر (!m_lift) واپس;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        اگر (m_doNotLiftp == nodep) واپس;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        اگر (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) واپس;
        // Do not lift if pure
        اگر (nodep->is_خالص()) واپس;
        // Do not lift if LValue
        اگر (nodep->isLValue()) واپس;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_سطر* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("Expr", nodep);
        nodep->replaceWith(نیا AstVarRef{flp, varp, VAccess::READ});
        جوڑو_Stmtps(نیا Ast_تفویض{flp, نیا AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        اگر (!m_lift) واپس;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        اگر (m_doNotLiftp == nodep) واپس;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        اگر (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) واپس;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        اگر (VN_IS(nodep, TaskRef)) واپس;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_سطر* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(نیا AstVarRef{flp, varp, VAccess::READ});
        جوڑو_Stmtps(نیا Ast_تفویض{flp, نیا AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    خالی دورہ(AstMemberSel* nodep) override {
        اگر (!m_lift) واپس;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), گرہ_FTask_Ref)
                           ? nodep->fromp()
                           : خالی_اشاریہ;
        iterateChildren(nodep);
        // Done if not a special case
        اگر (!m_doNotLiftp) واپس;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_سطر* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("LvalCall", nodep->fromp());
        جوڑو_Stmtps(نیا Ast_تفویض{flp, نیا AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlink_Fr_عقب()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(نیا AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    خالی دورہ(AstCond* nodep) override {
        اگر (!m_lift) واپس;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = خالی_اشاریہ;

        ثابت منطقی sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_درخت(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_گرہ* ثابت thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        اگر (sameType) varp = getExistingVar(nodep->thenp());
        اگر (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_گرہ* ثابت elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        اگر (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        اگر (!thenStmtps && !elseStmtps) واپس;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_سطر* ثابت flp = nodep->fileline();
        اگر (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* ثابت ifp = نیا AstIf{flp, nodep->condp()->unlink_Fr_عقب()};
        جوڑو_Stmtps(ifp);
        ifp->جوڑو_Thensp(thenStmtps);
        ifp->جوڑو_Thensp(تفویض_If_Different(flp, varp, nodep->thenp()));
        ifp->جوڑو_Elsesp(elseStmtps);
        ifp->جوڑو_Elsesp(تفویض_If_Different(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(نیا AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(AstLogAnd* nodep) override {
        اگر (!m_lift) واپس;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        اگر (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_گرہ* ثابت rhsStmtps = lift(nodep->rhsp());
        اگر (!rhsStmtps) واپس;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_سطر* ثابت flp = nodep->fileline();
        اگر (!varp) varp = getExistingVar(nodep->rhsp());
        اگر (!varp) varp = newVar("LogAnd", nodep);
        جوڑو_Stmtps(تفویض_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ثابت ifp = نیا AstIf{flp, نیا AstVarRef{flp, varp, VAccess::READ}};
        جوڑو_Stmtps(ifp);
        ifp->جوڑو_Thensp(rhsStmtps);
        ifp->جوڑو_Thensp(تفویض_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(نیا AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(AstLogOr* nodep) override {
        اگر (!m_lift) واپس;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        اگر (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_گرہ* ثابت rhsStmtps = lift(nodep->rhsp());
        اگر (!rhsStmtps) واپس;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_سطر* ثابت flp = nodep->fileline();
        اگر (!varp) varp = getExistingVar(nodep->rhsp());
        اگر (!varp) varp = newVar("LogOr", nodep);
        جوڑو_Stmtps(تفویض_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ثابت ifp = نیا AstIf{flp, نیا AstVarRef{flp, varp, VAccess::READ}};
        جوڑو_Stmtps(ifp);
        ifp->جوڑو_Elsesp(rhsStmtps);
        ifp->جوڑو_Elsesp(تفویض_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(نیا AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(AstLogIf* nodep) override {
        اگر (!m_lift) واپس;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_گرہ* ثابت rhsStmtps = lift(nodep->rhsp());
        اگر (!rhsStmtps) واپس;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_سطر* ثابت flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        اگر (!varp) varp = newVar("LogIf", nodep);
        جوڑو_Stmtps(نیا Ast_تفویض{flp, نیا AstVarRef{flp, varp, VAccess::WRITE},
                                نیا AstLogNot{flp, nodep->lhsp()->unlink_Fr_عقب()}});
        AstIf* ثابت ifp = نیا AstIf{flp, نیا AstVarRef{flp, varp, VAccess::READ}};
        جوڑو_Stmtps(ifp);
        ifp->جوڑو_Elsesp(rhsStmtps);
        ifp->جوڑو_Elsesp(تفویض_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(نیا AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        اگر (!m_lift) واپس;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_اظہاریہ_Stmts;
        iterate(nodep->stmtsp());
        جوڑو_Stmtps(nodep->stmtsp()->unlink_Fr_عقب_With_اگلا());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlink_Fr_عقب());
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    خالی دورہ(AstConst*) override {}
    خالی دورہ(AstVarRef*) override {}
    خالی دورہ(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    خالی دورہ(AstAssocSel* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    خالی دورہ(AstCExpr* nodep) override { iterateChildren(nodep); }
    خالی دورہ(AstCExprUser* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح Lift_اظہاریہ_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_درخت();
        iterate(nodep);
        VIsCached::clear_Cache_درخت();
        اگر (m_newStmtps) m_newStmtps->dump_درخت_And_اگلا(std::خارج, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_اظہاریہ_Visitor() override {
        V3Stats::جوڑو_Stat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::جوڑو_Stat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::جوڑو_Stat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::جوڑو_Stat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::جوڑو_Stat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::جوڑو_Stat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::جوڑو_Stat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::جوڑو_Stat("LiftExpr, lifted ExprStmt", m_stat_Lifted_اظہاریہ_Stmts);
        V3Stats::جوڑو_Stat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::جوڑو_Stat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

خالی V3_Lift_اظہاریہ::lift_اظہاریہ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_اظہاریہ_Visitor{nodep}; }
    V3Global::dump_Check_Global_درخت("lift_expr", 0, dump_درخت_Either_Level() >= 3);
}
