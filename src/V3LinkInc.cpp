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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkInc.h"

#अन्तर्भाव "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग LinkIncVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    अचर VNUser1InUse m_inuser1;

    // STATE
    Ast_गाँठो_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_गाँठो_Module* m_modp = nullptr;  // Module we're inside
    पूर्णाङ्क m_mod_Compound_Assignments_अङ्क = 0;  // Var name counter
    Ast_गाँठो* m_insStmtp = nullptr;  // Where to insert statement
    तार्किक m_inCovergroup = असत्य;  // Under a covergroup's coverpoint, cross, or option
    तार्किक m_condEvalContext = असत्य;  // ++/-- is in a conditionally-evaluated position
    Ast_गाँठो_अभिव्यक्ति* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    शून्य घुसाउ_On_शीर्ष(Ast_गाँठो* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        यदि (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } अन्यथा {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            यदि (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    शून्य घुसाउ_Before_Stmt(Ast_गाँठो* nodep, Ast_गाँठो* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        यदि (nodep->dead()) फर्काऊ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_अङ्क);
        m_modp = nodep;
        m_mod_Compound_Assignments_अङ्क = 0;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    शून्य visitCovergroup(Ast_गाँठो* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    शून्य visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    शून्य visit(Ast_गाँठो_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        यदि (AstAssert* अचर assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } अन्यथा यदि (AstAssertIntrinsic* अचर intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    शून्य visit(Ast_गाँठो_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    शून्य visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    शून्य visit(Ast_गाँठो_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    शून्य visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = सत्य;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    शून्य visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    शून्य visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    शून्य visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    शून्य visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    शून्य visit(Ast_Stmt_अभिव्यक्ति* nodep) override {
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = nodep->exprp();
        यदि (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(थप_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            फर्काऊ;
        }
        visit(static_cast<Ast_गाँठो_Stmt*>(nodep));
    }
    शून्य visit(Ast_गाँठो_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    शून्य unsupported_visit(Ast_गाँठो* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = सत्य;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_गाँठो_अभिव्यक्ति* captureLogicalLhsToTemp(Ast_गाँठो_Biop* अचर nodep) {
        File_पङ्क्ति* अचर fl = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp()->unlinkFrBack();
        अचर string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_अङ्क);
        AstVar* अचर varp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नूतन AstRefDType{fl, AstRefDType::Flag_Type_Of_अभिव्यक्ति{}, lhsp->clone_रूख(सत्य)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        यदि (m_ftaskp) varp->funcLocal(सत्य);
        घुसाउ_On_शीर्ष(varp);
        Ast_गाँठो* tempInitp = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        यदि (m_incCondp) {
            tempInitp = नूतन AstIf{fl, m_incCondp->clone_रूख_Pure(सत्य), tempInitp, nullptr};
        }
        घुसाउ_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(नूतन AstVarRef{fl, varp, VAccess::READ});
        फर्काऊ नूतन AstVarRef{fl, varp, VAccess::READ};
    }
    शून्य handleShortCircuit(Ast_गाँठो_Biop* अचर nodep, तार्किक isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        यदि (nodep->user1SetOnce()) फर्काऊ;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = सत्य;
        Ast_गाँठो_अभिव्यक्ति* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        यदि (nodep->rhsp()->exists([](अचर Ast_गाँठो* np) {
                फर्काऊ VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_गाँठो_अभिव्यक्ति* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_रूख_Pure(सत्य)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            यदि (isOr) lhsRefp = नूतन AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? नूतन AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_रूख_Pure(सत्य), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        यदि (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_रूख(), ownedCondp);
    }
    शून्य visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/असत्य); }
    शून्य visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/सत्य); }
    शून्य visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    शून्य visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    शून्य visit(AstCond* nodep) override { unsupported_visit(nodep); }
    शून्य visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    शून्य prepost_visit(Ast_गाँठो_Uniop* अचर nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        अचर तार्किक isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* अचर selbitp = VN_CAST(nodep->lhsp(), SelBit);
        यदि (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), गाँठो_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } अन्यथा {
            यदि (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } अन्यथा {
                prepost_अभिव्यक्ति_visit(nodep);
            }
        }
    }
    Ast_गाँठो_अभिव्यक्ति* getOperationp(Ast_गाँठो* अचर nodep, Ast_गाँठो_अभिव्यक्ति* अचर lhsp,
                               Ast_गाँठो_अभिव्यक्ति* अचर rhsp) {
        यदि (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            फर्काऊ नूतन AstSub{nodep->fileline(), lhsp, rhsp};
        }
        यदि (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            फर्काऊ नूतन AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        यदि (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            विकल्प (assignp->operation()) {
            पक्ष AstAssignCompound::operation::Add:
                फर्काऊ नूतन AstAdd{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::And:
                फर्काऊ नूतन AstAnd{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::Div:
                फर्काऊ नूतन AstDiv{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::ModDiv:
                फर्काऊ नूतन AstModDiv{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::Mul:
                फर्काऊ नूतन AstMul{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::Or: फर्काऊ नूतन AstOr{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::ShiftL:
                फर्काऊ नूतन AstShiftL{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::ShiftR:
                फर्काऊ नूतन AstShiftR{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::ShiftRS:
                फर्काऊ नूतन AstShiftRS{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::Sub:
                फर्काऊ नूतन AstSub{nodep->fileline(), lhsp, rhsp};
            पक्ष AstAssignCompound::operation::Xor:
                फर्काऊ नूतन AstXor{nodep->fileline(), lhsp, rhsp};
            स्वभाव:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    शून्य prepost_stmt_sel_visit(Ast_गाँठो_Uniop* अचर nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_पङ्क्ति* fl = nodep->fileline();
        V3_सङ्ख्या अङ्क_One{fl, 32, 1, असत्य};
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = नूतन AstConst{nodep->fileline(), अङ्क_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    शून्य prepost_stmt_sel_visit(AstAssignCompound* अचर nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    शून्य prepost_stmt_sel_visit(Ast_गाँठो* अचर nodep, Ast_गाँठो_अभिव्यक्ति* अचर lhsp,
                                Ast_गाँठो_अभिव्यक्ति* अचर exprp) {
        AstSelBit* अचर rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_गाँठो_Var_Ref* अचर rdFromp = VN_AS(rdSelbitp->fromp()->clone_रूख_Pure(सत्य), गाँठो_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_गाँठो_अभिव्यक्ति* अचर rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* अचर wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_गाँठो_अभिव्यक्ति* अचर wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_पङ्क्ति* अचर fl = nodep->fileline();
        अचर string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_अङ्क);
        AstVar* अचर varp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नूतन AstRefDType{fl, AstRefDType::Flag_Type_Of_अभिव्यक्ति{}, rdBitp->clone_रूख(सत्य)}};
        यदि (m_ftaskp) varp->funcLocal(सत्य);

        // Declare the variable
        घुसाउ_On_शीर्ष(varp);

        // Define what operation will we be doing
        AstAssign* अचर varAssignp
            = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_गाँठो* अचर newp = varAssignp;

        Ast_गाँठो_अभिव्यक्ति* अचर valuep
            = नूतन AstSelBit{fl, rdFromp, नूतन AstVarRef{fl, varp, VAccess::READ}};
        Ast_गाँठो_अभिव्यक्ति* अचर store_शीर्ष
            = नूतन AstSelBit{fl, wrFromp, नूतन AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = नूतन AstAssign{nodep->fileline(), store_शीर्ष, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    }
    शून्य prepost_stmt_visit(Ast_गाँठो_Uniop* अचर nodep) {
        iterateChildren(nodep);
        Ast_गाँठो_अभिव्यक्ति* अचर store_शीर्ष = nodep->lhsp()->clone_रूख_Pure(सत्य);
        Ast_गाँठो_अभिव्यक्ति* अचर valuep = nodep->lhsp()->unlinkFrBack();
        File_पङ्क्ति* अचर fl = nodep->fileline();
        V3_सङ्ख्या अङ्क_One{fl, 32, 1, असत्य};
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = नूतन AstConst{nodep->fileline(), अङ्क_One};

        prepost_stmt_visit(nodep, exprp, store_शीर्ष, valuep);
    }
    शून्य prepost_stmt_visit(AstAssignCompound* अचर nodep) {
        Ast_गाँठो_अभिव्यक्ति* अचर exprp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठो_अभिव्यक्ति* अचर store_शीर्ष = nodep->lhsp()->clone_रूख_Pure(सत्य);
        Ast_गाँठो_अभिव्यक्ति* अचर valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_शीर्ष, valuep);
    }
    शून्य prepost_stmt_visit(Ast_गाँठो* अचर nodep, Ast_गाँठो_अभिव्यक्ति* अचर exprp,
                            Ast_गाँठो_अभिव्यक्ति* अचर store_शीर्ष, Ast_गाँठो_अभिव्यक्ति* अचर valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* अचर assignp
            = नूतन AstAssign{nodep->fileline(), store_शीर्ष, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    }
    शून्य prepost_अभिव्यक्ति_visit(Ast_गाँठो_Uniop* अचर nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        यदि (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            फर्काऊ;
        }
        अचर तार्किक needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_गाँठो_अभिव्यक्ति* अचर readp = nodep->lhsp();
        Ast_गाँठो_अभिव्यक्ति* अचर writep = nodep->lhsp()->clone_रूख_Pure(सत्य);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_पङ्क्ति* अचर fl = nodep->fileline();
        V3_सङ्ख्या अङ्क_One{fl, 32, 1, असत्य};
        Ast_गाँठो_अभिव्यक्ति* अचर newconstp = नूतन AstConst{nodep->fileline(), अङ्क_One};

        // Prepare a temporary variable
        अचर string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_अङ्क);
        AstVar* अचर varp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नूतन AstRefDType{fl, AstRefDType::Flag_Type_Of_अभिव्यक्ति{}, readp->clone_रूख_Pure(सत्य)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        यदि (m_ftaskp) varp->funcLocal(सत्य);

        // Declare the variable
        घुसाउ_On_शीर्ष(varp);

        // Define what operation will we be doing
        Ast_गाँठो_अभिव्यक्ति* अचर operp = getOperationp(nodep, readp->clone_रूख_Pure(सत्य), newconstp);

        Ast_गाँठो* stmtsp = nullptr;
        यदि (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* अचर assignp = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_रूख_Pure(सत्य)};
            Ast_गाँठो* अचर incp = नूतन AstAssign{fl, writep, operp};
            यदि (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_रूख_Pure(सत्य)});
            }
            assignp->addNextHere(नूतन AstIf{fl, m_incCondp->clone_रूख_Pure(सत्य), incp, nullptr});
            stmtsp = assignp;
        } अन्यथा यदि (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* अचर assignp
                = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(नूतन AstAssign{fl, writep, नूतन AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } अन्यथा {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* अचर assignp = नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_रूख_Pure(सत्य)};
            // Increment the original variable by one
            assignp->addNext(नूतन AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_गाँठो_अभिव्यक्ति* newp = नूतन AstVarRef{readp->fileline(), varp, VAccess::READ};
        यदि (m_inCovergroup) {
            newp = नूतन Ast_अभिव्यक्ति_Stmt{fl, stmtsp, newp};
        } अन्यथा {
            घुसाउ_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    }
    शून्य visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    शून्य visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    शून्य visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    शून्य visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    शून्य visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_गाँठो_Stmt*>(nodep));
        AstSelBit* अचर selbitp = VN_CAST(nodep->lhsp(), SelBit);
        यदि (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), गाँठो_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } अन्यथा {
            prepost_stmt_visit(nodep);
        }
    }
    शून्य visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = स्वभाव;
};

//######################################################################
// Task class functions

शून्य V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("linkinc", 0, dump_रूख_Either_Level() >= 3);
}
