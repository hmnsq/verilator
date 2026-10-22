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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkInc.h"

#അന്തര്ഭാവ "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം LinkIncVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Module we're inside
    പൂർണ്ണസംഖ്യ m_mod_Compound_Assignments_അക്കം = 0;  // Var name counter
    Ast_കണ്ണി* m_insStmtp = nullptr;  // Where to insert statement
    തർക്കം m_inCovergroup = അസത്യം;  // Under a covergroup's coverpoint, cross, or option
    തർക്കം m_condEvalContext = അസത്യം;  // ++/-- is in a conditionally-evaluated position
    Ast_കണ്ണി_വ്യഞ്ജകം* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    ശൂന്യം തിരുകുക_On_മുകൾ(Ast_കണ്ണി* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        എങ്കിൽ (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            എങ്കിൽ (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    ശൂന്യം തിരുകുക_Before_Stmt(Ast_കണ്ണി* nodep, Ast_കണ്ണി* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        എങ്കിൽ (nodep->dead()) തിരികെ;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_അക്കം);
        m_modp = nodep;
        m_mod_Compound_Assignments_അക്കം = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    ശൂന്യം visitCovergroup(Ast_കണ്ണി* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    ശൂന്യം visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        എങ്കിൽ (AstAssert* സ്ഥിരം assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstAssertIntrinsic* സ്ഥിരം intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ശൂന്യം visit(Ast_കണ്ണി_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ശൂന്യം visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    ശൂന്യം visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = സത്യം;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    ശൂന്യം visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ശൂന്യം visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ശൂന്യം visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->exprp();
        എങ്കിൽ (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            തിരികെ;
        }
        visit(static_cast<Ast_കണ്ണി_Stmt*>(nodep));
    }
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    ശൂന്യം unsupported_visit(Ast_കണ്ണി* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = സത്യം;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = nullptr;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_കണ്ണി_വ്യഞ്ജകം* captureLogicalLhsToTemp(Ast_കണ്ണി_Biop* സ്ഥിരം nodep) {
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
        സ്ഥിരം string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_അക്കം);
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            പുതിയ AstRefDType{fl, AstRefDType::Flag_Type_Of_വ്യഞ്ജകം{}, lhsp->clone_വൃക്ഷം(സത്യം)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        എങ്കിൽ (m_ftaskp) varp->funcLocal(സത്യം);
        തിരുകുക_On_മുകൾ(varp);
        Ast_കണ്ണി* tempInitp = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        എങ്കിൽ (m_incCondp) {
            tempInitp = പുതിയ AstIf{fl, m_incCondp->clone_വൃക്ഷം_Pure(സത്യം), tempInitp, nullptr};
        }
        തിരുകുക_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(പുതിയ AstVarRef{fl, varp, VAccess::READ});
        തിരികെ പുതിയ AstVarRef{fl, varp, VAccess::READ};
    }
    ശൂന്യം handleShortCircuit(Ast_കണ്ണി_Biop* സ്ഥിരം nodep, തർക്കം isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = സത്യം;
        Ast_കണ്ണി_വ്യഞ്ജകം* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        എങ്കിൽ (nodep->rhsp()->exists([](സ്ഥിരം Ast_കണ്ണി* np) {
                തിരികെ VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_കണ്ണി_വ്യഞ്ജകം* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_വൃക്ഷം_Pure(സത്യം)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            എങ്കിൽ (isOr) lhsRefp = പുതിയ AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? പുതിയ AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_വൃക്ഷം_Pure(സത്യം), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        എങ്കിൽ (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_വൃക്ഷം(), ownedCondp);
    }
    ശൂന്യം visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/അസത്യം); }
    ശൂന്യം visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/സത്യം); }
    ശൂന്യം visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    ശൂന്യം visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    ശൂന്യം visit(AstCond* nodep) override { unsupported_visit(nodep); }
    ശൂന്യം visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    ശൂന്യം prepost_visit(Ast_കണ്ണി_Uniop* സ്ഥിരം nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        സ്ഥിരം തർക്കം isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* സ്ഥിരം selbitp = VN_CAST(nodep->lhsp(), SelBit);
        എങ്കിൽ (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), കണ്ണി_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } അല്ലെങ്കിൽ {
                prepost_വ്യഞ്ജകം_visit(nodep);
            }
        }
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* getOperationp(Ast_കണ്ണി* സ്ഥിരം nodep, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp,
                               Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp) {
        എങ്കിൽ (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            തിരികെ പുതിയ AstSub{nodep->fileline(), lhsp, rhsp};
        }
        എങ്കിൽ (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            തിരികെ പുതിയ AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        എങ്കിൽ (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            തിരഞ്ഞെടുപ്പ് (assignp->operation()) {
            അവസ്ഥ AstAssignCompound::operation::Add:
                തിരികെ പുതിയ AstAdd{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::And:
                തിരികെ പുതിയ AstAnd{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::Div:
                തിരികെ പുതിയ AstDiv{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::ModDiv:
                തിരികെ പുതിയ AstModDiv{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::Mul:
                തിരികെ പുതിയ AstMul{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::Or: തിരികെ പുതിയ AstOr{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::ShiftL:
                തിരികെ പുതിയ AstShiftL{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::ShiftR:
                തിരികെ പുതിയ AstShiftR{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::ShiftRS:
                തിരികെ പുതിയ AstShiftRS{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::Sub:
                തിരികെ പുതിയ AstSub{nodep->fileline(), lhsp, rhsp};
            അവസ്ഥ AstAssignCompound::operation::Xor:
                തിരികെ പുതിയ AstXor{nodep->fileline(), lhsp, rhsp};
            സ്ഥിരസ്ഥിതി:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    ശൂന്യം prepost_stmt_sel_visit(Ast_കണ്ണി_Uniop* സ്ഥിരം nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_വരി* fl = nodep->fileline();
        V3_സംഖ്യ അക്കം_One{fl, 32, 1, അസത്യം};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = പുതിയ AstConst{nodep->fileline(), അക്കം_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    ശൂന്യം prepost_stmt_sel_visit(AstAssignCompound* സ്ഥിരം nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    ശൂന്യം prepost_stmt_sel_visit(Ast_കണ്ണി* സ്ഥിരം nodep, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp,
                                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp) {
        AstSelBit* സ്ഥിരം rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_കണ്ണി_Var_Ref* സ്ഥിരം rdFromp = VN_AS(rdSelbitp->fromp()->clone_വൃക്ഷം_Pure(സത്യം), കണ്ണി_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* സ്ഥിരം wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        സ്ഥിരം string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_അക്കം);
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            പുതിയ AstRefDType{fl, AstRefDType::Flag_Type_Of_വ്യഞ്ജകം{}, rdBitp->clone_വൃക്ഷം(സത്യം)}};
        എങ്കിൽ (m_ftaskp) varp->funcLocal(സത്യം);

        // Declare the variable
        തിരുകുക_On_മുകൾ(varp);

        // Define what operation will we be doing
        AstAssign* സ്ഥിരം varAssignp
            = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_കണ്ണി* സ്ഥിരം newp = varAssignp;

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep
            = പുതിയ AstSelBit{fl, rdFromp, പുതിയ AstVarRef{fl, varp, VAccess::READ}};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം store_മുകൾ
            = പുതിയ AstSelBit{fl, wrFromp, പുതിയ AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = പുതിയ AstAssign{nodep->fileline(), store_മുകൾ, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം prepost_stmt_visit(Ast_കണ്ണി_Uniop* സ്ഥിരം nodep) {
        iterateChildren(nodep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം store_മുകൾ = nodep->lhsp()->clone_വൃക്ഷം_Pure(സത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = nodep->lhsp()->unlinkFrBack();
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        V3_സംഖ്യ അക്കം_One{fl, 32, 1, അസത്യം};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = പുതിയ AstConst{nodep->fileline(), അക്കം_One};

        prepost_stmt_visit(nodep, exprp, store_മുകൾ, valuep);
    }
    ശൂന്യം prepost_stmt_visit(AstAssignCompound* സ്ഥിരം nodep) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം store_മുകൾ = nodep->lhsp()->clone_വൃക്ഷം_Pure(സത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_മുകൾ, valuep);
    }
    ശൂന്യം prepost_stmt_visit(Ast_കണ്ണി* സ്ഥിരം nodep, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp,
                            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം store_മുകൾ, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* സ്ഥിരം assignp
            = പുതിയ AstAssign{nodep->fileline(), store_മുകൾ, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം prepost_വ്യഞ്ജകം_visit(Ast_കണ്ണി_Uniop* സ്ഥിരം nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        എങ്കിൽ (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            തിരികെ;
        }
        സ്ഥിരം തർക്കം needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം readp = nodep->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം writep = nodep->lhsp()->clone_വൃക്ഷം_Pure(സത്യം);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_വരി* സ്ഥിരം fl = nodep->fileline();
        V3_സംഖ്യ അക്കം_One{fl, 32, 1, അസത്യം};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newconstp = പുതിയ AstConst{nodep->fileline(), അക്കം_One};

        // Prepare a temporary variable
        സ്ഥിരം string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_അക്കം);
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            പുതിയ AstRefDType{fl, AstRefDType::Flag_Type_Of_വ്യഞ്ജകം{}, readp->clone_വൃക്ഷം_Pure(സത്യം)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        എങ്കിൽ (m_ftaskp) varp->funcLocal(സത്യം);

        // Declare the variable
        തിരുകുക_On_മുകൾ(varp);

        // Define what operation will we be doing
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം operp = getOperationp(nodep, readp->clone_വൃക്ഷം_Pure(സത്യം), newconstp);

        Ast_കണ്ണി* stmtsp = nullptr;
        എങ്കിൽ (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_വൃക്ഷം_Pure(സത്യം)};
            Ast_കണ്ണി* സ്ഥിരം incp = പുതിയ AstAssign{fl, writep, operp};
            എങ്കിൽ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_വൃക്ഷം_Pure(സത്യം)});
            }
            assignp->addNextHere(പുതിയ AstIf{fl, m_incCondp->clone_വൃക്ഷം_Pure(സത്യം), incp, nullptr});
            stmtsp = assignp;
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* സ്ഥിരം assignp
                = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(പുതിയ AstAssign{fl, writep, പുതിയ AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } അല്ലെങ്കിൽ {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_വൃക്ഷം_Pure(സത്യം)};
            // Increment the original variable by one
            assignp->addNext(പുതിയ AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = പുതിയ AstVarRef{readp->fileline(), varp, VAccess::READ};
        എങ്കിൽ (m_inCovergroup) {
            newp = പുതിയ Ast_വ്യഞ്ജകം_Stmt{fl, stmtsp, newp};
        } അല്ലെങ്കിൽ {
            തിരുകുക_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    ശൂന്യം visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    ശൂന്യം visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    ശൂന്യം visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    ശൂന്യം visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_കണ്ണി_Stmt*>(nodep));
        AstSelBit* സ്ഥിരം selbitp = VN_CAST(nodep->lhsp(), SelBit);
        എങ്കിൽ (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), കണ്ണി_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } അല്ലെങ്കിൽ {
            prepost_stmt_visit(nodep);
        }
    }
    ശൂന്യം visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Task class functions

ശൂന്യം V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("linkinc", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
