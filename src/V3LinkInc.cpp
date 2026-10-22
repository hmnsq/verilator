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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkInc.h"

#ଅନ୍ତର୍ଭାଵ "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ LinkIncVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Module we're inside
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_mod_Compound_Assignments_ଅଙ୍କ = 0;  // Var name counter
    Ast_ଗଣ୍ଠି* m_insStmtp = nullptr;  // Where to insert statement
    ତାର୍କିକ m_inCovergroup = ମିଥ୍ୟା;  // Under a covergroup's coverpoint, cross, or option
    ତାର୍କିକ m_condEvalContext = ମିଥ୍ୟା;  // ++/-- is in a conditionally-evaluated position
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_On_ଶୀର୍ଷ(Ast_ଗଣ୍ଠି* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        ଯଦି (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            ଯଦି (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Before_Stmt(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (nodep->dead()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_ଅଙ୍କ);
        m_modp = nodep;
        m_mod_Compound_Assignments_ଅଙ୍କ = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    ଶୂନ୍ୟ visitCovergroup(Ast_ଗଣ୍ଠି* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    ଶୂନ୍ୟ visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        ଯଦି (AstAssert* ଅଚଳ assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } ଅନ୍ୟଥା ଯଦି (AstAssertIntrinsic* ଅଚଳ intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ଶୂନ୍ୟ visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = ସତ୍ୟ;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    ଶୂନ୍ୟ visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ଶୂନ୍ୟ visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ଶୂନ୍ୟ visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->exprp();
        ଯଦି (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            ଫେରନ୍ତୁ;
        }
        visit(static_cast<Ast_ଗଣ୍ଠି_Stmt*>(nodep));
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ଶୂନ୍ୟ unsupported_visit(Ast_ଗଣ୍ଠି* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = ସତ୍ୟ;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* captureLogicalLhsToTemp(Ast_ଗଣ୍ଠି_Biop* ଅଚଳ nodep) {
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
        ଅଚଳ string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_ଅଙ୍କ);
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            ନୂତନ AstRefDType{fl, AstRefDType::Flag_Type_Of_ପଦସମଷ୍ଟି{}, lhsp->clone_ବୃକ୍ଷ(ସତ୍ୟ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ଯଦି (m_ftaskp) varp->funcLocal(ସତ୍ୟ);
        ଭର୍ତ୍ତି_On_ଶୀର୍ଷ(varp);
        Ast_ଗଣ୍ଠି* tempInitp = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        ଯଦି (m_incCondp) {
            tempInitp = ନୂତନ AstIf{fl, m_incCondp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ), tempInitp, nullptr};
        }
        ଭର୍ତ୍ତି_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(ନୂତନ AstVarRef{fl, varp, VAccess::READ});
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, varp, VAccess::READ};
    }
    ଶୂନ୍ୟ handleShortCircuit(Ast_ଗଣ୍ଠି_Biop* ଅଚଳ nodep, ତାର୍କିକ isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = ସତ୍ୟ;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        ଯଦି (nodep->rhsp()->exists([](ଅଚଳ Ast_ଗଣ୍ଠି* np) {
                ଫେରନ୍ତୁ VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            ଯଦି (isOr) lhsRefp = ନୂତନ AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? ନୂତନ AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        ଯଦି (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_ବୃକ୍ଷ(), ownedCondp);
    }
    ଶୂନ୍ୟ visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/ମିଥ୍ୟା); }
    ଶୂନ୍ୟ visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/ସତ୍ୟ); }
    ଶୂନ୍ୟ visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    ଶୂନ୍ୟ visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    ଶୂନ୍ୟ visit(AstCond* nodep) override { unsupported_visit(nodep); }
    ଶୂନ୍ୟ visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    ଶୂନ୍ୟ prepost_visit(Ast_ଗଣ୍ଠି_Uniop* ଅଚଳ nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ଅଚଳ ତାର୍କିକ isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* ଅଚଳ selbitp = VN_CAST(nodep->lhsp(), SelBit);
        ଯଦି (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), ଗଣ୍ଠି_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } ଅନ୍ୟଥା {
            ଯଦି (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } ଅନ୍ୟଥା {
                prepost_ପଦସମଷ୍ଟି_visit(nodep);
            }
        }
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getOperationp(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp,
                               Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp) {
        ଯଦି (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            ଫେରନ୍ତୁ ନୂତନ AstSub{nodep->fileline(), lhsp, rhsp};
        }
        ଯଦି (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            ଫେରନ୍ତୁ ନୂତନ AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        ଯଦି (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            ବାଛ (assignp->operation()) {
            ସ୍ଥିତି AstAssignCompound::operation::Add:
                ଫେରନ୍ତୁ ନୂତନ AstAdd{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::And:
                ଫେରନ୍ତୁ ନୂତନ AstAnd{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::Div:
                ଫେରନ୍ତୁ ନୂତନ AstDiv{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::ModDiv:
                ଫେରନ୍ତୁ ନୂତନ AstModDiv{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::Mul:
                ଫେରନ୍ତୁ ନୂତନ AstMul{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::Or: ଫେରନ୍ତୁ ନୂତନ AstOr{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::ShiftL:
                ଫେରନ୍ତୁ ନୂତନ AstShiftL{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::ShiftR:
                ଫେରନ୍ତୁ ନୂତନ AstShiftR{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::ShiftRS:
                ଫେରନ୍ତୁ ନୂତନ AstShiftRS{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::Sub:
                ଫେରନ୍ତୁ ନୂତନ AstSub{nodep->fileline(), lhsp, rhsp};
            ସ୍ଥିତି AstAssignCompound::operation::Xor:
                ଫେରନ୍ତୁ ନୂତନ AstXor{nodep->fileline(), lhsp, rhsp};
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    ଶୂନ୍ୟ prepost_stmt_sel_visit(Ast_ଗଣ୍ଠି_Uniop* ଅଚଳ nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_ଧାଡ଼ି* fl = nodep->fileline();
        V3_ସଂଖ୍ୟା ଅଙ୍କ_One{fl, 32, 1, ମିଥ୍ୟା};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = ନୂତନ AstConst{nodep->fileline(), ଅଙ୍କ_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    ଶୂନ୍ୟ prepost_stmt_sel_visit(AstAssignCompound* ଅଚଳ nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    ଶୂନ୍ୟ prepost_stmt_sel_visit(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp,
                                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp) {
        AstSelBit* ଅଚଳ rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ rdFromp = VN_AS(rdSelbitp->fromp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ), ଗଣ୍ଠି_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* ଅଚଳ wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଅଚଳ string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_ଅଙ୍କ);
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            ନୂତନ AstRefDType{fl, AstRefDType::Flag_Type_Of_ପଦସମଷ୍ଟି{}, rdBitp->clone_ବୃକ୍ଷ(ସତ୍ୟ)}};
        ଯଦି (m_ftaskp) varp->funcLocal(ସତ୍ୟ);

        // Declare the variable
        ଭର୍ତ୍ତି_On_ଶୀର୍ଷ(varp);

        // Define what operation will we be doing
        AstAssign* ଅଚଳ varAssignp
            = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_ଗଣ୍ଠି* ଅଚଳ newp = varAssignp;

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep
            = ନୂତନ AstSelBit{fl, rdFromp, ନୂତନ AstVarRef{fl, varp, VAccess::READ}};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ store_ଶୀର୍ଷ
            = ନୂତନ AstSelBit{fl, wrFromp, ନୂତନ AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = ନୂତନ AstAssign{nodep->fileline(), store_ଶୀର୍ଷ, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ prepost_stmt_visit(Ast_ଗଣ୍ଠି_Uniop* ଅଚଳ nodep) {
        iterateChildren(nodep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ store_ଶୀର୍ଷ = nodep->lhsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = nodep->lhsp()->unlinkFrBack();
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        V3_ସଂଖ୍ୟା ଅଙ୍କ_One{fl, 32, 1, ମିଥ୍ୟା};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = ନୂତନ AstConst{nodep->fileline(), ଅଙ୍କ_One};

        prepost_stmt_visit(nodep, exprp, store_ଶୀର୍ଷ, valuep);
    }
    ଶୂନ୍ୟ prepost_stmt_visit(AstAssignCompound* ଅଚଳ nodep) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ store_ଶୀର୍ଷ = nodep->lhsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_ଶୀର୍ଷ, valuep);
    }
    ଶୂନ୍ୟ prepost_stmt_visit(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp,
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ store_ଶୀର୍ଷ, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* ଅଚଳ assignp
            = ନୂତନ AstAssign{nodep->fileline(), store_ଶୀର୍ଷ, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ prepost_ପଦସମଷ୍ଟି_visit(Ast_ଗଣ୍ଠି_Uniop* ଅଚଳ nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        ଯଦି (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ ତାର୍କିକ needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ readp = nodep->lhsp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ writep = nodep->lhsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        V3_ସଂଖ୍ୟା ଅଙ୍କ_One{fl, 32, 1, ମିଥ୍ୟା};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newconstp = ନୂତନ AstConst{nodep->fileline(), ଅଙ୍କ_One};

        // Prepare a temporary variable
        ଅଚଳ string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_ଅଙ୍କ);
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            ନୂତନ AstRefDType{fl, AstRefDType::Flag_Type_Of_ପଦସମଷ୍ଟି{}, readp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ଯଦି (m_ftaskp) varp->funcLocal(ସତ୍ୟ);

        // Declare the variable
        ଭର୍ତ୍ତି_On_ଶୀର୍ଷ(varp);

        // Define what operation will we be doing
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ operp = getOperationp(nodep, readp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ), newconstp);

        Ast_ଗଣ୍ଠି* stmtsp = nullptr;
        ଯଦି (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)};
            Ast_ଗଣ୍ଠି* ଅଚଳ incp = ନୂତନ AstAssign{fl, writep, operp};
            ଯଦି (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)});
            }
            assignp->addNextHere(ନୂତନ AstIf{fl, m_incCondp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ), incp, nullptr});
            stmtsp = assignp;
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* ଅଚଳ assignp
                = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(ନୂତନ AstAssign{fl, writep, ନୂତନ AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } ଅନ୍ୟଥା {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)};
            // Increment the original variable by one
            assignp->addNext(ନୂତନ AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = ନୂତନ AstVarRef{readp->fileline(), varp, VAccess::READ};
        ଯଦି (m_inCovergroup) {
            newp = ନୂତନ Ast_ପଦସମଷ୍ଟି_Stmt{fl, stmtsp, newp};
        } ଅନ୍ୟଥା {
            ଭର୍ତ୍ତି_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    ଶୂନ୍ୟ visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    ଶୂନ୍ୟ visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    ଶୂନ୍ୟ visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    ଶୂନ୍ୟ visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_ଗଣ୍ଠି_Stmt*>(nodep));
        AstSelBit* ଅଚଳ selbitp = VN_CAST(nodep->lhsp(), SelBit);
        ଯଦି (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), ଗଣ୍ଠି_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } ଅନ୍ୟଥା {
            prepost_stmt_visit(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Task class functions

ଶୂନ୍ୟ V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linkinc", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
