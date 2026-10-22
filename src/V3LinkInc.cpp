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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3LinkInc.h"

#समावेश "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी LinkIncVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    स्थिर VNUser1InUse m_inuser1;

    // STATE
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_गाँठ_Module* m_modp = nullptr;  // Module we're inside
    पूर्णांक m_mod_Compound_Assignments_अंक = 0;  // Var name counter
    Ast_गाँठ* m_insStmtp = nullptr;  // Where to insert statement
    bool m_inCovergroup = false;  // Under a covergroup's coverpoint, cross, or option
    bool m_condEvalContext = false;  // ++/-- is in a conditionally-evaluated position
    Ast_गाँठ_व्यंजक* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    व्योम जोड़ो_On_शीर्ष(Ast_गाँठ* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        अगर (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } अन्यथा {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            अगर (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    व्योम जोड़ो_Before_Stmt(Ast_गाँठ* nodep, Ast_गाँठ* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->add_Here_This_As_अग्र(newp);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        अगर (nodep->dead()) वापस;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_अंक);
        m_modp = nodep;
        m_mod_Compound_Assignments_अंक = 0;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    व्योम visitCovergroup(Ast_गाँठ* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = true;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    व्योम visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    व्योम visit(Ast_गाँठ_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_अग्र_Null(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        अगर (AstAssert* स्थिर assertp = VN_CAST(nodep, Assert)) {
            iterate_And_अग्र_Null(assertp->failsp());
        } अन्यथा अगर (AstAssertIntrinsic* स्थिर intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_अग्र_Null(intrinsicp->failsp());
        }
        iterate_And_अग्र_Null(nodep->passsp());
    }
    व्योम visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    व्योम visit(Ast_गाँठ_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    व्योम visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    व्योम visit(Ast_गाँठ_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_अग्र_Null(nodep->condp());
        m_insStmtp = nullptr;
        iterate_And_अग्र_Null(nodep->thensp());
        iterate_And_अग्र_Null(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    व्योम visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = true;
            iterate_And_अग्र_Null(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterate_And_अग्र_Null(nodep->stmtsp());
    }
    व्योम visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_अग्र_Null(nodep->lhsp());
        m_insStmtp = nullptr;
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    व्योम visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    व्योम visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_अग्र_Null(nodep->condp());
        m_insStmtp = nullptr;
        iterate_And_अग्र_Null(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    व्योम visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    व्योम visit(Ast_Stmt_व्यंजक* nodep) override {
        Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->exprp();
        अगर (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            वापस;
        }
        visit(static_cast<Ast_गाँठ_Stmt*>(nodep));
    }
    व्योम visit(Ast_गाँठ_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    व्योम unsupported_visit(Ast_गाँठ* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = true;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_गाँठ_व्यंजक* capture_Logical_Lhs_To_अस्थायी(Ast_गाँठ_Biop* स्थिर nodep) {
        File_पंक्ति* स्थिर fl = nodep->fileline();
        Ast_गाँठ_व्यंजक* स्थिर lhsp = nodep->lhsp()->unlinkFrBack();
        स्थिर string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_अंक);
        AstVar* स्थिर varp = नया AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नया AstRefDType{fl, AstRefDType::Flag_Type_Of_व्यंजक{}, lhsp->clone_वृक्ष(true)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        अगर (m_ftaskp) varp->funcLocal(true);
        जोड़ो_On_शीर्ष(varp);
        Ast_गाँठ* अस्थायी_Initp = नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        अगर (m_incCondp) {
            अस्थायी_Initp = नया AstIf{fl, m_incCondp->clone_वृक्ष_Pure(true), अस्थायी_Initp, nullptr};
        }
        जोड़ो_Before_Stmt(nodep, अस्थायी_Initp);
        nodep->lhsp(नया AstVarRef{fl, varp, VAccess::READ});
        वापस नया AstVarRef{fl, varp, VAccess::READ};
    }
    व्योम handleShortCircuit(Ast_गाँठ_Biop* स्थिर nodep, bool isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        अगर (nodep->user1SetOnce()) वापस;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_अग्र_Null(nodep->lhsp());
        m_condEvalContext = true;
        Ast_गाँठ_व्यंजक* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        अगर (nodep->rhsp()->exists([](स्थिर Ast_गाँठ* np) {
                वापस VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_गाँठ_व्यंजक* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_वृक्ष_Pure(true)
                                       : capture_Logical_Lhs_To_अस्थायी(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            अगर (isOr) lhsRefp = नया AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? नया AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_वृक्ष_Pure(true), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_अग्र_Null(nodep->rhsp());
        अगर (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_वृक्ष(), ownedCondp);
    }
    व्योम visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/false); }
    व्योम visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/true); }
    व्योम visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    व्योम visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    व्योम visit(AstCond* nodep) override { unsupported_visit(nodep); }
    व्योम visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    व्योम prepost_visit(Ast_गाँठ_Uniop* स्थिर nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        स्थिर bool isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* स्थिर selbitp = VN_CAST(nodep->lhsp(), SelBit);
        अगर (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), गाँठ_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } अन्यथा {
            अगर (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } अन्यथा {
                prepost_व्यंजक_visit(nodep);
            }
        }
    }
    Ast_गाँठ_व्यंजक* getOperationp(Ast_गाँठ* स्थिर nodep, Ast_गाँठ_व्यंजक* स्थिर lhsp,
                               Ast_गाँठ_व्यंजक* स्थिर rhsp) {
        अगर (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            वापस नया AstSub{nodep->fileline(), lhsp, rhsp};
        }
        अगर (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            वापस नया AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        अगर (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            चयन (assignp->operation()) {
            हाल AstAssignCompound::operation::Add:
                वापस नया AstAdd{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::And:
                वापस नया AstAnd{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::Div:
                वापस नया AstDiv{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::ModDiv:
                वापस नया AstModDiv{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::Mul:
                वापस नया AstMul{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::Or: वापस नया AstOr{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::ShiftL:
                वापस नया AstShiftL{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::ShiftR:
                वापस नया AstShiftR{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::ShiftRS:
                वापस नया AstShiftRS{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::Sub:
                वापस नया AstSub{nodep->fileline(), lhsp, rhsp};
            हाल AstAssignCompound::operation::Xor:
                वापस नया AstXor{nodep->fileline(), lhsp, rhsp};
            शेष:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    व्योम prepost_stmt_sel_visit(Ast_गाँठ_Uniop* स्थिर nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_पंक्ति* fl = nodep->fileline();
        V3_संख्या अंक_One{fl, 32, 1, false};
        Ast_गाँठ_व्यंजक* स्थिर exprp = नया AstConst{nodep->fileline(), अंक_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    व्योम prepost_stmt_sel_visit(AstAssignCompound* स्थिर nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    व्योम prepost_stmt_sel_visit(Ast_गाँठ* स्थिर nodep, Ast_गाँठ_व्यंजक* स्थिर lhsp,
                                Ast_गाँठ_व्यंजक* स्थिर exprp) {
        AstSelBit* स्थिर rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_गाँठ_Var_Ref* स्थिर rdFromp = VN_AS(rdSelbitp->fromp()->clone_वृक्ष_Pure(true), गाँठ_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_गाँठ_व्यंजक* स्थिर rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* स्थिर wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_गाँठ_व्यंजक* स्थिर wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_पंक्ति* स्थिर fl = nodep->fileline();
        स्थिर string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_अंक);
        AstVar* स्थिर varp = नया AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नया AstRefDType{fl, AstRefDType::Flag_Type_Of_व्यंजक{}, rdBitp->clone_वृक्ष(true)}};
        अगर (m_ftaskp) varp->funcLocal(true);

        // Declare the variable
        जोड़ो_On_शीर्ष(varp);

        // Define what operation will we be doing
        AstAssign* स्थिर varAssignp
            = नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_गाँठ* स्थिर newp = varAssignp;

        Ast_गाँठ_व्यंजक* स्थिर valuep
            = नया AstSelBit{fl, rdFromp, नया AstVarRef{fl, varp, VAccess::READ}};
        Ast_गाँठ_व्यंजक* स्थिर store_शीर्ष
            = नया AstSelBit{fl, wrFromp, नया AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = नया AstAssign{nodep->fileline(), store_शीर्ष, getOperationp(nodep, valuep, exprp)};
        newp->add_अग्र(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम prepost_stmt_visit(Ast_गाँठ_Uniop* स्थिर nodep) {
        iterateChildren(nodep);
        Ast_गाँठ_व्यंजक* स्थिर store_शीर्ष = nodep->lhsp()->clone_वृक्ष_Pure(true);
        Ast_गाँठ_व्यंजक* स्थिर valuep = nodep->lhsp()->unlinkFrBack();
        File_पंक्ति* स्थिर fl = nodep->fileline();
        V3_संख्या अंक_One{fl, 32, 1, false};
        Ast_गाँठ_व्यंजक* स्थिर exprp = नया AstConst{nodep->fileline(), अंक_One};

        prepost_stmt_visit(nodep, exprp, store_शीर्ष, valuep);
    }
    व्योम prepost_stmt_visit(AstAssignCompound* स्थिर nodep) {
        Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर store_शीर्ष = nodep->lhsp()->clone_वृक्ष_Pure(true);
        Ast_गाँठ_व्यंजक* स्थिर valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_शीर्ष, valuep);
    }
    व्योम prepost_stmt_visit(Ast_गाँठ* स्थिर nodep, Ast_गाँठ_व्यंजक* स्थिर exprp,
                            Ast_गाँठ_व्यंजक* स्थिर store_शीर्ष, Ast_गाँठ_व्यंजक* स्थिर valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* स्थिर assignp
            = नया AstAssign{nodep->fileline(), store_शीर्ष, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम prepost_व्यंजक_visit(Ast_गाँठ_Uniop* स्थिर nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        अगर (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            वापस;
        }
        स्थिर bool needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_गाँठ_व्यंजक* स्थिर readp = nodep->lhsp();
        Ast_गाँठ_व्यंजक* स्थिर writep = nodep->lhsp()->clone_वृक्ष_Pure(true);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_पंक्ति* स्थिर fl = nodep->fileline();
        V3_संख्या अंक_One{fl, 32, 1, false};
        Ast_गाँठ_व्यंजक* स्थिर newconstp = नया AstConst{nodep->fileline(), अंक_One};

        // Prepare a temporary variable
        स्थिर string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_अंक);
        AstVar* स्थिर varp = नया AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            नया AstRefDType{fl, AstRefDType::Flag_Type_Of_व्यंजक{}, readp->clone_वृक्ष_Pure(true)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        अगर (m_ftaskp) varp->funcLocal(true);

        // Declare the variable
        जोड़ो_On_शीर्ष(varp);

        // Define what operation will we be doing
        Ast_गाँठ_व्यंजक* स्थिर operp = getOperationp(nodep, readp->clone_वृक्ष_Pure(true), newconstp);

        Ast_गाँठ* stmtsp = nullptr;
        अगर (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* स्थिर assignp = नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_वृक्ष_Pure(true)};
            Ast_गाँठ* स्थिर incp = नया AstAssign{fl, writep, operp};
            अगर (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->add_अग्र(नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_वृक्ष_Pure(true)});
            }
            assignp->add_अग्र_Here(नया AstIf{fl, m_incCondp->clone_वृक्ष_Pure(true), incp, nullptr});
            stmtsp = assignp;
        } अन्यथा अगर (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* स्थिर assignp
                = नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->add_अग्र(नया AstAssign{fl, writep, नया AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } अन्यथा {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* स्थिर assignp = नया AstAssign{fl, नया AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_वृक्ष_Pure(true)};
            // Increment the original variable by one
            assignp->add_अग्र(नया AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_गाँठ_व्यंजक* newp = नया AstVarRef{readp->fileline(), varp, VAccess::READ};
        अगर (m_inCovergroup) {
            newp = नया Ast_व्यंजक_Stmt{fl, stmtsp, newp};
        } अन्यथा {
            जोड़ो_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    व्योम visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    व्योम visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    व्योम visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    व्योम visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_गाँठ_Stmt*>(nodep));
        AstSelBit* स्थिर selbitp = VN_CAST(nodep->lhsp(), SelBit);
        अगर (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), गाँठ_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } अन्यथा {
            prepost_stmt_visit(nodep);
        }
    }
    व्योम visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = शेष;
};

//######################################################################
// Task class functions

व्योम V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("linkinc", 0, dump_वृक्ष_Either_Level() >= 3);
}
