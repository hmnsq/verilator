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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "verilatedos.h"

#ضمن "V3LiftExpr.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Inst.h"
#ضمن "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف Lift_عبارت_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstNodeStmt::user1()  -> bool. Statement already processed
    // AstVar::user1()       -> bool. Variable is a lifted temporary
    // AstNodeExpr::user1p() -> AstVar*. Existing temporary variable usable for this expression
    ثابت VNUser1_درون_Use m_user1_درون_Use;

    // STATE
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Current function/task
    منطقي m_lift = خطأ;  // Lift encountered expressions
    // Statements lifted out of current node. TODO: Make this an AstNodeStmt* after #6280
    Ast_گره* m_newStmtps = اشاره_تهی;
    // Expressions in some special locations need not, or should not be lifted
    Ast_گره_عبارت* m_doNotLiftp = اشاره_تهی;
    اندازه_t m_nTmps = 0;  // Sequence numbers for temporary variables
    // Statistics
    VDouble0 m_statLiftedExprs;
    VDouble0 m_statLiftedCalls;
    VDouble0 m_statLiftedLvalCalls;
    VDouble0 m_statLiftedConds;
    VDouble0 m_statLiftedLogAnds;
    VDouble0 m_statLiftedLogOrs;
    VDouble0 m_statLiftedLogIfs;
    VDouble0 m_stat_Lifted_عبارت_Stmts;
    VDouble0 m_statTemporariesCreated;
    VDouble0 m_statTemporariesReused;

    // METHODS
    AstVar* newVar(ثابت حرف* پایه_Name, ثابت Ast_گره_عبارت* exprp,
                   ثابت std::string& suffix = "") {
        // Reuse existing temporary if available
        إذا (exprp->user1p()) {
            ++m_statTemporariesReused;
            أرجع VN_AS(exprp->user1p(), Var);
        }

        // Need a separate name in functions vs modules, as module inlining
        // can otherwise cause spurious VARHIDDEN warnings
        std::string name = m_ftaskp ? "__Vlef" : "__Vlem";
        name += پایه_Name;
        name += "_" + std::مقصد_string(m_nTmps++);
        إذا (!suffix.خالی()) name += "__" + suffix;

        // Create variable
        ++m_statTemporariesCreated;
        AstVar* ثابت varp = جديد AstVar{exprp->fileline(), VVarType::MODULETEMP, name,
                                        exprp->dtypep()->skipRefp()};
        varp->is_داخلی(درست);
        varp->noReset(درست);
        varp->user1(1);  // Mark as lifted temporary so it can be reused

        إذا (m_ftaskp) {
            varp->funcLocal(درست);
            m_ftaskp->prependStmtsp(varp);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        } وإلا {
            m_modp->prependStmtsp(varp);
            // 'automatic' on a variable inside a class actually means 'member'
            varp->lifetime(VN_IS(m_modp, Class) ? VLifetime::STATIC_EXPLICIT
                                                : VLifetime::AUTOMATIC_EXPLICIT);
        }
        أرجع varp;
    }

    // If the expression is a reference to an existing temporary, return it, otherwise nullptr
    AstVar* getExistingVar(Ast_گره_عبارت* nodep) {
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            إذا (refp->varp()->user1()) أرجع refp->varp();
        }
        أرجع اشاره_تهی;
    }

    // Unlink and assign given expression to temporary, unless the expression
    // is a reference to the same temporary
    Ast_تخصیص_بده* تخصیص_بده_If_Different(File_سطر* flp, AstVar* varp, Ast_گره_عبارت* nodep) {
        إذا (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            إذا (refp->varp() == varp) أرجع اشاره_تهی;
        }
        أرجع جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, nodep->unlink_Fr_پشت()};
    }

    فراغ جمع_کن_Stmtps(Ast_گره* stmtp) {
        إذا (!stmtp) أرجع;
        // No need to process again, so mark
        لكل (Ast_گره* nodep = stmtp; nodep; nodep = nodep->nextp()) nodep->user1(1);
        m_newStmtps = Ast_گره::جمع_کن_بعدی(m_newStmtps, stmtp);
    }

    // Lift expressions from expression, return lifted statements
    Ast_گره* lift(Ast_گره_عبارت* nodep) {
        إذا (!nodep) أرجع اشاره_تهی;
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = درست;
        m_newStmtps = اشاره_تهی;
        iterate(nodep);
        أرجع m_newStmtps;
    }

    // Lift expressions from children of given statement, return lifted statements
    Ast_گره* liftChildren(Ast_گره_Stmt* nodep) {
        VL_RESTORER(m_lift);
        VL_RESTORER(m_newStmtps);
        m_lift = درست;
        m_newStmtps = اشاره_تهی;
        iterateChildren(nodep);
        أرجع m_newStmtps;
    }

    // VISITORS - non-statement, non-expression
    فراغ بازدید(Ast_گره* nodep) override {
        VL_RESTORER(m_lift);
        m_lift = خطأ;  // Conservatively do not lift if unknown construct
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        // Reset names on root module only (there can be classes nested in modules)
        إذا (!m_modp) m_nTmps = 0;
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_nTmps);
        m_ftaskp = nodep;
        m_nTmps = 0;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_Case_قلم* nodep) override {
        // Do not lift from the case expressions, there is nowhere to put them
        iterate_And_بعدی_Null(nodep->stmtsp());
    }
    فراغ بازدید(AstCell* nodep) override {
        // No need to fix up port connections, V3Tristate called pinReconnectSimple,
        // so all writes are to simple AstVars, assuming it did it right ...
    }
    فراغ بازدید(AstAlias* nodep) override {}
    فراغ بازدید(AstArg* nodep) override {
        // Lift argument expressions
        iterateChildren(nodep);
    }

    // VISITORS - statements
    فراغ بازدید(AstWait* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = اشاره_تهی;
        // Keep lifted statements inside the repeatedly evaluated condition.
        إذا (Ast_گره* ثابت newStmtps = lift(nodep->condp())) {
            Ast_گره_عبارت* ثابت condp = nodep->condp()->unlink_Fr_پشت();
            nodep->condp(جديد Ast_عبارت_Stmt{nodep->fileline(), newStmtps, condp});
        }
        iterate_And_بعدی_Null(nodep->stmtsp());
    }
    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        VL_RESTORER(m_doNotLiftp);
        m_doNotLiftp = اشاره_تهی;
        إذا (Ast_گره* ثابت newStmtps = liftChildren(nodep)) nodep->جمع_کن_اینجا_This_As_بعدی(newStmtps);
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift the RHS if this is already a simple assignment to a variable
        m_doNotLiftp = VN_IS(nodep->lhsp(), گره_Var_Ref) ? nodep->rhsp() : اشاره_تهی;
        إذا (Ast_گره* ثابت newStmtps = lift(nodep->rhsp())) {
            nodep->جمع_کن_اینجا_This_As_بعدی(newStmtps);
        }

        إذا (nodep->timingControlp()) أرجع;

        إذا (Ast_گره* ثابت newStmtps = lift(nodep->lhsp())) {
            nodep->جمع_کن_اینجا_This_As_بعدی(newStmtps);
        }
    }
    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        // Ignore super class constructor calls - can't insert statements before them
        إذا (VN_IS(nodep->exprp(), New)) أرجع;
        VL_RESTORER(m_doNotLiftp);
        // Do not lift if the expression itself. This AstStmtExpr is required to
        // throw away the return value if any, and V3Task can inline without using
        // AstExprStmt in this case. Can still lift all sub-expressions though.
        m_doNotLiftp = nodep->exprp();
        إذا (Ast_گره* ثابت newStmtps = liftChildren(nodep)) nodep->جمع_کن_اینجا_This_As_بعدی(newStmtps);
    }
    // Don't know whether these are sensitive to lifting, assume so
    فراغ بازدید(AstCStmt* nodep) override {}
    فراغ بازدید(AstCStmtUser* nodep) override {}

    // VISITORS - expressions
    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        إذا (!m_lift) أرجع;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        إذا (m_doNotLiftp == nodep) أرجع;
        // No need to lift void expressions, these should be under StmtExpr, but just in case ...
        إذا (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) أرجع;
        // Do not lift if pure
        إذا (nodep->is_خالص()) أرجع;
        // Do not lift if LValue
        إذا (nodep->isLValue()) أرجع;

        // Extract expression into a temporary variable
        ++m_statLiftedExprs;
        File_سطر* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("Expr", nodep);
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        جمع_کن_Stmtps(جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        إذا (!m_lift) أرجع;

        iterateChildren(nodep);

        // Do not lift if already in normal form
        إذا (m_doNotLiftp == nodep) أرجع;
        // No need to lift void functions, these should be under StmtExpr, but just in case ...
        إذا (VN_IS(nodep->dtypep()->skipRefp(), VoidDType)) أرجع;
        // Do not lift Taskref, it's always in statement position and cleanly inlineable.
        إذا (VN_IS(nodep, TaskRef)) أرجع;

        // Extract expression into a temporary variable
        ++m_statLiftedCalls;
        File_سطر* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("Call", nodep, nodep->taskp()->name());
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        جمع_کن_Stmtps(جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, nodep});
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        إذا (!m_lift) أرجع;
        // MemberSel is special as it can appear as an LValue selecting from a call
        // that returns a class handle. Note this is not in IEEE-1800, but is supported
        // for compatibility. Here we turn it into an IEEE-1800 compliant LValue by
        // lifting the call and use the return value via a temporary variable.

        VL_RESTORER(m_doNotLiftp);
        // If it's an LValue call, do not lift the fromp, will do it explicitly below
        m_doNotLiftp = nodep->access().isWriteOrRW() && VN_IS(nodep->fromp(), گره_FTask_Ref)
                           ? nodep->fromp()
                           : اشاره_تهی;
        iterateChildren(nodep);
        // Done if not a special case
        إذا (!m_doNotLiftp) أرجع;

        // Extract LValue call into a temporary variable
        ++m_statLiftedLvalCalls;
        File_سطر* ثابت flp = nodep->fileline();
        AstVar* ثابت varp = newVar("LvalCall", nodep->fromp());
        جمع_کن_Stmtps(جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, varp, VAccess::WRITE},
                                nodep->fromp()->unlink_Fr_پشت()});
        // This one is a WRITE or READWRITE, same as the MemberSel
        nodep->fromp(جديد AstVarRef{flp, varp, nodep->access()});
    }

    // VISITORS - RValue expressions
    فراغ بازدید(AstCond* nodep) override {
        إذا (!m_lift) أرجع;

        // Lift from condition
        iterate(nodep->condp());

        // Temporary variable to use for this expression
        AstVar* varp = اشاره_تهی;

        ثابت منطقي sameType
            = nodep->thenp()->dtypep()->skipRefp()->same_درخت(nodep->elsep()->dtypep()->skipRefp());

        // Lift from the Then branch
        Ast_گره* ثابت thenStmtps = lift(nodep->thenp());
        // If lifted the Then branch, we can reuse the temporary if same type
        إذا (sameType) varp = getExistingVar(nodep->thenp());
        إذا (varp) nodep->elsep()->user1p(varp);

        // Lift from the Else branch
        Ast_گره* ثابت elseStmtps = lift(nodep->elsep());
        // If lifted the Else branch, we can reuse the temporary if same type
        إذا (sameType && !varp) varp = getExistingVar(nodep->elsep());

        // If nothing lifted, then nothing to do
        إذا (!thenStmtps && !elseStmtps) أرجع;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedConds;
        File_سطر* ثابت flp = nodep->fileline();
        إذا (!varp) varp = newVar("Cond", nodep);
        // Create the AstIf
        AstIf* ثابت ifp = جديد AstIf{flp, nodep->condp()->unlink_Fr_پشت()};
        جمع_کن_Stmtps(ifp);
        ifp->جمع_کن_Thensp(thenStmtps);
        ifp->جمع_کن_Thensp(تخصیص_بده_If_Different(flp, varp, nodep->thenp()));
        ifp->جمع_کن_Elsesp(elseStmtps);
        ifp->جمع_کن_Elsesp(تخصیص_بده_If_Different(flp, varp, nodep->elsep()));
        // Replace the expression with a reference to the temporary variable
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstLogAnd* nodep) override {
        إذا (!m_lift) أرجع;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        إذا (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_گره* ثابت rhsStmtps = lift(nodep->rhsp());
        إذا (!rhsStmtps) أرجع;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogAnds;
        File_سطر* ثابت flp = nodep->fileline();
        إذا (!varp) varp = getExistingVar(nodep->rhsp());
        إذا (!varp) varp = newVar("LogAnd", nodep);
        جمع_کن_Stmtps(تخصیص_بده_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, varp, VAccess::READ}};
        جمع_کن_Stmtps(ifp);
        ifp->جمع_کن_Thensp(rhsStmtps);
        ifp->جمع_کن_Thensp(تخصیص_بده_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstLogOr* nodep) override {
        إذا (!m_lift) أرجع;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Temporary variable to use for this expression
        AstVar* varp = getExistingVar(nodep->lhsp());
        // If lifted the LHS, we can reuse the temporary
        إذا (varp) nodep->rhsp()->user1p(varp);

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_گره* ثابت rhsStmtps = lift(nodep->rhsp());
        إذا (!rhsStmtps) أرجع;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogOrs;
        File_سطر* ثابت flp = nodep->fileline();
        إذا (!varp) varp = getExistingVar(nodep->rhsp());
        إذا (!varp) varp = newVar("LogOr", nodep);
        جمع_کن_Stmtps(تخصیص_بده_If_Different(flp, varp, nodep->lhsp()));
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, varp, VAccess::READ}};
        جمع_کن_Stmtps(ifp);
        ifp->جمع_کن_Elsesp(rhsStmtps);
        ifp->جمع_کن_Elsesp(تخصیص_بده_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstLogIf* nodep) override {
        إذا (!m_lift) أرجع;

        // Lift from LHS
        iterate(nodep->lhsp());

        // Lift from RHS, if nothing lifted, then nothing to do
        Ast_گره* ثابت rhsStmtps = lift(nodep->rhsp());
        إذا (!rhsStmtps) أرجع;

        // Otherwise convert to an AstIf with a temporary variable
        ++m_statLiftedLogIfs;
        File_سطر* ثابت flp = nodep->fileline();
        AstVar* varp = getExistingVar(nodep->rhsp());
        إذا (!varp) varp = newVar("LogIf", nodep);
        جمع_کن_Stmtps(جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, varp, VAccess::WRITE},
                                جديد AstLogNot{flp, nodep->lhsp()->unlink_Fr_پشت()}});
        AstIf* ثابت ifp = جديد AstIf{flp, جديد AstVarRef{flp, varp, VAccess::READ}};
        جمع_کن_Stmtps(ifp);
        ifp->جمع_کن_Elsesp(rhsStmtps);
        ifp->جمع_کن_Elsesp(تخصیص_بده_If_Different(flp, varp, nodep->rhsp()));
        nodep->replaceWith(جديد AstVarRef{flp, varp, VAccess::READ});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(Ast_عبارت_Stmt* nodep) override {
        إذا (!m_lift) أرجع;

        // Eliminate AstExprStmt by lifting the content entirely
        ++m_stat_Lifted_عبارت_Stmts;
        iterate(nodep->stmtsp());
        جمع_کن_Stmtps(nodep->stmtsp()->unlink_Fr_پشت_With_بعدی());
        iterate(nodep->resultp());
        nodep->replaceWith(nodep->resultp()->unlink_Fr_پشت());
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    // VISITORS - Accelerate pure leaf expressions
    فراغ بازدید(AstConst*) override {}
    فراغ بازدید(AstVarRef*) override {}
    فراغ بازدید(AstVarXRef*) override {}

    // VISITORS - Expression special cases
    // These return C++ references rather than values, cannot be lifted
    فراغ بازدید(AstAssocSel* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(AstCMethodHard* nodep) override { iterateChildren(nodep); }
    // Don't know whether these may return non-values, assume so
    فراغ بازدید(AstCExpr* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(AstCExprUser* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح Lift_عبارت_Visitor(AstNetlist* nodep) {
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_درخت();
        iterate(nodep);
        VIsCached::clear_Cache_درخت();
        إذا (m_newStmtps) m_newStmtps->dump_درخت_And_بعدی(std::خروجی, "Leftover:");
        UASSERT_OBJ(!m_newStmtps, m_newStmtps, "Failed to insert statements");
    }
    ~Lift_عبارت_Visitor() override {
        V3Stats::جمع_کن_Stat("LiftExpr, lifted impure expressions", m_statLiftedExprs);
        V3Stats::جمع_کن_Stat("LiftExpr, lifted calls", m_statLiftedCalls);
        V3Stats::جمع_کن_Stat("LiftExpr, lifted calls as lvalue", m_statLiftedLvalCalls);
        V3Stats::جمع_کن_Stat("LiftExpr, lifted Cond", m_statLiftedConds);
        V3Stats::جمع_کن_Stat("LiftExpr, lifted LogAnd", m_statLiftedLogAnds);
        V3Stats::جمع_کن_Stat("LiftExpr, lifted LogOr", m_statLiftedLogOrs);
        V3Stats::جمع_کن_Stat("LiftExpr, lifted LogIf", m_statLiftedLogIfs);
        V3Stats::جمع_کن_Stat("LiftExpr, lifted ExprStmt", m_stat_Lifted_عبارت_Stmts);
        V3Stats::جمع_کن_Stat("LiftExpr, temporaries created", m_statTemporariesCreated);
        V3Stats::جمع_کن_Stat("LiftExpr, temporaries reused", m_statTemporariesReused);
    }
};

//######################################################################
// Unknown class functions

فراغ V3_Lift_عبارت::lift_عبارت_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Lift_عبارت_Visitor{nodep}; }
    V3Global::dump_Check_Global_درخت("lift_expr", 0, dump_درخت_Either_Level() >= 3);
}
