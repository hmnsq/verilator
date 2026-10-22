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

رده LinkIncVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    ثابت VNUser1InUse m_inuser1;

    // STATE
    Ast_گره_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_گره_Module* m_modp = nullptr;  // Module we're inside
    صحیح m_mod_Compound_Assignments_رقم = 0;  // Var name counter
    Ast_گره* m_insStmtp = nullptr;  // Where to insert statement
    منطقی m_inCovergroup = نادرست;  // Under a covergroup's coverpoint, cross, or option
    منطقی m_condEvalContext = نادرست;  // ++/-- is in a conditionally-evaluated position
    Ast_گره_عبارت* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    تهی درج_On_بالاترین(Ast_گره* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        اگر (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } وگرنه {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            اگر (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    تهی درج_Before_Stmt(Ast_گره* nodep, Ast_گره* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        اگر (nodep->dead()) بازگشت;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_رقم);
        m_modp = nodep;
        m_mod_Compound_Assignments_رقم = 0;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    تهی visitCovergroup(Ast_گره* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = درست;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    تهی visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    تهی visit(Ast_گره_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        اگر (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } وگرنه اگر (AstAssertIntrinsic* ثابت intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    تهی visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    تهی visit(Ast_گره_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    تهی visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    تهی visit(Ast_گره_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    تهی visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = درست;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    تهی visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    تهی visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    تهی visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    تهی visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    تهی visit(Ast_Stmt_عبارت* nodep) override {
        Ast_گره_عبارت* ثابت exprp = nodep->exprp();
        اگر (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            بازگشت;
        }
        visit(static_cast<Ast_گره_Stmt*>(nodep));
    }
    تهی visit(Ast_گره_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    تهی unsupported_visit(Ast_گره* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = درست;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_گره_عبارت* captureLogicalLhsToTemp(Ast_گره_Biop* ثابت nodep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        ثابت string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جدید AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            جدید AstRefDType{fl, AstRefDType::Flag_Type_Of_عبارت{}, lhsp->clone_درخت(درست)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        اگر (m_ftaskp) varp->funcLocal(درست);
        درج_On_بالاترین(varp);
        Ast_گره* tempInitp = جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        اگر (m_incCondp) {
            tempInitp = جدید AstIf{fl, m_incCondp->clone_درخت_Pure(درست), tempInitp, nullptr};
        }
        درج_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(جدید AstVarRef{fl, varp, VAccess::READ});
        بازگشت جدید AstVarRef{fl, varp, VAccess::READ};
    }
    تهی handleShortCircuit(Ast_گره_Biop* ثابت nodep, منطقی isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        اگر (nodep->user1SetOnce()) بازگشت;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = درست;
        Ast_گره_عبارت* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        اگر (nodep->rhsp()->exists([](ثابت Ast_گره* np) {
                بازگشت VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_گره_عبارت* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_درخت_Pure(درست)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            اگر (isOr) lhsRefp = جدید AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? جدید AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_درخت_Pure(درست), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        اگر (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_درخت(), ownedCondp);
    }
    تهی visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/نادرست); }
    تهی visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/درست); }
    تهی visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    تهی visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    تهی visit(AstCond* nodep) override { unsupported_visit(nodep); }
    تهی visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    تهی prepost_visit(Ast_گره_Uniop* ثابت nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ثابت منطقی isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* ثابت selbitp = VN_CAST(nodep->lhsp(), SelBit);
        اگر (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), گره_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } وگرنه {
            اگر (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } وگرنه {
                prepost_عبارت_visit(nodep);
            }
        }
    }
    Ast_گره_عبارت* getOperationp(Ast_گره* ثابت nodep, Ast_گره_عبارت* ثابت lhsp,
                               Ast_گره_عبارت* ثابت rhsp) {
        اگر (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            بازگشت جدید AstSub{nodep->fileline(), lhsp, rhsp};
        }
        اگر (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            بازگشت جدید AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        اگر (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            انتخاب (assignp->operation()) {
            حالت AstAssignCompound::operation::Add:
                بازگشت جدید AstAdd{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::And:
                بازگشت جدید AstAnd{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::Div:
                بازگشت جدید AstDiv{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::ModDiv:
                بازگشت جدید AstModDiv{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::Mul:
                بازگشت جدید AstMul{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::Or: بازگشت جدید AstOr{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::ShiftL:
                بازگشت جدید AstShiftL{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::ShiftR:
                بازگشت جدید AstShiftR{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::ShiftRS:
                بازگشت جدید AstShiftRS{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::Sub:
                بازگشت جدید AstSub{nodep->fileline(), lhsp, rhsp};
            حالت AstAssignCompound::operation::Xor:
                بازگشت جدید AstXor{nodep->fileline(), lhsp, rhsp};
            پیشفرض:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    تهی prepost_stmt_sel_visit(Ast_گره_Uniop* ثابت nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_سطر* fl = nodep->fileline();
        V3_عدد رقم_One{fl, 32, 1, نادرست};
        Ast_گره_عبارت* ثابت exprp = جدید AstConst{nodep->fileline(), رقم_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    تهی prepost_stmt_sel_visit(AstAssignCompound* ثابت nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_گره_عبارت* ثابت exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    تهی prepost_stmt_sel_visit(Ast_گره* ثابت nodep, Ast_گره_عبارت* ثابت lhsp,
                                Ast_گره_عبارت* ثابت exprp) {
        AstSelBit* ثابت rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_گره_Var_Ref* ثابت rdFromp = VN_AS(rdSelbitp->fromp()->clone_درخت_Pure(درست), گره_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_گره_عبارت* ثابت rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* ثابت wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_گره_عبارت* ثابت wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_سطر* ثابت fl = nodep->fileline();
        ثابت string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جدید AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            جدید AstRefDType{fl, AstRefDType::Flag_Type_Of_عبارت{}, rdBitp->clone_درخت(درست)}};
        اگر (m_ftaskp) varp->funcLocal(درست);

        // Declare the variable
        درج_On_بالاترین(varp);

        // Define what operation will we be doing
        AstAssign* ثابت varAssignp
            = جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_گره* ثابت newp = varAssignp;

        Ast_گره_عبارت* ثابت valuep
            = جدید AstSelBit{fl, rdFromp, جدید AstVarRef{fl, varp, VAccess::READ}};
        Ast_گره_عبارت* ثابت store_بالاترین
            = جدید AstSelBit{fl, wrFromp, جدید AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = جدید AstAssign{nodep->fileline(), store_بالاترین, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی prepost_stmt_visit(Ast_گره_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        Ast_گره_عبارت* ثابت store_بالاترین = nodep->lhsp()->clone_درخت_Pure(درست);
        Ast_گره_عبارت* ثابت valuep = nodep->lhsp()->unlinkFrBack();
        File_سطر* ثابت fl = nodep->fileline();
        V3_عدد رقم_One{fl, 32, 1, نادرست};
        Ast_گره_عبارت* ثابت exprp = جدید AstConst{nodep->fileline(), رقم_One};

        prepost_stmt_visit(nodep, exprp, store_بالاترین, valuep);
    }
    تهی prepost_stmt_visit(AstAssignCompound* ثابت nodep) {
        Ast_گره_عبارت* ثابت exprp = nodep->rhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت store_بالاترین = nodep->lhsp()->clone_درخت_Pure(درست);
        Ast_گره_عبارت* ثابت valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_بالاترین, valuep);
    }
    تهی prepost_stmt_visit(Ast_گره* ثابت nodep, Ast_گره_عبارت* ثابت exprp,
                            Ast_گره_عبارت* ثابت store_بالاترین, Ast_گره_عبارت* ثابت valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* ثابت assignp
            = جدید AstAssign{nodep->fileline(), store_بالاترین, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی prepost_عبارت_visit(Ast_گره_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        اگر (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            بازگشت;
        }
        ثابت منطقی needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_گره_عبارت* ثابت readp = nodep->lhsp();
        Ast_گره_عبارت* ثابت writep = nodep->lhsp()->clone_درخت_Pure(درست);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_سطر* ثابت fl = nodep->fileline();
        V3_عدد رقم_One{fl, 32, 1, نادرست};
        Ast_گره_عبارت* ثابت newconstp = جدید AstConst{nodep->fileline(), رقم_One};

        // Prepare a temporary variable
        ثابت string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جدید AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            جدید AstRefDType{fl, AstRefDType::Flag_Type_Of_عبارت{}, readp->clone_درخت_Pure(درست)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        اگر (m_ftaskp) varp->funcLocal(درست);

        // Declare the variable
        درج_On_بالاترین(varp);

        // Define what operation will we be doing
        Ast_گره_عبارت* ثابت operp = getOperationp(nodep, readp->clone_درخت_Pure(درست), newconstp);

        Ast_گره* stmtsp = nullptr;
        اگر (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* ثابت assignp = جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_درخت_Pure(درست)};
            Ast_گره* ثابت incp = جدید AstAssign{fl, writep, operp};
            اگر (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_درخت_Pure(درست)});
            }
            assignp->addNextHere(جدید AstIf{fl, m_incCondp->clone_درخت_Pure(درست), incp, nullptr});
            stmtsp = assignp;
        } وگرنه اگر (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* ثابت assignp
                = جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(جدید AstAssign{fl, writep, جدید AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } وگرنه {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* ثابت assignp = جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_درخت_Pure(درست)};
            // Increment the original variable by one
            assignp->addNext(جدید AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_گره_عبارت* newp = جدید AstVarRef{readp->fileline(), varp, VAccess::READ};
        اگر (m_inCovergroup) {
            newp = جدید Ast_عبارت_Stmt{fl, stmtsp, newp};
        } وگرنه {
            درج_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    تهی visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    تهی visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    تهی visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    تهی visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_گره_Stmt*>(nodep));
        AstSelBit* ثابت selbitp = VN_CAST(nodep->lhsp(), SelBit);
        اگر (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), گره_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } وگرنه {
            prepost_stmt_visit(nodep);
        }
    }
    تهی visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = پیشفرض;
};

//######################################################################
// Task class functions

تهی V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkinc", 0, dump_درخت_Either_Level() >= 3);
}
