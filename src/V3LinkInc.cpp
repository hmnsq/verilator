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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkInc.h"

#شامل "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت LinkIncVisitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    ثابت VNUser1_اندر_Use m_inuser1;

    // STATE
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Function or task we're inside
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Module we're inside
    صحیح m_mod_Compound_Assignments_عدد = 0;  // Var name counter
    Ast_گرہ* m_insStmtp = خالی_اشاریہ;  // Where to insert statement
    منطقی m_اندر_Covergroup = جھوٹ;  // Under a covergroup's coverpoint, cross, or option
    منطقی m_condEvalContext = جھوٹ;  // ++/-- is in a conditionally-evaluated position
    Ast_گرہ_اظہاریہ* m_incCondp = خالی_اشاریہ;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    خالی داخل_کرو_On_چوٹی(Ast_گرہ* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        اگر (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } ورنہ {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            اگر (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    خالی داخل_کرو_Before_Stmt(Ast_گرہ* nodep, Ast_گرہ* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->جوڑو_یہاں_This_As_اگلا(newp);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        اگر (nodep->dead()) واپس;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_عدد);
        m_modp = nodep;
        m_mod_Compound_Assignments_عدد = 0;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    خالی دورہ_Covergroup(Ast_گرہ* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_اندر_Covergroup);
        m_ftaskp = خالی_اشاریہ;
        m_اندر_Covergroup = سچ;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Func_Cov_شے* nodep) override { دورہ_Covergroup(nodep); }
    خالی دورہ(Ast_Cg_Option_تفویض* nodep) override { دورہ_Covergroup(nodep); }
    خالی دورہ(Ast_گرہ_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_اگلا_Null(nodep->propp());
        m_insStmtp = خالی_اشاریہ;
        // Note: no iterating over sentreep here as they will be ignored anyway
        اگر (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterate_And_اگلا_Null(assertp->failsp());
        } ورنہ اگر (AstAssertIntrinsic* ثابت intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_اگلا_Null(intrinsicp->failsp());
        }
        iterate_And_اگلا_Null(nodep->passsp());
    }
    خالی دورہ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = خالی_اشاریہ;  // First thing should be new statement
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_insStmtp = خالی_اشاریہ;  // Next thing should be new statement
    }
    خالی دورہ(Ast_گرہ_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = خالی_اشاریہ;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = خالی_اشاریہ;  // Next thing should be new statement
    }
    خالی دورہ(Ast_Jump_بلاک* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = خالی_اشاریہ;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = خالی_اشاریہ;  // Next thing should be new statement
    }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_اگلا_Null(nodep->condp());
        m_insStmtp = خالی_اشاریہ;
        iterate_And_اگلا_Null(nodep->thensp());
        iterate_And_اگلا_Null(nodep->elsesp());
        m_insStmtp = خالی_اشاریہ;
    }
    خالی دورہ(Ast_Case_شے* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = سچ;
            iterate_And_اگلا_Null(nodep->condsp());
        }
        m_insStmtp = خالی_اشاریہ;  // Next thing should be new statement
        iterate_And_اگلا_Null(nodep->stmtsp());
    }
    خالی دورہ(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_اگلا_Null(nodep->lhsp());
        m_insStmtp = خالی_اشاریہ;
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_insStmtp = خالی_اشاریہ;
    }
    خالی دورہ(AstEventControl* nodep) override {
        m_insStmtp = خالی_اشاریہ;
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_insStmtp = خالی_اشاریہ;
    }
    خالی دورہ(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_اگلا_Null(nodep->condp());
        m_insStmtp = خالی_اشاریہ;
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_insStmtp = خالی_اشاریہ;
    }
    خالی دورہ(AstBegin* nodep) override {
        m_insStmtp = خالی_اشاریہ;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = خالی_اشاریہ;  // Next thing should be new statement
    }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp();
        اگر (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_عقب());
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            m_insStmtp = خالی_اشاریہ;
            iterate(exprp);
            m_insStmtp = خالی_اشاریہ;
            واپس;
        }
        دورہ(static_cast<Ast_گرہ_Stmt*>(nodep));
    }
    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = خالی_اشاریہ;  // Next thing should be new statement
    }
    خالی unsupported_دورہ(Ast_گرہ* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = سچ;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = خالی_اشاریہ;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_گرہ_اظہاریہ* capture_Logical_Lhs_مقصد_عارضی(Ast_گرہ_Biop* ثابت nodep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_عقب();
        ثابت string name = "__VincGate"s + cvt_مقصد_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = نیا AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_بچہ_DType{},
            نیا AstRefDType{fl, AstRefDType::Flag_Type_کا_اظہاریہ{}, lhsp->clone_درخت(سچ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        اگر (m_ftaskp) varp->funcLocal(سچ);
        داخل_کرو_On_چوٹی(varp);
        Ast_گرہ* عارضی_Initp = نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        اگر (m_incCondp) {
            عارضی_Initp = نیا AstIf{fl, m_incCondp->clone_درخت_خالص(سچ), عارضی_Initp, خالی_اشاریہ};
        }
        داخل_کرو_Before_Stmt(nodep, عارضی_Initp);
        nodep->lhsp(نیا AstVarRef{fl, varp, VAccess::READ});
        واپس نیا AstVarRef{fl, varp, VAccess::READ};
    }
    خالی handleShortCircuit(Ast_گرہ_Biop* ثابت nodep, منطقی isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        اگر (nodep->user1_سیٹ_Once()) واپس;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_اگلا_Null(nodep->lhsp());
        m_condEvalContext = سچ;
        Ast_گرہ_اظہاریہ* ownedCondp = خالی_اشاریہ;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        اگر (nodep->rhsp()->exists([](ثابت Ast_گرہ* np) {
                واپس VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, تفویض_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_گرہ_اظہاریہ* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_درخت_خالص(سچ)
                                       : capture_Logical_Lhs_مقصد_عارضی(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            اگر (isOr) lhsRefp = نیا AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? نیا AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_درخت_خالص(سچ), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_اگلا_Null(nodep->rhsp());
        اگر (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_درخت(), ownedCondp);
    }
    خالی دورہ(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/جھوٹ); }
    خالی دورہ(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/سچ); }
    خالی دورہ(AstLogEq* nodep) override { unsupported_دورہ(nodep); }
    خالی دورہ(AstLogIf* nodep) override { unsupported_دورہ(nodep); }
    خالی دورہ(AstCond* nodep) override { unsupported_دورہ(nodep); }
    خالی دورہ(AstPropSpec* nodep) override { unsupported_دورہ(nodep); }
    خالی prepost_دورہ(Ast_گرہ_Uniop* ثابت nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ثابت منطقی isStandaloneStmt = !m_insStmtp && !m_اندر_Covergroup;
        Ast_Sel_ثنائی_ہندسہ* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_ثنائی_ہندسہ);
        اگر (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), گرہ_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_دورہ(nodep);
        } ورنہ {
            اگر (isStandaloneStmt) {
                prepost_stmt_دورہ(nodep);
            } ورنہ {
                prepost_اظہاریہ_دورہ(nodep);
            }
        }
    }
    Ast_گرہ_اظہاریہ* getOperationp(Ast_گرہ* ثابت nodep, Ast_گرہ_اظہاریہ* ثابت lhsp,
                               Ast_گرہ_اظہاریہ* ثابت rhsp) {
        اگر (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            واپس نیا AstSub{nodep->fileline(), lhsp, rhsp};
        }
        اگر (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            واپس نیا Ast_جوڑو{nodep->fileline(), lhsp, rhsp};
        }
        اگر (Ast_تفویض_Compound* assignp = VN_CAST(nodep, تفویض_Compound)) {
            انتخاب (assignp->operation()) {
            حالت Ast_تفویض_Compound::operation::جوڑو_2:
                واپس نیا Ast_جوڑو{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::And:
                واپس نیا AstAnd{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::Div:
                واپس نیا AstDiv{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::ModDiv:
                واپس نیا AstModDiv{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::Mul:
                واپس نیا AstMul{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::Or: واپس نیا AstOr{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::سرکاؤ_L:
                واپس نیا Ast_سرکاؤ_L{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::سرکاؤ_R:
                واپس نیا Ast_سرکاؤ_R{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::سرکاؤ_RS:
                واپس نیا Ast_سرکاؤ_RS{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::Sub:
                واپس نیا AstSub{nodep->fileline(), lhsp, rhsp};
            حالت Ast_تفویض_Compound::operation::Xor:
                واپس نیا AstXor{nodep->fileline(), lhsp, rhsp};
            طےشدہ:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    خالی prepost_stmt_sel_دورہ(Ast_گرہ_Uniop* ثابت nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_سطر* fl = nodep->fileline();
        V3_نمبر عدد_One{fl, 32, 1, جھوٹ};
        Ast_گرہ_اظہاریہ* ثابت exprp = نیا AstConst{nodep->fileline(), عدد_One};

        prepost_stmt_sel_دورہ(nodep, nodep->lhsp(), exprp);
    }
    خالی prepost_stmt_sel_دورہ(Ast_تفویض_Compound* ثابت nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->rhsp()->unlink_Fr_عقب();

        prepost_stmt_sel_دورہ(nodep, nodep->lhsp(), exprp);
    }
    خالی prepost_stmt_sel_دورہ(Ast_گرہ* ثابت nodep, Ast_گرہ_اظہاریہ* ثابت lhsp,
                                Ast_گرہ_اظہاریہ* ثابت exprp) {
        Ast_Sel_ثنائی_ہندسہ* ثابت rdSelbitp = VN_AS(lhsp, Sel_ثنائی_ہندسہ);
        Ast_گرہ_Var_Ref* ثابت rdFromp = VN_AS(rdSelbitp->fromp()->clone_درخت_خالص(سچ), گرہ_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_گرہ_اظہاریہ* ثابت rdBitp = rdSelbitp->bitp()->unlink_Fr_عقب();
        Ast_Sel_ثنائی_ہندسہ* ثابت wrSelbitp = VN_CAST(lhsp, Sel_ثنائی_ہندسہ);
        Ast_گرہ_اظہاریہ* ثابت wrFromp = wrSelbitp->fromp()->unlink_Fr_عقب();
        V3LinkLValue::link_LValue_سیٹ(wrFromp);

        // Prepare a temporary variable
        File_سطر* ثابت fl = nodep->fileline();
        ثابت string name = "__VtempIndex"s + cvt_مقصد_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = نیا AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_بچہ_DType{},
            نیا AstRefDType{fl, AstRefDType::Flag_Type_کا_اظہاریہ{}, rdBitp->clone_درخت(سچ)}};
        اگر (m_ftaskp) varp->funcLocal(سچ);

        // Declare the variable
        داخل_کرو_On_چوٹی(varp);

        // Define what operation will we be doing
        Ast_تفویض* ثابت varAssignp
            = نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_گرہ* ثابت newp = varAssignp;

        Ast_گرہ_اظہاریہ* ثابت valuep
            = نیا Ast_Sel_ثنائی_ہندسہ{fl, rdFromp, نیا AstVarRef{fl, varp, VAccess::READ}};
        Ast_گرہ_اظہاریہ* ثابت store_چوٹی
            = نیا Ast_Sel_ثنائی_ہندسہ{fl, wrFromp, نیا AstVarRef{fl, varp, VAccess::READ}};

        Ast_تفویض* assignp
            = نیا Ast_تفویض{nodep->fileline(), store_چوٹی, getOperationp(nodep, valuep, exprp)};
        newp->جوڑو_اگلا(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی prepost_stmt_دورہ(Ast_گرہ_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        Ast_گرہ_اظہاریہ* ثابت store_چوٹی = nodep->lhsp()->clone_درخت_خالص(سچ);
        Ast_گرہ_اظہاریہ* ثابت valuep = nodep->lhsp()->unlink_Fr_عقب();
        File_سطر* ثابت fl = nodep->fileline();
        V3_نمبر عدد_One{fl, 32, 1, جھوٹ};
        Ast_گرہ_اظہاریہ* ثابت exprp = نیا AstConst{nodep->fileline(), عدد_One};

        prepost_stmt_دورہ(nodep, exprp, store_چوٹی, valuep);
    }
    خالی prepost_stmt_دورہ(Ast_تفویض_Compound* ثابت nodep) {
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->rhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت store_چوٹی = nodep->lhsp()->clone_درخت_خالص(سچ);
        Ast_گرہ_اظہاریہ* ثابت valuep = nodep->lhsp()->unlink_Fr_عقب();

        prepost_stmt_دورہ(nodep, exprp, store_چوٹی, valuep);
    }
    خالی prepost_stmt_دورہ(Ast_گرہ* ثابت nodep, Ast_گرہ_اظہاریہ* ثابت exprp,
                            Ast_گرہ_اظہاریہ* ثابت store_چوٹی, Ast_گرہ_اظہاریہ* ثابت valuep) {
        V3LinkLValue::link_LValue_سیٹ(valuep, VAccess::READ);
        Ast_تفویض* ثابت assignp
            = نیا Ast_تفویض{nodep->fileline(), store_چوٹی, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی prepost_اظہاریہ_دورہ(Ast_گرہ_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        اگر (m_condEvalContext && !m_incCondp && !m_اندر_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            واپس;
        }
        ثابت منطقی needGating = m_condEvalContext && m_incCondp && !m_اندر_Covergroup;
        Ast_گرہ_اظہاریہ* ثابت readp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت writep = nodep->lhsp()->clone_درخت_خالص(سچ);
        V3LinkLValue::link_LValue_سیٹ(readp, VAccess::READ);

        File_سطر* ثابت fl = nodep->fileline();
        V3_نمبر عدد_One{fl, 32, 1, جھوٹ};
        Ast_گرہ_اظہاریہ* ثابت newconstp = نیا AstConst{nodep->fileline(), عدد_One};

        // Prepare a temporary variable
        ثابت string name = "__Vincrement"s + cvt_مقصد_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = نیا AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_بچہ_DType{},
            نیا AstRefDType{fl, AstRefDType::Flag_Type_کا_اظہاریہ{}, readp->clone_درخت_خالص(سچ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        اگر (m_ftaskp) varp->funcLocal(سچ);

        // Declare the variable
        داخل_کرو_On_چوٹی(varp);

        // Define what operation will we be doing
        Ast_گرہ_اظہاریہ* ثابت operp = getOperationp(nodep, readp->clone_درخت_خالص(سچ), newconstp);

        Ast_گرہ* stmtsp = خالی_اشاریہ;
        اگر (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_تفویض* ثابت assignp = نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_درخت_خالص(سچ)};
            Ast_گرہ* ثابت incp = نیا Ast_تفویض{fl, writep, operp};
            اگر (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->جوڑو_اگلا(نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_درخت_خالص(سچ)});
            }
            assignp->جوڑو_اگلا_یہاں(نیا AstIf{fl, m_incCondp->clone_درخت_خالص(سچ), incp, خالی_اشاریہ});
            stmtsp = assignp;
        } ورنہ اگر (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_تفویض* ثابت assignp
                = نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->جوڑو_اگلا(نیا Ast_تفویض{fl, writep, نیا AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } ورنہ {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_تفویض* ثابت assignp = نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_درخت_خالص(سچ)};
            // Increment the original variable by one
            assignp->جوڑو_اگلا(نیا Ast_تفویض{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_گرہ_اظہاریہ* newp = نیا AstVarRef{readp->fileline(), varp, VAccess::READ};
        اگر (m_اندر_Covergroup) {
            newp = نیا Ast_اظہاریہ_Stmt{fl, stmtsp, newp};
        } ورنہ {
            داخل_کرو_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(AstPreInc* nodep) override { prepost_دورہ(nodep); }
    خالی دورہ(AstPostInc* nodep) override { prepost_دورہ(nodep); }
    خالی دورہ(AstPreDec* nodep) override { prepost_دورہ(nodep); }
    خالی دورہ(AstPostDec* nodep) override { prepost_دورہ(nodep); }
    خالی دورہ(Ast_تفویض_Compound* nodep) override {
        دورہ(static_cast<Ast_گرہ_Stmt*>(nodep));
        Ast_Sel_ثنائی_ہندسہ* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_ثنائی_ہندسہ);
        اگر (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), گرہ_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_دورہ(nodep);
        } ورنہ {
            prepost_stmt_دورہ(nodep);
        }
    }
    خالی دورہ(AstGenFor* nodep) override { iterateChildren(nodep); }
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = طےشدہ;
};

//######################################################################
// Task class functions

خالی V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkinc", 0, dump_درخت_Either_Level() >= 3);
}
