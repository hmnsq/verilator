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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkInc.h"

#ضمن "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف LinkIncVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    ثابت VNUser1InUse m_inuser1;

    // STATE
    Ast_عقدة_FTask* m_ftaskp = nullptr;  // Function or task we're inside
    Ast_عقدة_Module* m_modp = nullptr;  // Module we're inside
    صحيح m_mod_Compound_Assignments_رقم = 0;  // Var name counter
    Ast_عقدة* m_insStmtp = nullptr;  // Where to insert statement
    منطقي m_inCovergroup = خطأ;  // Under a covergroup's coverpoint, cross, or option
    منطقي m_condEvalContext = خطأ;  // ++/-- is in a conditionally-evaluated position
    Ast_عقدة_تعبير* m_incCondp = nullptr;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    فراغ أدرج_On_قمة(Ast_عقدة* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        إذا (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } وإلا {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            إذا (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    فراغ أدرج_Before_Stmt(Ast_عقدة* nodep, Ast_عقدة* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->addHereThisAsNext(newp);
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
        إذا (nodep->dead()) إرجاع;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_رقم);
        m_modp = nodep;
        m_mod_Compound_Assignments_رقم = 0;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    فراغ visitCovergroup(Ast_عقدة* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_inCovergroup);
        m_ftaskp = nullptr;
        m_inCovergroup = true;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_عقدة_Func_Cov_Item* nodep) override { visitCovergroup(nodep); }
    فراغ visit(AstCgOptionAssign* nodep) override { visitCovergroup(nodep); }
    فراغ visit(Ast_عقدة_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->propp());
        m_insStmtp = nullptr;
        // Note: no iterating over sentreep here as they will be ignored anyway
        إذا (AstAssert* ثابت assertp = VN_CAST(nodep, Assert)) {
            iterateAndNextNull(assertp->failsp());
        } وإلا إذا (AstAssertIntrinsic* ثابت intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterateAndNextNull(intrinsicp->failsp());
        }
        iterateAndNextNull(nodep->passsp());
    }
    فراغ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    فراغ visit(Ast_عقدة_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    فراغ visit(AstJumpBlock* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = nullptr;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    فراغ visit(Ast_عقدة_If* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->thensp());
        iterateAndNextNull(nodep->elsesp());
        m_insStmtp = nullptr;
    }
    فراغ visit(AstCaseItem* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = true;
            iterateAndNextNull(nodep->condsp());
        }
        m_insStmtp = nullptr;  // Next thing should be new statement
        iterateAndNextNull(nodep->stmtsp());
    }
    فراغ visit(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->lhsp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    فراغ visit(AstEventControl* nodep) override {
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    فراغ visit(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterateAndNextNull(nodep->condp());
        m_insStmtp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
        m_insStmtp = nullptr;
    }
    فراغ visit(AstBegin* nodep) override {
        m_insStmtp = nullptr;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    فراغ visit(Ast_Stmt_تعبير* nodep) override {
        Ast_عقدة_تعبير* ثابت exprp = nodep->exprp();
        إذا (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlinkFrBack());
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
            m_insStmtp = nullptr;
            iterate(exprp);
            m_insStmtp = nullptr;
            إرجاع;
        }
        visit(static_cast<Ast_عقدة_Stmt*>(nodep));
    }
    فراغ visit(Ast_عقدة_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = nullptr;  // Next thing should be new statement
    }
    فراغ unsupported_visit(Ast_عقدة* nodep) {
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
    Ast_عقدة_تعبير* captureLogicalLhsToTemp(Ast_عقدة_Biop* ثابت nodep) {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_عقدة_تعبير* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
        ثابت string name = "__VincGate"s + cvtToStr(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_Of_تعبير{}, lhsp->clone_شجرة(true)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(true);
        أدرج_On_قمة(varp);
        Ast_عقدة* tempInitp = جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        إذا (m_incCondp) {
            tempInitp = جديد AstIf{fl, m_incCondp->clone_شجرة_Pure(true), tempInitp, nullptr};
        }
        أدرج_Before_Stmt(nodep, tempInitp);
        nodep->lhsp(جديد AstVarRef{fl, varp, VAccess::READ});
        إرجاع جديد AstVarRef{fl, varp, VAccess::READ};
    }
    فراغ handleShortCircuit(Ast_عقدة_Biop* ثابت nodep, منطقي isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        إذا (nodep->user1SetOnce()) إرجاع;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterateAndNextNull(nodep->lhsp());
        m_condEvalContext = true;
        Ast_عقدة_تعبير* ownedCondp = nullptr;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        إذا (nodep->rhsp()->exists([](ثابت Ast_عقدة* np) {
                إرجاع VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, AssignCompound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_عقدة_تعبير* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_شجرة_Pure(true)
                                       : captureLogicalLhsToTemp(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            إذا (isOr) lhsRefp = جديد AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? جديد AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_شجرة_Pure(true), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterateAndNextNull(nodep->rhsp());
        إذا (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_شجرة(), ownedCondp);
    }
    فراغ visit(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/خطأ); }
    فراغ visit(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/true); }
    فراغ visit(AstLogEq* nodep) override { unsupported_visit(nodep); }
    فراغ visit(AstLogIf* nodep) override { unsupported_visit(nodep); }
    فراغ visit(AstCond* nodep) override { unsupported_visit(nodep); }
    فراغ visit(AstPropSpec* nodep) override { unsupported_visit(nodep); }
    فراغ prepost_visit(Ast_عقدة_Uniop* ثابت nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        ثابت منطقي isStandaloneStmt = !m_insStmtp && !m_inCovergroup;
        AstSelBit* ثابت selbitp = VN_CAST(nodep->lhsp(), SelBit);
        إذا (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), عقدة_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } وإلا {
            إذا (isStandaloneStmt) {
                prepost_stmt_visit(nodep);
            } وإلا {
                prepost_تعبير_visit(nodep);
            }
        }
    }
    Ast_عقدة_تعبير* getOperationp(Ast_عقدة* ثابت nodep, Ast_عقدة_تعبير* ثابت lhsp,
                               Ast_عقدة_تعبير* ثابت rhsp) {
        إذا (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            إرجاع جديد AstSub{nodep->fileline(), lhsp, rhsp};
        }
        إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            إرجاع جديد AstAdd{nodep->fileline(), lhsp, rhsp};
        }
        إذا (AstAssignCompound* assignp = VN_CAST(nodep, AssignCompound)) {
            بدل (assignp->operation()) {
            حالة AstAssignCompound::operation::Add:
                إرجاع جديد AstAdd{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::And:
                إرجاع جديد AstAnd{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::Div:
                إرجاع جديد AstDiv{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::ModDiv:
                إرجاع جديد AstModDiv{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::Mul:
                إرجاع جديد AstMul{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::Or: إرجاع جديد AstOr{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::ShiftL:
                إرجاع جديد AstShiftL{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::ShiftR:
                إرجاع جديد AstShiftR{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::ShiftRS:
                إرجاع جديد AstShiftRS{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::Sub:
                إرجاع جديد AstSub{nodep->fileline(), lhsp, rhsp};
            حالة AstAssignCompound::operation::Xor:
                إرجاع جديد AstXor{nodep->fileline(), lhsp, rhsp};
            افتراضي:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    فراغ prepost_stmt_sel_visit(Ast_عقدة_Uniop* ثابت nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_سطر* fl = nodep->fileline();
        V3_العدد رقم_One{fl, 32, 1, خطأ};
        Ast_عقدة_تعبير* ثابت exprp = جديد AstConst{nodep->fileline(), رقم_One};

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_visit(AstAssignCompound* ثابت nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_عقدة_تعبير* ثابت exprp = nodep->rhsp()->unlinkFrBack();

        prepost_stmt_sel_visit(nodep, nodep->lhsp(), exprp);
    }
    فراغ prepost_stmt_sel_visit(Ast_عقدة* ثابت nodep, Ast_عقدة_تعبير* ثابت lhsp,
                                Ast_عقدة_تعبير* ثابت exprp) {
        AstSelBit* ثابت rdSelbitp = VN_AS(lhsp, SelBit);
        Ast_عقدة_Var_Ref* ثابت rdFromp = VN_AS(rdSelbitp->fromp()->clone_شجرة_Pure(true), عقدة_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_عقدة_تعبير* ثابت rdBitp = rdSelbitp->bitp()->unlinkFrBack();
        AstSelBit* ثابت wrSelbitp = VN_CAST(lhsp, SelBit);
        Ast_عقدة_تعبير* ثابت wrFromp = wrSelbitp->fromp()->unlinkFrBack();
        V3LinkLValue::linkLValueSet(wrFromp);

        // Prepare a temporary variable
        File_سطر* ثابت fl = nodep->fileline();
        ثابت string name = "__VtempIndex"s + cvtToStr(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_Of_تعبير{}, rdBitp->clone_شجرة(true)}};
        إذا (m_ftaskp) varp->funcLocal(true);

        // Declare the variable
        أدرج_On_قمة(varp);

        // Define what operation will we be doing
        AstAssign* ثابت varAssignp
            = جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_عقدة* ثابت newp = varAssignp;

        Ast_عقدة_تعبير* ثابت valuep
            = جديد AstSelBit{fl, rdFromp, جديد AstVarRef{fl, varp, VAccess::READ}};
        Ast_عقدة_تعبير* ثابت store_قمة
            = جديد AstSelBit{fl, wrFromp, جديد AstVarRef{fl, varp, VAccess::READ}};

        AstAssign* assignp
            = جديد AstAssign{nodep->fileline(), store_قمة, getOperationp(nodep, valuep, exprp)};
        newp->addNext(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
    }
    فراغ prepost_stmt_visit(Ast_عقدة_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        Ast_عقدة_تعبير* ثابت store_قمة = nodep->lhsp()->clone_شجرة_Pure(true);
        Ast_عقدة_تعبير* ثابت valuep = nodep->lhsp()->unlinkFrBack();
        File_سطر* ثابت fl = nodep->fileline();
        V3_العدد رقم_One{fl, 32, 1, خطأ};
        Ast_عقدة_تعبير* ثابت exprp = جديد AstConst{nodep->fileline(), رقم_One};

        prepost_stmt_visit(nodep, exprp, store_قمة, valuep);
    }
    فراغ prepost_stmt_visit(AstAssignCompound* ثابت nodep) {
        Ast_عقدة_تعبير* ثابت exprp = nodep->rhsp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت store_قمة = nodep->lhsp()->clone_شجرة_Pure(true);
        Ast_عقدة_تعبير* ثابت valuep = nodep->lhsp()->unlinkFrBack();

        prepost_stmt_visit(nodep, exprp, store_قمة, valuep);
    }
    فراغ prepost_stmt_visit(Ast_عقدة* ثابت nodep, Ast_عقدة_تعبير* ثابت exprp,
                            Ast_عقدة_تعبير* ثابت store_قمة, Ast_عقدة_تعبير* ثابت valuep) {
        V3LinkLValue::linkLValueSet(valuep, VAccess::READ);
        AstAssign* ثابت assignp
            = جديد AstAssign{nodep->fileline(), store_قمة, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
    }
    فراغ prepost_تعبير_visit(Ast_عقدة_Uniop* ثابت nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        إذا (m_condEvalContext && !m_incCondp && !m_inCovergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            إرجاع;
        }
        ثابت منطقي needGating = m_condEvalContext && m_incCondp && !m_inCovergroup;
        Ast_عقدة_تعبير* ثابت readp = nodep->lhsp();
        Ast_عقدة_تعبير* ثابت writep = nodep->lhsp()->clone_شجرة_Pure(true);
        V3LinkLValue::linkLValueSet(readp, VAccess::READ);

        File_سطر* ثابت fl = nodep->fileline();
        V3_العدد رقم_One{fl, 32, 1, خطأ};
        Ast_عقدة_تعبير* ثابت newconstp = جديد AstConst{nodep->fileline(), رقم_One};

        // Prepare a temporary variable
        ثابت string name = "__Vincrement"s + cvtToStr(++m_mod_Compound_Assignments_رقم);
        AstVar* ثابت varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlagChildDType{},
            جديد AstRefDType{fl, AstRefDType::Flag_Type_Of_تعبير{}, readp->clone_شجرة_Pure(true)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        إذا (m_ftaskp) varp->funcLocal(true);

        // Declare the variable
        أدرج_On_قمة(varp);

        // Define what operation will we be doing
        Ast_عقدة_تعبير* ثابت operp = getOperationp(nodep, readp->clone_شجرة_Pure(true), newconstp);

        Ast_عقدة* stmtsp = nullptr;
        إذا (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            AstAssign* ثابت assignp = جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_شجرة_Pure(true)};
            Ast_عقدة* ثابت incp = جديد AstAssign{fl, writep, operp};
            إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->addNext(جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_شجرة_Pure(true)});
            }
            assignp->addNextHere(جديد AstIf{fl, m_incCondp->clone_شجرة_Pure(true), incp, nullptr});
            stmtsp = assignp;
        } وإلا إذا (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            AstAssign* ثابت assignp
                = جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->addNext(جديد AstAssign{fl, writep, جديد AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } وإلا {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            AstAssign* ثابت assignp = جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_شجرة_Pure(true)};
            // Increment the original variable by one
            assignp->addNext(جديد AstAssign{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_عقدة_تعبير* newp = جديد AstVarRef{readp->fileline(), varp, VAccess::READ};
        إذا (m_inCovergroup) {
            newp = جديد Ast_تعبير_Stmt{fl, stmtsp, newp};
        } وإلا {
            أدرج_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
    }
    فراغ visit(AstPreInc* nodep) override { prepost_visit(nodep); }
    فراغ visit(AstPostInc* nodep) override { prepost_visit(nodep); }
    فراغ visit(AstPreDec* nodep) override { prepost_visit(nodep); }
    فراغ visit(AstPostDec* nodep) override { prepost_visit(nodep); }
    فراغ visit(AstAssignCompound* nodep) override {
        visit(static_cast<Ast_عقدة_Stmt*>(nodep));
        AstSelBit* ثابت selbitp = VN_CAST(nodep->lhsp(), SelBit);
        إذا (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), عقدة_Var_Ref)
            && !selbitp->bitp()->isPure()) {
            prepost_stmt_sel_visit(nodep);
        } وإلا {
            prepost_stmt_visit(nodep);
        }
    }
    فراغ visit(AstGenFor* nodep) override { iterateChildren(nodep); }
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("linkinc", 0, dump_شجرة_Either_Level() >= 3);
}
