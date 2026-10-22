// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Replace increments/decrements with new variables
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
// V3LinkInc's Transformations:
//
//      prepost_expr_visit
//        PREINC/PREDEC
//          Create a temporary __VIncrementX variable, assign the value of
//          the current variable value to it, substitute the current
//          variable with the temporary one in the statement.
//          Increment/decrement the original variable with by the given
//          value.
//        POSTINC/POSTDEC
//          Increment/decrement the current variable by the given value.
//          Create a temporary __VIncrementX variable, assign the value of
//          of the current variable (after the operation) to it. Substitute
//          The original variable with the temporary one in the statement.
//        In a covergroup expression, under no statement, the assignments and
//        the temporary, a class member, form an expression statement instead.
//
//      prepost_stmt_visit
//        PREINC/PREDEC/POSTINC/POSTDEC
//          Increment/decrement the current variable by the given value.
//          The order (pre/post) doesn't matter outside statements thus
//          the pre/post operations are treated equally and there is no
//          need for a temporary variable.
//
//      prepost_stmt_sel_visit
//        For e.g. 'array[something_with_side_eff]++', common in UVM etc
//        PREADD/PRESUB/POSTADD/POSTSUB
//          Create temporary with array index.
//          Increment/decrement using index of the temporary.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkInc.h"

