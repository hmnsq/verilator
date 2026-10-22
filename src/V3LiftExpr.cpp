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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3LiftExpr.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3Inst.h"
#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ Lift_પદાવલિ_Visitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    અચલ VNUser1InUse m_user1InUse;

    // STATE
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    તાર્કિક m_lift = ખોટું;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_ગાંઠ* m_newStmtps = nullptr;
    // Expressions in some special locations need not, or should not be lifted
    Ast_ગાંઠ_પદાવલિ* m_doNotLiftp = nullptr;
    size_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_પદાવલિ_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(અચલ અક્ષર* baseName, અચલ Ast_ગાંઠ_પદાવલિ* exprp,
                   અચલ std::string& suffix = "") {
        // Reuse existing temporary if available
        જો (exprp->user1p()) {
            ++m_statTemporariesReused;
            પાછા VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += baseName;
        name += "_" + std::to_string(m_nTmps++);
        જો (!suffix.empty()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* અચલ varp = નવું AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->isInternal(સાચું);
        varp->noReset(સાચું);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        જો (m_ftaskp) {
            varp->funcLocal(સાચું);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } નહીંતર {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        પાછા varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) {
            જો (refp->varp()->user1()) પાછા refp->varp();
        }
        પાછા nullptr;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    AstAssign* assignIfDifferent(File_લીટી* flp, AstVar* varp, Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) {
            જો (refp->varp() == varp) પાછા nullptr;
        }
        પાછા નવું AstAssign{flp, નવું AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlinkFrBack()};
    }

    શૂન્ય addStmtps(Ast_ગાંઠ* stmtp) {
        જો (!stmtp) પાછા;
        // No need to process again, so mark
        માટે (Ast_ગાંઠ* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_ગાંઠ::addNext(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_ગાંઠ* lift(Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (!nodep) પાછા nullptr;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = સાચું;
        m_newStmtps = nullptr;
        iterate(nodep);
        પાછા m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_ગાંઠ* liftChildren(Ast_ગાંઠ_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = સાચું;
        m_newStmtps = nullptr;
        iterateChildren(nodep);
        પાછા m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = ખોટું;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        જો (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCaseItem* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterateAndNextNull(nodep->stmtsp());
    }
    શૂન્ય visit(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    શૂન્ય visit(AstAlias* nodep) override {}
    શૂન્ય visit(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    શૂન્ય visit(AstWait* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        // Keep lifted statements inside the repeatedly evaluated condition.
        જો (Ast_ગાંઠ* અચલ newStmtps = lift(nodep->condp())) {
            Ast_ગાંઠ_પદાવલિ* અચલ condp = nodep->condp()->unlinkFrBack();
            nodep->condp(નવું Ast_પદાવલિ_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterateAndNextNull(nodep->stmtsp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        જો (Ast_ગાંઠ* અચલ newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), ગાંઠ_Var_Ref) ? nodep->rhsp() : nullptr;
        જો (Ast_ગાંઠ* અચલ newStmtps = lift(nodep->rhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }

        જો (nodep->timingControlp()) પાછા;

        જો (Ast_ગાંઠ* અચલ newStmtps = lift(nodep->lhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }
    }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        // Ignore super class constructor calls - can't insert statements before them
        જો (VN_IS(nodep->exprp(), New)) પાછા;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        જો (Ast_ગાંઠ* અચલ newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    શૂન્ય visit(AstCStmt* nodep) override {}
    શૂન્ય visit(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ* nodep) override {
        જો (!m_lift) પાછા;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        જો (m_doNotLiftp == nodep) પાછા;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        જો (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) પાછા;
        // Do not lift if pure
        જો (nodep->isPure()) પાછા;
        // Do not lift if LValue
        જો (nodep->isLValue()) પાછા;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_લીટી* અચલ flp = nodep->fileline();
        AstVar* અચલ varp = newVar("Expr", nodep);
        nodep->replaceWith(નવું AstVarRef{flp, varp, VAccess::READ});
        addStmtps(નવું AstAssign{flp, નવું AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        જો (!m_lift) પાછા;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        જો (m_doNotLiftp == nodep) પાછા;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        જો (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) પાછા;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        જો (VN_IS(nodep, TaskRef)) પાછા;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_લીટી* અચલ flp = nodep->fileline();
        AstVar* અચલ varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(નવું AstVarRef{flp, varp, VAccess::READ});
        addStmtps(નવું AstAssign{flp, નવું AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        જો (!m_lift) પાછા;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), ગાંઠ_FTask_Ref)
                           ? nodep->fromp()
                           : nullptr;
        iterateChildren(nodep);
        // Done if not a special case
        જો (!m_doNotLiftp) પાછા;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_લીટી* અચલ flp = nodep->fileline();
        AstVar* અચલ varp = newVar("LvalCall", nodep->fromp());
        addStmtps(નવું AstAssign{flp, નવું AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlinkFrBack()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(નવું AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    શૂન્ય visit(AstCond* nodep) override {
        જો (!m_lift) પાછા;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = nullptr;

        અચલ તાર્કિક sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_વૃક્ષ(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_ગાંઠ* અચલ thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        જો (sameType) varp = getExistingVar(nodep->thenp());
        જો (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_ગાંઠ* અચલ elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        જો (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        જો (!thenStmtps && !elseStmtps) પાછા;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_લીટી* અચલ flp = nodep->fileline();
        જો (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* અચલ ifp = નવું AstIf{flp, nodep->condp()->unlinkFrBack()};
        addStmtps(ifp);
        ifp->addThensp(thenStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->thenp()));
        ifp->addElsesp(elseStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(નવું AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstLogAnd* nodep) override {
        જો (!m_lift) પાછા;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        જો (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ગાંઠ* અચલ rhsStmtps = lift(nodep->rhsp());
        જો (!rhsStmtps) પાછા;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_લીટી* અચલ flp = nodep->fileline();
        જો (!varp) varp = getExistingVar(nodep->rhsp());
        જો (!varp) varp = newVar("LogAnd", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* અચલ ifp = નવું AstIf{flp, નવું AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addThensp(rhsStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(નવું AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstLogOr* nodep) override {
        જો (!m_lift) પાછા;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        જો (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ગાંઠ* અચલ rhsStmtps = lift(nodep->rhsp());
        જો (!rhsStmtps) પાછા;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_લીટી* અચલ flp = nodep->fileline();
        જો (!varp) varp = getExistingVar(nodep->rhsp());
        જો (!varp) varp = newVar("LogOr", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* અચલ ifp = નવું AstIf{flp, નવું AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(નવું AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstLogIf* nodep) override {
        જો (!m_lift) પાછા;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ગાંઠ* અચલ rhsStmtps = lift(nodep->rhsp());
        જો (!rhsStmtps) પાછા;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_લીટી* અચલ flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        જો (!varp) varp = newVar("LogIf", nodep);
        addStmtps(નવું AstAssign{flp, નવું AstVarRef{flp, varp, VAccess::WRITE},
                                નવું AstLogNot{flp, nodep->lhsp()->unlinkFrBack()}});
        AstIf* અચલ ifp = નવું AstIf{flp, નવું AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(નવું AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        જો (!m_lift) પાછા;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_પદાવલિ_Stmts;
        iterate(nodep->stmtsp());
        addStmtps(nodep->stmtsp()->unlinkFrBackWithNext());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    શૂન્ય visit(AstConst*) override {}
    શૂન્ય visit(AstVarRef*) override {}
    શૂન્ય visit(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    શૂન્ય visit(AstAssocSel* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    શૂન્ય visit(AstCExpr* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstCExprUser* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit Lift_પદાવલિ_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_વૃક્ષ();
        iterate(nodep);
        VIsCached::clear_Cache_વૃક્ષ();
        જો (m_newStmtps) m_newStmtps->dump_વૃક્ષ_And_Next(std::cout, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_પદાવલિ_Visitor() override {
        V3Stats::addStat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::addStat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::addStat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::addStat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::addStat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::addStat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::addStat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::addStat("LiftExpr, lifted ExprStmt", m_stat_Lifted_પદાવલિ_Stmts);
        V3Stats::addStat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::addStat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

શૂન્ય V3_Lift_પદાવલિ::lift_પદાવલિ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_પદાવલિ_Visitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("lift_expr", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
