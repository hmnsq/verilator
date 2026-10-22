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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkInc.h"

#అన్తర్భావ "V3LinkLValue.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి LinkIncVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstLogAnd/AstLogOr::user1()  -> bool.  True if already lowered
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Function or task we're inside
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Module we're inside
    పూర్ణాంకం m_mod_Compound_Assignments_అంకె = 0;  // Var name counter
    Ast_కణుపు* m_insStmtp = శూన్యసూచిక;  // Where to insert statement
    తార్కిక m_లోపల_Covergroup = అబద్ధం;  // Under a covergroup's coverpoint, cross, or option
    తార్కిక m_condEvalContext = అబద్ధం;  // ++/-- is in a conditionally-evaluated position
    Ast_కణుపు_సమాసం* m_incCondp = శూన్యసూచిక;  // Gating condition for ++/-- in short-circuit context

    // METHODS
    శూన్య చొప్పించు_On_శిఖరం(Ast_కణుపు* newp) {
        // Add the thing directly under the current TFunc/Module, or class, as a member
        // Must not unlink the statements, as one of them is being iterated
        ఒకవేళ (m_ftaskp) {
            m_ftaskp->prependStmtsp(newp);
        } లేకపోతే {
            UASSERT_OBJ(m_modp, newp, "Variable not under FTASK/MODULE");
            ఒకవేళ (VN_IS(m_modp, Class)) VN_AS(newp, Var)->varType(VVarType::MEMBER);
            m_modp->prependStmtsp(newp);
        }
    }
    శూన్య చొప్పించు_Before_Stmt(Ast_కణుపు* nodep, Ast_కణుపు* newp) {
        // Return node that must be visited, if any
        UINFOTREE(9, newp, "", "newstmt");
        UASSERT_OBJ(m_insStmtp, nodep, "Expression not underneath a statement");
        m_insStmtp->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (nodep->dead()) తిరిగి;
        VL_RESTORER(m_modp);
        VL_RESTORER(m_mod_Compound_Assignments_అంకె);
        m_modp = nodep;
        m_mod_Compound_Assignments_అంకె = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    // A covergroup's expressions are under no statement, and V3Covergroup moves them to its
    // constructor or sample(), so their variables are members of its class
    శూన్య సందర్శించు_Covergroup(Ast_కణుపు* nodep) {
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_లోపల_Covergroup);
        m_ftaskp = శూన్యసూచిక;
        m_లోపల_Covergroup = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Func_Cov_అంశం* nodep) override { సందర్శించు_Covergroup(nodep); }
    శూన్య సందర్శించు(Ast_Cg_Option_కేటాయించు* nodep) override { సందర్శించు_Covergroup(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_Cover_Or_Assert* nodep) override {
        VL_RESTORER(m_insStmtp);
        m_insStmtp = nodep;
        iterate_And_తదుపరి_Null(nodep->propp());
        m_insStmtp = శూన్యసూచిక;
        // Note: no iterating over sentreep here as they will be ignored anyway
        ఒకవేళ (AstAssert* స్థిరరాశి assertp = VN_CAST(nodep, Assert)) {
            iterate_And_తదుపరి_Null(assertp->failsp());
        } లేకపోతే ఒకవేళ (AstAssertIntrinsic* స్థిరరాశి intrinsicp = VN_CAST(nodep, AssertIntrinsic)) {
            iterate_And_తదుపరి_Null(intrinsicp->failsp());
        }
        iterate_And_తదుపరి_Null(nodep->passsp());
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        m_insStmtp = శూన్యసూచిక;  // First thing should be new statement
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        m_insStmtp = శూన్యసూచిక;  // Next thing should be new statement
    }
    శూన్య సందర్శించు(Ast_కణుపు_Foreach* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = శూన్యసూచిక;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = శూన్యసూచిక;  // Next thing should be new statement
    }
    శూన్య సందర్శించు(Ast_Jump_ఖండం* nodep) override {
        // Special, as statements need to be put in different places
        // Body insert just before themselves
        m_insStmtp = శూన్యసూచిక;  // First thing should be new statement
        iterateChildren(nodep);
        // Done the loop
        m_insStmtp = శూన్యసూచిక;  // Next thing should be new statement
    }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        m_insStmtp = nodep;
        iterate_And_తదుపరి_Null(nodep->condp());
        m_insStmtp = శూన్యసూచిక;
        iterate_And_తదుపరి_Null(nodep->thensp());
        iterate_And_తదుపరి_Null(nodep->elsesp());
        m_insStmtp = శూన్యసూచిక;
    }
    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        {
            VL_RESTORER(m_condEvalContext);
            m_condEvalContext = నిజం;
            iterate_And_తదుపరి_Null(nodep->condsp());
        }
        m_insStmtp = శూన్యసూచిక;  // Next thing should be new statement
        iterate_And_తదుపరి_Null(nodep->stmtsp());
    }
    శూన్య సందర్శించు(AstDelay* nodep) override {
        m_insStmtp = nodep;
        iterate_And_తదుపరి_Null(nodep->lhsp());
        m_insStmtp = శూన్యసూచిక;
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        m_insStmtp = శూన్యసూచిక;
    }
    శూన్య సందర్శించు(AstEventControl* nodep) override {
        m_insStmtp = శూన్యసూచిక;
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        m_insStmtp = శూన్యసూచిక;
    }
    శూన్య సందర్శించు(AstWait* nodep) override {
        m_insStmtp = nodep;
        iterate_And_తదుపరి_Null(nodep->condp());
        m_insStmtp = శూన్యసూచిక;
        iterate_And_తదుపరి_Null(nodep->stmtsp());
        m_insStmtp = శూన్యసూచిక;
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        m_insStmtp = శూన్యసూచిక;  // Pretend not a statement TODO: parse ++/-- as ExprStmt
        iterateChildren(nodep);
        m_insStmtp = శూన్యసూచిక;  // Next thing should be new statement
    }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->exprp();
        ఒకవేళ (VN_IS(exprp, PostInc) || VN_IS(exprp, PostDec) || VN_IS(exprp, PreInc)
            || VN_IS(exprp, PreDec)) {
            // Repalce this StmtExpr with the expression, visiting it will turn it into a NodeStmt
            nodep->replaceWith(exprp->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            m_insStmtp = శూన్యసూచిక;
            iterate(exprp);
            m_insStmtp = శూన్యసూచిక;
            తిరిగి;
        }
        సందర్శించు(static_cast<Ast_కణుపు_Stmt*>(nodep));
    }
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        m_insStmtp = nodep;
        iterateChildren(nodep);
        m_insStmtp = శూన్యసూచిక;  // Next thing should be new statement
    }
    శూన్య unsupported_సందర్శించు(Ast_కణుపు* nodep) {
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        m_condEvalContext = నిజం;
        // Not rescuable by short-circuit gating; drop the gate so nested ++/-- errors
        m_incCondp = శూన్యసూచిక;
        UINFO(9, "Marking unsupported " << nodep);
        iterateChildren(nodep);
    }
    // Hoist LHS into a BLOCKTEMP so it is evaluated exactly once; otherwise the
    // gated RHS ++/-- could modify a variable the LHS reads.
    Ast_కణుపు_సమాసం* capture_Logical_Lhs_గమ్యం_తాత్కాలిక(Ast_కణుపు_Biop* స్థిరరాశి nodep) {
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
        స్థిరరాశి string name = "__VincGate"s + cvt_గమ్యం_Str(++m_mod_Compound_Assignments_అంకె);
        AstVar* స్థిరరాశి varp = కొత్త AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_బిడ్డ_DType{},
            కొత్త AstRefDType{fl, AstRefDType::Flag_Type_యొక్క_సమాసం{}, lhsp->clone_వృక్షం(నిజం)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ఒకవేళ (m_ftaskp) varp->funcLocal(నిజం);
        చొప్పించు_On_శిఖరం(varp);
        Ast_కణుపు* తాత్కాలిక_Initp = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, lhsp};
        // Gate by the enclosing condition so a side-effecting LHS isn't run on short-circuit
        ఒకవేళ (m_incCondp) {
            తాత్కాలిక_Initp = కొత్త AstIf{fl, m_incCondp->clone_వృక్షం_శుద్ధ(నిజం), తాత్కాలిక_Initp, శూన్యసూచిక};
        }
        చొప్పించు_Before_Stmt(nodep, తాత్కాలిక_Initp);
        nodep->lhsp(కొత్త AstVarRef{fl, varp, VAccess::READ});
        తిరిగి కొత్త AstVarRef{fl, varp, VAccess::READ};
    }
    శూన్య handleShortCircuit(Ast_కణుపు_Biop* స్థిరరాశి nodep, తార్కిక isOr) {
        // insertBeforeStmt retargets the parent iterator; skip on re-visit
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        VL_RESTORER(m_condEvalContext);
        VL_RESTORER(m_incCondp);
        iterate_And_తదుపరి_Null(nodep->lhsp());
        m_condEvalContext = నిజం;
        Ast_కణుపు_సమాసం* ownedCondp = శూన్యసూచిక;
        // Gate the RHS by the LHS condition if it evaluates any ++/-- anywhere within
        ఒకవేళ (nodep->rhsp()->exists([](స్థిరరాశి Ast_కణుపు* np) {
                తిరిగి VN_IS(np, PreInc) || VN_IS(np, PreDec) || VN_IS(np, PostInc)
                       || VN_IS(np, PostDec) || VN_IS(np, కేటాయించు_Compound);
            })) {
            // Const LHS or no statement context: clone-and-evaluate-twice is safe
            Ast_కణుపు_సమాసం* lhsRefp = (!m_insStmtp || VN_IS(nodep->lhsp(), Const))
                                       ? nodep->lhsp()->clone_వృక్షం_శుద్ధ(నిజం)
                                       : capture_Logical_Lhs_గమ్యం_తాత్కాలిక(nodep);
            // For ||, RHS runs when LHS is false; gate by !LHS
            ఒకవేళ (isOr) lhsRefp = కొత్త AstLogNot{nodep->fileline(), lhsRefp};
            ownedCondp = m_incCondp ? కొత్త AstLogAnd{nodep->fileline(),
                                                    m_incCondp->clone_వృక్షం_శుద్ధ(నిజం), lhsRefp}
                                    : lhsRefp;
            m_incCondp = ownedCondp;
        }
        iterate_And_తదుపరి_Null(nodep->rhsp());
        ఒకవేళ (ownedCondp) VL_DO_DANGLING(ownedCondp->delete_వృక్షం(), ownedCondp);
    }
    శూన్య సందర్శించు(AstLogAnd* nodep) override { handleShortCircuit(nodep, /*isOr=*/అబద్ధం); }
    శూన్య సందర్శించు(AstLogOr* nodep) override { handleShortCircuit(nodep, /*isOr=*/నిజం); }
    శూన్య సందర్శించు(AstLogEq* nodep) override { unsupported_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstLogIf* nodep) override { unsupported_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstCond* nodep) override { unsupported_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstPropSpec* nodep) override { unsupported_సందర్శించు(nodep); }
    శూన్య prepost_సందర్శించు(Ast_కణుపు_Uniop* స్థిరరాశి nodep) {
        // Not underneath a statement, the increment is one, unless in a covergroup expression,
        // which is under none
        స్థిరరాశి తార్కిక isStandaloneStmt = !m_insStmtp && !m_లోపల_Covergroup;
        Ast_Sel_బిట్టు* స్థిరరాశి selbitp = VN_CAST(nodep->lhsp(), Sel_బిట్టు);
        ఒకవేళ (isStandaloneStmt && selbitp && VN_IS(selbitp->fromp(), కణుపు_Var_Ref)
            && !selbitp->bitp()->is_శుద్ధ()) {
            prepost_stmt_sel_సందర్శించు(nodep);
        } లేకపోతే {
            ఒకవేళ (isStandaloneStmt) {
                prepost_stmt_సందర్శించు(nodep);
            } లేకపోతే {
                prepost_సమాసం_సందర్శించు(nodep);
            }
        }
    }
    Ast_కణుపు_సమాసం* getOperationp(Ast_కణుపు* స్థిరరాశి nodep, Ast_కణుపు_సమాసం* స్థిరరాశి lhsp,
                               Ast_కణుపు_సమాసం* స్థిరరాశి rhsp) {
        ఒకవేళ (VN_IS(nodep, PreDec) || VN_IS(nodep, PostDec)) {
            తిరిగి కొత్త AstSub{nodep->fileline(), lhsp, rhsp};
        }
        ఒకవేళ (VN_IS(nodep, PreInc) || VN_IS(nodep, PostInc)) {
            తిరిగి కొత్త Ast_కూడు{nodep->fileline(), lhsp, rhsp};
        }
        ఒకవేళ (Ast_కేటాయించు_Compound* assignp = VN_CAST(nodep, కేటాయించు_Compound)) {
            ఎంపిక (assignp->operation()) {
            స్థితి Ast_కేటాయించు_Compound::operation::కూడు_2:
                తిరిగి కొత్త Ast_కూడు{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::And:
                తిరిగి కొత్త AstAnd{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::Div:
                తిరిగి కొత్త AstDiv{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::ModDiv:
                తిరిగి కొత్త AstModDiv{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::Mul:
                తిరిగి కొత్త AstMul{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::Or: తిరిగి కొత్త AstOr{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::జరుపు_L:
                తిరిగి కొత్త Ast_జరుపు_L{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::జరుపు_R:
                తిరిగి కొత్త Ast_జరుపు_R{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::జరుపు_RS:
                తిరిగి కొత్త Ast_జరుపు_RS{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::Sub:
                తిరిగి కొత్త AstSub{nodep->fileline(), lhsp, rhsp};
            స్థితి Ast_కేటాయించు_Compound::operation::Xor:
                తిరిగి కొత్త AstXor{nodep->fileline(), lhsp, rhsp};
            అప్రమేయ:;  // Error below  // LCOV_EXCL_LINE
            }
        }
        nodep->v3fatalSrc("Unhandled compound assignment operation");
    }
    శూన్య prepost_stmt_sel_సందర్శించు(Ast_కణుపు_Uniop* స్థిరరాశి nodep) {
        // Special case array[something]++, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        iterateChildren(nodep);
        File_పంక్తి* fl = nodep->fileline();
        V3_సంఖ్య అంకె_One{fl, 32, 1, అబద్ధం};
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = కొత్త AstConst{nodep->fileline(), అంకె_One};

        prepost_stmt_sel_సందర్శించు(nodep, nodep->lhsp(), exprp);
    }
    శూన్య prepost_stmt_sel_సందర్శించు(Ast_కేటాయించు_Compound* స్థిరరాశి nodep) {
        // Special case array[something] += expr, see comments at file top
        // UINFOTREE(9, nodep, "", "pp-stmt-sel-in");
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->rhsp()->unlink_Fr_వెనుక();

        prepost_stmt_sel_సందర్శించు(nodep, nodep->lhsp(), exprp);
    }
    శూన్య prepost_stmt_sel_సందర్శించు(Ast_కణుపు* స్థిరరాశి nodep, Ast_కణుపు_సమాసం* స్థిరరాశి lhsp,
                                Ast_కణుపు_సమాసం* స్థిరరాశి exprp) {
        Ast_Sel_బిట్టు* స్థిరరాశి rdSelbitp = VN_AS(lhsp, Sel_బిట్టు);
        Ast_కణుపు_Var_Ref* స్థిరరాశి rdFromp = VN_AS(rdSelbitp->fromp()->clone_వృక్షం_శుద్ధ(నిజం), కణుపు_Var_Ref);
        rdFromp->access(VAccess::READ);
        Ast_కణుపు_సమాసం* స్థిరరాశి rdBitp = rdSelbitp->bitp()->unlink_Fr_వెనుక();
        Ast_Sel_బిట్టు* స్థిరరాశి wrSelbitp = VN_CAST(lhsp, Sel_బిట్టు);
        Ast_కణుపు_సమాసం* స్థిరరాశి wrFromp = wrSelbitp->fromp()->unlink_Fr_వెనుక();
        V3LinkLValue::link_LValue_సమితి(wrFromp);

        // Prepare a temporary variable
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        స్థిరరాశి string name = "__VtempIndex"s + cvt_గమ్యం_Str(++m_mod_Compound_Assignments_అంకె);
        AstVar* స్థిరరాశి varp = కొత్త AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_బిడ్డ_DType{},
            కొత్త AstRefDType{fl, AstRefDType::Flag_Type_యొక్క_సమాసం{}, rdBitp->clone_వృక్షం(నిజం)}};
        ఒకవేళ (m_ftaskp) varp->funcLocal(నిజం);

        // Declare the variable
        చొప్పించు_On_శిఖరం(varp);

        // Define what operation will we be doing
        Ast_కేటాయించు* స్థిరరాశి varAssignp
            = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, rdBitp};
        Ast_కణుపు* స్థిరరాశి newp = varAssignp;

        Ast_కణుపు_సమాసం* స్థిరరాశి valuep
            = కొత్త Ast_Sel_బిట్టు{fl, rdFromp, కొత్త AstVarRef{fl, varp, VAccess::READ}};
        Ast_కణుపు_సమాసం* స్థిరరాశి store_శిఖరం
            = కొత్త Ast_Sel_బిట్టు{fl, wrFromp, కొత్త AstVarRef{fl, varp, VAccess::READ}};

        Ast_కేటాయించు* assignp
            = కొత్త Ast_కేటాయించు{nodep->fileline(), store_శిఖరం, getOperationp(nodep, valuep, exprp)};
        newp->కూడు_తదుపరి(assignp);

        // if (debug() >= 9) newp->dumpTreeAndNext("-pp-stmt-sel-new: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య prepost_stmt_సందర్శించు(Ast_కణుపు_Uniop* స్థిరరాశి nodep) {
        iterateChildren(nodep);
        Ast_కణుపు_సమాసం* స్థిరరాశి store_శిఖరం = nodep->lhsp()->clone_వృక్షం_శుద్ధ(నిజం);
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = nodep->lhsp()->unlink_Fr_వెనుక();
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        V3_సంఖ్య అంకె_One{fl, 32, 1, అబద్ధం};
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = కొత్త AstConst{nodep->fileline(), అంకె_One};

        prepost_stmt_సందర్శించు(nodep, exprp, store_శిఖరం, valuep);
    }
    శూన్య prepost_stmt_సందర్శించు(Ast_కేటాయించు_Compound* స్థిరరాశి nodep) {
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి store_శిఖరం = nodep->lhsp()->clone_వృక్షం_శుద్ధ(నిజం);
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = nodep->lhsp()->unlink_Fr_వెనుక();

        prepost_stmt_సందర్శించు(nodep, exprp, store_శిఖరం, valuep);
    }
    శూన్య prepost_stmt_సందర్శించు(Ast_కణుపు* స్థిరరాశి nodep, Ast_కణుపు_సమాసం* స్థిరరాశి exprp,
                            Ast_కణుపు_సమాసం* స్థిరరాశి store_శిఖరం, Ast_కణుపు_సమాసం* స్థిరరాశి valuep) {
        V3LinkLValue::link_LValue_సమితి(valuep, VAccess::READ);
        Ast_కేటాయించు* స్థిరరాశి assignp
            = కొత్త Ast_కేటాయించు{nodep->fileline(), store_శిఖరం, getOperationp(nodep, valuep, exprp)};
        nodep->replaceWith(assignp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య prepost_సమాసం_సందర్శించు(Ast_కణుపు_Uniop* స్థిరరాశి nodep) {
        iterateChildren(nodep);
        // In a covergroup, whose expressions are under no statement, the increment forms an
        // expression statement in place, as the parser makes '(x += 1)', so it is evaluated only
        // where and when its expression is
        ఒకవేళ (m_condEvalContext && !m_incCondp && !m_లోపల_Covergroup) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Pre/post increment/decrement operator"
                                         " within a logical expression (&&, ||, ?:, etc.)");
            తిరిగి;
        }
        స్థిరరాశి తార్కిక needGating = m_condEvalContext && m_incCondp && !m_లోపల_Covergroup;
        Ast_కణుపు_సమాసం* స్థిరరాశి readp = nodep->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి writep = nodep->lhsp()->clone_వృక్షం_శుద్ధ(నిజం);
        V3LinkLValue::link_LValue_సమితి(readp, VAccess::READ);

        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        V3_సంఖ్య అంకె_One{fl, 32, 1, అబద్ధం};
        Ast_కణుపు_సమాసం* స్థిరరాశి newconstp = కొత్త AstConst{nodep->fileline(), అంకె_One};

        // Prepare a temporary variable
        స్థిరరాశి string name = "__Vincrement"s + cvt_గమ్యం_Str(++m_mod_Compound_Assignments_అంకె);
        AstVar* స్థిరరాశి varp = కొత్త AstVar{
            fl, VVarType::BLOCKTEMP, name, VFlag_బిడ్డ_DType{},
            కొత్త AstRefDType{fl, AstRefDType::Flag_Type_యొక్క_సమాసం{}, readp->clone_వృక్షం_శుద్ధ(నిజం)}};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ఒకవేళ (m_ftaskp) varp->funcLocal(నిజం);

        // Declare the variable
        చొప్పించు_On_శిఖరం(varp);

        // Define what operation will we be doing
        Ast_కణుపు_సమాసం* స్థిరరాశి operp = getOperationp(nodep, readp->clone_వృక్షం_శుద్ధ(నిజం), newconstp);

        Ast_కణుపు* stmtsp = శూన్యసూచిక;
        ఒకవేళ (needGating) {
            // Short-circuit context: mutate the variable only when the gate is true.
            // The temp holds the old value, plus the new value for pre-inc/dec.
            Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_వృక్షం_శుద్ధ(నిజం)};
            Ast_కణుపు* స్థిరరాశి incp = కొత్త Ast_కేటాయించు{fl, writep, operp};
            ఒకవేళ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
                incp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE},
                                            readp->clone_వృక్షం_శుద్ధ(నిజం)});
            }
            assignp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstIf{fl, m_incCondp->clone_వృక్షం_శుద్ధ(నిజం), incp, శూన్యసూచిక});
            stmtsp = assignp;
        } లేకపోతే ఒకవేళ (VN_IS(nodep, PreInc) || VN_IS(nodep, PreDec)) {
            // PreInc/PreDec operations
            // Immediately after declaration - increment it by one
            Ast_కేటాయించు* స్థిరరాశి assignp
                = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE}, operp};
            // Immediately after incrementing - assign it to the original variable
            assignp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{fl, writep, కొత్త AstVarRef{fl, varp, VAccess::READ}});
            stmtsp = assignp;
        } లేకపోతే {
            // PostInc/PostDec operations
            // Assign the original variable to the temporary one
            Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE},
                                                     readp->clone_వృక్షం_శుద్ధ(నిజం)};
            // Increment the original variable by one
            assignp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{fl, writep, operp});
            stmtsp = assignp;
        }

        // Replace the node with the temporary, after the statements
        Ast_కణుపు_సమాసం* newp = కొత్త AstVarRef{readp->fileline(), varp, VAccess::READ};
        ఒకవేళ (m_లోపల_Covergroup) {
            newp = కొత్త Ast_సమాసం_Stmt{fl, stmtsp, newp};
        } లేకపోతే {
            చొప్పించు_Before_Stmt(nodep, stmtsp);
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstPreInc* nodep) override { prepost_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstPostInc* nodep) override { prepost_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstPreDec* nodep) override { prepost_సందర్శించు(nodep); }
    శూన్య సందర్శించు(AstPostDec* nodep) override { prepost_సందర్శించు(nodep); }
    శూన్య సందర్శించు(Ast_కేటాయించు_Compound* nodep) override {
        సందర్శించు(static_cast<Ast_కణుపు_Stmt*>(nodep));
        Ast_Sel_బిట్టు* స్థిరరాశి selbitp = VN_CAST(nodep->lhsp(), Sel_బిట్టు);
        ఒకవేళ (!m_insStmtp && selbitp && VN_IS(selbitp->fromp(), కణుపు_Var_Ref)
            && !selbitp->bitp()->is_శుద్ధ()) {
            prepost_stmt_sel_సందర్శించు(nodep);
        } లేకపోతే {
            prepost_stmt_సందర్శించు(nodep);
        }
    }
    శూన్య సందర్శించు(AstGenFor* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన LinkIncVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~LinkIncVisitor() override = అప్రమేయ;
};

//######################################################################
// Task class functions

శూన్య V3LinkInc::linkIncrements(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { LinkIncVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("linkinc", 0, dump_వృక్షం_Either_Level() >= 3);
}