#ضمن "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف LinkIncVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    ثابت VNUser1_درون_Use m_inuser1;

    // STATE
    Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Function or task we're inside
    Ast_گره_Module* m_modp = اشاره_تهی;  // Module we're inside
    صحيح m_mod_Compound_Assignments_رقم = 0;  // Var name counter
    Ast_گره* m_insStmtp = اشاره_تهی;  // Where to insert statement
    منطقي m_درون_Covergroup = خطأ;  // Under a covergroup's coverpoint, cross, or option
    منطقي m_condEvalContext = خطأ;  // ++/-- is in a conditionally-evaluated position
    Ast_گره_عبارت* m_incCondp = اشاره_تهی;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    فراغ درج_On_بالاترین(Ast_گره* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        إذا (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } وإلا {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            إذا (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    فراغ درج_Before_Stmt(Ast_گره* nodep, Ast_گره* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->جمع_کن_اینجا_This_As_بعدی(newp);
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_رقم);
        m_modp = nodep;
        m_mod_Compound_Assignments_رقم = 0;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    فراغ بازدید_Covergroup(Ast_گره* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_درون_Covergroup);
        m_ftaskp = اشاره_تهی;
        m_درون_Covergroup = درست;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_Func_Cov_قلم* nodep) override { بازدید_Covergroup(nodep); }
    فراغ بازدید(Ast_Cg_Option_تخصیص_بده* nodep) override { بازدید_Covergroup(nodep); }
    فراغ بازدید(Ast_گره_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_بعدی_Null(nodep->propp());
        m_insStmtp = اشاره_تهی;
        // Note: no iterating over sentreep here as they will be ignored anyway
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterate_And_بعدی_Null(assertp->failsp());
        } وإلا إذا (AstAssertIntrinsic* ثابت intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_بعدی_Null(intrinsicp->failsp());
        }
        iterate_And_بعدی_Null(nodep->passsp());
    }
    فراغ بازدید(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = اشاره_تهی;  // First thing should be new statement
        iterate_And_بعدی_Null(nodep->stmtsp());
        m_insStmtp = اشاره_تهی;  // Next thing should be new statement
    }
    فراغ بازدید(Ast_گره_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = اشاره_تهی;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = اشاره_تهی;  // Next thing should be new statement
    }
    فراغ بازدید(Ast_Jump_بلوک* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = اشاره_تهی;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = اشاره_تهی;  // Next thing should be new statement
    }
    فراغ بازدید(Ast_گره_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_بعدی_Null(nodep->condp());
        m_insStmtp = اشاره_تهی;
        iterate_And_بعدی_Null(nodep->thensp());
        iterate_And_بعدی_Null(nodep->elsesp());
        m_insStmtp = اشاره_تهی;
    }
    فراغ بازدید(Ast_Case_قلم* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = درست;
            iterate_And_بعدی_Null(nodep->condsp());
        }
        m_insStmtp = اشاره_تهی;  // Next thing should be new statement
        iterate_And_بعدی_Null(nodep->stmtsp());
    }
    فراغ بازدید(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_بعدی_Null(nodep->lhsp());
        m_insStmtp = اشاره_تهی;
        iterate_And_بعدی_Null(nodep->stmtsp());
        m_insStmtp = اشاره_تهی;
    }
    فراغ بازدید(AstEventControl* nodep) override {
        m_insStmtp = اشاره_تهی;
        iterate_And_بعدی_Null(nodep->stmtsp());
        m_insStmtp = اشاره_تهی;
    }
    فراغ بازدید(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_بعدی_Null(nodep->condp());
        m_insStmtp = اشاره_تهی;
        iterate_And_بعدی_Null(nodep->stmtsp());
        m_insStmtp = اشاره_تهی;
    }
    فراغ بازدید(AstBegin* nodep) override {
        m_insStmtp = اشاره_تهی;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = اشاره_تهی;  // Next thing should be new statement
    }
    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {
        Ast_گره_عبارت* ثابت exprp = nodep->exprp();
        إذا (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_پشت());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            m_insStmtp = اشاره_تهی;
            iterate(exprp);
            m_insStmtp = اشاره_تهی;
            أرجع;
        }
        بازدید(static_cast<Ast_گره_Stmt*>(nodep));
    }
    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = اشاره_تهی;  // Next thing should be new statement
    }
    فراغ unsupported_بازدید(Ast_گره* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = درست;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = اشاره_تهی;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_گره_عبارت* capture_Logical_Lhs_مقصد_موقت(Ast_گره_Biop* ثابت nodep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
        ثابت string name = "__VincGate"s + cvt_مقصد_Str(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_فرزند_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_از_عبارت{}, lhsp->clone_درخت(درست)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(درست);
        درج_On_بالاترین(varp);
        Ast_گره* موقت_Initp = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        إذا (m_incCondp) {
            موقت_Initp = جديد AstIf{fl, m_incCondp->clone_درخت_خالص(درست), موقت_Initp, اشاره_تهی};
        }
        درج_Before_Stmt(nodep, موقت_Initp);
        nodep->lhsp(جديد AstVarRef{fl, varp, VAccess::READ});
        أرجع جديد AstVarRef{fl, varp, VAccess::READ};
    }
    فراغ handleShortCircuit(Ast_گره_Biop* ثابت nodep, منطقي isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_بعدی_Null(nodep->lhsp());
        m_condEvalContext = درست;
        Ast_گره_عبارت* ownedCondp = اشاره_تهی;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        إذا (nodep->rhsp()->exists([](ثابت Ast_گره* np) {
                أرجع VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, تخصیص_بده_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_گره_عبارت* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_درخت_خالص(درست)
                                       : capture_Logical_Lhs_مقصد_موقت(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            إذا (isOr) lhsRefp = جديد AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? جديد AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_درخت_خالص(درست), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_بعدی_Null(nodep->rhsp());
        إذا (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_درخت(), ownedCondp);
    }
    فراغ بازدید(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/خطأ); }
    فراغ بازدید(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/درست); }
    فراغ بازدید(AstLogEq* nodep) override { unsupported_بازدید(nodep); }
    فراغ بازدید(AstLogIf* nodep) override { unsupported_بازدید(nodep); }
    فراغ بازدید(AstCond* nodep) override { unsupported_بازدید(nodep); }
    فراغ بازدید(AstPropSpec* nodep) override { unsupported_بازدید(nodep); }
    فراغ prepost_بازدید(Ast_گره_Uniop* ثابت nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ثابت منطقي isStandaloneStmt = !m_insStmtp && !m_درون_Covergroup;
        Ast_Sel_بیت* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_بیت);
        إذا (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), گره_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_بازدید(nodep);
        } وإلا {
            إذا (isStandaloneStmt) {
                prepost_stmt_بازدید(nodep);
            } وإلا {
                prepost_عبارت_بازدید(nodep);
            }
        }
    }
    Ast_گره_عبارت* getOperationp(Ast_گره* ثابت nodep, Ast_گره_عبارت* ثابت lhsp,
                               Ast_گره_عبارت* ثابت rhsp) {
        إذا (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            أرجع جديد AstSub{nodep->fileline(), lhsp, rhsp};
        }
        إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            أرجع جديد Ast_جمع_کن{nodep->fileline(), lhsp, rhsp};
        }
        إذا (Ast_تخصیص_بده_Compound* assignp = VN_CAST(nodep, تخصیص_بده_Compound)) {
            بدل (assignp->operation()) {
            حالة Ast_تخصیص_بده_Compound::operation::جمع_کن_2:
                أرجع جديد Ast_جمع_کن{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::And:
                أرجع جديد AstAnd{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::Div:
                أرجع جديد AstDiv{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::ModDiv:
                أرجع جديد AstModDiv{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::Mul:
                أرجع جديد AstMul{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::Or: أرجع جديد AstOr{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::لغزش_L:
                أرجع جديد Ast_لغزش_L{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::لغزش_R:
                أرجع جديد Ast_لغزش_R{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::لغزش_RS:
                أرجع جديد Ast_لغزش_RS{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::Sub:
                أرجع جديد AstSub{nodep->fileline(), lhsp, rhsp};
            حالة Ast_تخصیص_بده_Compound::operation::Xor:
                أرجع جديد AstXor{nodep->fileline(), lhsp, rhsp};
            افتراضي:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    فراغ prepost_stmt_sel_بازدید(Ast_گره_Uniop* ثابت nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_سطر* fl = nodep->fileline();
        V3_عدد رقم_One{fl, 32, 1, خطأ};
        Ast_گره_عبارت* ثابت exprp = جديد AstConst{nodep->fileline(), رقم_One};

        prepost_stmt_sel_بازدید(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_بازدید(Ast_تخصیص_بده_Compound* ثابت nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_گره_عبارت* ثابت exprp = nodep->rhsp()->unlink_Fr_پشت();

        prepost_stmt_sel_بازدید(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_بازدید(Ast_گره* ثابت nodep, Ast_گره_عبارت* ثابت lhsp,
                                Ast_گره_عبارت* ثابت exprp) {
        Ast_Sel_بیت* ثابت rdSelbitp = VN_AS(lhsp, Sel_بیت);
        Ast_گره_Var_Ref* ثابت rdFromp = VN_AS(rdSelbitp->fromp()->clone_درخت_خالص(درست), گره_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_گره_عبارت* ثابت rdBitp = rdSelbitp->bitp()->unlink_Fr_پشت();
        Ast_Sel_بیت* ثابت wrSelbitp = VN_CAST(lhsp, Sel_بیت);
        Ast_گره_عبارت* ثابت wrFromp = wrSelbitp->fromp()->unlink_Fr_پشت();
        V3LinkLValue::link_LValue_مجموعه(wrFromp);

        // Prepare a temporary variable
        File_سطر* ثابت fl = nodep->fileline();
        ثابت string name = "__VtempIndex"s + cvt_مقصد_Str(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_فرزند_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_از_عبارت{}, rdBitp->clone_درخت(درست)}};
        إذا (m_ftaskp) varp->funcLocal(درست);

        // Declare the variable
        درج_On_بالاترین(varp);

        // Define what operation will we be doing
        Ast_تخصیص_بده* ثابت varAssignp
            = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_گره* ثابت newp = varAssignp;

        Ast_گره_عبارت* ثابت valuep
            = جديد Ast_Sel_بیت{fl, rdFromp, جديد AstVarRef{fl, varp, VAccess::READ}};
        Ast_گره_عبارت* ثابت store_بالاترین
            = جديد Ast_Sel_بیت{fl, wrFromp, جديد AstVarRef{fl, varp, VAccess::READ}};

        Ast_تخصیص_بده* assignp
            = جديد Ast_تخصیص_بده{nodep->fileline(), store_بالاترین, getOperationp(nodep, valuep, exprp)};
        newp->جمع_کن_بعدی(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ prepost_stmt_بازدید(Ast_گره_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        Ast_گره_عبارت* ثابت store_بالاترین = nodep->lhsp()->clone_درخت_خالص(درست);
        Ast_گره_عبارت* ثابت valuep = nodep->lhsp()->unlink_Fr_پشت();
        File_سطر* ثابت fl = nodep->fileline();
        V3_عدد رقم_One{fl, 32, 1, خطأ};
        Ast_گره_عبارت* ثابت exprp = جديد AstConst{nodep->fileline(), رقم_One};

        prepost_stmt_بازدید(nodep, exprp, store_بالاترین, valuep);
    }
    فراغ prepost_stmt_بازدید(Ast_تخصیص_بده_Compound* ثابت nodep) {
        Ast_گره_عبارت* ثابت exprp = nodep->rhsp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت store_بالاترین = nodep->lhsp()->clone_درخت_خالص(درست);
        Ast_گره_عبارت* ثابت valuep = nodep->lhsp()->unlink_Fr_پشت();

        prepost_stmt_بازدید(nodep, exprp, store_بالاترین, valuep);
    }
    فراغ prepost_stmt_بازدید(Ast_گره* ثابت nodep, Ast_گره_عبارت* ثابت exprp,
                            Ast_گره_عبارت* ثابت store_بالاترین, Ast_گره_عبارت* ثابت valuep) {
        V3LinkLValue::link_LValue_مجموعه(valuep, VAccess::READ);
        Ast_تخصیص_بده* ثابت assignp
            = جديد Ast_تخصیص_بده{nodep->fileline(), store_بالاترین, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ prepost_عبارت_بازدید(Ast_گره_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        إذا (m_condEvalContext && !m_incCondp && !m_درون_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            أرجع;
        }
        ثابت منطقي needGating = m_condEvalContext && m_incCondp && !m_درون_Covergroup;
        Ast_گره_عبارت* ثابت readp = nodep->lhsp();
        Ast_گره_عبارت* ثابت writep = nodep->lhsp()->clone_درخت_خالص(درست);
        V3LinkLValue::link_LValue_مجموعه(readp, VAccess::READ);

        File_سطر* ثابت fl = nodep->fileline();
        V3_عدد رقم_One{fl, 32, 1, خطأ};
        Ast_گره_عبارت* ثابت newconstp = جديد AstConst{nodep->fileline(), رقم_One};

        // Prepare a temporary variable
        ثابت string name = "__Vincrement"s + cvt_مقصد_Str(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_فرزند_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_از_عبارت{}, readp->clone_درخت_خالص(درست)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(درست);

        // Declare the variable
        درج_On_بالاترین(varp);

        // Define what operation will we be doing
        Ast_گره_عبارت* ثابت operp = getOperationp(nodep, readp->clone_درخت_خالص(درست), newconstp);

        Ast_گره* stmtsp = اشاره_تهی;
        إذا (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_تخصیص_بده* ثابت assignp = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_درخت_خالص(درست)};
            Ast_گره* ثابت incp = جديد Ast_تخصیص_بده{fl, writep, operp};
            إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->جمع_کن_بعدی(جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_درخت_خالص(درست)});
            }
            assignp->جمع_کن_بعدی_اینجا(جديد AstIf{fl, m_incCondp->clone_درخت_خالص(درست), incp, اشاره_تهی});
            stmtsp = assignp;
        } وإلا إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_تخصیص_بده* ثابت assignp
                = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->جمع_کن_بعدی(جديد Ast_تخصیص_بده{fl, writep, جديد AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } وإلا {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_تخصیص_بده* ثابت assignp = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_درخت_خالص(درست)};
            // Increment the original variable by one
            assignp->جمع_کن_بعدی(جديد Ast_تخصیص_بده{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_گره_عبارت* newp = جديد AstVarRef{readp->fileline(), varp, VAccess::READ};
        إذا (m_درون_Covergroup) {
            newp = جديد Ast_عبارت_Stmt{fl, stmtsp, newp};
        } وإلا {
            درج_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstPreInc* nodep) override { prepost_بازدید(nodep); }
    فراغ بازدید(AstPostInc* nodep) override { prepost_بازدید(nodep); }
    فراغ بازدید(AstPreDec* nodep) override { prepost_بازدید(nodep); }
    فراغ بازدید(AstPostDec* nodep) override { prepost_بازدید(nodep); }
    فراغ بازدید(Ast_تخصیص_بده_Compound* nodep) override {
        بازدید(static_cast<Ast_گره_Stmt*>(nodep));
        Ast_Sel_بیت* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_بیت);
        إذا (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), گره_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_بازدید(nodep);
        } وإلا {
            prepost_stmt_بازدید(nodep);
        }
    }
    فراغ بازدید(AstGenFor* nodep) override { iterateChildren(nodep); }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkinc", 0, dump_درخت_Either_Level() >= 3);
}
