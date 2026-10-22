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
    ثابت VNUser1_اندريون_Use m_inuser1;

    // STATE
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Function or task we're inside
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Module we're inside
    صحيح m_mod_Compound_Assignments_انگ = 0;  // Var name counter
    Ast_ڳنڍ* m_insStmtp = خالي_اشارو;  // Where to insert statement
    منطقي m_اندريون_Covergroup = خطأ;  // Under a covergroup's coverpoint, cross, or option
    منطقي m_condEvalContext = خطأ;  // ++/-- is in a conditionally-evaluated position
    Ast_ڳنڍ_اظهار* m_incCondp = خالي_اشارو;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    فراغ وجهو_On_چوٽي(Ast_ڳنڍ* newp) {
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
    فراغ وجهو_Before_Stmt(Ast_ڳنڍ* nodep, Ast_ڳنڍ* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->جوڙيو_هتي_This_As_ايندڙ(newp);
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_انگ);
        m_modp = nodep;
        m_mod_Compound_Assignments_انگ = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    فراغ گهمو_Covergroup(Ast_ڳنڍ* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_اندريون_Covergroup);
        m_ftaskp = خالي_اشارو;
        m_اندريون_Covergroup = سچ;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Func_Cov_شي* nodep) override { گهمو_Covergroup(nodep); }
    فراغ گهمو(Ast_Cg_Option_سونپيو* nodep) override { گهمو_Covergroup(nodep); }
    فراغ گهمو(Ast_ڳنڍ_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_ايندڙ_Null(nodep->propp());
        m_insStmtp = خالي_اشارو;
        // Note: no iterating over sentreep here as they will be ignored anyway
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterate_And_ايندڙ_Null(assertp->failsp());
        } وإلا إذا (AstAssertIntrinsic* ثابت intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_ايندڙ_Null(intrinsicp->failsp());
        }
        iterate_And_ايندڙ_Null(nodep->passsp());
    }
    فراغ گهمو(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = خالي_اشارو;  // First thing should be new statement
        iterate_And_ايندڙ_Null(nodep->stmtsp());
        m_insStmtp = خالي_اشارو;  // Next thing should be new statement
    }
    فراغ گهمو(Ast_ڳنڍ_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = خالي_اشارو;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = خالي_اشارو;  // Next thing should be new statement
    }
    فراغ گهمو(Ast_Jump_بلاڪ* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = خالي_اشارو;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = خالي_اشارو;  // Next thing should be new statement
    }
    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ايندڙ_Null(nodep->condp());
        m_insStmtp = خالي_اشارو;
        iterate_And_ايندڙ_Null(nodep->thensp());
        iterate_And_ايندڙ_Null(nodep->elsesp());
        m_insStmtp = خالي_اشارو;
    }
    فراغ گهمو(Ast_Case_شي* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = سچ;
            iterate_And_ايندڙ_Null(nodep->condsp());
        }
        m_insStmtp = خالي_اشارو;  // Next thing should be new statement
        iterate_And_ايندڙ_Null(nodep->stmtsp());
    }
    فراغ گهمو(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ايندڙ_Null(nodep->lhsp());
        m_insStmtp = خالي_اشارو;
        iterate_And_ايندڙ_Null(nodep->stmtsp());
        m_insStmtp = خالي_اشارو;
    }
    فراغ گهمو(AstEventControl* nodep) override {
        m_insStmtp = خالي_اشارو;
        iterate_And_ايندڙ_Null(nodep->stmtsp());
        m_insStmtp = خالي_اشارو;
    }
    فراغ گهمو(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ايندڙ_Null(nodep->condp());
        m_insStmtp = خالي_اشارو;
        iterate_And_ايندڙ_Null(nodep->stmtsp());
        m_insStmtp = خالي_اشارو;
    }
    فراغ گهمو(AstBegin* nodep) override {
        m_insStmtp = خالي_اشارو;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = خالي_اشارو;  // Next thing should be new statement
    }
    فراغ گهمو(Ast_Stmt_اظهار* nodep) override {
        Ast_ڳنڍ_اظهار* ثابت exprp = nodep->exprp();
        إذا (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_پٺ());
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            m_insStmtp = خالي_اشارو;
            iterate(exprp);
            m_insStmtp = خالي_اشارو;
            أرجع;
        }
        گهمو(static_cast<Ast_ڳنڍ_Stmt*>(nodep));
    }
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = خالي_اشارو;  // Next thing should be new statement
    }
    فراغ unsupported_گهمو(Ast_ڳنڍ* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = سچ;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = خالي_اشارو;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_ڳنڍ_اظهار* capture_Logical_Lhs_منزل_عارضي(Ast_ڳنڍ_Biop* ثابت nodep) {
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٺ();
        ثابت string name = "__VincGate"s + cvt_منزل_Str(++m_mod_Compound_Assignments_انگ);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ٻار_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_جو_اظهار{}, lhsp->clone_وڻ(سچ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(سچ);
        وجهو_On_چوٽي(varp);
        Ast_ڳنڍ* عارضي_Initp = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        إذا (m_incCondp) {
            عارضي_Initp = جديد AstIf{fl, m_incCondp->clone_وڻ_خالص(سچ), عارضي_Initp, خالي_اشارو};
        }
        وجهو_Before_Stmt(nodep, عارضي_Initp);
        nodep->lhsp(جديد AstVarRef{fl, varp, VAccess::READ});
        أرجع جديد AstVarRef{fl, varp, VAccess::READ};
    }
    فراغ handleShortCircuit(Ast_ڳنڍ_Biop* ثابت nodep, منطقي isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_ايندڙ_Null(nodep->lhsp());
        m_condEvalContext = سچ;
        Ast_ڳنڍ_اظهار* ownedCondp = خالي_اشارو;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        إذا (nodep->rhsp()->exists([](ثابت Ast_ڳنڍ* np) {
                أرجع VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, سونپيو_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_ڳنڍ_اظهار* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_وڻ_خالص(سچ)
                                       : capture_Logical_Lhs_منزل_عارضي(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            إذا (isOr) lhsRefp = جديد AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? جديد AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_وڻ_خالص(سچ), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_ايندڙ_Null(nodep->rhsp());
        إذا (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_وڻ(), ownedCondp);
    }
    فراغ گهمو(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/خطأ); }
    فراغ گهمو(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/سچ); }
    فراغ گهمو(AstLogEq* nodep) override { unsupported_گهمو(nodep); }
    فراغ گهمو(AstLogIf* nodep) override { unsupported_گهمو(nodep); }
    فراغ گهمو(AstCond* nodep) override { unsupported_گهمو(nodep); }
    فراغ گهمو(AstPropSpec* nodep) override { unsupported_گهمو(nodep); }
    فراغ prepost_گهمو(Ast_ڳنڍ_Uniop* ثابت nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ثابت منطقي isStandaloneStmt = !m_insStmtp && !m_اندريون_Covergroup;
        Ast_Sel_ٻٽو_انگ* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_ٻٽو_انگ);
        إذا (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), ڳنڍ_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_گهمو(nodep);
        } وإلا {
            إذا (isStandaloneStmt) {
                prepost_stmt_گهمو(nodep);
            } وإلا {
                prepost_اظهار_گهمو(nodep);
            }
        }
    }
    Ast_ڳنڍ_اظهار* getOperationp(Ast_ڳنڍ* ثابت nodep, Ast_ڳنڍ_اظهار* ثابت lhsp,
                               Ast_ڳنڍ_اظهار* ثابت rhsp) {
        إذا (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            أرجع جديد AstSub{nodep->fileline(), lhsp, rhsp};
        }
        إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            أرجع جديد Ast_جوڙيو{nodep->fileline(), lhsp, rhsp};
        }
        إذا (Ast_سونپيو_Compound* assignp = VN_CAST(nodep, سونپيو_Compound)) {
            بدل (assignp->operation()) {
            حالة Ast_سونپيو_Compound::operation::جوڙيو_2:
                أرجع جديد Ast_جوڙيو{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::And:
                أرجع جديد AstAnd{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::Div:
                أرجع جديد AstDiv{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::ModDiv:
                أرجع جديد AstModDiv{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::Mul:
                أرجع جديد AstMul{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::Or: أرجع جديد AstOr{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::سرڪايو_L:
                أرجع جديد Ast_سرڪايو_L{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::سرڪايو_R:
                أرجع جديد Ast_سرڪايو_R{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::سرڪايو_RS:
                أرجع جديد Ast_سرڪايو_RS{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::Sub:
                أرجع جديد AstSub{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپيو_Compound::operation::Xor:
                أرجع جديد AstXor{nodep->fileline(), lhsp, rhsp};
            افتراضي:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    فراغ prepost_stmt_sel_گهمو(Ast_ڳنڍ_Uniop* ثابت nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_سٽ* fl = nodep->fileline();
        V3_عدد انگ_One{fl, 32, 1, خطأ};
        Ast_ڳنڍ_اظهار* ثابت exprp = جديد AstConst{nodep->fileline(), انگ_One};

        prepost_stmt_sel_گهمو(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_گهمو(Ast_سونپيو_Compound* ثابت nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_ڳنڍ_اظهار* ثابت exprp = nodep->rhsp()->unlink_Fr_پٺ();

        prepost_stmt_sel_گهمو(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_گهمو(Ast_ڳنڍ* ثابت nodep, Ast_ڳنڍ_اظهار* ثابت lhsp,
                                Ast_ڳنڍ_اظهار* ثابت exprp) {
        Ast_Sel_ٻٽو_انگ* ثابت rdSelbitp = VN_AS(lhsp, Sel_ٻٽو_انگ);
        Ast_ڳنڍ_Var_Ref* ثابت rdFromp = VN_AS(rdSelbitp->fromp()->clone_وڻ_خالص(سچ), ڳنڍ_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_ڳنڍ_اظهار* ثابت rdBitp = rdSelbitp->bitp()->unlink_Fr_پٺ();
        Ast_Sel_ٻٽو_انگ* ثابت wrSelbitp = VN_CAST(lhsp, Sel_ٻٽو_انگ);
        Ast_ڳنڍ_اظهار* ثابت wrFromp = wrSelbitp->fromp()->unlink_Fr_پٺ();
        V3LinkLValue::link_LValue_سيٽ(wrFromp);

        // Prepare a temporary variable
        File_سٽ* ثابت fl = nodep->fileline();
        ثابت string name = "__VtempIndex"s + cvt_منزل_Str(++m_mod_Compound_Assignments_انگ);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ٻار_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_جو_اظهار{}, rdBitp->clone_وڻ(سچ)}};
        إذا (m_ftaskp) varp->funcLocal(سچ);

        // Declare the variable
        وجهو_On_چوٽي(varp);

        // Define what operation will we be doing
        Ast_سونپيو* ثابت varAssignp
            = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_ڳنڍ* ثابت newp = varAssignp;

        Ast_ڳنڍ_اظهار* ثابت valuep
            = جديد Ast_Sel_ٻٽو_انگ{fl, rdFromp, جديد AstVarRef{fl, varp, VAccess::READ}};
        Ast_ڳنڍ_اظهار* ثابت store_چوٽي
            = جديد Ast_Sel_ٻٽو_انگ{fl, wrFromp, جديد AstVarRef{fl, varp, VAccess::READ}};

        Ast_سونپيو* assignp
            = جديد Ast_سونپيو{nodep->fileline(), store_چوٽي, getOperationp(nodep, valuep, exprp)};
        newp->جوڙيو_ايندڙ(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ prepost_stmt_گهمو(Ast_ڳنڍ_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        Ast_ڳنڍ_اظهار* ثابت store_چوٽي = nodep->lhsp()->clone_وڻ_خالص(سچ);
        Ast_ڳنڍ_اظهار* ثابت valuep = nodep->lhsp()->unlink_Fr_پٺ();
        File_سٽ* ثابت fl = nodep->fileline();
        V3_عدد انگ_One{fl, 32, 1, خطأ};
        Ast_ڳنڍ_اظهار* ثابت exprp = جديد AstConst{nodep->fileline(), انگ_One};

        prepost_stmt_گهمو(nodep, exprp, store_چوٽي, valuep);
    }
    فراغ prepost_stmt_گهمو(Ast_سونپيو_Compound* ثابت nodep) {
        Ast_ڳنڍ_اظهار* ثابت exprp = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت store_چوٽي = nodep->lhsp()->clone_وڻ_خالص(سچ);
        Ast_ڳنڍ_اظهار* ثابت valuep = nodep->lhsp()->unlink_Fr_پٺ();

        prepost_stmt_گهمو(nodep, exprp, store_چوٽي, valuep);
    }
    فراغ prepost_stmt_گهمو(Ast_ڳنڍ* ثابت nodep, Ast_ڳنڍ_اظهار* ثابت exprp,
                            Ast_ڳنڍ_اظهار* ثابت store_چوٽي, Ast_ڳنڍ_اظهار* ثابت valuep) {
        V3LinkLValue::link_LValue_سيٽ(valuep, VAccess::READ);
        Ast_سونپيو* ثابت assignp
            = جديد Ast_سونپيو{nodep->fileline(), store_چوٽي, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ prepost_اظهار_گهمو(Ast_ڳنڍ_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        إذا (m_condEvalContext && !m_incCondp && !m_اندريون_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            أرجع;
        }
        ثابت منطقي needGating = m_condEvalContext && m_incCondp && !m_اندريون_Covergroup;
        Ast_ڳنڍ_اظهار* ثابت readp = nodep->lhsp();
        Ast_ڳنڍ_اظهار* ثابت writep = nodep->lhsp()->clone_وڻ_خالص(سچ);
        V3LinkLValue::link_LValue_سيٽ(readp, VAccess::READ);

        File_سٽ* ثابت fl = nodep->fileline();
        V3_عدد انگ_One{fl, 32, 1, خطأ};
        Ast_ڳنڍ_اظهار* ثابت newconstp = جديد AstConst{nodep->fileline(), انگ_One};

        // Prepare a temporary variable
        ثابت string name = "__Vincrement"s + cvt_منزل_Str(++m_mod_Compound_Assignments_انگ);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ٻار_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_جو_اظهار{}, readp->clone_وڻ_خالص(سچ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(سچ);

        // Declare the variable
        وجهو_On_چوٽي(varp);

        // Define what operation will we be doing
        Ast_ڳنڍ_اظهار* ثابت operp = getOperationp(nodep, readp->clone_وڻ_خالص(سچ), newconstp);

        Ast_ڳنڍ* stmtsp = خالي_اشارو;
        إذا (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_وڻ_خالص(سچ)};
            Ast_ڳنڍ* ثابت incp = جديد Ast_سونپيو{fl, writep, operp};
            إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->جوڙيو_ايندڙ(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_وڻ_خالص(سچ)});
            }
            assignp->جوڙيو_ايندڙ_هتي(جديد AstIf{fl, m_incCondp->clone_وڻ_خالص(سچ), incp, خالي_اشارو});
            stmtsp = assignp;
        } وإلا إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_سونپيو* ثابت assignp
                = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->جوڙيو_ايندڙ(جديد Ast_سونپيو{fl, writep, جديد AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } وإلا {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_وڻ_خالص(سچ)};
            // Increment the original variable by one
            assignp->جوڙيو_ايندڙ(جديد Ast_سونپيو{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_ڳنڍ_اظهار* newp = جديد AstVarRef{readp->fileline(), varp, VAccess::READ};
        إذا (m_اندريون_Covergroup) {
            newp = جديد Ast_اظهار_Stmt{fl, stmtsp, newp};
        } وإلا {
            وجهو_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstPreInc* nodep) override { prepost_گهمو(nodep); }
    فراغ گهمو(AstPostInc* nodep) override { prepost_گهمو(nodep); }
    فراغ گهمو(AstPreDec* nodep) override { prepost_گهمو(nodep); }
    فراغ گهمو(AstPostDec* nodep) override { prepost_گهمو(nodep); }
    فراغ گهمو(Ast_سونپيو_Compound* nodep) override {
        گهمو(static_cast<Ast_ڳنڍ_Stmt*>(nodep));
        Ast_Sel_ٻٽو_انگ* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_ٻٽو_انگ);
        إذا (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), ڳنڍ_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_گهمو(nodep);
        } وإلا {
            prepost_stmt_گهمو(nodep);
        }
    }
    فراغ گهمو(AstGenFor* nodep) override { iterateChildren(nodep); }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("linkinc", 0, dump_وڻ_Either_Level() >= 3);
}
