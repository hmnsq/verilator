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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3LiftExpr.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Inst.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ Lift_ପଦସମଷ୍ଟି_Visitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    ଅଚଳ VNUser1InUse m_user1InUse;

    // STATE
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    ତାର୍କିକ m_lift = ମିଥ୍ୟା;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_ଗଣ୍ଠି* m_newStmtps = nullptr;
    // Expressions in some special locations need not, or should not be lifted
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_doNotLiftp = nullptr;
    size_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_ପଦସମଷ୍ଟି_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(ଅଚଳ ଅକ୍ଷର* baseName, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                   ଅଚଳ std::string& suffix = "") {
        // Reuse existing temporary if available
        ଯଦି (exprp->user1p()) {
            ++m_statTemporariesReused;
            ଫେରନ୍ତୁ VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += baseName;
        name += "_" + std::to_string(m_nTmps++);
        ଯଦି (!suffix.empty()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->isInternal(ସତ୍ୟ);
        varp->noReset(ସତ୍ୟ);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        ଯଦି (m_ftaskp) {
            varp->funcLocal(ସତ୍ୟ);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } ଅନ୍ୟଥା {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        ଫେରନ୍ତୁ varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
            ଯଦି (refp->varp()->user1()) ଫେରନ୍ତୁ refp->varp();
        }
        ଫେରନ୍ତୁ nullptr;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    AstAssign* assignIfDifferent(File_ଧାଡ଼ି* flp, AstVar* varp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
            ଯଦି (refp->varp() == varp) ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlinkFrBack()};
    }

    ଶୂନ୍ୟ addStmtps(Ast_ଗଣ୍ଠି* stmtp) {
        ଯଦି (!stmtp) ଫେରନ୍ତୁ;
        // No need to process again, so mark
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_ଗଣ୍ଠି::addNext(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_ଗଣ୍ଠି* lift(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ nullptr;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = ସତ୍ୟ;
        m_newStmtps = nullptr;
        iterate(nodep);
        ଫେରନ୍ତୁ m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_ଗଣ୍ଠି* liftChildren(Ast_ଗଣ୍ଠି_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = ସତ୍ୟ;
        m_newStmtps = nullptr;
        iterateChildren(nodep);
        ଫେରନ୍ତୁ m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = ମିଥ୍ୟା;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        ଯଦି (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterateAndNextNull(nodep->stmtsp());
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    ଶୂନ୍ୟ visit(AstAlias* nodep) override {}
    ଶୂନ୍ୟ visit(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    ଶୂନ୍ୟ visit(AstWait* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        // Keep lifted statements inside the repeatedly evaluated condition.
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ newStmtps = lift(nodep->condp())) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = nodep->condp()->unlinkFrBack();
            nodep->condp(ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterateAndNextNull(nodep->stmtsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), ଗଣ୍ଠି_Var_Ref) ? nodep->rhsp() : nullptr;
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ newStmtps = lift(nodep->rhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }

        ଯଦି (nodep->timingControlp()) ଫେରନ୍ତୁ;

        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ newStmtps = lift(nodep->lhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }
    }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        // Ignore super class constructor calls - can't insert statements before them
        ଯଦି (VN_IS(nodep->exprp(), New)) ଫେରନ୍ତୁ;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    ଶୂନ୍ୟ visit(AstCStmt* nodep) override {}
    ଶୂନ୍ୟ visit(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (!m_lift) ଫେରନ୍ତୁ;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        ଯଦି (m_doNotLiftp == nodep) ଫେରନ୍ତୁ;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        ଯଦି (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) ଫେରନ୍ତୁ;
        // Do not lift if pure
        ଯଦି (nodep->isPure()) ଫେରନ୍ତୁ;
        // Do not lift if LValue
        ଯଦି (nodep->isLValue()) ଫେରନ୍ତୁ;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstVar* ଅଚଳ varp = newVar("Expr", nodep);
        nodep->replaceWith(ନୂତନ AstVarRef{flp, varp, VAccess::READ});
        addStmtps(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (!m_lift) ଫେରନ୍ତୁ;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        ଯଦି (m_doNotLiftp == nodep) ଫେରନ୍ତୁ;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        ଯଦି (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) ଫେରନ୍ତୁ;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        ଯଦି (VN_IS(nodep, TaskRef)) ଫେରନ୍ତୁ;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstVar* ଅଚଳ varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(ନୂତନ AstVarRef{flp, varp, VAccess::READ});
        addStmtps(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଯଦି (!m_lift) ଫେରନ୍ତୁ;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), ଗଣ୍ଠି_FTask_Ref)
                           ? nodep->fromp()
                           : nullptr;
        iterateChildren(nodep);
        // Done if not a special case
        ଯଦି (!m_doNotLiftp) ଫେରନ୍ତୁ;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstVar* ଅଚଳ varp = newVar("LvalCall", nodep->fromp());
        addStmtps(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlinkFrBack()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(ନୂତନ AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        ଯଦି (!m_lift) ଫେରନ୍ତୁ;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = nullptr;

        ଅଚଳ ତାର୍କିକ sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_ବୃକ୍ଷ(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_ଗଣ୍ଠି* ଅଚଳ thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        ଯଦି (sameType) varp = getExistingVar(nodep->thenp());
        ଯଦି (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_ଗଣ୍ଠି* ଅଚଳ elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        ଯଦି (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        ଯଦି (!thenStmtps && !elseStmtps) ଫେରନ୍ତୁ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଯଦି (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, nodep->condp()->unlinkFrBack()};
        addStmtps(ifp);
        ifp->addThensp(thenStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->thenp()));
        ifp->addElsesp(elseStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(ନୂତନ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstLogAnd* nodep) override {
        ଯଦି (!m_lift) ଫେରନ୍ତୁ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        ଯଦି (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ଗଣ୍ଠି* ଅଚଳ rhsStmtps = lift(nodep->rhsp());
        ଯଦି (!rhsStmtps) ଫେରନ୍ତୁ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଯଦି (!varp) varp = getExistingVar(nodep->rhsp());
        ଯଦି (!varp) varp = newVar("LogAnd", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addThensp(rhsStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ନୂତନ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstLogOr* nodep) override {
        ଯଦି (!m_lift) ଫେରନ୍ତୁ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        ଯଦି (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ଗଣ୍ଠି* ଅଚଳ rhsStmtps = lift(nodep->rhsp());
        ଯଦି (!rhsStmtps) ଫେରନ୍ତୁ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଯଦି (!varp) varp = getExistingVar(nodep->rhsp());
        ଯଦି (!varp) varp = newVar("LogOr", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ନୂତନ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstLogIf* nodep) override {
        ଯଦି (!m_lift) ଫେରନ୍ତୁ;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ଗଣ୍ଠି* ଅଚଳ rhsStmtps = lift(nodep->rhsp());
        ଯଦି (!rhsStmtps) ଫେରନ୍ତୁ;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        ଯଦି (!varp) varp = newVar("LogIf", nodep);
        addStmtps(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, varp, VAccess::WRITE},
                                ନୂତନ AstLogNot{flp, nodep->lhsp()->unlinkFrBack()}});
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, ନୂତନ AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(ନୂତନ AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        ଯଦି (!m_lift) ଫେରନ୍ତୁ;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_ପଦସମଷ୍ଟି_Stmts;
        iterate(nodep->stmtsp());
        addStmtps(nodep->stmtsp()->unlinkFrBackWithNext());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    ଶୂନ୍ୟ visit(AstConst*) override {}
    ଶୂନ୍ୟ visit(AstVarRef*) override {}
    ଶୂନ୍ୟ visit(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    ଶୂନ୍ୟ visit(AstAssocSel* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    ଶୂନ୍ୟ visit(AstCExpr* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstCExprUser* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit Lift_ପଦସମଷ୍ଟି_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_ବୃକ୍ଷ();
        iterate(nodep);
        VIsCached::clear_Cache_ବୃକ୍ଷ();
        ଯଦି (m_newStmtps) m_newStmtps->dump_ବୃକ୍ଷ_And_Next(std::cout, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_ପଦସମଷ୍ଟି_Visitor() override {
        V3Stats::addStat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::addStat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::addStat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::addStat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::addStat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::addStat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::addStat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::addStat("LiftExpr, lifted ExprStmt", m_stat_Lifted_ପଦସମଷ୍ଟି_Stmts);
        V3Stats::addStat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::addStat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

ଶୂନ୍ୟ V3_Lift_ପଦସମଷ୍ଟି::lift_ପଦସମଷ୍ଟି_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_ପଦସମଷ୍ଟି_Visitor{nodep}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("lift_expr", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
