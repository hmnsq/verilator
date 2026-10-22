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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkInc.h"

#અન્તર્ભાવ "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ LinkIncVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    અચલ VNUser1InUse m_inuser1;

    // STATE
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Module we're inside
    પૂર્ણાંક m_mod_Compound_Assignments_અંક = 0;  // Var name counter
    Ast_ગાંઠ* m_insStmtp = nullptr;  // Where to insert statement
    તાર્કિક m_inCovergroup = ખોટું;  // Under a covergroup's coverpoint, cross, or option
    તાર્કિક m_condEvalContext = ખોટું;  // ++/-- is in a conditionally-evaluated position
    Ast_ગાંઠ_પદાવલિ* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    શૂન્ય ઉમેરો_On_ટોચ(Ast_ગાંઠ* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        જો (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } નહીંતર {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            જો (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    શૂન્ય ઉમેરો_Before_Stmt(Ast_ગાંઠ* nodep, Ast_ગાંઠ* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (nodep->dead()) પાછા;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_અંક);
        m_modp = nodep;
        m_mod_Compound_Assignments_અંક = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    શૂન્ય visitCovergroup(Ast_ગાંઠ* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    શૂન્ય visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    શૂન્ય visit(Ast_ગાંઠ_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        જો (AstAssert* અચલ assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } નહીંતર જો (AstAssertIntrinsic* અચલ intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    શૂન્ય visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    શૂન્ય visit(Ast_ગાંઠ_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    શૂન્ય visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    શૂન્ય visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = સાચું;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    શૂન્ય visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    શૂન્ય visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    શૂન્ય visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    શૂન્ય visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->exprp();
        જો (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            પાછા;
        }
        visit(static_cast<Ast_ગાંઠ_Stmt*>(nodep));
    }
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    શૂન્ય unsupported_visit(Ast_ગાંઠ* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = સાચું;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_ગાંઠ_પદાવલિ* captureLogicalLhsToTemp(Ast_ગાંઠ_Biop* અચલ nodep) {
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
        અચલ string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_અંક);
        AstVar* અચલ varp = નવું AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            નવું AstRefDType{fl, AstRefDType::Flag_Type_Of_પદાવલિ{}, lhsp->clone_વૃક્ષ(સાચું)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        જો (m_ftaskp) varp->funcLocal(સાચું);
        ઉમેરો_On_ટોચ(varp);
        Ast_ગાંઠ* tempInitp = નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        જો (m_incCondp) {
            tempInitp = નવું AstIf{fl, m_incCondp->clone_વૃક્ષ_Pure(સાચું), tempInitp, nullptr};
        }
        ઉમેરો_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(નવું AstVarRef{fl, varp, VAccess::READ});
        પાછા નવું AstVarRef{fl, varp, VAccess::READ};
    }
    શૂન્ય handleShortCircuit(Ast_ગાંઠ_Biop* અચલ nodep, તાર્કિક isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        જો (nodep->user1SetOnce()) પાછા;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = સાચું;
        Ast_ગાંઠ_પદાવલિ* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        જો (nodep->rhsp()->exists([](અચલ Ast_ગાંઠ* np) {
                પાછા VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_ગાંઠ_પદાવલિ* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_વૃક્ષ_Pure(સાચું)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            જો (isOr) lhsRefp = નવું AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? નવું AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_વૃક્ષ_Pure(સાચું), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        જો (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_વૃક્ષ(), ownedCondp);
    }
    શૂન્ય visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/ખોટું); }
    શૂન્ય visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/સાચું); }
    શૂન્ય visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    શૂન્ય visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    શૂન્ય visit(AstCond* nodep) override { unsupported_visit(nodep); }
    શૂન્ય visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    શૂન્ય prepost_visit(Ast_ગાંઠ_Uniop* અચલ nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        અચલ તાર્કિક isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* અચલ selbitp = VN_CAST(nodep->lhsp(), SelBit);
        જો (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), ગાંઠ_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } નહીંતર {
            જો (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } નહીંતર {
                prepost_પદાવલિ_visit(nodep);
            }
        }
    }
    Ast_ગાંઠ_પદાવલિ* getOperationp(Ast_ગાંઠ* અચલ nodep, Ast_ગાંઠ_પદાવલિ* અચલ lhsp,
                               Ast_ગાંઠ_પદાવલિ* અચલ rhsp) {
        જો (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            પાછા નવું AstSub{nodep->fileline(), lhsp, rhsp};
        }
        જો (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            પાછા નવું AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        જો (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            પસંદગી (assignp->operation()) {
            સ્થિતિ AstAssignCompound::operation::Add:
                પાછા નવું AstAdd{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::And:
                પાછા નવું AstAnd{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::Div:
                પાછા નવું AstDiv{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::ModDiv:
                પાછા નવું AstModDiv{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::Mul:
                પાછા નવું AstMul{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::Or: પાછા નવું AstOr{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::ShiftL:
                પાછા નવું AstShiftL{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::ShiftR:
                પાછા નવું AstShiftR{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::ShiftRS:
                પાછા નવું AstShiftRS{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::Sub:
                પાછા નવું AstSub{nodep->fileline(), lhsp, rhsp};
            સ્થિતિ AstAssignCompound::operation::Xor:
                પાછા નવું AstXor{nodep->fileline(), lhsp, rhsp};
            મૂળભૂત:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    શૂન્ય prepost_stmt_sel_visit(Ast_ગાંઠ_Uniop* અચલ nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_લીટી* fl = nodep->fileline();
        V3_સંખ્યા અંક_One{fl, 32, 1, ખોટું};
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = નવું AstConst{nodep->fileline(), અંક_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    શૂન્ય prepost_stmt_sel_visit(AstAssignCompound* અચલ nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    શૂન્ય prepost_stmt_sel_visit(Ast_ગાંઠ* અચલ nodep, Ast_ગાંઠ_પદાવલિ* અચલ lhsp,
                                Ast_ગાંઠ_પદાવલિ* અચલ exprp) {
        AstSelBit* અચલ rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_ગાંઠ_Var_Ref* અચલ rdFromp = VN_AS(rdSelbitp->fromp()->clone_વૃક્ષ_Pure(સાચું), ગાંઠ_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_ગાંઠ_પદાવલિ* અચલ rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* અચલ wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_ગાંઠ_પદાવલિ* અચલ wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_લીટી* અચલ fl = nodep->fileline();
        અચલ string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_અંક);
        AstVar* અચલ varp = નવું AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            નવું AstRefDType{fl, AstRefDType::Flag_Type_Of_પદાવલિ{}, rdBitp->clone_વૃક્ષ(સાચું)}};
        જો (m_ftaskp) varp->funcLocal(સાચું);

        // Declare the variable
        ઉમેરો_On_ટોચ(varp);

        // Define what operation will we be doing
        AstAssign* અચલ varAssignp
            = નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_ગાંઠ* અચલ newp = varAssignp;

        Ast_ગાંઠ_પદાવલિ* અચલ valuep
            = નવું AstSelBit{fl, rdFromp, નવું AstVarRef{fl, varp, VAccess::READ}};
        Ast_ગાંઠ_પદાવલિ* અચલ store_ટોચ
            = નવું AstSelBit{fl, wrFromp, નવું AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = નવું AstAssign{nodep->fileline(), store_ટોચ, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય prepost_stmt_visit(Ast_ગાંઠ_Uniop* અચલ nodep) {
        iterateChildren(nodep);
        Ast_ગાંઠ_પદાવલિ* અચલ store_ટોચ = nodep->lhsp()->clone_વૃક્ષ_Pure(સાચું);
        Ast_ગાંઠ_પદાવલિ* અચલ valuep = nodep->lhsp()->unlinkFrBack();
        File_લીટી* અચલ fl = nodep->fileline();
        V3_સંખ્યા અંક_One{fl, 32, 1, ખોટું};
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = નવું AstConst{nodep->fileline(), અંક_One};

        prepost_stmt_visit(nodep, exprp, store_ટોચ, valuep);
    }
    શૂન્ય prepost_stmt_visit(AstAssignCompound* અચલ nodep) {
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ store_ટોચ = nodep->lhsp()->clone_વૃક્ષ_Pure(સાચું);
        Ast_ગાંઠ_પદાવલિ* અચલ valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_ટોચ, valuep);
    }
    શૂન્ય prepost_stmt_visit(Ast_ગાંઠ* અચલ nodep, Ast_ગાંઠ_પદાવલિ* અચલ exprp,
                            Ast_ગાંઠ_પદાવલિ* અચલ store_ટોચ, Ast_ગાંઠ_પદાવલિ* અચલ valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* અચલ assignp
            = નવું AstAssign{nodep->fileline(), store_ટોચ, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય prepost_પદાવલિ_visit(Ast_ગાંઠ_Uniop* અચલ nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        જો (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            પાછા;
        }
        અચલ તાર્કિક needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_ગાંઠ_પદાવલિ* અચલ readp = nodep->lhsp();
        Ast_ગાંઠ_પદાવલિ* અચલ writep = nodep->lhsp()->clone_વૃક્ષ_Pure(સાચું);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_લીટી* અચલ fl = nodep->fileline();
        V3_સંખ્યા અંક_One{fl, 32, 1, ખોટું};
        Ast_ગાંઠ_પદાવલિ* અચલ newconstp = નવું AstConst{nodep->fileline(), અંક_One};

        // Prepare a temporary variable
        અચલ string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_અંક);
        AstVar* અચલ varp = નવું AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            નવું AstRefDType{fl, AstRefDType::Flag_Type_Of_પદાવલિ{}, readp->clone_વૃક્ષ_Pure(સાચું)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        જો (m_ftaskp) varp->funcLocal(સાચું);

        // Declare the variable
        ઉમેરો_On_ટોચ(varp);

        // Define what operation will we be doing
        Ast_ગાંઠ_પદાવલિ* અચલ operp = getOperationp(nodep, readp->clone_વૃક્ષ_Pure(સાચું), newconstp);

        Ast_ગાંઠ* stmtsp = nullptr;
        જો (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* અચલ assignp = નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_વૃક્ષ_Pure(સાચું)};
            Ast_ગાંઠ* અચલ incp = નવું AstAssign{fl, writep, operp};
            જો (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_વૃક્ષ_Pure(સાચું)});
            }
            assignp->addNextHere(નવું AstIf{fl, m_incCondp->clone_વૃક્ષ_Pure(સાચું), incp, nullptr});
            stmtsp = assignp;
        } નહીંતર જો (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* અચલ assignp
                = નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(નવું AstAssign{fl, writep, નવું AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } નહીંતર {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* અચલ assignp = નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_વૃક્ષ_Pure(સાચું)};
            // Increment the original variable by one
            assignp->addNext(નવું AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_ગાંઠ_પદાવલિ* newp = નવું AstVarRef{readp->fileline(), varp, VAccess::READ};
        જો (m_inCovergroup) {
            newp = નવું Ast_પદાવલિ_Stmt{fl, stmtsp, newp};
        } નહીંતર {
            ઉમેરો_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    શૂન્ય visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    શૂન્ય visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    શૂન્ય visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    શૂન્ય visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_ગાંઠ_Stmt*>(nodep));
        AstSelBit* અચલ selbitp = VN_CAST(nodep->lhsp(), SelBit);
        જો (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), ગાંઠ_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } નહીંતર {
            prepost_stmt_visit(nodep);
        }
    }
    શૂન્ય visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = મૂળભૂત;
};

//######################################################################
// Task class functions

શૂન્ય V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("linkinc", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
