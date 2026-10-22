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
    ثابت VNUser1_دننه_Use m_inuser1;

    // STATE
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Function or task we're inside
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Module we're inside
    صحيح m_mod_Compound_Assignments_عدد = 0;  // Var name counter
    Ast_غوټه* m_insStmtp = تش_اشاره;  // Where to insert statement
    منطقي m_دننه_Covergroup = خطأ;  // Under a covergroup's coverpoint, cross, or option
    منطقي m_condEvalContext = خطأ;  // ++/-- is in a conditionally-evaluated position
    Ast_غوټه_بیان* m_incCondp = تش_اشاره;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    فراغ ورننباسه_On_سر(Ast_غوټه* newp) {
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
    فراغ ورننباسه_Before_Stmt(Ast_غوټه* nodep, Ast_غوټه* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->جمع_کړه_دلته_This_As_بل(newp);
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_عدد);
        m_modp = nodep;
        m_mod_Compound_Assignments_عدد = 0;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    فراغ لیدنه_Covergroup(Ast_غوټه* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_دننه_Covergroup);
        m_ftaskp = تش_اشاره;
        m_دننه_Covergroup = ریښتیا;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Func_Cov_توکی* nodep) override { لیدنه_Covergroup(nodep); }
    فراغ لیدنه(Ast_Cg_Option_ټاکل* nodep) override { لیدنه_Covergroup(nodep); }
    فراغ لیدنه(Ast_غوټه_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_بل_Null(nodep->propp());
        m_insStmtp = تش_اشاره;
        // Note: no iterating over sentreep here as they will be ignored anyway
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterate_And_بل_Null(assertp->failsp());
        } وإلا إذا (AstAssertIntrinsic* ثابت intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_بل_Null(intrinsicp->failsp());
        }
        iterate_And_بل_Null(nodep->passsp());
    }
    فراغ لیدنه(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = تش_اشاره;  // First thing should be new statement
        iterate_And_بل_Null(nodep->stmtsp());
        m_insStmtp = تش_اشاره;  // Next thing should be new statement
    }
    فراغ لیدنه(Ast_غوټه_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = تش_اشاره;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = تش_اشاره;  // Next thing should be new statement
    }
    فراغ لیدنه(Ast_Jump_بلاک* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = تش_اشاره;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = تش_اشاره;  // Next thing should be new statement
    }
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_بل_Null(nodep->condp());
        m_insStmtp = تش_اشاره;
        iterate_And_بل_Null(nodep->thensp());
        iterate_And_بل_Null(nodep->elsesp());
        m_insStmtp = تش_اشاره;
    }
    فراغ لیدنه(Ast_Case_توکی* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = ریښتیا;
            iterate_And_بل_Null(nodep->condsp());
        }
        m_insStmtp = تش_اشاره;  // Next thing should be new statement
        iterate_And_بل_Null(nodep->stmtsp());
    }
    فراغ لیدنه(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_بل_Null(nodep->lhsp());
        m_insStmtp = تش_اشاره;
        iterate_And_بل_Null(nodep->stmtsp());
        m_insStmtp = تش_اشاره;
    }
    فراغ لیدنه(AstEventControl* nodep) override {
        m_insStmtp = تش_اشاره;
        iterate_And_بل_Null(nodep->stmtsp());
        m_insStmtp = تش_اشاره;
    }
    فراغ لیدنه(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_بل_Null(nodep->condp());
        m_insStmtp = تش_اشاره;
        iterate_And_بل_Null(nodep->stmtsp());
        m_insStmtp = تش_اشاره;
    }
    فراغ لیدنه(AstBegin* nodep) override {
        m_insStmtp = تش_اشاره;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = تش_اشاره;  // Next thing should be new statement
    }
    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {
        Ast_غوټه_بیان* ثابت exprp = nodep->exprp();
        إذا (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            m_insStmtp = تش_اشاره;
            iterate(exprp);
            m_insStmtp = تش_اشاره;
            أرجع;
        }
        لیدنه(static_cast<Ast_غوټه_Stmt*>(nodep));
    }
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = تش_اشاره;  // Next thing should be new statement
    }
    فراغ unsupported_لیدنه(Ast_غوټه* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = ریښتیا;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = تش_اشاره;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_غوټه_بیان* capture_Logical_Lhs_منزل_لنډمهاله(Ast_غوټه_Biop* ثابت nodep) {
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
        ثابت string name = "__VincGate"s + cvt_منزل_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ماشوم_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_د_بیان{}, lhsp->clone_ونه(ریښتیا)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(ریښتیا);
        ورننباسه_On_سر(varp);
        Ast_غوټه* لنډمهاله_Initp = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        إذا (m_incCondp) {
            لنډمهاله_Initp = جديد AstIf{fl, m_incCondp->clone_ونه_خالص(ریښتیا), لنډمهاله_Initp, تش_اشاره};
        }
        ورننباسه_Before_Stmt(nodep, لنډمهاله_Initp);
        nodep->lhsp(جديد AstVarRef{fl, varp, VAccess::READ});
        أرجع جديد AstVarRef{fl, varp, VAccess::READ};
    }
    فراغ handleShortCircuit(Ast_غوټه_Biop* ثابت nodep, منطقي isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        إذا (nodep->user1_سېټ_Once()) أرجع;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_بل_Null(nodep->lhsp());
        m_condEvalContext = ریښتیا;
        Ast_غوټه_بیان* ownedCondp = تش_اشاره;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        إذا (nodep->rhsp()->exists([](ثابت Ast_غوټه* np) {
                أرجع VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, ټاکل_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_غوټه_بیان* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_ونه_خالص(ریښتیا)
                                       : capture_Logical_Lhs_منزل_لنډمهاله(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            إذا (isOr) lhsRefp = جديد AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? جديد AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_ونه_خالص(ریښتیا), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_بل_Null(nodep->rhsp());
        إذا (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_ونه(), ownedCondp);
    }
    فراغ لیدنه(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/خطأ); }
    فراغ لیدنه(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/ریښتیا); }
    فراغ لیدنه(AstLogEq* nodep) override { unsupported_لیدنه(nodep); }
    فراغ لیدنه(AstLogIf* nodep) override { unsupported_لیدنه(nodep); }
    فراغ لیدنه(AstCond* nodep) override { unsupported_لیدنه(nodep); }
    فراغ لیدنه(AstPropSpec* nodep) override { unsupported_لیدنه(nodep); }
    فراغ prepost_لیدنه(Ast_غوټه_Uniop* ثابت nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ثابت منطقي isStandaloneStmt = !m_insStmtp && !m_دننه_Covergroup;
        Ast_Sel_بټ* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_بټ);
        إذا (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), غوټه_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_لیدنه(nodep);
        } وإلا {
            إذا (isStandaloneStmt) {
                prepost_stmt_لیدنه(nodep);
            } وإلا {
                prepost_بیان_لیدنه(nodep);
            }
        }
    }
    Ast_غوټه_بیان* getOperationp(Ast_غوټه* ثابت nodep, Ast_غوټه_بیان* ثابت lhsp,
                               Ast_غوټه_بیان* ثابت rhsp) {
        إذا (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            أرجع جديد AstSub{nodep->fileline(), lhsp, rhsp};
        }
        إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            أرجع جديد Ast_جمع_کړه{nodep->fileline(), lhsp, rhsp};
        }
        إذا (Ast_ټاکل_Compound* assignp = VN_CAST(nodep, ټاکل_Compound)) {
            بدل (assignp->operation()) {
            حالة Ast_ټاکل_Compound::operation::جمع_کړه_2:
                أرجع جديد Ast_جمع_کړه{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::And:
                أرجع جديد AstAnd{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::Div:
                أرجع جديد AstDiv{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::ModDiv:
                أرجع جديد AstModDiv{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::Mul:
                أرجع جديد AstMul{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::Or: أرجع جديد AstOr{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::ښويدنه_L:
                أرجع جديد Ast_ښويدنه_L{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::ښويدنه_R:
                أرجع جديد Ast_ښويدنه_R{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::ښويدنه_RS:
                أرجع جديد Ast_ښويدنه_RS{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::Sub:
                أرجع جديد AstSub{nodep->fileline(), lhsp, rhsp};
            حالة Ast_ټاکل_Compound::operation::Xor:
                أرجع جديد AstXor{nodep->fileline(), lhsp, rhsp};
            افتراضي:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    فراغ prepost_stmt_sel_لیدنه(Ast_غوټه_Uniop* ثابت nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_کرښه* fl = nodep->fileline();
        V3_شمېره عدد_One{fl, 32, 1, خطأ};
        Ast_غوټه_بیان* ثابت exprp = جديد AstConst{nodep->fileline(), عدد_One};

        prepost_stmt_sel_لیدنه(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_لیدنه(Ast_ټاکل_Compound* ثابت nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_غوټه_بیان* ثابت exprp = nodep->rhsp()->unlink_Fr_شا();

        prepost_stmt_sel_لیدنه(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_لیدنه(Ast_غوټه* ثابت nodep, Ast_غوټه_بیان* ثابت lhsp,
                                Ast_غوټه_بیان* ثابت exprp) {
        Ast_Sel_بټ* ثابت rdSelbitp = VN_AS(lhsp, Sel_بټ);
        Ast_غوټه_Var_Ref* ثابت rdFromp = VN_AS(rdSelbitp->fromp()->clone_ونه_خالص(ریښتیا), غوټه_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_غوټه_بیان* ثابت rdBitp = rdSelbitp->bitp()->unlink_Fr_شا();
        Ast_Sel_بټ* ثابت wrSelbitp = VN_CAST(lhsp, Sel_بټ);
        Ast_غوټه_بیان* ثابت wrFromp = wrSelbitp->fromp()->unlink_Fr_شا();
        V3LinkLValue::link_LValue_سېټ(wrFromp);

        // Prepare a temporary variable
        File_کرښه* ثابت fl = nodep->fileline();
        ثابت string name = "__VtempIndex"s + cvt_منزل_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ماشوم_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_د_بیان{}, rdBitp->clone_ونه(ریښتیا)}};
        إذا (m_ftaskp) varp->funcLocal(ریښتیا);

        // Declare the variable
        ورننباسه_On_سر(varp);

        // Define what operation will we be doing
        Ast_ټاکل* ثابت varAssignp
            = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_غوټه* ثابت newp = varAssignp;

        Ast_غوټه_بیان* ثابت valuep
            = جديد Ast_Sel_بټ{fl, rdFromp, جديد AstVarRef{fl, varp, VAccess::READ}};
        Ast_غوټه_بیان* ثابت store_سر
            = جديد Ast_Sel_بټ{fl, wrFromp, جديد AstVarRef{fl, varp, VAccess::READ}};

        Ast_ټاکل* assignp
            = جديد Ast_ټاکل{nodep->fileline(), store_سر, getOperationp(nodep, valuep, exprp)};
        newp->جمع_کړه_بل(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ prepost_stmt_لیدنه(Ast_غوټه_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        Ast_غوټه_بیان* ثابت store_سر = nodep->lhsp()->clone_ونه_خالص(ریښتیا);
        Ast_غوټه_بیان* ثابت valuep = nodep->lhsp()->unlink_Fr_شا();
        File_کرښه* ثابت fl = nodep->fileline();
        V3_شمېره عدد_One{fl, 32, 1, خطأ};
        Ast_غوټه_بیان* ثابت exprp = جديد AstConst{nodep->fileline(), عدد_One};

        prepost_stmt_لیدنه(nodep, exprp, store_سر, valuep);
    }
    فراغ prepost_stmt_لیدنه(Ast_ټاکل_Compound* ثابت nodep) {
        Ast_غوټه_بیان* ثابت exprp = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت store_سر = nodep->lhsp()->clone_ونه_خالص(ریښتیا);
        Ast_غوټه_بیان* ثابت valuep = nodep->lhsp()->unlink_Fr_شا();

        prepost_stmt_لیدنه(nodep, exprp, store_سر, valuep);
    }
    فراغ prepost_stmt_لیدنه(Ast_غوټه* ثابت nodep, Ast_غوټه_بیان* ثابت exprp,
                            Ast_غوټه_بیان* ثابت store_سر, Ast_غوټه_بیان* ثابت valuep) {
        V3LinkLValue::link_LValue_سېټ(valuep, VAccess::READ);
        Ast_ټاکل* ثابت assignp
            = جديد Ast_ټاکل{nodep->fileline(), store_سر, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ prepost_بیان_لیدنه(Ast_غوټه_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        إذا (m_condEvalContext && !m_incCondp && !m_دننه_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            أرجع;
        }
        ثابت منطقي needGating = m_condEvalContext && m_incCondp && !m_دننه_Covergroup;
        Ast_غوټه_بیان* ثابت readp = nodep->lhsp();
        Ast_غوټه_بیان* ثابت writep = nodep->lhsp()->clone_ونه_خالص(ریښتیا);
        V3LinkLValue::link_LValue_سېټ(readp, VAccess::READ);

        File_کرښه* ثابت fl = nodep->fileline();
        V3_شمېره عدد_One{fl, 32, 1, خطأ};
        Ast_غوټه_بیان* ثابت newconstp = جديد AstConst{nodep->fileline(), عدد_One};

        // Prepare a temporary variable
        ثابت string name = "__Vincrement"s + cvt_منزل_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ماشوم_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_د_بیان{}, readp->clone_ونه_خالص(ریښتیا)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(ریښتیا);

        // Declare the variable
        ورننباسه_On_سر(varp);

        // Define what operation will we be doing
        Ast_غوټه_بیان* ثابت operp = getOperationp(nodep, readp->clone_ونه_خالص(ریښتیا), newconstp);

        Ast_غوټه* stmtsp = تش_اشاره;
        إذا (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ونه_خالص(ریښتیا)};
            Ast_غوټه* ثابت incp = جديد Ast_ټاکل{fl, writep, operp};
            إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->جمع_کړه_بل(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_ونه_خالص(ریښتیا)});
            }
            assignp->جمع_کړه_بل_دلته(جديد AstIf{fl, m_incCondp->clone_ونه_خالص(ریښتیا), incp, تش_اشاره});
            stmtsp = assignp;
        } وإلا إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_ټاکل* ثابت assignp
                = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->جمع_کړه_بل(جديد Ast_ټاکل{fl, writep, جديد AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } وإلا {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ونه_خالص(ریښتیا)};
            // Increment the original variable by one
            assignp->جمع_کړه_بل(جديد Ast_ټاکل{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_غوټه_بیان* newp = جديد AstVarRef{readp->fileline(), varp, VAccess::READ};
        إذا (m_دننه_Covergroup) {
            newp = جديد Ast_بیان_Stmt{fl, stmtsp, newp};
        } وإلا {
            ورننباسه_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstPreInc* nodep) override { prepost_لیدنه(nodep); }
    فراغ لیدنه(AstPostInc* nodep) override { prepost_لیدنه(nodep); }
    فراغ لیدنه(AstPreDec* nodep) override { prepost_لیدنه(nodep); }
    فراغ لیدنه(AstPostDec* nodep) override { prepost_لیدنه(nodep); }
    فراغ لیدنه(Ast_ټاکل_Compound* nodep) override {
        لیدنه(static_cast<Ast_غوټه_Stmt*>(nodep));
        Ast_Sel_بټ* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_بټ);
        إذا (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), غوټه_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_لیدنه(nodep);
        } وإلا {
            prepost_stmt_لیدنه(nodep);
        }
    }
    فراغ لیدنه(AstGenFor* nodep) override { iterateChildren(nodep); }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("linkinc", 0, dump_ونه_Either_Level() >= 3);
}
