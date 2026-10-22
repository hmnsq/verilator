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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkInc.h"

#ಅನ್ತರ್ಭಾವ "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ LinkIncVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // STATE
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Function or task we're inside
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Module we're inside
    ಪೂರ್ಣಾಂಕ m_mod_Compound_Assignments_ಅಂಕೆ = 0;  // Var name counter
    Ast_ಗಂಟು* m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Where to insert statement
    ತಾರ್ಕಿಕ m_ಒಳ_Covergroup = ಸುಳ್ಳು;  // Under a covergroup's coverpoint, cross, or option
    ತಾರ್ಕಿಕ m_condEvalContext = ಸುಳ್ಳು;  // ++/-- is in a conditionally-evaluated position
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_incCondp = ಶೂನ್ಯಸೂಚಕ;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    ಶೂನ್ಯ ಸೇರಿಸು_On_ತುದಿ(Ast_ಗಂಟು* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        ಆದರೆ (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            ಆದರೆ (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    ಶೂನ್ಯ ಸೇರಿಸು_Before_Stmt(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (nodep->dead()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_ಅಂಕೆ);
        m_modp = nodep;
        m_mod_Compound_Assignments_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    ಶೂನ್ಯ ಭೇಟಿ_Covergroup(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_ಒಳ_Covergroup);
        m_ftaskp = ಶೂನ್ಯಸೂಚಕ;
        m_ಒಳ_Covergroup = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Func_Cov_ಅಂಶ* nodep) override { ಭೇಟಿ_Covergroup(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cg_Option_ನಿಯೋಜಿಸು* nodep) override { ಭೇಟಿ_Covergroup(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_ಮುಂದಿನ_Null(nodep->propp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
        // Note: no iterating over sentreep here as they will be ignored anyway
        ಆದರೆ (AstAssert* ಸ್ಥಿರಾಂಕ assertp = VN_CAST(nodep, Assert)) {
            iterate_And_ಮುಂದಿನ_Null(assertp->failsp());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstAssertIntrinsic* ಸ್ಥಿರಾಂಕ intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_ಮುಂದಿನ_Null(intrinsicp->failsp());
        }
        iterate_And_ಮುಂದಿನ_Null(nodep->passsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // First thing should be new statement
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Next thing should be new statement
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Next thing should be new statement
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Next thing should be new statement
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ಮುಂದಿನ_Null(nodep->condp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
        iterate_And_ಮುಂದಿನ_Null(nodep->thensp());
        iterate_And_ಮುಂದಿನ_Null(nodep->elsesp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Null(nodep->condsp());
        }
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Next thing should be new statement
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEventControl* nodep) override {
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_ಮುಂದಿನ_Null(nodep->condp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
        iterate_And_ಮುಂದಿನ_Null(nodep->stmtsp());
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Next thing should be new statement
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->exprp();
        ಆದರೆ (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
            iterate(exprp);
            m_insStmtp = ಶೂನ್ಯಸೂಚಕ;
            ಹಿಂತಿರುಗಿ;
        }
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = ಶೂನ್ಯಸೂಚಕ;  // Next thing should be new statement
    }
    ಶೂನ್ಯ unsupported_ಭೇಟಿ(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = ನಿಜ;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = ಶೂನ್ಯಸೂಚಕ;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* capture_Logical_Lhs_ಗಮ್ಯ_ತಾತ್ಕಾಲಿಕ(Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ nodep) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ string name = "__VincGate"s + cvt_ಗಮ್ಯ_Str(++m_mod_Compound_Assignments_ಅಂಕೆ);
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ಮಗು_DType{},
            ಹೊಸ AstRefDType{fl, AstRefDType::Flag_Type_ದ_ಅಭಿವ್ಯಕ್ತಿ{}, lhsp->clone_ವೃಕ್ಷ(ನಿಜ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ಆದರೆ (m_ftaskp) varp->funcLocal(ನಿಜ);
        ಸೇರಿಸು_On_ತುದಿ(varp);
        Ast_ಗಂಟು* ತಾತ್ಕಾಲಿಕ_Initp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        ಆದರೆ (m_incCondp) {
            ತಾತ್ಕಾಲಿಕ_Initp = ಹೊಸ AstIf{fl, m_incCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ), ತಾತ್ಕಾಲಿಕ_Initp, ಶೂನ್ಯಸೂಚಕ};
        }
        ಸೇರಿಸು_Before_Stmt(nodep, ತಾತ್ಕಾಲಿಕ_Initp);
        nodep->lhsp(ಹೊಸ AstVarRef{fl, varp, VAccess::READ});
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
    }
    ಶೂನ್ಯ handleShortCircuit(Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ nodep, ತಾರ್ಕಿಕ isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        m_condEvalContext = ನಿಜ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ownedCondp = ಶೂನ್ಯಸೂಚಕ;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        ಆದರೆ (nodep->rhsp()->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* np) {
                ಹಿಂತಿರುಗಿ VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, ನಿಯೋಜಿಸು_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)
                                       : capture_Logical_Lhs_ಗಮ್ಯ_ತಾತ್ಕಾಲಿಕ(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            ಆದರೆ (isOr) lhsRefp = ಹೊಸ AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? ಹೊಸ AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
        ಆದರೆ (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_ವೃಕ್ಷ(), ownedCondp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/ಸುಳ್ಳು); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/ನಿಜ); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogEq* nodep) override { unsupported_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogIf* nodep) override { unsupported_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override { unsupported_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPropSpec* nodep) override { unsupported_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ prepost_ಭೇಟಿ(Ast_ಗಂಟು_Uniop* ಸ್ಥಿರಾಂಕ nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isStandaloneStmt = !m_insStmtp && !m_ಒಳ_Covergroup;
        Ast_Sel_ಬಿಟ್* ಸ್ಥಿರಾಂಕ selbitp = VN_CAST(nodep->lhsp(), Sel_ಬಿಟ್);
        ಆದರೆ (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), ಗಂಟು_Var_Ref)
            && !selbitp->bitp()->is_ಶುದ್ಧ()) {
            prepost_stmt_sel_ಭೇಟಿ(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (isStandaloneStmt) {
                prepost_stmt_ಭೇಟಿ(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                prepost_ಅಭಿವ್ಯಕ್ತಿ_ಭೇಟಿ(nodep);
            }
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getOperationp(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp,
                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp) {
        ಆದರೆ (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstSub{nodep->fileline(), lhsp, rhsp};
        }
        ಆದರೆ (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಕೂಡಿಸು{nodep->fileline(), lhsp, rhsp};
        }
        ಆದರೆ (Ast_ನಿಯೋಜಿಸು_Compound* assignp = VN_CAST(nodep, ನಿಯೋಜಿಸು_Compound)) {
            ಆಯ್ಕೆ (assignp->operation()) {
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::ಕೂಡಿಸು_2:
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಕೂಡಿಸು{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::And:
                ಹಿಂತಿರುಗಿ ಹೊಸ AstAnd{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::Div:
                ಹಿಂತಿರುಗಿ ಹೊಸ AstDiv{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::ModDiv:
                ಹಿಂತಿರುಗಿ ಹೊಸ AstModDiv{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::Mul:
                ಹಿಂತಿರುಗಿ ಹೊಸ AstMul{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::Or: ಹಿಂತಿರುಗಿ ಹೊಸ AstOr{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::ಸರಿಸು_L:
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಸರಿಸು_L{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::ಸರಿಸು_R:
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಸರಿಸು_R{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::ಸರಿಸು_RS:
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ಸರಿಸು_RS{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::Sub:
                ಹಿಂತಿರುಗಿ ಹೊಸ AstSub{nodep->fileline(), lhsp, rhsp};
            ಸ್ಥಿತಿ Ast_ನಿಯೋಜಿಸು_Compound::operation::Xor:
                ಹಿಂತಿರುಗಿ ಹೊಸ AstXor{nodep->fileline(), lhsp, rhsp};
            ಪೂರ್ವನಿಯೋಜಿತ:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    ಶೂನ್ಯ prepost_stmt_sel_ಭೇಟಿ(Ast_ಗಂಟು_Uniop* ಸ್ಥಿರಾಂಕ nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_ಸಾಲು* fl = nodep->fileline();
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, 32, 1, ಸುಳ್ಳು};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ AstConst{nodep->fileline(), ಅಂಕೆ_One};

        prepost_stmt_sel_ಭೇಟಿ(nodep, nodep->lhsp(), exprp);
    }
    ಶೂನ್ಯ prepost_stmt_sel_ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Compound* ಸ್ಥಿರಾಂಕ nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();

        prepost_stmt_sel_ಭೇಟಿ(nodep, nodep->lhsp(), exprp);
    }
    ಶೂನ್ಯ prepost_stmt_sel_ಭೇಟಿ(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp,
                                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp) {
        Ast_Sel_ಬಿಟ್* ಸ್ಥಿರಾಂಕ rdSelbitp = VN_AS(lhsp, Sel_ಬಿಟ್);
        Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ rdFromp = VN_AS(rdSelbitp->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ), ಗಂಟು_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rdBitp = rdSelbitp->bitp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_Sel_ಬಿಟ್* ಸ್ಥಿರಾಂಕ wrSelbitp = VN_CAST(lhsp, Sel_ಬಿಟ್);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ wrFromp = wrSelbitp->fromp()->unlink_Fr_ಹಿಂಭಾಗ();
        V3LinkLValue::link_LValue_ಗಣ(wrFromp);

        // Prepare a temporary variable
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಸ್ಥಿರಾಂಕ string name = "__VtempIndex"s + cvt_ಗಮ್ಯ_Str(++m_mod_Compound_Assignments_ಅಂಕೆ);
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ಮಗು_DType{},
            ಹೊಸ AstRefDType{fl, AstRefDType::Flag_Type_ದ_ಅಭಿವ್ಯಕ್ತಿ{}, rdBitp->clone_ವೃಕ್ಷ(ನಿಜ)}};
        ಆದರೆ (m_ftaskp) varp->funcLocal(ನಿಜ);

        // Declare the variable
        ಸೇರಿಸು_On_ತುದಿ(varp);

        // Define what operation will we be doing
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ varAssignp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = varAssignp;

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep
            = ಹೊಸ Ast_Sel_ಬಿಟ್{fl, rdFromp, ಹೊಸ AstVarRef{fl, varp, VAccess::READ}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ store_ತುದಿ
            = ಹೊಸ Ast_Sel_ಬಿಟ್{fl, wrFromp, ಹೊಸ AstVarRef{fl, varp, VAccess::READ}};

        Ast_ನಿಯೋಜಿಸು* assignp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{nodep->fileline(), store_ತುದಿ, getOperationp(nodep, valuep, exprp)};
        newp->ಕೂಡಿಸು_ಮುಂದಿನ(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ prepost_stmt_ಭೇಟಿ(Ast_ಗಂಟು_Uniop* ಸ್ಥಿರಾಂಕ nodep) {
        iterateChildren(nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ store_ತುದಿ = nodep->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, 32, 1, ಸುಳ್ಳು};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ AstConst{nodep->fileline(), ಅಂಕೆ_One};

        prepost_stmt_ಭೇಟಿ(nodep, exprp, store_ತುದಿ, valuep);
    }
    ಶೂನ್ಯ prepost_stmt_ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Compound* ಸ್ಥಿರಾಂಕ nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ store_ತುದಿ = nodep->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();

        prepost_stmt_ಭೇಟಿ(nodep, exprp, store_ತುದಿ, valuep);
    }
    ಶೂನ್ಯ prepost_stmt_ಭೇಟಿ(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ store_ತುದಿ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep) {
        V3LinkLValue::link_LValue_ಗಣ(valuep, VAccess::READ);
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{nodep->fileline(), store_ತುದಿ, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ prepost_ಅಭಿವ್ಯಕ್ತಿ_ಭೇಟಿ(Ast_ಗಂಟು_Uniop* ಸ್ಥಿರಾಂಕ nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        ಆದರೆ (m_condEvalContext && !m_incCondp && !m_ಒಳ_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needGating = m_condEvalContext && m_incCondp && !m_ಒಳ_Covergroup;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ readp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ writep = nodep->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ);
        V3LinkLValue::link_LValue_ಗಣ(readp, VAccess::READ);

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, 32, 1, ಸುಳ್ಳು};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newconstp = ಹೊಸ AstConst{nodep->fileline(), ಅಂಕೆ_One};

        // Prepare a temporary variable
        ಸ್ಥಿರಾಂಕ string name = "__Vincrement"s + cvt_ಗಮ್ಯ_Str(++m_mod_Compound_Assignments_ಅಂಕೆ);
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_ಮಗು_DType{},
            ಹೊಸ AstRefDType{fl, AstRefDType::Flag_Type_ದ_ಅಭಿವ್ಯಕ್ತಿ{}, readp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ಆದರೆ (m_ftaskp) varp->funcLocal(ನಿಜ);

        // Declare the variable
        ಸೇರಿಸು_On_ತುದಿ(varp);

        // Define what operation will we be doing
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ operp = getOperationp(nodep, readp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ), newconstp);

        Ast_ಗಂಟು* stmtsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)};
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ incp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, writep, operp};
            ಆದರೆ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)});
            }
            assignp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstIf{fl, m_incCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ), incp, ಶೂನ್ಯಸೂಚಕ});
            stmtsp = assignp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, writep, ಹೊಸ AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)};
            // Increment the original variable by one
            assignp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಹೊಸ AstVarRef{readp->fileline(), varp, VAccess::READ};
        ಆದರೆ (m_ಒಳ_Covergroup) {
            newp = ಹೊಸ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{fl, stmtsp, newp};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸೇರಿಸು_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPreInc* nodep) override { prepost_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPostInc* nodep) override { prepost_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPreDec* nodep) override { prepost_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPostDec* nodep) override { prepost_ಭೇಟಿ(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Compound* nodep) override {
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
        Ast_Sel_ಬಿಟ್* ಸ್ಥಿರಾಂಕ selbitp = VN_CAST(nodep->lhsp(), Sel_ಬಿಟ್);
        ಆದರೆ (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), ಗಂಟು_Var_Ref)
            && !selbitp->bitp()->is_ಶುದ್ಧ()) {
            prepost_stmt_sel_ಭೇಟಿ(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            prepost_stmt_ಭೇಟಿ(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstGenFor* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Task class functions

ಶೂನ್ಯ V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkinc", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
