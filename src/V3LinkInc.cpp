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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkInc.h"

#অন্তর্ভাব "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

বর্গ LinkIncVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    ধ্রুব VNUser1InUse m_inuser1;

    // STATE
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Module we're inside
    পূর্ণাঙ্ক m_mod_Compound_Assignments_অঙ্ক = 0;  // Var name counter
    Ast_গ্রন্থি* m_insStmtp = nullptr;  // Where to insert statement
    তার্কিক m_inCovergroup = অসত্য;  // Under a covergroup's coverpoint, cross, or option
    তার্কিক m_condEvalContext = অসত্য;  // ++/-- is in a conditionally-evaluated position
    Ast_গ্রন্থি_রাশিমালা* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    শূন্য ঢোকাও_On_শীর্ষ(Ast_গ্রন্থি* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        যদি (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } অন্যথা {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            যদি (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    শূন্য ঢোকাও_Before_Stmt(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        যদি (nodep->dead()) প্রত্যাগম;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_অঙ্ক);
        m_modp = nodep;
        m_mod_Compound_Assignments_অঙ্ক = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    শূন্য visitCovergroup(Ast_গ্রন্থি* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    শূন্য visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    শূন্য visit(Ast_গ্রন্থি_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        যদি (AstAssert* ধ্রুব assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } অন্যথা যদি (AstAssertIntrinsic* ধ্রুব intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    শূন্য visit(Ast_গ্রন্থি_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    শূন্য visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    শূন্য visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = সত্য;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    শূন্য visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    শূন্য visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    শূন্য visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    শূন্য visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->exprp();
        যদি (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            প্রত্যাগম;
        }
        visit(static_cast<Ast_গ্রন্থি_Stmt*>(nodep));
    }
    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    শূন্য unsupported_visit(Ast_গ্রন্থি* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = সত্য;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_গ্রন্থি_রাশিমালা* captureLogicalLhsToTemp(Ast_গ্রন্থি_Biop* ধ্রুব nodep) {
        File_সারি* ধ্রুব fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp = nodep->lhsp()->unlinkFrBack();
        ধ্রুব string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_অঙ্ক);
        AstVar* ধ্রুব varp = নূতন AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            নূতন AstRefDType{fl, AstRefDType::Flag_Type_Of_রাশিমালা{}, lhsp->clone_বৃক্ষ(সত্য)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        যদি (m_ftaskp) varp->funcLocal(সত্য);
        ঢোকাও_On_শীর্ষ(varp);
        Ast_গ্রন্থি* tempInitp = নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        যদি (m_incCondp) {
            tempInitp = নূতন AstIf{fl, m_incCondp->clone_বৃক্ষ_Pure(সত্য), tempInitp, nullptr};
        }
        ঢোকাও_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(নূতন AstVarRef{fl, varp, VAccess::READ});
        প্রত্যাগম নূতন AstVarRef{fl, varp, VAccess::READ};
    }
    শূন্য handleShortCircuit(Ast_গ্রন্থি_Biop* ধ্রুব nodep, তার্কিক isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = সত্য;
        Ast_গ্রন্থি_রাশিমালা* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        যদি (nodep->rhsp()->exists([](ধ্রুব Ast_গ্রন্থি* np) {
                প্রত্যাগম VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_গ্রন্থি_রাশিমালা* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_বৃক্ষ_Pure(সত্য)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            যদি (isOr) lhsRefp = নূতন AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? নূতন AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_বৃক্ষ_Pure(সত্য), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        যদি (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_বৃক্ষ(), ownedCondp);
    }
    শূন্য visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/অসত্য); }
    শূন্য visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/সত্য); }
    শূন্য visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    শূন্য visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    শূন্য visit(AstCond* nodep) override { unsupported_visit(nodep); }
    শূন্য visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    শূন্য prepost_visit(Ast_গ্রন্থি_Uniop* ধ্রুব nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ধ্রুব তার্কিক isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* ধ্রুব selbitp = VN_CAST(nodep->lhsp(), SelBit);
        যদি (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), গ্রন্থি_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } অন্যথা {
            যদি (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } অন্যথা {
                prepost_রাশিমালা_visit(nodep);
            }
        }
    }
    Ast_গ্রন্থি_রাশিমালা* getOperationp(Ast_গ্রন্থি* ধ্রুব nodep, Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp,
                               Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp) {
        যদি (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            প্রত্যাগম নূতন AstSub{nodep->fileline(), lhsp, rhsp};
        }
        যদি (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            প্রত্যাগম নূতন AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        যদি (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            বিকল্প (assignp->operation()) {
            পক্ষ AstAssignCompound::operation::Add:
                প্রত্যাগম নূতন AstAdd{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::And:
                প্রত্যাগম নূতন AstAnd{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::Div:
                প্রত্যাগম নূতন AstDiv{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::ModDiv:
                প্রত্যাগম নূতন AstModDiv{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::Mul:
                প্রত্যাগম নূতন AstMul{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::Or: প্রত্যাগম নূতন AstOr{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::ShiftL:
                প্রত্যাগম নূতন AstShiftL{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::ShiftR:
                প্রত্যাগম নূতন AstShiftR{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::ShiftRS:
                প্রত্যাগম নূতন AstShiftRS{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::Sub:
                প্রত্যাগম নূতন AstSub{nodep->fileline(), lhsp, rhsp};
            পক্ষ AstAssignCompound::operation::Xor:
                প্রত্যাগম নূতন AstXor{nodep->fileline(), lhsp, rhsp};
            স্বভাব:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    শূন্য prepost_stmt_sel_visit(Ast_গ্রন্থি_Uniop* ধ্রুব nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_সারি* fl = nodep->fileline();
        V3_সংখ্যা অঙ্ক_One{fl, 32, 1, অসত্য};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = নূতন AstConst{nodep->fileline(), অঙ্ক_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    শূন্য prepost_stmt_sel_visit(AstAssignCompound* ধ্রুব nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    শূন্য prepost_stmt_sel_visit(Ast_গ্রন্থি* ধ্রুব nodep, Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhsp,
                                Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp) {
        AstSelBit* ধ্রুব rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_গ্রন্থি_Var_Ref* ধ্রুব rdFromp = VN_AS(rdSelbitp->fromp()->clone_বৃক্ষ_Pure(সত্য), গ্রন্থি_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* ধ্রুব wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_সারি* ধ্রুব fl = nodep->fileline();
        ধ্রুব string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_অঙ্ক);
        AstVar* ধ্রুব varp = নূতন AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            নূতন AstRefDType{fl, AstRefDType::Flag_Type_Of_রাশিমালা{}, rdBitp->clone_বৃক্ষ(সত্য)}};
        যদি (m_ftaskp) varp->funcLocal(সত্য);

        // Declare the variable
        ঢোকাও_On_শীর্ষ(varp);

        // Define what operation will we be doing
        AstAssign* ধ্রুব varAssignp
            = নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_গ্রন্থি* ধ্রুব newp = varAssignp;

        Ast_গ্রন্থি_রাশিমালা* ধ্রুব valuep
            = নূতন AstSelBit{fl, rdFromp, নূতন AstVarRef{fl, varp, VAccess::READ}};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব store_শীর্ষ
            = নূতন AstSelBit{fl, wrFromp, নূতন AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = নূতন AstAssign{nodep->fileline(), store_শীর্ষ, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য prepost_stmt_visit(Ast_গ্রন্থি_Uniop* ধ্রুব nodep) {
        iterateChildren(nodep);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব store_শীর্ষ = nodep->lhsp()->clone_বৃক্ষ_Pure(সত্য);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব valuep = nodep->lhsp()->unlinkFrBack();
        File_সারি* ধ্রুব fl = nodep->fileline();
        V3_সংখ্যা অঙ্ক_One{fl, 32, 1, অসত্য};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = নূতন AstConst{nodep->fileline(), অঙ্ক_One};

        prepost_stmt_visit(nodep, exprp, store_শীর্ষ, valuep);
    }
    শূন্য prepost_stmt_visit(AstAssignCompound* ধ্রুব nodep) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব store_শীর্ষ = nodep->lhsp()->clone_বৃক্ষ_Pure(সত্য);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_শীর্ষ, valuep);
    }
    শূন্য prepost_stmt_visit(Ast_গ্রন্থি* ধ্রুব nodep, Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp,
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুব store_শীর্ষ, Ast_গ্রন্থি_রাশিমালা* ধ্রুব valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* ধ্রুব assignp
            = নূতন AstAssign{nodep->fileline(), store_শীর্ষ, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য prepost_রাশিমালা_visit(Ast_গ্রন্থি_Uniop* ধ্রুব nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        যদি (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            প্রত্যাগম;
        }
        ধ্রুব তার্কিক needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব readp = nodep->lhsp();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব writep = nodep->lhsp()->clone_বৃক্ষ_Pure(সত্য);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_সারি* ধ্রুব fl = nodep->fileline();
        V3_সংখ্যা অঙ্ক_One{fl, 32, 1, অসত্য};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব newconstp = নূতন AstConst{nodep->fileline(), অঙ্ক_One};

        // Prepare a temporary variable
        ধ্রুব string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_অঙ্ক);
        AstVar* ধ্রুব varp = নূতন AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            নূতন AstRefDType{fl, AstRefDType::Flag_Type_Of_রাশিমালা{}, readp->clone_বৃক্ষ_Pure(সত্য)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        যদি (m_ftaskp) varp->funcLocal(সত্য);

        // Declare the variable
        ঢোকাও_On_শীর্ষ(varp);

        // Define what operation will we be doing
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব operp = getOperationp(nodep, readp->clone_বৃক্ষ_Pure(সত্য), newconstp);

        Ast_গ্রন্থি* stmtsp = nullptr;
        যদি (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* ধ্রুব assignp = নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_বৃক্ষ_Pure(সত্য)};
            Ast_গ্রন্থি* ধ্রুব incp = নূতন AstAssign{fl, writep, operp};
            যদি (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_বৃক্ষ_Pure(সত্য)});
            }
            assignp->addNextHere(নূতন AstIf{fl, m_incCondp->clone_বৃক্ষ_Pure(সত্য), incp, nullptr});
            stmtsp = assignp;
        } অন্যথা যদি (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* ধ্রুব assignp
                = নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(নূতন AstAssign{fl, writep, নূতন AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } অন্যথা {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* ধ্রুব assignp = নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_বৃক্ষ_Pure(সত্য)};
            // Increment the original variable by one
            assignp->addNext(নূতন AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_গ্রন্থি_রাশিমালা* newp = নূতন AstVarRef{readp->fileline(), varp, VAccess::READ};
        যদি (m_inCovergroup) {
            newp = নূতন Ast_রাশিমালা_Stmt{fl, stmtsp, newp};
        } অন্যথা {
            ঢোকাও_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    শূন্য visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    শূন্য visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    শূন্য visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    শূন্য visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_গ্রন্থি_Stmt*>(nodep));
        AstSelBit* ধ্রুব selbitp = VN_CAST(nodep->lhsp(), SelBit);
        যদি (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), গ্রন্থি_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } অন্যথা {
            prepost_stmt_visit(nodep);
        }
    }
    শূন্য visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = স্বভাব;
};

//######################################################################
// Task class functions

শূন্য V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("linkinc", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
