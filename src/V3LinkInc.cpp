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
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Module we're inside
    ಪೂರ್ಣಾಙ್ಕ m_mod_Compound_Assignments_ಅಂಕೆ = 0;  // Var name counter
    Ast_ಗಂಟು* m_insStmtp = nullptr;  // Where to insert statement
    ತಾರ್ಕಿಕ m_inCovergroup = ಅಸತ್ಯ;  // Under a covergroup's coverpoint, cross, or option
    ತಾರ್ಕಿಕ m_condEvalContext = ಅಸತ್ಯ;  // ++/-- is in a conditionally-evaluated position
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    ಶೂನ್ಯ ಸೇರಿಸು_On_ತುದಿ(Ast_ಗಂಟು* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        ಯದಿ (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            ಯದಿ (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    ಶೂನ್ಯ ಸೇರಿಸು_Before_Stmt(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (nodep->dead()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_ಅಂಕೆ);
        m_modp = nodep;
        m_mod_Compound_Assignments_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    ಶೂನ್ಯ visitCovergroup(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    ಶೂನ್ಯ visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        ಯದಿ (AstAssert* ಧ್ರುವ assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } ಅನ್ಯಥಾ ಯದಿ (AstAssertIntrinsic* ಧ್ರುವ intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ಶೂನ್ಯ visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    ಶೂನ್ಯ visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = ಸತ್ಯ;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    ಶೂನ್ಯ visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ಶೂನ್ಯ visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ಶೂನ್ಯ visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->exprp();
        ಯದಿ (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            ಪ್ರತ್ಯಾಗಮ;
        }
        visit(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ಶೂನ್ಯ unsupported_visit(Ast_ಗಂಟು* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = ಸತ್ಯ;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* captureLogicalLhsToTemp(Ast_ಗಂಟು_Biop* ಧ್ರುವ nodep) {
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
        ಧ್ರುವ string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_ಅಂಕೆ);
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            ನೂತನ AstRefDType{fl, AstRefDType::Flag_Type_Of_ಅಭಿವ್ಯಕ್ತಿ{}, lhsp->clone_ವೃಕ್ಷ(ಸತ್ಯ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ಯದಿ (m_ftaskp) varp->funcLocal(ಸತ್ಯ);
        ಸೇರಿಸು_On_ತುದಿ(varp);
        Ast_ಗಂಟು* tempInitp = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        ಯದಿ (m_incCondp) {
            tempInitp = ನೂತನ AstIf{fl, m_incCondp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ), tempInitp, nullptr};
        }
        ಸೇರಿಸು_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(ನೂತನ AstVarRef{fl, varp, VAccess::READ});
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, varp, VAccess::READ};
    }
    ಶೂನ್ಯ handleShortCircuit(Ast_ಗಂಟು_Biop* ಧ್ರುವ nodep, ತಾರ್ಕಿಕ isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = ಸತ್ಯ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        ಯದಿ (nodep->rhsp()->exists([](ಧ್ರುವ Ast_ಗಂಟು* np) {
                ಪ್ರತ್ಯಾಗಮ VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            ಯದಿ (isOr) lhsRefp = ನೂತನ AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? ನೂತನ AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        ಯದಿ (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_ವೃಕ್ಷ(), ownedCondp);
    }
    ಶೂನ್ಯ visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/ಅಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/ಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    ಶೂನ್ಯ visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    ಶೂನ್ಯ visit(AstCond* nodep) override { unsupported_visit(nodep); }
    ಶೂನ್ಯ visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    ಶೂನ್ಯ prepost_visit(Ast_ಗಂಟು_Uniop* ಧ್ರುವ nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ಧ್ರುವ ತಾರ್ಕಿಕ isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* ಧ್ರುವ selbitp = VN_CAST(nodep->lhsp(), SelBit);
        ಯದಿ (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), ಗಂಟು_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } ಅನ್ಯಥಾ {
            ಯದಿ (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } ಅನ್ಯಥಾ {
                prepost_ಅಭಿವ್ಯಕ್ತಿ_visit(nodep);
            }
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getOperationp(Ast_ಗಂಟು* ಧ್ರುವ nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp,
                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp) {
        ಯದಿ (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSub{nodep->fileline(), lhsp, rhsp};
        }
        ಯದಿ (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        ಯದಿ (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            ವಿಕಲ್ಪ (assignp->operation()) {
            ಪಕ್ಷ AstAssignCompound::operation::Add:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAdd{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::And:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAnd{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::Div:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstDiv{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::ModDiv:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstModDiv{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::Mul:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstMul{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::Or: ಪ್ರತ್ಯಾಗಮ ನೂತನ AstOr{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::ShiftL:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstShiftL{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::ShiftR:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstShiftR{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::ShiftRS:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstShiftRS{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::Sub:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSub{nodep->fileline(), lhsp, rhsp};
            ಪಕ್ಷ AstAssignCompound::operation::Xor:
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstXor{nodep->fileline(), lhsp, rhsp};
            ಸ್ವಭಾವ:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    ಶೂನ್ಯ prepost_stmt_sel_visit(Ast_ಗಂಟು_Uniop* ಧ್ರುವ nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_ಸಾಲು* fl = nodep->fileline();
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, 32, 1, ಅಸತ್ಯ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = ನೂತನ AstConst{nodep->fileline(), ಅಂಕೆ_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    ಶೂನ್ಯ prepost_stmt_sel_visit(AstAssignCompound* ಧ್ರುವ nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    ಶೂನ್ಯ prepost_stmt_sel_visit(Ast_ಗಂಟು* ಧ್ರುವ nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp,
                                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp) {
        AstSelBit* ಧ್ರುವ rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ rdFromp = VN_AS(rdSelbitp->fromp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ), ಗಂಟು_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* ಧ್ರುವ wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಧ್ರುವ string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_ಅಂಕೆ);
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            ನೂತನ AstRefDType{fl, AstRefDType::Flag_Type_Of_ಅಭಿವ್ಯಕ್ತಿ{}, rdBitp->clone_ವೃಕ್ಷ(ಸತ್ಯ)}};
        ಯದಿ (m_ftaskp) varp->funcLocal(ಸತ್ಯ);

        // Declare the variable
        ಸೇರಿಸು_On_ತುದಿ(varp);

        // Define what operation will we be doing
        AstAssign* ಧ್ರುವ varAssignp
            = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_ಗಂಟು* ಧ್ರುವ newp = varAssignp;

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep
            = ನೂತನ AstSelBit{fl, rdFromp, ನೂತನ AstVarRef{fl, varp, VAccess::READ}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ store_ತುದಿ
            = ನೂತನ AstSelBit{fl, wrFromp, ನೂತನ AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = ನೂತನ AstAssign{nodep->fileline(), store_ತುದಿ, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ prepost_stmt_visit(Ast_ಗಂಟು_Uniop* ಧ್ರುವ nodep) {
        iterateChildren(nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ store_ತುದಿ = nodep->lhsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = nodep->lhsp()->unlinkFrBack();
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, 32, 1, ಅಸತ್ಯ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = ನೂತನ AstConst{nodep->fileline(), ಅಂಕೆ_One};

        prepost_stmt_visit(nodep, exprp, store_ತುದಿ, valuep);
    }
    ಶೂನ್ಯ prepost_stmt_visit(AstAssignCompound* ಧ್ರುವ nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ store_ತುದಿ = nodep->lhsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_ತುದಿ, valuep);
    }
    ಶೂನ್ಯ prepost_stmt_visit(Ast_ಗಂಟು* ಧ್ರುವ nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ store_ತುದಿ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* ಧ್ರುವ assignp
            = ನೂತನ AstAssign{nodep->fileline(), store_ತುದಿ, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ prepost_ಅಭಿವ್ಯಕ್ತಿ_visit(Ast_ಗಂಟು_Uniop* ಧ್ರುವ nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        ಯದಿ (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ readp = nodep->lhsp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ writep = nodep->lhsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, 32, 1, ಅಸತ್ಯ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newconstp = ನೂತನ AstConst{nodep->fileline(), ಅಂಕೆ_One};

        // Prepare a temporary variable
        ಧ್ರುವ string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_ಅಂಕೆ);
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            ನೂತನ AstRefDType{fl, AstRefDType::Flag_Type_Of_ಅಭಿವ್ಯಕ್ತಿ{}, readp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ಯದಿ (m_ftaskp) varp->funcLocal(ಸತ್ಯ);

        // Declare the variable
        ಸೇರಿಸು_On_ತುದಿ(varp);

        // Define what operation will we be doing
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ operp = getOperationp(nodep, readp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ), newconstp);

        Ast_ಗಂಟು* stmtsp = nullptr;
        ಯದಿ (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)};
            Ast_ಗಂಟು* ಧ್ರುವ incp = ನೂತನ AstAssign{fl, writep, operp};
            ಯದಿ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)});
            }
            assignp->addNextHere(ನೂತನ AstIf{fl, m_incCondp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ), incp, nullptr});
            stmtsp = assignp;
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* ಧ್ರುವ assignp
                = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(ನೂತನ AstAssign{fl, writep, ನೂತನ AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } ಅನ್ಯಥಾ {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)};
            // Increment the original variable by one
            assignp->addNext(ನೂತನ AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ನೂತನ AstVarRef{readp->fileline(), varp, VAccess::READ};
        ಯದಿ (m_inCovergroup) {
            newp = ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{fl, stmtsp, newp};
        } ಅನ್ಯಥಾ {
            ಸೇರಿಸು_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    ಶೂನ್ಯ visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    ಶೂನ್ಯ visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    ಶೂನ್ಯ visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    ಶೂನ್ಯ visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
        AstSelBit* ಧ್ರುವ selbitp = VN_CAST(nodep->lhsp(), SelBit);
        ಯದಿ (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), ಗಂಟು_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } ಅನ್ಯಥಾ {
            prepost_stmt_visit(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Task class functions

ಶೂನ್ಯ V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkinc", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
