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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3LiftExpr.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Inst.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ Lift_ಅಭಿವ್ಯಕ್ತಿ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // STATE
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    ತಾರ್ಕಿಕ m_lift = ಸುಳ್ಳು;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_ಗಂಟು* m_newStmtps = ಶೂನ್ಯಸೂಚಕ;
    // Expressions in some special locations need not, or should not be lifted
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_doNotLiftp = ಶೂನ್ಯಸೂಚಕ;
    ಗಾತ್ರ_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_ಅಭಿವ್ಯಕ್ತಿ_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ತಳ_Name, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                   ಸ್ಥಿರಾಂಕ std::string& suffix = "") {
        // Reuse existing temporary if available
        ಆದರೆ (exprp->user1p()) {
            ++m_statTemporariesReused;
            ಹಿಂತಿರುಗಿ VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += ತಳ_Name;
        name += "_" + std::ಗಮ್ಯ_string(m_nTmps++);
        ಆದರೆ (!suffix.ಖಾಲಿಗಳು()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->is_ಆಂತರಿಕ(ನಿಜ);
        varp->noReset(ನಿಜ);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        ಆದರೆ (m_ftaskp) {
            varp->funcLocal(ನಿಜ);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        ಹಿಂತಿರುಗಿ varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
            ಆದರೆ (refp->varp()->user1()) ಹಿಂತಿರುಗಿ refp->varp();
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    Ast_ನಿಯೋಜಿಸು* ನಿಯೋಜಿಸು_If_Different(File_ಸಾಲು* flp, AstVar* varp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
            ಆದರೆ (refp->varp() == varp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlink_Fr_ಹಿಂಭಾಗ()};
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Stmtps(Ast_ಗಂಟು* stmtp) {
        ಆದರೆ (!stmtp) ಹಿಂತಿರುಗಿ;
        // No need to process again, so mark
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_ಗಂಟು* lift(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = ನಿಜ;
        m_newStmtps = ಶೂನ್ಯಸೂಚಕ;
        iterate(nodep);
        ಹಿಂತಿರುಗಿ m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_ಗಂಟು* liftChildren(Ast_ಗಂಟು_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = ನಿಜ;
        m_newStmtps = ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
        ಹಿಂತಿರುಗಿ m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = ಸುಳ್ಳು;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        ಆದರೆ (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlias* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    ಶೂನ್ಯ ಭೇಟಿ(AstWait* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = ಶೂನ್ಯಸೂಚಕ;
        // Keep lifted statements inside the repeatedly evaluated condition.
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newStmtps = lift(nodep->condp())) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ();
            nodep->condp(ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newStmtps = liftChildren(nodep)) nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newStmtps);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), ಗಂಟು_Var_Ref) ? nodep->rhsp() : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newStmtps = lift(nodep->rhsp())) {
            nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newStmtps);
        }

        ಆದರೆ (nodep->timingControlp()) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newStmtps = lift(nodep->lhsp())) {
            nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newStmtps);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        // Ignore super class constructor calls - can't insert statements before them
        ಆದರೆ (VN_IS(nodep->exprp(), New)) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newStmtps = liftChildren(nodep)) nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmt* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (!m_lift) ಹಿಂತಿರುಗಿ;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        ಆದರೆ (m_doNotLiftp == nodep) ಹಿಂತಿರುಗಿ;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        ಆದರೆ (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) ಹಿಂತಿರುಗಿ;
        // Do not lift if pure
        ಆದರೆ (nodep->is_ಶುದ್ಧ()) ಹಿಂತಿರುಗಿ;
        // Do not lift if LValue
        ಆದರೆ (nodep->isLValue()) ಹಿಂತಿರುಗಿ;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ varp = newVar("Expr", nodep);
        nodep->replaceWith(ಹೊಸ AstVarRef{flp, varp, VAccess::READ});
        ಕೂಡಿಸು_Stmtps(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (!m_lift) ಹಿಂತಿರುಗಿ;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        ಆದರೆ (m_doNotLiftp == nodep) ಹಿಂತಿರುಗಿ;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        ಆದರೆ (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) ಹಿಂತಿರುಗಿ;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        ಆದರೆ (VN_IS(nodep, TaskRef)) ಹಿಂತಿರುಗಿ;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(ಹೊಸ AstVarRef{flp, varp, VAccess::READ});
        ಕೂಡಿಸು_Stmtps(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಆದರೆ (!m_lift) ಹಿಂತಿರುಗಿ;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), ಗಂಟು_FTask_Ref)
                           ? nodep->fromp()
                           : ಶೂನ್ಯಸೂಚಕ;
        iterateChildren(nodep);
        // Done if not a special case
        ಆದರೆ (!m_doNotLiftp) ಹಿಂತಿರುಗಿ;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ varp = newVar("LvalCall", nodep->fromp());
        ಕೂಡಿಸು_Stmtps(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(ಹೊಸ AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        ಆದರೆ (!m_lift) ಹಿಂತಿರುಗಿ;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = ಶೂನ್ಯಸೂಚಕ;

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_ವೃಕ್ಷ(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        ಆದರೆ (sameType) varp = getExistingVar(nodep->thenp());
        ಆದರೆ (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        ಆದರೆ (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        ಆದರೆ (!thenStmtps && !elseStmtps) ಹಿಂತಿರುಗಿ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಆದರೆ (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ()};
        ಕೂಡಿಸು_Stmtps(ifp);
        ifp->ಕೂಡಿಸು_Thensp(thenStmtps);
        ifp->ಕೂಡಿಸು_Thensp(ನಿಯೋಜಿಸು_If_Different(flp, varp, nodep->thenp()));
        ifp->ಕೂಡಿಸು_Elsesp(elseStmtps);
        ifp->ಕೂಡಿಸು_Elsesp(ನಿಯೋಜಿಸು_If_Different(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(ಹೊಸ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogAnd* nodep) override {
        ಆದರೆ (!m_lift) ಹಿಂತಿರುಗಿ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        ಆದರೆ (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsStmtps = lift(nodep->rhsp());
        ಆದರೆ (!rhsStmtps) ಹಿಂತಿರುಗಿ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಆದರೆ (!varp) varp = getExistingVar(nodep->rhsp());
        ಆದರೆ (!varp) varp = newVar("LogAnd", nodep);
        ಕೂಡಿಸು_Stmtps(ನಿಯೋಜಿಸು_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::READ}};
        ಕೂಡಿಸು_Stmtps(ifp);
        ifp->ಕೂಡಿಸು_Thensp(rhsStmtps);
        ifp->ಕೂಡಿಸು_Thensp(ನಿಯೋಜಿಸು_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ಹೊಸ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogOr* nodep) override {
        ಆದರೆ (!m_lift) ಹಿಂತಿರುಗಿ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        ಆದರೆ (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsStmtps = lift(nodep->rhsp());
        ಆದರೆ (!rhsStmtps) ಹಿಂತಿರುಗಿ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        ಆದರೆ (!varp) varp = getExistingVar(nodep->rhsp());
        ಆದರೆ (!varp) varp = newVar("LogOr", nodep);
        ಕೂಡಿಸು_Stmtps(ನಿಯೋಜಿಸು_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::READ}};
        ಕೂಡಿಸು_Stmtps(ifp);
        ifp->ಕೂಡಿಸು_Elsesp(rhsStmtps);
        ifp->ಕೂಡಿಸು_Elsesp(ನಿಯೋಜಿಸು_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ಹೊಸ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogIf* nodep) override {
        ಆದರೆ (!m_lift) ಹಿಂತಿರುಗಿ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsStmtps = lift(nodep->rhsp());
        ಆದರೆ (!rhsStmtps) ಹಿಂತಿರುಗಿ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        ಆದರೆ (!varp) varp = newVar("LogIf", nodep);
        ಕೂಡಿಸು_Stmtps(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::WRITE},
                                ಹೊಸ AstLogNot{flp, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ()}});
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, ಹೊಸ AstVarRef{flp, varp, VAccess::READ}};
        ಕೂಡಿಸು_Stmtps(ifp);
        ifp->ಕೂಡಿಸು_Elsesp(rhsStmtps);
        ifp->ಕೂಡಿಸು_Elsesp(ನಿಯೋಜಿಸು_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ಹೊಸ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        ಆದರೆ (!m_lift) ಹಿಂತಿರುಗಿ;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_ಅಭಿವ್ಯಕ್ತಿ_Stmts;
        iterate(nodep->stmtsp());
        ಕೂಡಿಸು_Stmtps(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    ಶೂನ್ಯ ಭೇಟಿ(AstConst*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    ಶೂನ್ಯ ಭೇಟಿ(AstAssocSel* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    ಶೂನ್ಯ ಭೇಟಿ(AstCExpr* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Lift_ಅಭಿವ್ಯಕ್ತಿ_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_ವೃಕ್ಷ();
        iterate(nodep);
        VIsCached::clear_Cache_ವೃಕ್ಷ();
        ಆದರೆ (m_newStmtps) m_newStmtps->dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(std::ಹೊರಗೆ, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_ಅಭಿವ್ಯಕ್ತಿ_Visitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, lifted ExprStmt", m_stat_Lifted_ಅಭಿವ್ಯಕ್ತಿ_Stmts);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::ಕೂಡಿಸು_Stat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

ಶೂನ್ಯ V3_Lift_ಅಭಿವ್ಯಕ್ತಿ::lift_ಅಭಿವ್ಯಕ್ತಿ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_ಅಭಿವ್ಯಕ್ತಿ_Visitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("lift_expr", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
