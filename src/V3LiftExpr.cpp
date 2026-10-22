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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3LiftExpr.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3Inst.h"
#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి Lift_సమాసం_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // STATE
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    తార్కిక m_lift = అబద్ధం;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_కణుపు* m_newStmtps = శూన్యసూచిక;
    // Expressions in some special locations need not, or should not be lifted
    Ast_కణుపు_సమాసం* m_doNotLiftp = శూన్యసూచిక;
    కొలత_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_సమాసం_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(స్థిరరాశి అక్షరం* పునాది_Name, స్థిరరాశి Ast_కణుపు_సమాసం* exprp,
                   స్థిరరాశి std::string& suffix = "") {
        // Reuse existing temporary if available
        ఒకవేళ (exprp->user1p()) {
            ++m_statTemporariesReused;
            తిరిగి VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += పునాది_Name;
        name += "_" + std::గమ్యం_string(m_nTmps++);
        ఒకవేళ (!suffix.ఖాళీలు()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* స్థిరరాశి varp = కొత్త AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->is_అంతర్గత(నిజం);
        varp->noReset(నిజం);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        ఒకవేళ (m_ftaskp) {
            varp->funcLocal(నిజం);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } లేకపోతే {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        తిరిగి varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
            ఒకవేళ (refp->varp()->user1()) తిరిగి refp->varp();
        }
        తిరిగి శూన్యసూచిక;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    Ast_కేటాయించు* కేటాయించు_If_Different(File_పంక్తి* flp, AstVar* varp, Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
            ఒకవేళ (refp->varp() == varp) తిరిగి శూన్యసూచిక;
        }
        తిరిగి కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlink_Fr_వెనుక()};
    }

    శూన్య కూడు_Stmtps(Ast_కణుపు* stmtp) {
        ఒకవేళ (!stmtp) తిరిగి;
        // No need to process again, so mark
        కొరకు (Ast_కణుపు* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_కణుపు::కూడు_తదుపరి(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_కణుపు* lift(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (!nodep) తిరిగి శూన్యసూచిక;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = నిజం;
        m_newStmtps = శూన్యసూచిక;
        iterate(nodep);
        తిరిగి m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_కణుపు* liftChildren(Ast_కణుపు_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = నిజం;
        m_newStmtps = శూన్యసూచిక;
        iterateChildren(nodep);
        తిరిగి m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = అబద్ధం;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        ఒకవేళ (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterate_And_తదుపరి_Null(nodep->stmtsp());
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    శూన్య సందర్శించు(AstAlias* nodep) override {}
    శూన్య సందర్శించు(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    శూన్య సందర్శించు(AstWait* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = శూన్యసూచిక;
        // Keep lifted statements inside the repeatedly evaluated condition.
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి newStmtps = lift(nodep->condp())) {
            Ast_కణుపు_సమాసం* స్థిరరాశి condp = nodep->condp()->unlink_Fr_వెనుక();
            nodep->condp(కొత్త Ast_సమాసం_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterate_And_తదుపరి_Null(nodep->stmtsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = శూన్యసూచిక;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి newStmtps = liftChildren(nodep)) nodep->కూడు_ఇక్కడ_This_As_తదుపరి(newStmtps);
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), కణుపు_Var_Ref) ? nodep->rhsp() : శూన్యసూచిక;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి newStmtps = lift(nodep->rhsp())) {
            nodep->కూడు_ఇక్కడ_This_As_తదుపరి(newStmtps);
        }

        ఒకవేళ (nodep->timingControlp()) తిరిగి;

        ఒకవేళ (Ast_కణుపు* స్థిరరాశి newStmtps = lift(nodep->lhsp())) {
            nodep->కూడు_ఇక్కడ_This_As_తదుపరి(newStmtps);
        }
    }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        // Ignore super class constructor calls - can't insert statements before them
        ఒకవేళ (VN_IS(nodep->exprp(), New)) తిరిగి;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి newStmtps = liftChildren(nodep)) nodep->కూడు_ఇక్కడ_This_As_తదుపరి(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    శూన్య సందర్శించు(AstCStmt* nodep) override {}
    శూన్య సందర్శించు(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        ఒకవేళ (!m_lift) తిరిగి;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        ఒకవేళ (m_doNotLiftp == nodep) తిరిగి;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        ఒకవేళ (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) తిరిగి;
        // Do not lift if pure
        ఒకవేళ (nodep->is_శుద్ధ()) తిరిగి;
        // Do not lift if LValue
        ఒకవేళ (nodep->isLValue()) తిరిగి;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstVar* స్థిరరాశి varp = newVar("Expr", nodep);
        nodep->replaceWith(కొత్త AstVarRef{flp, varp, VAccess::READ});
        కూడు_Stmtps(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (!m_lift) తిరిగి;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        ఒకవేళ (m_doNotLiftp == nodep) తిరిగి;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        ఒకవేళ (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) తిరిగి;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        ఒకవేళ (VN_IS(nodep, TaskRef)) తిరిగి;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstVar* స్థిరరాశి varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(కొత్త AstVarRef{flp, varp, VAccess::READ});
        కూడు_Stmtps(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        ఒకవేళ (!m_lift) తిరిగి;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), కణుపు_FTask_Ref)
                           ? nodep->fromp()
                           : శూన్యసూచిక;
        iterateChildren(nodep);
        // Done if not a special case
        ఒకవేళ (!m_doNotLiftp) తిరిగి;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstVar* స్థిరరాశి varp = newVar("LvalCall", nodep->fromp());
        కూడు_Stmtps(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlink_Fr_వెనుక()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(కొత్త AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    శూన్య సందర్శించు(AstCond* nodep) override {
        ఒకవేళ (!m_lift) తిరిగి;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = శూన్యసూచిక;

        స్థిరరాశి తార్కిక sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_వృక్షం(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_కణుపు* స్థిరరాశి thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        ఒకవేళ (sameType) varp = getExistingVar(nodep->thenp());
        ఒకవేళ (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_కణుపు* స్థిరరాశి elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        ఒకవేళ (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        ఒకవేళ (!thenStmtps && !elseStmtps) తిరిగి;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ఒకవేళ (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, nodep->condp()->unlink_Fr_వెనుక()};
        కూడు_Stmtps(ifp);
        ifp->కూడు_Thensp(thenStmtps);
        ifp->కూడు_Thensp(కేటాయించు_If_Different(flp, varp, nodep->thenp()));
        ifp->కూడు_Elsesp(elseStmtps);
        ifp->కూడు_Elsesp(కేటాయించు_If_Different(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(కొత్త AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstLogAnd* nodep) override {
        ఒకవేళ (!m_lift) తిరిగి;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        ఒకవేళ (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_కణుపు* స్థిరరాశి rhsStmtps = lift(nodep->rhsp());
        ఒకవేళ (!rhsStmtps) తిరిగి;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ఒకవేళ (!varp) varp = getExistingVar(nodep->rhsp());
        ఒకవేళ (!varp) varp = newVar("LogAnd", nodep);
        కూడు_Stmtps(కేటాయించు_If_Different(flp, varp, nodep->lhsp()));
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, varp, VAccess::READ}};
        కూడు_Stmtps(ifp);
        ifp->కూడు_Thensp(rhsStmtps);
        ifp->కూడు_Thensp(కేటాయించు_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(కొత్త AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstLogOr* nodep) override {
        ఒకవేళ (!m_lift) తిరిగి;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        ఒకవేళ (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_కణుపు* స్థిరరాశి rhsStmtps = lift(nodep->rhsp());
        ఒకవేళ (!rhsStmtps) తిరిగి;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ఒకవేళ (!varp) varp = getExistingVar(nodep->rhsp());
        ఒకవేళ (!varp) varp = newVar("LogOr", nodep);
        కూడు_Stmtps(కేటాయించు_If_Different(flp, varp, nodep->lhsp()));
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, varp, VAccess::READ}};
        కూడు_Stmtps(ifp);
        ifp->కూడు_Elsesp(rhsStmtps);
        ifp->కూడు_Elsesp(కేటాయించు_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(కొత్త AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstLogIf* nodep) override {
        ఒకవేళ (!m_lift) తిరిగి;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_కణుపు* స్థిరరాశి rhsStmtps = lift(nodep->rhsp());
        ఒకవేళ (!rhsStmtps) తిరిగి;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        ఒకవేళ (!varp) varp = newVar("LogIf", nodep);
        కూడు_Stmtps(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, varp, VAccess::WRITE},
                                కొత్త AstLogNot{flp, nodep->lhsp()->unlink_Fr_వెనుక()}});
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, కొత్త AstVarRef{flp, varp, VAccess::READ}};
        కూడు_Stmtps(ifp);
        ifp->కూడు_Elsesp(rhsStmtps);
        ifp->కూడు_Elsesp(కేటాయించు_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(కొత్త AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        ఒకవేళ (!m_lift) తిరిగి;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_సమాసం_Stmts;
        iterate(nodep->stmtsp());
        కూడు_Stmtps(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlink_Fr_వెనుక());
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    శూన్య సందర్శించు(AstConst*) override {}
    శూన్య సందర్శించు(AstVarRef*) override {}
    శూన్య సందర్శించు(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    శూన్య సందర్శించు(AstAssocSel* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    శూన్య సందర్శించు(AstCExpr* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstCExprUser* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన Lift_సమాసం_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_వృక్షం();
        iterate(nodep);
        VIsCached::clear_Cache_వృక్షం();
        ఒకవేళ (m_newStmtps) m_newStmtps->dump_వృక్షం_And_తదుపరి(std::బయటకు, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_సమాసం_Visitor() override {
        V3Stats::కూడు_Stat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::కూడు_Stat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::కూడు_Stat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::కూడు_Stat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::కూడు_Stat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::కూడు_Stat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::కూడు_Stat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::కూడు_Stat("LiftExpr, lifted ExprStmt", m_stat_Lifted_సమాసం_Stmts);
        V3Stats::కూడు_Stat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::కూడు_Stat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

శూన్య V3_Lift_సమాసం::lift_సమాసం_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_సమాసం_Visitor{nodep}; }
    V3Global::dump_Check_Global_వృక్షం("lift_expr", 0, dump_వృక్షం_Either_Level() >= 3);
}
