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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LiftExpr.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Inst.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ Lift_ਵਿਅੰਜਕ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // STATE
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    ਤਾਰਕਿਕ m_lift = ਝੂਠ;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_ਗੰਢ* m_newStmtps = ਖਾਲੀ_ਸੂਚਕ;
    // Expressions in some special locations need not, or should not be lifted
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_doNotLiftp = ਖਾਲੀ_ਸੂਚਕ;
    ਮਾਪ_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_ਵਿਅੰਜਕ_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(ਅਚਲ ਅੱਖਰ* ਅਧਾਰ_Name, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                   ਅਚਲ std::string& suffix = "") {
        // Reuse existing temporary if available
        ਜੇ (exprp->user1p()) {
            ++m_statTemporariesReused;
            ਵਾਪਸ VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += ਅਧਾਰ_Name;
        name += "_" + std::ਮੰਜ਼ਿਲ_string(m_nTmps++);
        ਜੇ (!suffix.ਖਾਲੀ_ਥਾਵਾਂ()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
        varp->noReset(ਸੱਚ);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        ਜੇ (m_ftaskp) {
            varp->funcLocal(ਸੱਚ);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } ਨਹੀਂਤਾਂ {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        ਵਾਪਸ varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
            ਜੇ (refp->varp()->user1()) ਵਾਪਸ refp->varp();
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    Ast_ਸੌਂਪੋ* ਸੌਂਪੋ_If_Different(File_ਸਤਰ* flp, AstVar* varp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
            ਜੇ (refp->varp() == varp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlink_Fr_ਪਿੱਠ()};
    }

    ਖਾਲੀ ਜੋੜੋ_Stmtps(Ast_ਗੰਢ* stmtp) {
        ਜੇ (!stmtp) ਵਾਪਸ;
        // No need to process again, so mark
        ਲਈ (Ast_ਗੰਢ* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_ਗੰਢ* lift(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (!nodep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = ਸੱਚ;
        m_newStmtps = ਖਾਲੀ_ਸੂਚਕ;
        iterate(nodep);
        ਵਾਪਸ m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_ਗੰਢ* liftChildren(Ast_ਗੰਢ_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = ਸੱਚ;
        m_newStmtps = ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
        ਵਾਪਸ m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = ਝੂਠ;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        ਜੇ (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    ਖਾਲੀ ਦੇਖੋ(AstWait* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = ਖਾਲੀ_ਸੂਚਕ;
        // Keep lifted statements inside the repeatedly evaluated condition.
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ newStmtps = lift(nodep->condp())) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = nodep->condp()->unlink_Fr_ਪਿੱਠ();
            nodep->condp(ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ newStmtps = liftChildren(nodep)) nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newStmtps);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), ਗੰਢ_Var_Ref) ? nodep->rhsp() : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ newStmtps = lift(nodep->rhsp())) {
            nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newStmtps);
        }

        ਜੇ (nodep->timingControlp()) ਵਾਪਸ;

        ਜੇ (Ast_ਗੰਢ* ਅਚਲ newStmtps = lift(nodep->lhsp())) {
            nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newStmtps);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        // Ignore super class constructor calls - can't insert statements before them
        ਜੇ (VN_IS(nodep->exprp(), New)) ਵਾਪਸ;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ newStmtps = liftChildren(nodep)) nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    ਖਾਲੀ ਦੇਖੋ(AstCStmt* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (!m_lift) ਵਾਪਸ;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        ਜੇ (m_doNotLiftp == nodep) ਵਾਪਸ;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        ਜੇ (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) ਵਾਪਸ;
        // Do not lift if pure
        ਜੇ (nodep->is_ਸ਼ੁੱਧ()) ਵਾਪਸ;
        // Do not lift if LValue
        ਜੇ (nodep->isLValue()) ਵਾਪਸ;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstVar* ਅਚਲ varp = newVar("Expr", nodep);
        nodep->replaceWith(ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ});
        ਜੋੜੋ_Stmtps(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (!m_lift) ਵਾਪਸ;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        ਜੇ (m_doNotLiftp == nodep) ਵਾਪਸ;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        ਜੇ (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) ਵਾਪਸ;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        ਜੇ (VN_IS(nodep, TaskRef)) ਵਾਪਸ;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstVar* ਅਚਲ varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ});
        ਜੋੜੋ_Stmtps(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਜੇ (!m_lift) ਵਾਪਸ;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), ਗੰਢ_FTask_Ref)
                           ? nodep->fromp()
                           : ਖਾਲੀ_ਸੂਚਕ;
        iterateChildren(nodep);
        // Done if not a special case
        ਜੇ (!m_doNotLiftp) ਵਾਪਸ;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstVar* ਅਚਲ varp = newVar("LvalCall", nodep->fromp());
        ਜੋੜੋ_Stmtps(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlink_Fr_ਪਿੱਠ()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(ਨਵਾਂ AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        ਜੇ (!m_lift) ਵਾਪਸ;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = ਖਾਲੀ_ਸੂਚਕ;

        ਅਚਲ ਤਾਰਕਿਕ sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_ਰੁੱਖ(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_ਗੰਢ* ਅਚਲ thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        ਜੇ (sameType) varp = getExistingVar(nodep->thenp());
        ਜੇ (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_ਗੰਢ* ਅਚਲ elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        ਜੇ (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        ਜੇ (!thenStmtps && !elseStmtps) ਵਾਪਸ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਜੇ (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, nodep->condp()->unlink_Fr_ਪਿੱਠ()};
        ਜੋੜੋ_Stmtps(ifp);
        ifp->ਜੋੜੋ_Thensp(thenStmtps);
        ifp->ਜੋੜੋ_Thensp(ਸੌਂਪੋ_If_Different(flp, varp, nodep->thenp()));
        ifp->ਜੋੜੋ_Elsesp(elseStmtps);
        ifp->ਜੋੜੋ_Elsesp(ਸੌਂਪੋ_If_Different(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogAnd* nodep) override {
        ਜੇ (!m_lift) ਵਾਪਸ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        ਜੇ (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ਗੰਢ* ਅਚਲ rhsStmtps = lift(nodep->rhsp());
        ਜੇ (!rhsStmtps) ਵਾਪਸ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਜੇ (!varp) varp = getExistingVar(nodep->rhsp());
        ਜੇ (!varp) varp = newVar("LogAnd", nodep);
        ਜੋੜੋ_Stmtps(ਸੌਂਪੋ_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ}};
        ਜੋੜੋ_Stmtps(ifp);
        ifp->ਜੋੜੋ_Thensp(rhsStmtps);
        ifp->ਜੋੜੋ_Thensp(ਸੌਂਪੋ_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogOr* nodep) override {
        ਜੇ (!m_lift) ਵਾਪਸ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        ਜੇ (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ਗੰਢ* ਅਚਲ rhsStmtps = lift(nodep->rhsp());
        ਜੇ (!rhsStmtps) ਵਾਪਸ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        ਜੇ (!varp) varp = getExistingVar(nodep->rhsp());
        ਜੇ (!varp) varp = newVar("LogOr", nodep);
        ਜੋੜੋ_Stmtps(ਸੌਂਪੋ_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ}};
        ਜੋੜੋ_Stmtps(ifp);
        ifp->ਜੋੜੋ_Elsesp(rhsStmtps);
        ifp->ਜੋੜੋ_Elsesp(ਸੌਂਪੋ_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogIf* nodep) override {
        ਜੇ (!m_lift) ਵਾਪਸ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ਗੰਢ* ਅਚਲ rhsStmtps = lift(nodep->rhsp());
        ਜੇ (!rhsStmtps) ਵਾਪਸ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        ਜੇ (!varp) varp = newVar("LogIf", nodep);
        ਜੋੜੋ_Stmtps(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE},
                                ਨਵਾਂ AstLogNot{flp, nodep->lhsp()->unlink_Fr_ਪਿੱਠ()}});
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ}};
        ਜੋੜੋ_Stmtps(ifp);
        ifp->ਜੋੜੋ_Elsesp(rhsStmtps);
        ifp->ਜੋੜੋ_Elsesp(ਸੌਂਪੋ_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        ਜੇ (!m_lift) ਵਾਪਸ;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_ਵਿਅੰਜਕ_Stmts;
        iterate(nodep->stmtsp());
        ਜੋੜੋ_Stmtps(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    ਖਾਲੀ ਦੇਖੋ(AstConst*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstVarRef*) override {}
    ਖਾਲੀ ਦੇਖੋ(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    ਖਾਲੀ ਦੇਖੋ(AstAssocSel* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    ਖਾਲੀ ਦੇਖੋ(AstCExpr* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ Lift_ਵਿਅੰਜਕ_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_ਰੁੱਖ();
        iterate(nodep);
        VIsCached::clear_Cache_ਰੁੱਖ();
        ਜੇ (m_newStmtps) m_newStmtps->dump_ਰੁੱਖ_And_ਅਗਲਾ(std::ਬਾਹਰ, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_ਵਿਅੰਜਕ_Visitor() override {
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, lifted ExprStmt", m_stat_Lifted_ਵਿਅੰਜਕ_Stmts);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::ਜੋੜੋ_Stat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

ਖਾਲੀ V3_Lift_ਵਿਅੰਜਕ::lift_ਵਿਅੰਜਕ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_ਵਿਅੰਜਕ_Visitor{nodep}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("lift_expr", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
