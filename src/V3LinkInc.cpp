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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LinkInc.h"

#அந்தர்பாவ "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு LinkIncVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    மாறிலி VNUser1InUse m_inuser1;

    // STATE
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_முனை_Module* m_modp = nullptr;  // Module we're inside
    முழு m_mod_Compound_Assignments_இலக்கம் = 0;  // Var name counter
    Ast_முனை* m_insStmtp = nullptr;  // Where to insert statement
    தருக்க m_inCovergroup = பொய்;  // Under a covergroup's coverpoint, cross, or option
    தருக்க m_condEvalContext = பொய்;  // ++/-- is in a conditionally-evaluated position
    Ast_முனை_கோவை* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    வெறுமை செருகு_On_உச்சி(Ast_முனை* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        எனில் (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } இல்லையெனில் {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            எனில் (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    வெறுமை செருகு_Before_Stmt(Ast_முனை* nodep, Ast_முனை* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (nodep->dead()) திரும்பு;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_இலக்கம்);
        m_modp = nodep;
        m_mod_Compound_Assignments_இலக்கம் = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    வெறுமை visitCovergroup(Ast_முனை* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    வெறுமை visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    வெறுமை visit(Ast_முனை_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        எனில் (AstAssert* மாறிலி assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } இல்லையெனில் எனில் (AstAssertIntrinsic* மாறிலி intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    வெறுமை visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    வெறுமை visit(Ast_முனை_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    வெறுமை visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    வெறுமை visit(Ast_முனை_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    வெறுமை visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = உண்மை;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    வெறுமை visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    வெறுமை visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    வெறுமை visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    வெறுமை visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        Ast_முனை_கோவை* மாறிலி exprp = nodep->exprp();
        எனில் (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            திரும்பு;
        }
        visit(static_cast<Ast_முனை_Stmt*>(nodep));
    }
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    வெறுமை unsupported_visit(Ast_முனை* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = உண்மை;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_முனை_கோவை* captureLogicalLhsToTemp(Ast_முனை_Biop* மாறிலி nodep) {
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
        மாறிலி string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_இலக்கம்);
        AstVar* மாறிலி varp = புதிய AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            புதிய AstRefDType{fl, AstRefDType::Flag_Type_Of_கோவை{}, lhsp->clone_மரம்(உண்மை)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        எனில் (m_ftaskp) varp->funcLocal(உண்மை);
        செருகு_On_உச்சி(varp);
        Ast_முனை* tempInitp = புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        எனில் (m_incCondp) {
            tempInitp = புதிய AstIf{fl, m_incCondp->clone_மரம்_Pure(உண்மை), tempInitp, nullptr};
        }
        செருகு_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(புதிய AstVarRef{fl, varp, VAccess::READ});
        திரும்பு புதிய AstVarRef{fl, varp, VAccess::READ};
    }
    வெறுமை handleShortCircuit(Ast_முனை_Biop* மாறிலி nodep, தருக்க isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        எனில் (nodep->user1SetOnce()) திரும்பு;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = உண்மை;
        Ast_முனை_கோவை* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        எனில் (nodep->rhsp()->exists([](மாறிலி Ast_முனை* np) {
                திரும்பு VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_முனை_கோவை* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_மரம்_Pure(உண்மை)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            எனில் (isOr) lhsRefp = புதிய AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? புதிய AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_மரம்_Pure(உண்மை), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        எனில் (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_மரம்(), ownedCondp);
    }
    வெறுமை visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/பொய்); }
    வெறுமை visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/உண்மை); }
    வெறுமை visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    வெறுமை visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    வெறுமை visit(AstCond* nodep) override { unsupported_visit(nodep); }
    வெறுமை visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    வெறுமை prepost_visit(Ast_முனை_Uniop* மாறிலி nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        மாறிலி தருக்க isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* மாறிலி selbitp = VN_CAST(nodep->lhsp(), SelBit);
        எனில் (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), முனை_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } இல்லையெனில் {
            எனில் (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } இல்லையெனில் {
                prepost_கோவை_visit(nodep);
            }
        }
    }
    Ast_முனை_கோவை* getOperationp(Ast_முனை* மாறிலி nodep, Ast_முனை_கோவை* மாறிலி lhsp,
                               Ast_முனை_கோவை* மாறிலி rhsp) {
        எனில் (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            திரும்பு புதிய AstSub{nodep->fileline(), lhsp, rhsp};
        }
        எனில் (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            திரும்பு புதிய AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        எனில் (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            தேர்வு (assignp->operation()) {
            நிலை AstAssignCompound::operation::Add:
                திரும்பு புதிய AstAdd{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::And:
                திரும்பு புதிய AstAnd{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::Div:
                திரும்பு புதிய AstDiv{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::ModDiv:
                திரும்பு புதிய AstModDiv{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::Mul:
                திரும்பு புதிய AstMul{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::Or: திரும்பு புதிய AstOr{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::ShiftL:
                திரும்பு புதிய AstShiftL{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::ShiftR:
                திரும்பு புதிய AstShiftR{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::ShiftRS:
                திரும்பு புதிய AstShiftRS{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::Sub:
                திரும்பு புதிய AstSub{nodep->fileline(), lhsp, rhsp};
            நிலை AstAssignCompound::operation::Xor:
                திரும்பு புதிய AstXor{nodep->fileline(), lhsp, rhsp};
            இயல்பு:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    வெறுமை prepost_stmt_sel_visit(Ast_முனை_Uniop* மாறிலி nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_வரி* fl = nodep->fileline();
        V3_எண் இலக்கம்_One{fl, 32, 1, பொய்};
        Ast_முனை_கோவை* மாறிலி exprp = புதிய AstConst{nodep->fileline(), இலக்கம்_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    வெறுமை prepost_stmt_sel_visit(AstAssignCompound* மாறிலி nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_முனை_கோவை* மாறிலி exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    வெறுமை prepost_stmt_sel_visit(Ast_முனை* மாறிலி nodep, Ast_முனை_கோவை* மாறிலி lhsp,
                                Ast_முனை_கோவை* மாறிலி exprp) {
        AstSelBit* மாறிலி rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_முனை_Var_Ref* மாறிலி rdFromp = VN_AS(rdSelbitp->fromp()->clone_மரம்_Pure(உண்மை), முனை_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_முனை_கோவை* மாறிலி rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* மாறிலி wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_முனை_கோவை* மாறிலி wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_வரி* மாறிலி fl = nodep->fileline();
        மாறிலி string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_இலக்கம்);
        AstVar* மாறிலி varp = புதிய AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            புதிய AstRefDType{fl, AstRefDType::Flag_Type_Of_கோவை{}, rdBitp->clone_மரம்(உண்மை)}};
        எனில் (m_ftaskp) varp->funcLocal(உண்மை);

        // Declare the variable
        செருகு_On_உச்சி(varp);

        // Define what operation will we be doing
        AstAssign* மாறிலி varAssignp
            = புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_முனை* மாறிலி newp = varAssignp;

        Ast_முனை_கோவை* மாறிலி valuep
            = புதிய AstSelBit{fl, rdFromp, புதிய AstVarRef{fl, varp, VAccess::READ}};
        Ast_முனை_கோவை* மாறிலி store_உச்சி
            = புதிய AstSelBit{fl, wrFromp, புதிய AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = புதிய AstAssign{nodep->fileline(), store_உச்சி, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை prepost_stmt_visit(Ast_முனை_Uniop* மாறிலி nodep) {
        iterateChildren(nodep);
        Ast_முனை_கோவை* மாறிலி store_உச்சி = nodep->lhsp()->clone_மரம்_Pure(உண்மை);
        Ast_முனை_கோவை* மாறிலி valuep = nodep->lhsp()->unlinkFrBack();
        File_வரி* மாறிலி fl = nodep->fileline();
        V3_எண் இலக்கம்_One{fl, 32, 1, பொய்};
        Ast_முனை_கோவை* மாறிலி exprp = புதிய AstConst{nodep->fileline(), இலக்கம்_One};

        prepost_stmt_visit(nodep, exprp, store_உச்சி, valuep);
    }
    வெறுமை prepost_stmt_visit(AstAssignCompound* மாறிலி nodep) {
        Ast_முனை_கோவை* மாறிலி exprp = nodep->rhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி store_உச்சி = nodep->lhsp()->clone_மரம்_Pure(உண்மை);
        Ast_முனை_கோவை* மாறிலி valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_உச்சி, valuep);
    }
    வெறுமை prepost_stmt_visit(Ast_முனை* மாறிலி nodep, Ast_முனை_கோவை* மாறிலி exprp,
                            Ast_முனை_கோவை* மாறிலி store_உச்சி, Ast_முனை_கோவை* மாறிலி valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* மாறிலி assignp
            = புதிய AstAssign{nodep->fileline(), store_உச்சி, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை prepost_கோவை_visit(Ast_முனை_Uniop* மாறிலி nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        எனில் (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            திரும்பு;
        }
        மாறிலி தருக்க needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_முனை_கோவை* மாறிலி readp = nodep->lhsp();
        Ast_முனை_கோவை* மாறிலி writep = nodep->lhsp()->clone_மரம்_Pure(உண்மை);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_வரி* மாறிலி fl = nodep->fileline();
        V3_எண் இலக்கம்_One{fl, 32, 1, பொய்};
        Ast_முனை_கோவை* மாறிலி newconstp = புதிய AstConst{nodep->fileline(), இலக்கம்_One};

        // Prepare a temporary variable
        மாறிலி string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_இலக்கம்);
        AstVar* மாறிலி varp = புதிய AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            புதிய AstRefDType{fl, AstRefDType::Flag_Type_Of_கோவை{}, readp->clone_மரம்_Pure(உண்மை)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        எனில் (m_ftaskp) varp->funcLocal(உண்மை);

        // Declare the variable
        செருகு_On_உச்சி(varp);

        // Define what operation will we be doing
        Ast_முனை_கோவை* மாறிலி operp = getOperationp(nodep, readp->clone_மரம்_Pure(உண்மை), newconstp);

        Ast_முனை* stmtsp = nullptr;
        எனில் (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* மாறிலி assignp = புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_மரம்_Pure(உண்மை)};
            Ast_முனை* மாறிலி incp = புதிய AstAssign{fl, writep, operp};
            எனில் (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_மரம்_Pure(உண்மை)});
            }
            assignp->addNextHere(புதிய AstIf{fl, m_incCondp->clone_மரம்_Pure(உண்மை), incp, nullptr});
            stmtsp = assignp;
        } இல்லையெனில் எனில் (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* மாறிலி assignp
                = புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(புதிய AstAssign{fl, writep, புதிய AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } இல்லையெனில் {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* மாறிலி assignp = புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_மரம்_Pure(உண்மை)};
            // Increment the original variable by one
            assignp->addNext(புதிய AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_முனை_கோவை* newp = புதிய AstVarRef{readp->fileline(), varp, VAccess::READ};
        எனில் (m_inCovergroup) {
            newp = புதிய Ast_கோவை_Stmt{fl, stmtsp, newp};
        } இல்லையெனில் {
            செருகு_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    வெறுமை visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    வெறுமை visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    வெறுமை visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    வெறுமை visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_முனை_Stmt*>(nodep));
        AstSelBit* மாறிலி selbitp = VN_CAST(nodep->lhsp(), SelBit);
        எனில் (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), முனை_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } இல்லையெனில் {
            prepost_stmt_visit(nodep);
        }
    }
    வெறுமை visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = இயல்பு;
};

//######################################################################
// Task class functions

வெறுமை V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("linkinc", 0, dump_மரம்_Either_Level() >= 3);
}
