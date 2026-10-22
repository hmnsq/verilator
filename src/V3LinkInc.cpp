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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkInc.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ LinkIncVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Function or task we're inside
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Module we're inside
    ਪੂਰਨ m_mod_Compound_Assignments_ਅੰਕ = 0;  // Var name counter
    Ast_ਗੰਢ* m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Where to insert statement
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Covergroup = ਝੂਠ;  // Under a covergroup's coverpoint, cross, or option
    ਤਾਰਕਿਕ m_condEvalContext = ਝੂਠ;  // ++/-- is in a conditionally-evaluated position
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_incCondp = ਖਾਲੀ_ਸੂਚਕ;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    ਖਾਲੀ ਪਾਓ_On_ਸਿਖਰ(Ast_ਗੰਢ* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        ਜੇ (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            ਜੇ (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    ਖਾਲੀ ਪਾਓ_Before_Stmt(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (nodep->dead()) ਵਾਪਸ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_ਅੰਕ);
        m_modp = nodep;
        m_mod_Compound_Assignments_ਅੰਕ = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    ਖਾਲੀ ਦੇਖੋ_Covergroup(Ast_ਗੰਢ* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_ਵਿੱਚ_Covergroup);
        m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;
        m_ਵਿੱਚ_Covergroup = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Func_Cov_ਵਸਤੂ* nodep) override { ਦੇਖੋ_Covergroup(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cg_Option_ਸੌਂਪੋ* nodep) override { ਦੇਖੋ_Covergroup(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_ਅਗਲਾ_Null(nodep->propp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
        // Note: no iterating over sentreep here as they will be ignored anyway
        ਜੇ (AstAssert* ਅਚਲ assertp = VN_CAST(nodep, Assert)) {
            iterate_And_ਅਗਲਾ_Null(assertp->failsp());
        } ਨਹੀਂਤਾਂ ਜੇ (AstAssertIntrinsic* ਅਚਲ intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_ਅਗਲਾ_Null(intrinsicp->failsp());
        }
        iterate_And_ਅਗਲਾ_Null(nodep->passsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // First thing should be new statement
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Next thing should be new statement
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Next thing should be new statement
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Next thing should be new statement
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ਅਗਲਾ_Null(nodep->condp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
        iterate_And_ਅਗਲਾ_Null(nodep->thensp());
        iterate_And_ਅਗਲਾ_Null(nodep->elsesp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Null(nodep->condsp());
        }
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Next thing should be new statement
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstEventControl* nodep) override {
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ਅਗਲਾ_Null(nodep->condp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
        iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Next thing should be new statement
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->exprp();
        ਜੇ (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
            iterate(exprp);
            m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;
            ਵਾਪਸ;
        }
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Stmt*>(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = ਖਾਲੀ_ਸੂਚਕ;  // Next thing should be new statement
    }
    ਖਾਲੀ unsupported_ਦੇਖੋ(Ast_ਗੰਢ* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = ਸੱਚ;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = ਖਾਲੀ_ਸੂਚਕ;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* capture_Logical_Lhs_ਮੰਜ਼ਿਲ_ਆਰਜ਼ੀ(Ast_ਗੰਢ_Biop* ਅਚਲ nodep) {
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ string name = "__VincGate"s + cvt_ਮੰਜ਼ਿਲ_Str(++m_mod_Compound_Assignments_ਅੰਕ);
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ਬੱਚਾ_DType{},
            ਨਵਾਂ AstRefDType{fl, AstRefDType::Flag_Type_ਦਾ_ਵਿਅੰਜਕ{}, lhsp->clone_ਰੁੱਖ(ਸੱਚ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ਜੇ (m_ftaskp) varp->funcLocal(ਸੱਚ);
        ਪਾਓ_On_ਸਿਖਰ(varp);
        Ast_ਗੰਢ* ਆਰਜ਼ੀ_Initp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        ਜੇ (m_incCondp) {
            ਆਰਜ਼ੀ_Initp = ਨਵਾਂ AstIf{fl, m_incCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ), ਆਰਜ਼ੀ_Initp, ਖਾਲੀ_ਸੂਚਕ};
        }
        ਪਾਓ_Before_Stmt(nodep, ਆਰਜ਼ੀ_Initp);
        nodep->lhsp(ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ});
        ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
    }
    ਖਾਲੀ handleShortCircuit(Ast_ਗੰਢ_Biop* ਅਚਲ nodep, ਤਾਰਕਿਕ isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        m_condEvalContext = ਸੱਚ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ownedCondp = ਖਾਲੀ_ਸੂਚਕ;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        ਜੇ (nodep->rhsp()->exists([](ਅਚਲ Ast_ਗੰਢ* np) {
                ਵਾਪਸ VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, ਸੌਂਪੋ_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)
                                       : capture_Logical_Lhs_ਮੰਜ਼ਿਲ_ਆਰਜ਼ੀ(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            ਜੇ (isOr) lhsRefp = ਨਵਾਂ AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? ਨਵਾਂ AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
        ਜੇ (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_ਰੁੱਖ(), ownedCondp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/ਝੂਠ); }
    ਖਾਲੀ ਦੇਖੋ(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/ਸੱਚ); }
    ਖਾਲੀ ਦੇਖੋ(AstLogEq* nodep) override { unsupported_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLogIf* nodep) override { unsupported_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override { unsupported_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPropSpec* nodep) override { unsupported_ਦੇਖੋ(nodep); }
    ਖਾਲੀ prepost_ਦੇਖੋ(Ast_ਗੰਢ_Uniop* ਅਚਲ nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ਅਚਲ ਤਾਰਕਿਕ isStandaloneStmt = !m_insStmtp && !m_ਵਿੱਚ_Covergroup;
        Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ* ਅਚਲ selbitp = VN_CAST(nodep->lhsp(), Sel_ਦੋਆਧਾਰੀ_ਅੰਕ);
        ਜੇ (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), ਗੰਢ_Var_Ref)
            && !selbitp->bitp()->is_ਸ਼ੁੱਧ()) {
            prepost_stmt_sel_ਦੇਖੋ(nodep);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (isStandaloneStmt) {
                prepost_stmt_ਦੇਖੋ(nodep);
            } ਨਹੀਂਤਾਂ {
                prepost_ਵਿਅੰਜਕ_ਦੇਖੋ(nodep);
            }
        }
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* getOperationp(Ast_ਗੰਢ* ਅਚਲ nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp,
                               Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp) {
        ਜੇ (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            ਵਾਪਸ ਨਵਾਂ AstSub{nodep->fileline(), lhsp, rhsp};
        }
        ਜੇ (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            ਵਾਪਸ ਨਵਾਂ Ast_ਜੋੜੋ{nodep->fileline(), lhsp, rhsp};
        }
        ਜੇ (Ast_ਸੌਂਪੋ_Compound* assignp = VN_CAST(nodep, ਸੌਂਪੋ_Compound)) {
            ਚੋਣ (assignp->operation()) {
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::ਜੋੜੋ_2:
                ਵਾਪਸ ਨਵਾਂ Ast_ਜੋੜੋ{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::And:
                ਵਾਪਸ ਨਵਾਂ AstAnd{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::Div:
                ਵਾਪਸ ਨਵਾਂ AstDiv{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::ModDiv:
                ਵਾਪਸ ਨਵਾਂ AstModDiv{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::Mul:
                ਵਾਪਸ ਨਵਾਂ AstMul{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::Or: ਵਾਪਸ ਨਵਾਂ AstOr{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::ਖਿਸਕਾਓ_L:
                ਵਾਪਸ ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::ਖਿਸਕਾਓ_R:
                ਵਾਪਸ ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::ਖਿਸਕਾਓ_RS:
                ਵਾਪਸ ਨਵਾਂ Ast_ਖਿਸਕਾਓ_RS{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::Sub:
                ਵਾਪਸ ਨਵਾਂ AstSub{nodep->fileline(), lhsp, rhsp};
            ਹਾਲਤ Ast_ਸੌਂਪੋ_Compound::operation::Xor:
                ਵਾਪਸ ਨਵਾਂ AstXor{nodep->fileline(), lhsp, rhsp};
            ਮੂਲ:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    ਖਾਲੀ prepost_stmt_sel_ਦੇਖੋ(Ast_ਗੰਢ_Uniop* ਅਚਲ nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_ਸਤਰ* fl = nodep->fileline();
        V3_ਸੰਖਿਆ ਅੰਕ_One{fl, 32, 1, ਝੂਠ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = ਨਵਾਂ AstConst{nodep->fileline(), ਅੰਕ_One};

        prepost_stmt_sel_ਦੇਖੋ(nodep, nodep->lhsp(), exprp);
    }
    ਖਾਲੀ prepost_stmt_sel_ਦੇਖੋ(Ast_ਸੌਂਪੋ_Compound* ਅਚਲ nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();

        prepost_stmt_sel_ਦੇਖੋ(nodep, nodep->lhsp(), exprp);
    }
    ਖਾਲੀ prepost_stmt_sel_ਦੇਖੋ(Ast_ਗੰਢ* ਅਚਲ nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp,
                                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp) {
        Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ* ਅਚਲ rdSelbitp = VN_AS(lhsp, Sel_ਦੋਆਧਾਰੀ_ਅੰਕ);
        Ast_ਗੰਢ_Var_Ref* ਅਚਲ rdFromp = VN_AS(rdSelbitp->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ), ਗੰਢ_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rdBitp = rdSelbitp->bitp()->unlink_Fr_ਪਿੱਠ();
        Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ* ਅਚਲ wrSelbitp = VN_CAST(lhsp, Sel_ਦੋਆਧਾਰੀ_ਅੰਕ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ wrFromp = wrSelbitp->fromp()->unlink_Fr_ਪਿੱਠ();
        V3LinkLValue::link_LValue_ਸੈੱਟ(wrFromp);

        // Prepare a temporary variable
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਅਚਲ string name = "__VtempIndex"s + cvt_ਮੰਜ਼ਿਲ_Str(++m_mod_Compound_Assignments_ਅੰਕ);
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ਬੱਚਾ_DType{},
            ਨਵਾਂ AstRefDType{fl, AstRefDType::Flag_Type_ਦਾ_ਵਿਅੰਜਕ{}, rdBitp->clone_ਰੁੱਖ(ਸੱਚ)}};
        ਜੇ (m_ftaskp) varp->funcLocal(ਸੱਚ);

        // Declare the variable
        ਪਾਓ_On_ਸਿਖਰ(varp);

        // Define what operation will we be doing
        Ast_ਸੌਂਪੋ* ਅਚਲ varAssignp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_ਗੰਢ* ਅਚਲ newp = varAssignp;

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep
            = ਨਵਾਂ Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ{fl, rdFromp, ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ store_ਸਿਖਰ
            = ਨਵਾਂ Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ{fl, wrFromp, ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}};

        Ast_ਸੌਂਪੋ* assignp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{nodep->fileline(), store_ਸਿਖਰ, getOperationp(nodep, valuep, exprp)};
        newp->ਜੋੜੋ_ਅਗਲਾ(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ prepost_stmt_ਦੇਖੋ(Ast_ਗੰਢ_Uniop* ਅਚਲ nodep) {
        iterateChildren(nodep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ store_ਸਿਖਰ = nodep->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        V3_ਸੰਖਿਆ ਅੰਕ_One{fl, 32, 1, ਝੂਠ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = ਨਵਾਂ AstConst{nodep->fileline(), ਅੰਕ_One};

        prepost_stmt_ਦੇਖੋ(nodep, exprp, store_ਸਿਖਰ, valuep);
    }
    ਖਾਲੀ prepost_stmt_ਦੇਖੋ(Ast_ਸੌਂਪੋ_Compound* ਅਚਲ nodep) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ store_ਸਿਖਰ = nodep->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();

        prepost_stmt_ਦੇਖੋ(nodep, exprp, store_ਸਿਖਰ, valuep);
    }
    ਖਾਲੀ prepost_stmt_ਦੇਖੋ(Ast_ਗੰਢ* ਅਚਲ nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp,
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ store_ਸਿਖਰ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep) {
        V3LinkLValue::link_LValue_ਸੈੱਟ(valuep, VAccess::READ);
        Ast_ਸੌਂਪੋ* ਅਚਲ assignp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{nodep->fileline(), store_ਸਿਖਰ, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ prepost_ਵਿਅੰਜਕ_ਦੇਖੋ(Ast_ਗੰਢ_Uniop* ਅਚਲ nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        ਜੇ (m_condEvalContext && !m_incCondp && !m_ਵਿੱਚ_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            ਵਾਪਸ;
        }
        ਅਚਲ ਤਾਰਕਿਕ needGating = m_condEvalContext && m_incCondp && !m_ਵਿੱਚ_Covergroup;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ readp = nodep->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ writep = nodep->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
        V3LinkLValue::link_LValue_ਸੈੱਟ(readp, VAccess::READ);

        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        V3_ਸੰਖਿਆ ਅੰਕ_One{fl, 32, 1, ਝੂਠ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newconstp = ਨਵਾਂ AstConst{nodep->fileline(), ਅੰਕ_One};

        // Prepare a temporary variable
        ਅਚਲ string name = "__Vincrement"s + cvt_ਮੰਜ਼ਿਲ_Str(++m_mod_Compound_Assignments_ਅੰਕ);
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ਬੱਚਾ_DType{},
            ਨਵਾਂ AstRefDType{fl, AstRefDType::Flag_Type_ਦਾ_ਵਿਅੰਜਕ{}, readp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ਜੇ (m_ftaskp) varp->funcLocal(ਸੱਚ);

        // Declare the variable
        ਪਾਓ_On_ਸਿਖਰ(varp);

        // Define what operation will we be doing
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ operp = getOperationp(nodep, readp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ), newconstp);

        Ast_ਗੰਢ* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)};
            Ast_ਗੰਢ* ਅਚਲ incp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, writep, operp};
            ਜੇ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)});
            }
            assignp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstIf{fl, m_incCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ), incp, ਖਾਲੀ_ਸੂਚਕ});
            stmtsp = assignp;
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, writep, ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } ਨਹੀਂਤਾਂ {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)};
            // Increment the original variable by one
            assignp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਨਵਾਂ AstVarRef{readp->fileline(), varp, VAccess::READ};
        ਜੇ (m_ਵਿੱਚ_Covergroup) {
            newp = ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{fl, stmtsp, newp};
        } ਨਹੀਂਤਾਂ {
            ਪਾਓ_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPreInc* nodep) override { prepost_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPostInc* nodep) override { prepost_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPreDec* nodep) override { prepost_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPostDec* nodep) override { prepost_ਦੇਖੋ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Compound* nodep) override {
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Stmt*>(nodep));
        Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ* ਅਚਲ selbitp = VN_CAST(nodep->lhsp(), Sel_ਦੋਆਧਾਰੀ_ਅੰਕ);
        ਜੇ (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), ਗੰਢ_Var_Ref)
            && !selbitp->bitp()->is_ਸ਼ੁੱਧ()) {
            prepost_stmt_sel_ਦੇਖੋ(nodep);
        } ਨਹੀਂਤਾਂ {
            prepost_stmt_ਦੇਖੋ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenFor* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = ਮੂਲ;
};

//######################################################################
// Task class functions

ਖਾਲੀ V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("linkinc", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
