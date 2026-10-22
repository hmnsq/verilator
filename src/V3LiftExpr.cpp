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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3LiftExpr.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Inst.h"
#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग Lift_अभिव्यक्ती_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    स्थिरांक VNUser1InUse m_user1InUse;

    // STATE
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    Ast_गाठ_FTask* m_ftaskp = nullptr;  // Current function/task
    तार्किक m_lift = असत्य;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_गाठ* m_newStmtps = nullptr;
    // Expressions in some special locations need not, or should not be lifted
    Ast_गाठ_अभिव्यक्ती* m_doNotLiftp = nullptr;
    माप_प्रकार m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_अभिव्यक्ती_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(स्थिरांक अक्षर* baseName, स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp,
                   स्थिरांक std::string& suffix = "") {
        // Reuse existing temporary if available
        जर (exprp->user1p()) {
            ++m_statTemporariesReused;
            परत VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += baseName;
        name += "_" + std::to_string(m_nTmps++);
        जर (!suffix.empty()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* स्थिरांक varp = नवीन AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->isInternal(सत्य);
        varp->noReset(सत्य);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        जर (m_ftaskp) {
            varp->funcLocal(सत्य);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } अन्यथा {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        परत varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) {
            जर (refp->varp()->user1()) परत refp->varp();
        }
        परत nullptr;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    AstAssign* assignIfDifferent(File_ओळ* flp, AstVar* varp, Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) {
            जर (refp->varp() == varp) परत nullptr;
        }
        परत नवीन AstAssign{flp, नवीन AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlinkFrBack()};
    }

    शून्य addStmtps(Ast_गाठ* stmtp) {
        जर (!stmtp) परत;
        // No need to process again, so mark
        साठी (Ast_गाठ* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_गाठ::addNext(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_गाठ* lift(Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (!nodep) परत nullptr;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = सत्य;
        m_newStmtps = nullptr;
        iterate(nodep);
        परत m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_गाठ* liftChildren(Ast_गाठ_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = सत्य;
        m_newStmtps = nullptr;
        iterateChildren(nodep);
        परत m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    शून्य visit(Ast_गाठ* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = असत्य;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        जर (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    शून्य visit(AstCaseItem* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterateAndNextNull(nodep->stmtsp());
    }
    शून्य visit(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    शून्य visit(AstAlias* nodep) override {}
    शून्य visit(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    शून्य visit(AstWait* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        // Keep lifted statements inside the repeatedly evaluated condition.
        जर (Ast_गाठ* स्थिरांक newStmtps = lift(nodep->condp())) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = nodep->condp()->unlinkFrBack();
            nodep->condp(नवीन Ast_अभिव्यक्ती_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterateAndNextNull(nodep->stmtsp());
    }
    शून्य visit(Ast_गाठ_Stmt* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = nullptr;
        जर (Ast_गाठ* स्थिरांक newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), गाठ_Var_Ref) ? nodep->rhsp() : nullptr;
        जर (Ast_गाठ* स्थिरांक newStmtps = lift(nodep->rhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }

        जर (nodep->timingControlp()) परत;

        जर (Ast_गाठ* स्थिरांक newStmtps = lift(nodep->lhsp())) {
            nodep->addHereThisAsNext(newStmtps);
        }
    }
    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override {
        जर (nodep->user1SetOnce()) परत;
        // Ignore super class constructor calls - can't insert statements before them
        जर (VN_IS(nodep->exprp(), New)) परत;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        जर (Ast_गाठ* स्थिरांक newStmtps = liftChildren(nodep)) nodep->addHereThisAsNext(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    शून्य visit(AstCStmt* nodep) override {}
    शून्य visit(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    शून्य visit(Ast_गाठ_अभिव्यक्ती* nodep) override {
        जर (!m_lift) परत;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        जर (m_doNotLiftp == nodep) परत;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        जर (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) परत;
        // Do not lift if pure
        जर (nodep->isPure()) परत;
        // Do not lift if LValue
        जर (nodep->isLValue()) परत;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstVar* स्थिरांक varp = newVar("Expr", nodep);
        nodep->replaceWith(नवीन AstVarRef{flp, varp, VAccess::READ});
        addStmtps(नवीन AstAssign{flp, नवीन AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        जर (!m_lift) परत;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        जर (m_doNotLiftp == nodep) परत;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        जर (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) परत;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        जर (VN_IS(nodep, TaskRef)) परत;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstVar* स्थिरांक varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(नवीन AstVarRef{flp, varp, VAccess::READ});
        addStmtps(नवीन AstAssign{flp, नवीन AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    शून्य visit(AstMemberSel* nodep) override {
        जर (!m_lift) परत;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), गाठ_FTask_Ref)
                           ? nodep->fromp()
                           : nullptr;
        iterateChildren(nodep);
        // Done if not a special case
        जर (!m_doNotLiftp) परत;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstVar* स्थिरांक varp = newVar("LvalCall", nodep->fromp());
        addStmtps(नवीन AstAssign{flp, नवीन AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlinkFrBack()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(नवीन AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    शून्य visit(AstCond* nodep) override {
        जर (!m_lift) परत;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = nullptr;

        स्थिरांक तार्किक sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_वृक्ष(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_गाठ* स्थिरांक thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        जर (sameType) varp = getExistingVar(nodep->thenp());
        जर (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_गाठ* स्थिरांक elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        जर (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        जर (!thenStmtps && !elseStmtps) परत;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        जर (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, nodep->condp()->unlinkFrBack()};
        addStmtps(ifp);
        ifp->addThensp(thenStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->thenp()));
        ifp->addElsesp(elseStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(नवीन AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstLogAnd* nodep) override {
        जर (!m_lift) परत;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        जर (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_गाठ* स्थिरांक rhsStmtps = lift(nodep->rhsp());
        जर (!rhsStmtps) परत;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        जर (!varp) varp = getExistingVar(nodep->rhsp());
        जर (!varp) varp = newVar("LogAnd", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, नवीन AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addThensp(rhsStmtps);
        ifp->addThensp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(नवीन AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstLogOr* nodep) override {
        जर (!m_lift) परत;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        जर (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_गाठ* स्थिरांक rhsStmtps = lift(nodep->rhsp());
        जर (!rhsStmtps) परत;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        जर (!varp) varp = getExistingVar(nodep->rhsp());
        जर (!varp) varp = newVar("LogOr", nodep);
        addStmtps(assignIfDifferent(flp, varp, nodep->lhsp()));
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, नवीन AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(नवीन AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstLogIf* nodep) override {
        जर (!m_lift) परत;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_गाठ* स्थिरांक rhsStmtps = lift(nodep->rhsp());
        जर (!rhsStmtps) परत;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        जर (!varp) varp = newVar("LogIf", nodep);
        addStmtps(नवीन AstAssign{flp, नवीन AstVarRef{flp, varp, VAccess::WRITE},
                                नवीन AstLogNot{flp, nodep->lhsp()->unlinkFrBack()}});
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, नवीन AstVarRef{flp, varp, VAccess::READ}};
        addStmtps(ifp);
        ifp->addElsesp(rhsStmtps);
        ifp->addElsesp(assignIfDifferent(flp, varp, nodep->rhsp()));
        nodep->replaceWith(नवीन AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        जर (!m_lift) परत;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_अभिव्यक्ती_Stmts;
        iterate(nodep->stmtsp());
        addStmtps(nodep->stmtsp()->unlinkFrBackWithNext());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    शून्य visit(AstConst*) override {}
    शून्य visit(AstVarRef*) override {}
    शून्य visit(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    शून्य visit(AstAssocSel* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    शून्य visit(AstCExpr* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstCExprUser* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit Lift_अभिव्यक्ती_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_वृक्ष();
        iterate(nodep);
        VIsCached::clear_Cache_वृक्ष();
        जर (m_newStmtps) m_newStmtps->dump_वृक्ष_And_Next(std::cout, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_अभिव्यक्ती_Visitor() override {
        V3Stats::addStat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::addStat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::addStat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::addStat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::addStat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::addStat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::addStat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::addStat("LiftExpr, lifted ExprStmt", m_stat_Lifted_अभिव्यक्ती_Stmts);
        V3Stats::addStat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::addStat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

शून्य V3_Lift_अभिव्यक्ती::lift_अभिव्यक्ती_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_अभिव्यक्ती_Visitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("lift_expr", 0, dump_वृक्ष_Either_Level() >= 3);
}
