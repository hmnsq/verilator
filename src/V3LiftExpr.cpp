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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3LiftExpr.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3Inst.h"
#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு Lift_கோவை_Visitor final : பொது VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    மாறிலி VNUser1InUse m_user1InUse;

    // STATE
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    தருக்க m_lift = பொய்;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_முனை* m_newStmtps = nullptr;
    // Expressions in some special locations need not, or should not be lifted
    Ast_முனை_கோவை* m_doNotLiftp = nullptr;
    size_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_கோவை_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(மாறிலி எழுத்து* baseName, மாறிலி Ast_முனை_கோவை* exprp,
                   மாறிலி std::string& suffix = "") {
        // Reuse existing temporary if available
        எனில் (exprp->user1p()) {
            ++m_statTemporariesReused;
            திரும்பு VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += baseName;
        name += "_" + std::to_string(m_nTmps++);
        எனில் (!suffix.empty()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* மாறிலி varp = புதிய AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->isInternal(உண்மை);
        varp->noReset(உண்மை);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        எனில் (m_ftaskp) {
            varp->funcLocal(உண்மை);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } இல்லையெனில் {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        திரும்பு varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_முனை_கோவை* nodep) {
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) {
            எனில் (refp->varp()->user1()) திரும்பு refp->varp();
        }
        திரும்பு nullptr;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    AstAssign* assignIfDifferent(File_வரி* flp, AstVar* varp, Ast_முனை_கோவை* nodep) {
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) {
            எனில் (refp->varp() == varp) திரும்பு nullptr;
        }
        திரும்பு புதிய AstAssign{flp, புதிய AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlinkFrBack()};
    }

    வெறுமை addStmtps(Ast_முனை* stmtp) {
        எனில் (!stmtp) திரும்பு;
        // No need to process again, so mark
        க்கு (Ast_முனை* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_முனை::addNext(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_முனை* lift(Ast_முனை_கோவை* nodep) {
        எனில் (!nodep) திரும்பு nullptr;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = உண்மை;
        m_newStmtps = nullptr;
        iterate(nodep);
        திரும்பு m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_முனை* liftChildren(Ast_முனை_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = உண்மை;
        m_newStmtps = nullptr;
        iterateChildren(nodep);
        திரும்பு m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    வெறுமை visit(Ast_முனை* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = பொய்;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        எனில் (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCaseItem* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterateAndNextNull(nodep->stmtsp());
    }
    வெறுமை visit(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    வெறுமை visit(AstAlias* nodep) override {}
    வெறுமை visit(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    வெறுமை visit(AstWait* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        // Keep lifted statements inside the repeatedly evaluated condition.
        எனில் (Ast_முனை* மாறிலி newStmtps = lift(nodep->condp())) {
            Ast_முனை_கோவை* மாறிலி condp = nodep->condp()->unlinkFrBack();
            nodep->condp(புதிய Ast_கோவை_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterateAndNextNull(nodep->stmtsp());
    }
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        எனில் (Ast_முனை* மாறிலி newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), முனை_Var_Ref) ? nodep->rhsp() : nullptr;
        எனில் (Ast_முனை* மாறிலி newStmtps = lift(nodep->rhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }

        எனில் (nodep->timingControlp()) திரும்பு;

        எனில் (Ast_முனை* மாறிலி newStmtps = lift(nodep->lhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }
    }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        // Ignore super class constructor calls - can't insert statements before them
        எனில் (VN_IS(nodep->exprp(), New)) திரும்பு;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        எனில் (Ast_முனை* மாறிலி newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    வெறுமை visit(AstCStmt* nodep) override {}
    வெறுமை visit(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    வெறுமை visit(Ast_முனை_கோவை* nodep) override {
        எனில் (!m_lift) திரும்பு;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        எனில் (m_doNotLiftp == nodep) திரும்பு;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        எனில் (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) திரும்பு;
        // Do not lift if pure
        எனில் (nodep->isPure()) திரும்பு;
        // Do not lift if LValue
        எனில் (nodep->isLValue()) திரும்பு;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_வரி* மாறிலி flp = nodep->fileline();
        AstVar* மாறிலி varp = newVar("Expr", nodep);
        nodep->replaceWith(புதிய AstVarRef{flp, varp, VAccess::READ});
        addStmtps(புதிய AstAssign{flp, புதிய AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        எனில் (!m_lift) திரும்பு;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        எனில் (m_doNotLiftp == nodep) திரும்பு;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        எனில் (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) திரும்பு;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        எனில் (VN_IS(nodep, TaskRef)) திரும்பு;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_வரி* மாறிலி flp = nodep->fileline();
        AstVar* மாறிலி varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(புதிய AstVarRef{flp, varp, VAccess::READ});
        addStmtps(புதிய AstAssign{flp, புதிய AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        எனில் (!m_lift) திரும்பு;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), முனை_FTask_Ref)
                           ? nodep->fromp()
                           : nullptr;
        iterateChildren(nodep);
        // Done if not a special case
        எனில் (!m_doNotLiftp) திரும்பு;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_வரி* மாறிலி flp = nodep->fileline();
        AstVar* மாறிலி varp = newVar("LvalCall", nodep->fromp());
        addStmtps(புதிய AstAssign{flp, புதிய AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlinkFrBack()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(புதிய AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    வெறுமை visit(AstCond* nodep) override {
        எனில் (!m_lift) திரும்பு;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = nullptr;

        மாறிலி தருக்க sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_மரம்(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_முனை* மாறிலி thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        எனில் (sameType) varp = getExistingVar(nodep->thenp());
        எனில் (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_முனை* மாறிலி elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        எனில் (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        எனில் (!thenStmtps && !elseStmtps) திரும்பு;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_வரி* மாறிலி flp = nodep->fileline();
        எனில் (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* மாறிலி ifp = புதிய AstIf{flp, nodep->condp()->unlinkFrBack()};
        addStmtps(ifp);
        ifp->addThensp(thenStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->thenp()));
        ifp->addElsesp(elseStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(புதிய AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstLogAnd* nodep) override {
        எனில் (!m_lift) திரும்பு;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        எனில் (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_முனை* மாறிலி rhsStmtps = lift(nodep->rhsp());
        எனில் (!rhsStmtps) திரும்பு;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_வரி* மாறிலி flp = nodep->fileline();
        எனில் (!varp) varp = getExistingVar(nodep->rhsp());
        எனில் (!varp) varp = newVar("LogAnd", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addThensp(rhsStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(புதிய AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstLogOr* nodep) override {
        எனில் (!m_lift) திரும்பு;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        எனில் (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_முனை* மாறிலி rhsStmtps = lift(nodep->rhsp());
        எனில் (!rhsStmtps) திரும்பு;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_வரி* மாறிலி flp = nodep->fileline();
        எனில் (!varp) varp = getExistingVar(nodep->rhsp());
        எனில் (!varp) varp = newVar("LogOr", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(புதிய AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstLogIf* nodep) override {
        எனில் (!m_lift) திரும்பு;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_முனை* மாறிலி rhsStmtps = lift(nodep->rhsp());
        எனில் (!rhsStmtps) திரும்பு;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_வரி* மாறிலி flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        எனில் (!varp) varp = newVar("LogIf", nodep);
        addStmtps(புதிய AstAssign{flp, புதிய AstVarRef{flp, varp, VAccess::WRITE},
                                புதிய AstLogNot{flp, nodep->lhsp()->unlinkFrBack()}});
        AstIf* மாறிலி ifp = புதிய AstIf{flp, புதிய AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(புதிய AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        எனில் (!m_lift) திரும்பு;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_கோவை_Stmts;
        iterate(nodep->stmtsp());
        addStmtps(nodep->stmtsp()->unlinkFrBackWithNext());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    வெறுமை visit(AstConst*) override {}
    வெறுமை visit(AstVarRef*) override {}
    வெறுமை visit(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    வெறுமை visit(AstAssocSel* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    வெறுமை visit(AstCExpr* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstCExprUser* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit Lift_கோவை_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_மரம்();
        iterate(nodep);
        VIsCached::clear_Cache_மரம்();
        எனில் (m_newStmtps) m_newStmtps->dump_மரம்_And_Next(std::cout, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_கோவை_Visitor() override {
        V3Stats::addStat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::addStat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::addStat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::addStat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::addStat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::addStat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::addStat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::addStat("LiftExpr, lifted ExprStmt", m_stat_Lifted_கோவை_Stmts);
        V3Stats::addStat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::addStat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

வெறுமை V3_Lift_கோவை::lift_கோவை_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_கோவை_Visitor{nodep}; }
    V3Global::dump_Check_Global_மரம்("lift_expr", 0, dump_மரம்_Either_Level() >= 3);
}
