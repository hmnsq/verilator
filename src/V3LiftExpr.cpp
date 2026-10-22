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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3LiftExpr.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3Inst.h"
#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

বর্গ Lift_রাশিমালা_Visitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    ধ্রুব VNUser1InUse m_user1InUse;

    // STATE
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    তার্কিক m_lift = অসত্য;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_গ্রন্থি* m_newStmtps = nullptr;
    // Expressions in some special locations need not, or should not be lifted
    Ast_গ্রন্থি_রাশিমালা* m_doNotLiftp = nullptr;
    size_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_রাশিমালা_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(ধ্রুব বর্ণ* baseName, ধ্রুব Ast_গ্রন্থি_রাশিমালা* exprp,
                   ধ্রুব std::string& suffix = "") {
        // Reuse existing temporary if available
        যদি (exprp->user1p()) {
            ++m_statTemporariesReused;
            প্রত্যাগম VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += baseName;
        name += "_" + std::to_string(m_nTmps++);
        যদি (!suffix.empty()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* ধ্রুব varp = নূতন AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->isInternal(সত্য);
        varp->noReset(সত্য);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        যদি (m_ftaskp) {
            varp->funcLocal(সত্য);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } অন্যথা {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        প্রত্যাগম varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstVarRef* ধ্রুব refp = VN_CAST(nodep, VarRef)) {
            যদি (refp->varp()->user1()) প্রত্যাগম refp->varp();
        }
        প্রত্যাগম nullptr;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    AstAssign* assignIfDifferent(File_সারি* flp, AstVar* varp, Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (AstVarRef* ধ্রুব refp = VN_CAST(nodep, VarRef)) {
            যদি (refp->varp() == varp) প্রত্যাগম nullptr;
        }
        প্রত্যাগম নূতন AstAssign{flp, নূতন AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlinkFrBack()};
    }

    শূন্য addStmtps(Ast_গ্রন্থি* stmtp) {
        যদি (!stmtp) প্রত্যাগম;
        // No need to process again, so mark
        প্রতি (Ast_গ্রন্থি* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_গ্রন্থি::addNext(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_গ্রন্থি* lift(Ast_গ্রন্থি_রাশিমালা* nodep) {
        যদি (!nodep) প্রত্যাগম nullptr;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = সত্য;
        m_newStmtps = nullptr;
        iterate(nodep);
        প্রত্যাগম m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_গ্রন্থি* liftChildren(Ast_গ্রন্থি_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = সত্য;
        m_newStmtps = nullptr;
        iterateChildren(nodep);
        প্রত্যাগম m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = অসত্য;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        যদি (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(AstCaseItem* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterateAndNextNull(nodep->stmtsp());
    }
    শূন্য visit(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    শূন্য visit(AstAlias* nodep) override {}
    শূন্য visit(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    শূন্য visit(AstWait* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        // Keep lifted statements inside the repeatedly evaluated condition.
        যদি (Ast_গ্রন্থি* ধ্রুব newStmtps = lift(nodep->condp())) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = nodep->condp()->unlinkFrBack();
            nodep->condp(নূতন Ast_রাশিমালা_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterateAndNextNull(nodep->stmtsp());
    }
    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        যদি (Ast_গ্রন্থি* ধ্রুব newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), গ্রন্থি_Var_Ref) ? nodep->rhsp() : nullptr;
        যদি (Ast_গ্রন্থি* ধ্রুব newStmtps = lift(nodep->rhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }

        যদি (nodep->timingControlp()) প্রত্যাগম;

        যদি (Ast_গ্রন্থি* ধ্রুব newStmtps = lift(nodep->lhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }
    }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        // Ignore super class constructor calls - can't insert statements before them
        যদি (VN_IS(nodep->exprp(), New)) প্রত্যাগম;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        যদি (Ast_গ্রন্থি* ধ্রুব newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    শূন্য visit(AstCStmt* nodep) override {}
    শূন্য visit(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা* nodep) override {
        যদি (!m_lift) প্রত্যাগম;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        যদি (m_doNotLiftp == nodep) প্রত্যাগম;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        যদি (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) প্রত্যাগম;
        // Do not lift if pure
        যদি (nodep->isPure()) প্রত্যাগম;
        // Do not lift if LValue
        যদি (nodep->isLValue()) প্রত্যাগম;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_সারি* ধ্রুব flp = nodep->fileline();
        AstVar* ধ্রুব varp = newVar("Expr", nodep);
        nodep->replaceWith(নূতন AstVarRef{flp, varp, VAccess::READ});
        addStmtps(নূতন AstAssign{flp, নূতন AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        যদি (!m_lift) প্রত্যাগম;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        যদি (m_doNotLiftp == nodep) প্রত্যাগম;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        যদি (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) প্রত্যাগম;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        যদি (VN_IS(nodep, TaskRef)) প্রত্যাগম;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_সারি* ধ্রুব flp = nodep->fileline();
        AstVar* ধ্রুব varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(নূতন AstVarRef{flp, varp, VAccess::READ});
        addStmtps(নূতন AstAssign{flp, নূতন AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    শূন্য visit(AstMemberSel* nodep) override {
        যদি (!m_lift) প্রত্যাগম;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), গ্রন্থি_FTask_Ref)
                           ? nodep->fromp()
                           : nullptr;
        iterateChildren(nodep);
        // Done if not a special case
        যদি (!m_doNotLiftp) প্রত্যাগম;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_সারি* ধ্রুব flp = nodep->fileline();
        AstVar* ধ্রুব varp = newVar("LvalCall", nodep->fromp());
        addStmtps(নূতন AstAssign{flp, নূতন AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlinkFrBack()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(নূতন AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    শূন্য visit(AstCond* nodep) override {
        যদি (!m_lift) প্রত্যাগম;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = nullptr;

        ধ্রুব তার্কিক sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_বৃক্ষ(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_গ্রন্থি* ধ্রুব thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        যদি (sameType) varp = getExistingVar(nodep->thenp());
        যদি (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_গ্রন্থি* ধ্রুব elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        যদি (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        যদি (!thenStmtps && !elseStmtps) প্রত্যাগম;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_সারি* ধ্রুব flp = nodep->fileline();
        যদি (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* ধ্রুব ifp = নূতন AstIf{flp, nodep->condp()->unlinkFrBack()};
        addStmtps(ifp);
        ifp->addThensp(thenStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->thenp()));
        ifp->addElsesp(elseStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(নূতন AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstLogAnd* nodep) override {
        যদি (!m_lift) প্রত্যাগম;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        যদি (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_গ্রন্থি* ধ্রুব rhsStmtps = lift(nodep->rhsp());
        যদি (!rhsStmtps) প্রত্যাগম;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_সারি* ধ্রুব flp = nodep->fileline();
        যদি (!varp) varp = getExistingVar(nodep->rhsp());
        যদি (!varp) varp = newVar("LogAnd", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* ধ্রুব ifp = নূতন AstIf{flp, নূতন AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addThensp(rhsStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(নূতন AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstLogOr* nodep) override {
        যদি (!m_lift) প্রত্যাগম;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        যদি (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_গ্রন্থি* ধ্রুব rhsStmtps = lift(nodep->rhsp());
        যদি (!rhsStmtps) প্রত্যাগম;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_সারি* ধ্রুব flp = nodep->fileline();
        যদি (!varp) varp = getExistingVar(nodep->rhsp());
        যদি (!varp) varp = newVar("LogOr", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* ধ্রুব ifp = নূতন AstIf{flp, নূতন AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(নূতন AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstLogIf* nodep) override {
        যদি (!m_lift) প্রত্যাগম;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_গ্রন্থি* ধ্রুব rhsStmtps = lift(nodep->rhsp());
        যদি (!rhsStmtps) প্রত্যাগম;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_সারি* ধ্রুব flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        যদি (!varp) varp = newVar("LogIf", nodep);
        addStmtps(নূতন AstAssign{flp, নূতন AstVarRef{flp, varp, VAccess::WRITE},
                                নূতন AstLogNot{flp, nodep->lhsp()->unlinkFrBack()}});
        AstIf* ধ্রুব ifp = নূতন AstIf{flp, নূতন AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(নূতন AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override {
        যদি (!m_lift) প্রত্যাগম;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_রাশিমালা_Stmts;
        iterate(nodep->stmtsp());
        addStmtps(nodep->stmtsp()->unlinkFrBackWithNext());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    শূন্য visit(AstConst*) override {}
    শূন্য visit(AstVarRef*) override {}
    শূন্য visit(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    শূন্য visit(AstAssocSel* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    শূন্য visit(AstCExpr* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstCExprUser* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit Lift_রাশিমালা_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_বৃক্ষ();
        iterate(nodep);
        VIsCached::clear_Cache_বৃক্ষ();
        যদি (m_newStmtps) m_newStmtps->dump_বৃক্ষ_And_Next(std::cout, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_রাশিমালা_Visitor() override {
        V3Stats::addStat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::addStat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::addStat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::addStat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::addStat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::addStat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::addStat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::addStat("LiftExpr, lifted ExprStmt", m_stat_Lifted_রাশিমালা_Stmts);
        V3Stats::addStat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::addStat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

শূন্য V3_Lift_রাশিমালা::lift_রাশিমালা_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_রাশিমালা_Visitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("lift_expr", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
