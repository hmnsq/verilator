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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Function or task we're inside
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Module we're inside
    पूर्णाङ्क m_mod_Compound_Assignments_अङ्कः = 0;  // Var name counter
    Ast_ग्रन्थिः* m_insStmtp = शून्यसूचकः;  // Where to insert statement
    तार्किक m_अन्तर्_Covergroup = असत्य;  // Under a covergroup's coverpoint, cross, or option
    तार्किक m_condEvalContext = असत्य;  // ++/-- is in a conditionally-evaluated position
    Ast_ग्रन्थिः_व्यञ्जकम्* m_incCondp = शून्यसूचकः;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    शून्य निवेशय_On_शिखरम्(Ast_ग्रन्थिः* newp) {
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
    शून्य निवेशय_Before_Stmt(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->सङ्कलय_अत्र_This_As_अग्र(newp);
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        यदि (nodep->dead()) प्रत्यागम;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_अङ्कः);
        m_modp = nodep;
        m_mod_Compound_Assignments_अङ्कः = 0;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    शून्य दर्शय_Covergroup(Ast_ग्रन्थिः* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_अन्तर्_Covergroup);
        m_ftaskp = शून्यसूचकः;
        m_अन्तर्_Covergroup = सत्य;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Func_Cov_Item* nodep) override { दर्शय_Covergroup(nodep); }
    शून्य दर्शय(Ast_Cg_Option_नियोजय* nodep) override { दर्शय_Covergroup(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_अग्र_Null(nodep->propp());
        m_insStmtp = शून्यसूचकः;
        // Note: no iterating over sentreep here as they will be ignored anyway
        यदि (AstAssert* ध्रुव assertp = VN_CAST(nodep, Assert)) {
            iterate_And_अग्र_Null(assertp->failsp());
        } अन्यथा यदि (AstAssertIntrinsic* ध्रुव intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_अग्र_Null(intrinsicp->failsp());
        }
        iterate_And_अग्र_Null(nodep->passsp());
    }
    शून्य दर्शय(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = शून्यसूचकः;  // First thing should be new statement
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_insStmtp = शून्यसूचकः;  // Next thing should be new statement
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = शून्यसूचकः;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = शून्यसूचकः;  // Next thing should be new statement
    }
    शून्य दर्शय(Ast_Jump_खण्डम्* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = शून्यसूचकः;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = शून्यसूचकः;  // Next thing should be new statement
    }
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_अग्र_Null(nodep->condp());
        m_insStmtp = शून्यसूचकः;
        iterate_And_अग्र_Null(nodep->thensp());
        iterate_And_अग्र_Null(nodep->elsesp());
        m_insStmtp = शून्यसूचकः;
    }
    शून्य दर्शय(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = सत्य;
            iterate_And_अग्र_Null(nodep->condsp());
        }
        m_insStmtp = शून्यसूचकः;  // Next thing should be new statement
        iterate_And_अग्र_Null(nodep->stmtsp());
    }
    शून्य दर्शय(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_अग्र_Null(nodep->lhsp());
        m_insStmtp = शून्यसूचकः;
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_insStmtp = शून्यसूचकः;
    }
    शून्य दर्शय(AstEventControl* nodep) override {
        m_insStmtp = शून्यसूचकः;
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_insStmtp = शून्यसूचकः;
    }
    शून्य दर्शय(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_अग्र_Null(nodep->condp());
        m_insStmtp = शून्यसूचकः;
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_insStmtp = शून्यसूचकः;
    }
    शून्य दर्शय(AstBegin* nodep) override {
        m_insStmtp = शून्यसूचकः;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = शून्यसूचकः;  // Next thing should be new statement
    }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp();
        यदि (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            m_insStmtp = शून्यसूचकः;
            iterate(exprp);
            m_insStmtp = शून्यसूचकः;
            प्रत्यागम;
        }
        दर्शय(static_cast<Ast_ग्रन्थिः_Stmt*>(nodep));
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = शून्यसूचकः;  // Next thing should be new statement
    }
    शून्य unsupported_दर्शय(Ast_ग्रन्थिः* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = सत्य;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = शून्यसूचकः;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_ग्रन्थिः_व्यञ्जकम्* capture_Logical_Lhs_गन्तव्यम्_अस्थायी(Ast_ग्रन्थिः_Biop* ध्रुव nodep) {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        ध्रुव string name = "__VincGate"s + cvt_गन्तव्यम्_Str(++m_mod_Compound_Assignments_अङ्कः);
        AstVar* ध्रुव varp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नूतन AstRefDType{fl, AstRefDType::Flag_Type_सम्बन्धि_व्यञ्जकम्{}, lhsp->clone_वृक्षः(सत्य)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        यदि (m_ftaskp) varp->funcLocal(सत्य);
        निवेशय_On_शिखरम्(varp);
        Ast_ग्रन्थिः* अस्थायी_Initp = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        यदि (m_incCondp) {
            अस्थायी_Initp = नूतन AstIf{fl, m_incCondp->clone_वृक्षः_विशुद्धम्(सत्य), अस्थायी_Initp, शून्यसूचकः};
        }
        निवेशय_Before_Stmt(nodep, अस्थायी_Initp);
        nodep->lhsp(नूतन AstVarRef{fl, varp, VAccess::READ});
        प्रत्यागम नूतन AstVarRef{fl, varp, VAccess::READ};
    }
    शून्य handleShortCircuit(Ast_ग्रन्थिः_Biop* ध्रुव nodep, तार्किक isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_अग्र_Null(nodep->lhsp());
        m_condEvalContext = सत्य;
        Ast_ग्रन्थिः_व्यञ्जकम्* ownedCondp = शून्यसूचकः;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        यदि (nodep->rhsp()->exists([](ध्रुव Ast_ग्रन्थिः* np) {
                प्रत्यागम VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, नियोजय_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_ग्रन्थिः_व्यञ्जकम्* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_वृक्षः_विशुद्धम्(सत्य)
                                       : capture_Logical_Lhs_गन्तव्यम्_अस्थायी(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            यदि (isOr) lhsRefp = नूतन AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? नूतन AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_वृक्षः_विशुद्धम्(सत्य), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_अग्र_Null(nodep->rhsp());
        यदि (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_वृक्षः(), ownedCondp);
    }
    शून्य दर्शय(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/असत्य); }
    शून्य दर्शय(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/सत्य); }
    शून्य दर्शय(AstLogEq* nodep) override { unsupported_दर्शय(nodep); }
    शून्य दर्शय(AstLogIf* nodep) override { unsupported_दर्शय(nodep); }
    शून्य दर्शय(AstCond* nodep) override { unsupported_दर्शय(nodep); }
    शून्य दर्शय(AstPropSpec* nodep) override { unsupported_दर्शय(nodep); }
    शून्य prepost_दर्शय(Ast_ग्रन्थिः_Uniop* ध्रुव nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ध्रुव तार्किक isStandaloneStmt = !m_insStmtp && !m_अन्तर्_Covergroup;
        AstSelBit* ध्रुव selbitp = VN_CAST(nodep->lhsp(), SelBit);
        यदि (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), ग्रन्थिः_Var_Ref)
            && !selbitp->bitp()->is_विशुद्धम्()) {
            prepost_stmt_sel_दर्शय(nodep);
        } अन्यथा {
            यदि (isStandaloneStmt) {
                prepost_stmt_दर्शय(nodep);
            } अन्यथा {
                prepost_व्यञ्जकम्_दर्शय(nodep);
            }
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* getOperationp(Ast_ग्रन्थिः* ध्रुव nodep, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp,
                               Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp) {
        यदि (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            प्रत्यागम नूतन AstSub{nodep->fileline(), lhsp, rhsp};
        }
        यदि (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            प्रत्यागम नूतन Ast_सङ्कलय{nodep->fileline(), lhsp, rhsp};
        }
        यदि (Ast_नियोजय_Compound* assignp = VN_CAST(nodep, नियोजय_Compound)) {
            विकल्प (assignp->operation()) {
            पक्ष Ast_नियोजय_Compound::operation::सङ्कलय_2:
                प्रत्यागम नूतन Ast_सङ्कलय{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::And:
                प्रत्यागम नूतन AstAnd{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::Div:
                प्रत्यागम नूतन AstDiv{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::ModDiv:
                प्रत्यागम नूतन AstModDiv{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::Mul:
                प्रत्यागम नूतन AstMul{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::Or: प्रत्यागम नूतन AstOr{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::सरणम्_L:
                प्रत्यागम नूतन Ast_सरणम्_L{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::सरणम्_R:
                प्रत्यागम नूतन Ast_सरणम्_R{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::सरणम्_RS:
                प्रत्यागम नूतन Ast_सरणम्_RS{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::Sub:
                प्रत्यागम नूतन AstSub{nodep->fileline(), lhsp, rhsp};
            पक्ष Ast_नियोजय_Compound::operation::Xor:
                प्रत्यागम नूतन AstXor{nodep->fileline(), lhsp, rhsp};
            स्वभाव:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    शून्य prepost_stmt_sel_दर्शय(Ast_ग्रन्थिः_Uniop* ध्रुव nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_पङ्क्तिः* fl = nodep->fileline();
        V3_सङ्ख्या अङ्कः_One{fl, 32, 1, असत्य};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = नूतन AstConst{nodep->fileline(), अङ्कः_One};

        prepost_stmt_sel_दर्शय(nodep, nodep->lhsp(), exprp);
    }
    शून्य prepost_stmt_sel_दर्शय(Ast_नियोजय_Compound* ध्रुव nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();

        prepost_stmt_sel_दर्शय(nodep, nodep->lhsp(), exprp);
    }
    शून्य prepost_stmt_sel_दर्शय(Ast_ग्रन्थिः* ध्रुव nodep, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp,
                                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
        AstSelBit* ध्रुव rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_ग्रन्थिः_Var_Ref* ध्रुव rdFromp = VN_AS(rdSelbitp->fromp()->clone_वृक्षः_विशुद्धम्(सत्य), ग्रन्थिः_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rdBitp = rdSelbitp->bitp()->unlink_Fr_पृष्ठभागः();
        AstSelBit* ध्रुव wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wrFromp = wrSelbitp->fromp()->unlink_Fr_पृष्ठभागः();
        V3LinkLValue::link_LValue_समुच्चयः(wrFromp);

        // Prepare a temporary variable
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        ध्रुव string name = "__VtempIndex"s + cvt_गन्तव्यम्_Str(++m_mod_Compound_Assignments_अङ्कः);
        AstVar* ध्रुव varp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नूतन AstRefDType{fl, AstRefDType::Flag_Type_सम्बन्धि_व्यञ्जकम्{}, rdBitp->clone_वृक्षः(सत्य)}};
        यदि (m_ftaskp) varp->funcLocal(सत्य);

        // Declare the variable
        निवेशय_On_शिखरम्(varp);

        // Define what operation will we be doing
        Ast_नियोजय* ध्रुव varAssignp
            = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_ग्रन्थिः* ध्रुव newp = varAssignp;

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep
            = नूतन AstSelBit{fl, rdFromp, नूतन AstVarRef{fl, varp, VAccess::READ}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव store_शिखरम्
            = नूतन AstSelBit{fl, wrFromp, नूतन AstVarRef{fl, varp, VAccess::READ}};

        Ast_नियोजय* assignp
            = नूतन Ast_नियोजय{nodep->fileline(), store_शिखरम्, getOperationp(nodep, valuep, exprp)};
        newp->सङ्कलय_अग्र(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य prepost_stmt_दर्शय(Ast_ग्रन्थिः_Uniop* ध्रुव nodep) {
        iterateChildren(nodep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव store_शिखरम् = nodep->lhsp()->clone_वृक्षः_विशुद्धम्(सत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        V3_सङ्ख्या अङ्कः_One{fl, 32, 1, असत्य};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = नूतन AstConst{nodep->fileline(), अङ्कः_One};

        prepost_stmt_दर्शय(nodep, exprp, store_शिखरम्, valuep);
    }
    शून्य prepost_stmt_दर्शय(Ast_नियोजय_Compound* ध्रुव nodep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव store_शिखरम् = nodep->lhsp()->clone_वृक्षः_विशुद्धम्(सत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = nodep->lhsp()->unlink_Fr_पृष्ठभागः();

        prepost_stmt_दर्शय(nodep, exprp, store_शिखरम्, valuep);
    }
    शून्य prepost_stmt_दर्शय(Ast_ग्रन्थिः* ध्रुव nodep, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp,
                            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव store_शिखरम्, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep) {
        V3LinkLValue::link_LValue_समुच्चयः(valuep, VAccess::READ);
        Ast_नियोजय* ध्रुव assignp
            = नूतन Ast_नियोजय{nodep->fileline(), store_शिखरम्, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य prepost_व्यञ्जकम्_दर्शय(Ast_ग्रन्थिः_Uniop* ध्रुव nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        यदि (m_condEvalContext && !m_incCondp && !m_अन्तर्_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            प्रत्यागम;
        }
        ध्रुव तार्किक needGating = m_condEvalContext && m_incCondp && !m_अन्तर्_Covergroup;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव readp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव writep = nodep->lhsp()->clone_वृक्षः_विशुद्धम्(सत्य);
        V3LinkLValue::link_LValue_समुच्चयः(readp, VAccess::READ);

        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        V3_सङ्ख्या अङ्कः_One{fl, 32, 1, असत्य};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newconstp = नूतन AstConst{nodep->fileline(), अङ्कः_One};

        // Prepare a temporary variable
        ध्रुव string name = "__Vincrement"s + cvt_गन्तव्यम्_Str(++m_mod_Compound_Assignments_अङ्कः);
        AstVar* ध्रुव varp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नूतन AstRefDType{fl, AstRefDType::Flag_Type_सम्बन्धि_व्यञ्जकम्{}, readp->clone_वृक्षः_विशुद्धम्(सत्य)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        यदि (m_ftaskp) varp->funcLocal(सत्य);

        // Declare the variable
        निवेशय_On_शिखरम्(varp);

        // Define what operation will we be doing
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव operp = getOperationp(nodep, readp->clone_वृक्षः_विशुद्धम्(सत्य), newconstp);

        Ast_ग्रन्थिः* stmtsp = शून्यसूचकः;
        यदि (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_वृक्षः_विशुद्धम्(सत्य)};
            Ast_ग्रन्थिः* ध्रुव incp = नूतन Ast_नियोजय{fl, writep, operp};
            यदि (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->सङ्कलय_अग्र(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_वृक्षः_विशुद्धम्(सत्य)});
            }
            assignp->सङ्कलय_अग्र_अत्र(नूतन AstIf{fl, m_incCondp->clone_वृक्षः_विशुद्धम्(सत्य), incp, शून्यसूचकः});
            stmtsp = assignp;
        } अन्यथा यदि (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_नियोजय* ध्रुव assignp
                = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->सङ्कलय_अग्र(नूतन Ast_नियोजय{fl, writep, नूतन AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } अन्यथा {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_वृक्षः_विशुद्धम्(सत्य)};
            // Increment the original variable by one
            assignp->सङ्कलय_अग्र(नूतन Ast_नियोजय{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = नूतन AstVarRef{readp->fileline(), varp, VAccess::READ};
        यदि (m_अन्तर्_Covergroup) {
            newp = नूतन Ast_व्यञ्जकम्_Stmt{fl, stmtsp, newp};
        } अन्यथा {
            निवेशय_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstPreInc* nodep) override { prepost_दर्शय(nodep); }
    शून्य दर्शय(AstPostInc* nodep) override { prepost_दर्शय(nodep); }
    शून्य दर्शय(AstPreDec* nodep) override { prepost_दर्शय(nodep); }
    शून्य दर्शय(AstPostDec* nodep) override { prepost_दर्शय(nodep); }
    शून्य दर्शय(Ast_नियोजय_Compound* nodep) override {
        दर्शय(static_cast<Ast_ग्रन्थिः_Stmt*>(nodep));
        AstSelBit* ध्रुव selbitp = VN_CAST(nodep->lhsp(), SelBit);
        यदि (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), ग्रन्थिः_Var_Ref)
            && !selbitp->bitp()->is_विशुद्धम्()) {
            prepost_stmt_sel_दर्शय(nodep);
        } अन्यथा {
            prepost_stmt_दर्शय(nodep);
        }
    }
    शून्य दर्शय(AstGenFor* nodep) override { iterateChildren(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = स्वभाव;
};

//######################################################################
// Task class functions

शून्य V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("linkinc", 0, dump_वृक्षः_Either_Level() >= 3);
}
