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
    ثابت VNUser1_وچ_Use m_inuser1;

    // STATE
    Ast_گرہ_FTask* m_ftaskp = خالی_سوچک;  // Function or task we're inside
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Module we're inside
    صحيح m_mod_Compound_Assignments_عدد = 0;  // Var name counter
    Ast_گرہ* m_insStmtp = خالی_سوچک;  // Where to insert statement
    منطقي m_وچ_Covergroup = خطأ;  // Under a covergroup's coverpoint, cross, or option
    منطقي m_condEvalContext = خطأ;  // ++/-- is in a conditionally-evaluated position
    Ast_گرہ_اظہاریہ* m_incCondp = خالی_سوچک;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    فراغ داخل_کرو_On_چوٹی(Ast_گرہ* newp) {
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
    فراغ داخل_کرو_Before_Stmt(Ast_گرہ* nodep, Ast_گرہ* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->جوڑو_ایتھے_This_As_اگلا(newp);
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        إذا (nodep->dead()) أرجع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_عدد);
        m_modp = nodep;
        m_mod_Compound_Assignments_عدد = 0;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    فراغ ویکھو_Covergroup(Ast_گرہ* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_وچ_Covergroup);
        m_ftaskp = خالی_سوچک;
        m_وچ_Covergroup = سچ;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Func_Cov_شے* nodep) override { ویکھو_Covergroup(nodep); }
    فراغ ویکھو(Ast_Cg_Option_سونپو* nodep) override { ویکھو_Covergroup(nodep); }
    فراغ ویکھو(Ast_گرہ_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_اگلا_Null(nodep->propp());
        m_insStmtp = خالی_سوچک;
        // Note: no iterating over sentreep here as they will be ignored anyway
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterate_And_اگلا_Null(assertp->failsp());
        } وإلا إذا (AstAssertIntrinsic* ثابت intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_اگلا_Null(intrinsicp->failsp());
        }
        iterate_And_اگلا_Null(nodep->passsp());
    }
    فراغ ویکھو(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = خالی_سوچک;  // First thing should be new statement
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_insStmtp = خالی_سوچک;  // Next thing should be new statement
    }
    فراغ ویکھو(Ast_گرہ_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = خالی_سوچک;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = خالی_سوچک;  // Next thing should be new statement
    }
    فراغ ویکھو(Ast_Jump_بلاک* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = خالی_سوچک;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = خالی_سوچک;  // Next thing should be new statement
    }
    فراغ ویکھو(Ast_گرہ_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_اگلا_Null(nodep->condp());
        m_insStmtp = خالی_سوچک;
        iterate_And_اگلا_Null(nodep->thensp());
        iterate_And_اگلا_Null(nodep->elsesp());
        m_insStmtp = خالی_سوچک;
    }
    فراغ ویکھو(Ast_Case_شے* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = سچ;
            iterate_And_اگلا_Null(nodep->condsp());
        }
        m_insStmtp = خالی_سوچک;  // Next thing should be new statement
        iterate_And_اگلا_Null(nodep->stmtsp());
    }
    فراغ ویکھو(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_اگلا_Null(nodep->lhsp());
        m_insStmtp = خالی_سوچک;
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_insStmtp = خالی_سوچک;
    }
    فراغ ویکھو(AstEventControl* nodep) override {
        m_insStmtp = خالی_سوچک;
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_insStmtp = خالی_سوچک;
    }
    فراغ ویکھو(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_اگلا_Null(nodep->condp());
        m_insStmtp = خالی_سوچک;
        iterate_And_اگلا_Null(nodep->stmtsp());
        m_insStmtp = خالی_سوچک;
    }
    فراغ ویکھو(AstBegin* nodep) override {
        m_insStmtp = خالی_سوچک;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = خالی_سوچک;  // Next thing should be new statement
    }
    فراغ ویکھو(Ast_Stmt_اظہاریہ* nodep) override {
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp();
        إذا (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_پٹھ());
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            m_insStmtp = خالی_سوچک;
            iterate(exprp);
            m_insStmtp = خالی_سوچک;
            أرجع;
        }
        ویکھو(static_cast<Ast_گرہ_Stmt*>(nodep));
    }
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = خالی_سوچک;  // Next thing should be new statement
    }
    فراغ unsupported_ویکھو(Ast_گرہ* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = سچ;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = خالی_سوچک;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_گرہ_اظہاریہ* capture_Logical_Lhs_منزل_عارضی(Ast_گرہ_Biop* ثابت nodep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٹھ();
        ثابت string name = "__VincGate"s + cvt_منزل_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_بچہ_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_دا_اظہاریہ{}, lhsp->clone_درخت(سچ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(سچ);
        داخل_کرو_On_چوٹی(varp);
        Ast_گرہ* عارضی_Initp = جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        إذا (m_incCondp) {
            عارضی_Initp = جديد AstIf{fl, m_incCondp->clone_درخت_خالص(سچ), عارضی_Initp, خالی_سوچک};
        }
        داخل_کرو_Before_Stmt(nodep, عارضی_Initp);
        nodep->lhsp(جديد AstVarRef{fl, varp, VAccess::READ});
        أرجع جديد AstVarRef{fl, varp, VAccess::READ};
    }
    فراغ handleShortCircuit(Ast_گرہ_Biop* ثابت nodep, منطقي isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_اگلا_Null(nodep->lhsp());
        m_condEvalContext = سچ;
        Ast_گرہ_اظہاریہ* ownedCondp = خالی_سوچک;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        إذا (nodep->rhsp()->exists([](ثابت Ast_گرہ* np) {
                أرجع VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, سونپو_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_گرہ_اظہاریہ* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_درخت_خالص(سچ)
                                       : capture_Logical_Lhs_منزل_عارضی(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            إذا (isOr) lhsRefp = جديد AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? جديد AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_درخت_خالص(سچ), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_اگلا_Null(nodep->rhsp());
        إذا (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_درخت(), ownedCondp);
    }
    فراغ ویکھو(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/خطأ); }
    فراغ ویکھو(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/سچ); }
    فراغ ویکھو(AstLogEq* nodep) override { unsupported_ویکھو(nodep); }
    فراغ ویکھو(AstLogIf* nodep) override { unsupported_ویکھو(nodep); }
    فراغ ویکھو(AstCond* nodep) override { unsupported_ویکھو(nodep); }
    فراغ ویکھو(AstPropSpec* nodep) override { unsupported_ویکھو(nodep); }
    فراغ prepost_ویکھو(Ast_گرہ_Uniop* ثابت nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ثابت منطقي isStandaloneStmt = !m_insStmtp && !m_وچ_Covergroup;
        Ast_Sel_ثنائی_ہندسہ* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_ثنائی_ہندسہ);
        إذا (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), گرہ_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_ویکھو(nodep);
        } وإلا {
            إذا (isStandaloneStmt) {
                prepost_stmt_ویکھو(nodep);
            } وإلا {
                prepost_اظہاریہ_ویکھو(nodep);
            }
        }
    }
    Ast_گرہ_اظہاریہ* getOperationp(Ast_گرہ* ثابت nodep, Ast_گرہ_اظہاریہ* ثابت lhsp,
                               Ast_گرہ_اظہاریہ* ثابت rhsp) {
        إذا (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            أرجع جديد AstSub{nodep->fileline(), lhsp, rhsp};
        }
        إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            أرجع جديد Ast_جوڑو{nodep->fileline(), lhsp, rhsp};
        }
        إذا (Ast_سونپو_Compound* assignp = VN_CAST(nodep, سونپو_Compound)) {
            بدل (assignp->operation()) {
            حالة Ast_سونپو_Compound::operation::جوڑو_2:
                أرجع جديد Ast_جوڑو{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::And:
                أرجع جديد AstAnd{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::Div:
                أرجع جديد AstDiv{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::ModDiv:
                أرجع جديد AstModDiv{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::Mul:
                أرجع جديد AstMul{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::Or: أرجع جديد AstOr{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::کھسکاؤ_L:
                أرجع جديد Ast_کھسکاؤ_L{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::کھسکاؤ_R:
                أرجع جديد Ast_کھسکاؤ_R{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::کھسکاؤ_RS:
                أرجع جديد Ast_کھسکاؤ_RS{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::Sub:
                أرجع جديد AstSub{nodep->fileline(), lhsp, rhsp};
            حالة Ast_سونپو_Compound::operation::Xor:
                أرجع جديد AstXor{nodep->fileline(), lhsp, rhsp};
            افتراضي:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    فراغ prepost_stmt_sel_ویکھو(Ast_گرہ_Uniop* ثابت nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_سطر* fl = nodep->fileline();
        V3_نمبر عدد_One{fl, 32, 1, خطأ};
        Ast_گرہ_اظہاریہ* ثابت exprp = جديد AstConst{nodep->fileline(), عدد_One};

        prepost_stmt_sel_ویکھو(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_ویکھو(Ast_سونپو_Compound* ثابت nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->rhsp()->unlink_Fr_پٹھ();

        prepost_stmt_sel_ویکھو(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_ویکھو(Ast_گرہ* ثابت nodep, Ast_گرہ_اظہاریہ* ثابت lhsp,
                                Ast_گرہ_اظہاریہ* ثابت exprp) {
        Ast_Sel_ثنائی_ہندسہ* ثابت rdSelbitp = VN_AS(lhsp, Sel_ثنائی_ہندسہ);
        Ast_گرہ_Var_Ref* ثابت rdFromp = VN_AS(rdSelbitp->fromp()->clone_درخت_خالص(سچ), گرہ_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_گرہ_اظہاریہ* ثابت rdBitp = rdSelbitp->bitp()->unlink_Fr_پٹھ();
        Ast_Sel_ثنائی_ہندسہ* ثابت wrSelbitp = VN_CAST(lhsp, Sel_ثنائی_ہندسہ);
        Ast_گرہ_اظہاریہ* ثابت wrFromp = wrSelbitp->fromp()->unlink_Fr_پٹھ();
        V3LinkLValue::link_LValue_سیٹ(wrFromp);

        // Prepare a temporary variable
        File_سطر* ثابت fl = nodep->fileline();
        ثابت string name = "__VtempIndex"s + cvt_منزل_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_بچہ_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_دا_اظہاریہ{}, rdBitp->clone_درخت(سچ)}};
        إذا (m_ftaskp) varp->funcLocal(سچ);

        // Declare the variable
        داخل_کرو_On_چوٹی(varp);

        // Define what operation will we be doing
        Ast_سونپو* ثابت varAssignp
            = جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_گرہ* ثابت newp = varAssignp;

        Ast_گرہ_اظہاریہ* ثابت valuep
            = جديد Ast_Sel_ثنائی_ہندسہ{fl, rdFromp, جديد AstVarRef{fl, varp, VAccess::READ}};
        Ast_گرہ_اظہاریہ* ثابت store_چوٹی
            = جديد Ast_Sel_ثنائی_ہندسہ{fl, wrFromp, جديد AstVarRef{fl, varp, VAccess::READ}};

        Ast_سونپو* assignp
            = جديد Ast_سونپو{nodep->fileline(), store_چوٹی, getOperationp(nodep, valuep, exprp)};
        newp->جوڑو_اگلا(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ prepost_stmt_ویکھو(Ast_گرہ_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        Ast_گرہ_اظہاریہ* ثابت store_چوٹی = nodep->lhsp()->clone_درخت_خالص(سچ);
        Ast_گرہ_اظہاریہ* ثابت valuep = nodep->lhsp()->unlink_Fr_پٹھ();
        File_سطر* ثابت fl = nodep->fileline();
        V3_نمبر عدد_One{fl, 32, 1, خطأ};
        Ast_گرہ_اظہاریہ* ثابت exprp = جديد AstConst{nodep->fileline(), عدد_One};

        prepost_stmt_ویکھو(nodep, exprp, store_چوٹی, valuep);
    }
    فراغ prepost_stmt_ویکھو(Ast_سونپو_Compound* ثابت nodep) {
        Ast_گرہ_اظہاریہ* ثابت exprp = nodep->rhsp()->unlink_Fr_پٹھ();
        Ast_گرہ_اظہاریہ* ثابت store_چوٹی = nodep->lhsp()->clone_درخت_خالص(سچ);
        Ast_گرہ_اظہاریہ* ثابت valuep = nodep->lhsp()->unlink_Fr_پٹھ();

        prepost_stmt_ویکھو(nodep, exprp, store_چوٹی, valuep);
    }
    فراغ prepost_stmt_ویکھو(Ast_گرہ* ثابت nodep, Ast_گرہ_اظہاریہ* ثابت exprp,
                            Ast_گرہ_اظہاریہ* ثابت store_چوٹی, Ast_گرہ_اظہاریہ* ثابت valuep) {
        V3LinkLValue::link_LValue_سیٹ(valuep, VAccess::READ);
        Ast_سونپو* ثابت assignp
            = جديد Ast_سونپو{nodep->fileline(), store_چوٹی, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ prepost_اظہاریہ_ویکھو(Ast_گرہ_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        إذا (m_condEvalContext && !m_incCondp && !m_وچ_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            أرجع;
        }
        ثابت منطقي needGating = m_condEvalContext && m_incCondp && !m_وچ_Covergroup;
        Ast_گرہ_اظہاریہ* ثابت readp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت writep = nodep->lhsp()->clone_درخت_خالص(سچ);
        V3LinkLValue::link_LValue_سیٹ(readp, VAccess::READ);

        File_سطر* ثابت fl = nodep->fileline();
        V3_نمبر عدد_One{fl, 32, 1, خطأ};
        Ast_گرہ_اظہاریہ* ثابت newconstp = جديد AstConst{nodep->fileline(), عدد_One};

        // Prepare a temporary variable
        ثابت string name = "__Vincrement"s + cvt_منزل_Str(++m_mod_Compound_Assignments_عدد);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_بچہ_DType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_دا_اظہاریہ{}, readp->clone_درخت_خالص(سچ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(سچ);

        // Declare the variable
        داخل_کرو_On_چوٹی(varp);

        // Define what operation will we be doing
        Ast_گرہ_اظہاریہ* ثابت operp = getOperationp(nodep, readp->clone_درخت_خالص(سچ), newconstp);

        Ast_گرہ* stmtsp = خالی_سوچک;
        إذا (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_سونپو* ثابت assignp = جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_درخت_خالص(سچ)};
            Ast_گرہ* ثابت incp = جديد Ast_سونپو{fl, writep, operp};
            إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->جوڑو_اگلا(جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_درخت_خالص(سچ)});
            }
            assignp->جوڑو_اگلا_ایتھے(جديد AstIf{fl, m_incCondp->clone_درخت_خالص(سچ), incp, خالی_سوچک});
            stmtsp = assignp;
        } وإلا إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_سونپو* ثابت assignp
                = جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->جوڑو_اگلا(جديد Ast_سونپو{fl, writep, جديد AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } وإلا {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_سونپو* ثابت assignp = جديد Ast_سونپو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_درخت_خالص(سچ)};
            // Increment the original variable by one
            assignp->جوڑو_اگلا(جديد Ast_سونپو{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_گرہ_اظہاریہ* newp = جديد AstVarRef{readp->fileline(), varp, VAccess::READ};
        إذا (m_وچ_Covergroup) {
            newp = جديد Ast_اظہاریہ_Stmt{fl, stmtsp, newp};
        } وإلا {
            داخل_کرو_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstPreInc* nodep) override { prepost_ویکھو(nodep); }
    فراغ ویکھو(AstPostInc* nodep) override { prepost_ویکھو(nodep); }
    فراغ ویکھو(AstPreDec* nodep) override { prepost_ویکھو(nodep); }
    فراغ ویکھو(AstPostDec* nodep) override { prepost_ویکھو(nodep); }
    فراغ ویکھو(Ast_سونپو_Compound* nodep) override {
        ویکھو(static_cast<Ast_گرہ_Stmt*>(nodep));
        Ast_Sel_ثنائی_ہندسہ* ثابت selbitp = VN_CAST(nodep->lhsp(), Sel_ثنائی_ہندسہ);
        إذا (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), گرہ_Var_Ref)
            && !selbitp->bitp()->is_خالص()) {
            prepost_stmt_sel_ویکھو(nodep);
        } وإلا {
            prepost_stmt_ویکھو(nodep);
        }
    }
    فراغ ویکھو(AstGenFor* nodep) override { iterateChildren(nodep); }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_درخت("linkinc", 0, dump_درخت_Either_Level() >= 3);
}
