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

वर्ग Lift_व्यञ्जकम्_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;

    // STATE
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current function/task
    तार्किक m_lift = असत्य;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_ग्रन्थिः* m_newStmtps = शून्यसूचकः;
    // Expressions in some special locations need not, or should not be lifted
    Ast_ग्रन्थिः_व्यञ्जकम्* m_doNotLiftp = शून्यसूचकः;
    आकार_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_व्यञ्जकम्_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(ध्रुव वर्ण* आधारः_Name, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                   ध्रुव std::string& suffix = "") {
        // Reuse existing temporary if available
        यदि (exprp->user1p()) {
            ++m_statTemporariesReused;
            प्रत्यागम VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += आधारः_Name;
        name += "_" + std::गन्तव्यम्_string(m_nTmps++);
        यदि (!suffix.रिक्तानि()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* ध्रुव varp = नूतन AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->is_आन्तरिकम्(सत्य);
        varp->noReset(सत्य);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        यदि (m_ftaskp) {
            varp->funcLocal(सत्य);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } अन्यथा {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        प्रत्यागम varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            यदि (refp->varp()->user1()) प्रत्यागम refp->varp();
        }
        प्रत्यागम शून्यसूचकः;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    Ast_नियोजय* नियोजय_If_Different(File_पङ्क्तिः* flp, AstVar* varp, Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            यदि (refp->varp() == varp) प्रत्यागम शून्यसूचकः;
        }
        प्रत्यागम नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlink_Fr_पृष्ठभागः()};
    }

    शून्य सङ्कलय_Stmtps(Ast_ग्रन्थिः* stmtp) {
        यदि (!stmtp) प्रत्यागम;
        // No need to process again, so mark
        प्रति (Ast_ग्रन्थिः* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_ग्रन्थिः* lift(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (!nodep) प्रत्यागम शून्यसूचकः;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = सत्य;
        m_newStmtps = शून्यसूचकः;
        iterate(nodep);
        प्रत्यागम m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_ग्रन्थिः* liftChildren(Ast_ग्रन्थिः_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = सत्य;
        m_newStmtps = शून्यसूचकः;
        iterateChildren(nodep);
        प्रत्यागम m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = असत्य;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        यदि (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCaseItem* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterate_And_अग्र_Null(nodep->stmtsp());
    }
    शून्य दर्शय(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    शून्य दर्शय(AstAlias* nodep) override {}
    शून्य दर्शय(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    शून्य दर्शय(AstWait* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = शून्यसूचकः;
        // Keep lifted statements inside the repeatedly evaluated condition.
        यदि (Ast_ग्रन्थिः* ध्रुव newStmtps = lift(nodep->condp())) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = nodep->condp()->unlink_Fr_पृष्ठभागः();
            nodep->condp(नूतन Ast_व्यञ्जकम्_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterate_And_अग्र_Null(nodep->stmtsp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = शून्यसूचकः;
        यदि (Ast_ग्रन्थिः* ध्रुव newStmtps = liftChildren(nodep)) nodep->सङ्कलय_अत्र_This_As_अग्र(newStmtps);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), ग्रन्थिः_Var_Ref) ? nodep->rhsp() : शून्यसूचकः;
        यदि (Ast_ग्रन्थिः* ध्रुव newStmtps = lift(nodep->rhsp())) {
            nodep->सङ्कलय_अत्र_This_As_अग्र(newStmtps);
        }

        यदि (nodep->timingControlp()) प्रत्यागम;

        यदि (Ast_ग्रन्थिः* ध्रुव newStmtps = lift(nodep->lhsp())) {
            nodep->सङ्कलय_अत्र_This_As_अग्र(newStmtps);
        }
    }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        // Ignore super class constructor calls - can't insert statements before them
        यदि (VN_IS(nodep->exprp(), New)) प्रत्यागम;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        यदि (Ast_ग्रन्थिः* ध्रुव newStmtps = liftChildren(nodep)) nodep->सङ्कलय_अत्र_This_As_अग्र(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    शून्य दर्शय(AstCStmt* nodep) override {}
    शून्य दर्शय(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        यदि (!m_lift) प्रत्यागम;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        यदि (m_doNotLiftp == nodep) प्रत्यागम;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        यदि (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) प्रत्यागम;
        // Do not lift if pure
        यदि (nodep->is_विशुद्धम्()) प्रत्यागम;
        // Do not lift if LValue
        यदि (nodep->isLValue()) प्रत्यागम;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstVar* ध्रुव varp = newVar("Expr", nodep);
        nodep->replaceWith(नूतन AstVarRef{flp, varp, VAccess::READ});
        सङ्कलय_Stmtps(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        यदि (!m_lift) प्रत्यागम;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        यदि (m_doNotLiftp == nodep) प्रत्यागम;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        यदि (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) प्रत्यागम;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        यदि (VN_IS(nodep, TaskRef)) प्रत्यागम;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstVar* ध्रुव varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(नूतन AstVarRef{flp, varp, VAccess::READ});
        सङ्कलय_Stmtps(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        यदि (!m_lift) प्रत्यागम;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), ग्रन्थिः_FTask_Ref)
                           ? nodep->fromp()
                           : शून्यसूचकः;
        iterateChildren(nodep);
        // Done if not a special case
        यदि (!m_doNotLiftp) प्रत्यागम;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstVar* ध्रुव varp = newVar("LvalCall", nodep->fromp());
        सङ्कलय_Stmtps(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlink_Fr_पृष्ठभागः()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(नूतन AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    शून्य दर्शय(AstCond* nodep) override {
        यदि (!m_lift) प्रत्यागम;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = शून्यसूचकः;

        ध्रुव तार्किक sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_वृक्षः(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_ग्रन्थिः* ध्रुव thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        यदि (sameType) varp = getExistingVar(nodep->thenp());
        यदि (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_ग्रन्थिः* ध्रुव elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        यदि (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        यदि (!thenStmtps && !elseStmtps) प्रत्यागम;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        यदि (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* ध्रुव ifp = नूतन AstIf{flp, nodep->condp()->unlink_Fr_पृष्ठभागः()};
        सङ्कलय_Stmtps(ifp);
        ifp->सङ्कलय_Thensp(thenStmtps);
        ifp->सङ्कलय_Thensp(नियोजय_If_Different(flp, varp, nodep->thenp()));
        ifp->सङ्कलय_Elsesp(elseStmtps);
        ifp->सङ्कलय_Elsesp(नियोजय_If_Different(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(नूतन AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstLogAnd* nodep) override {
        यदि (!m_lift) प्रत्यागम;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        यदि (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ग्रन्थिः* ध्रुव rhsStmtps = lift(nodep->rhsp());
        यदि (!rhsStmtps) प्रत्यागम;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        यदि (!varp) varp = getExistingVar(nodep->rhsp());
        यदि (!varp) varp = newVar("LogAnd", nodep);
        सङ्कलय_Stmtps(नियोजय_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, varp, VAccess::READ}};
        सङ्कलय_Stmtps(ifp);
        ifp->सङ्कलय_Thensp(rhsStmtps);
        ifp->सङ्कलय_Thensp(नियोजय_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(नूतन AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstLogOr* nodep) override {
        यदि (!m_lift) प्रत्यागम;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        यदि (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ग्रन्थिः* ध्रुव rhsStmtps = lift(nodep->rhsp());
        यदि (!rhsStmtps) प्रत्यागम;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        यदि (!varp) varp = getExistingVar(nodep->rhsp());
        यदि (!varp) varp = newVar("LogOr", nodep);
        सङ्कलय_Stmtps(नियोजय_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, varp, VAccess::READ}};
        सङ्कलय_Stmtps(ifp);
        ifp->सङ्कलय_Elsesp(rhsStmtps);
        ifp->सङ्कलय_Elsesp(नियोजय_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(नूतन AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstLogIf* nodep) override {
        यदि (!m_lift) प्रत्यागम;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_ग्रन्थिः* ध्रुव rhsStmtps = lift(nodep->rhsp());
        यदि (!rhsStmtps) प्रत्यागम;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        यदि (!varp) varp = newVar("LogIf", nodep);
        सङ्कलय_Stmtps(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE},
                                नूतन AstLogNot{flp, nodep->lhsp()->unlink_Fr_पृष्ठभागः()}});
        AstIf* ध्रुव ifp = नूतन AstIf{flp, नूतन AstVarRef{flp, varp, VAccess::READ}};
        सङ्कलय_Stmtps(ifp);
        ifp->सङ्कलय_Elsesp(rhsStmtps);
        ifp->सङ्कलय_Elsesp(नियोजय_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(नूतन AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        यदि (!m_lift) प्रत्यागम;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_व्यञ्जकम्_Stmts;
        iterate(nodep->stmtsp());
        सङ्कलय_Stmtps(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlink_Fr_पृष्ठभागः());
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    शून्य दर्शय(AstConst*) override {}
    शून्य दर्शय(AstVarRef*) override {}
    शून्य दर्शय(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    शून्य दर्शय(AstAssocSel* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    शून्य दर्शय(AstCExpr* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(AstCExprUser* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट Lift_व्यञ्जकम्_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_वृक्षः();
        iterate(nodep);
        VIsCached::clear_Cache_वृक्षः();
        यदि (m_newStmtps) m_newStmtps->dump_वृक्षः_And_अग्र(std::निर्गमः, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_व्यञ्जकम्_Visitor() override {
        V3Stats::सङ्कलय_Stat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::सङ्कलय_Stat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::सङ्कलय_Stat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::सङ्कलय_Stat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::सङ्कलय_Stat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::सङ्कलय_Stat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::सङ्कलय_Stat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::सङ्कलय_Stat("LiftExpr, lifted ExprStmt", m_stat_Lifted_व्यञ्जकम्_Stmts);
        V3Stats::सङ्कलय_Stat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::सङ्कलय_Stat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

शून्य V3_Lift_व्यञ्जकम्::lift_व्यञ्जकम्_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_व्यञ्जकम्_Visitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("lift_expr", 0, dump_वृक्षः_Either_Level() >= 3);
}
