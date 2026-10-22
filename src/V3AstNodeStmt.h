// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode sub-types representing statements
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
//
// This files contains all 'AstNode' sub-types that are statements.
//
//*************************************************************************

#ifndef VERILATOR_V3ASTNODESTMT_H_
#define VERILATOR_V3ASTNODESTMT_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#અન્તર્ભાવ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_ગાંઠ_Stmt VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // Procedural statement
સુરક્ષિત:
    Ast_ગાંઠ_Stmt(VNType t, File_લીટી* fl)
        : Ast_ગાંઠ{t, fl} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Stmt;
    // METHODS
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
};
class Ast_ગાંઠ_Assign VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
સુરક્ષિત:
    Ast_ગાંઠ_Assign(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp,
                  Ast_ગાંઠ* timingControlp = nullptr)
        : Ast_ગાંઠ_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Assign;
    // Clone single node, just get same type back.
    શૂન્ય dump(std::ostream& str) અચલ override;
    આભાસી Ast_ગાંઠ_Assign* cloneType(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) = 0;
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    આભાસી bool cleanRhs() અચલ { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    string verilogKwd() અચલ override { પાછા "="; }
    bool isTimingControl() અચલ override { પાછા timingControlp(); }
};
class Ast_ગાંઠ_Block VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
સુરક્ષિત:
    Ast_ગાંઠ_Block(VNType t, File_લીટી* fl, અચલ string& name)
        : Ast_ગાંઠ_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Block;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Block name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    bool unnamed() અચલ { પાછા m_unnamed; }
};
class Ast_ગાંઠ_Cover_Or_Assert VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    અચલ VAssertType m_userType;  // Assertion/cover type for user enable/disable
    અચલ VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_immediate = false;  // Immediate assert (may differ from userType being immediate)

જાહેર:
    Ast_ગાંઠ_Cover_Or_Assert(VNType t, File_લીટી* fl, Ast_ગાંઠ* propp, Ast_ગાંઠ* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         અચલ string& name = "")
        : Ast_ગાંઠ_Stmt{t, fl}
        , m_name{name}
        , m_userType{userType}
        , m_directive{directive} {
        this->propp(propp);
        addPasssp(passsp);
        m_immediate = m_userType.containsAny(VAssertType::SIMPLE_IMMEDIATE
                                             | VAssertType::OBSERVED_DEFERRED_IMMEDIATE
                                             | VAssertType::FINAL_DEFERRED_IMMEDIATE)
                      || m_userType == VAssertType::INTERNAL;
    }
    ASTGEN_MEMBERS_Ast_ગાંઠ_Cover_Or_Assert;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા samep->name() == name(); }
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string type() { પાછા ""; }
    VAssertType userType() અચલ VL_MT_SAFE { પાછા m_userType; }
    VAssertDirectiveType directive() અચલ { પાછા m_directive; }
    bool immediate() અચલ { પાછા m_immediate; }
    શૂન્ય immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() અચલ VL_MT_STABLE { પાછા m_senFromAlways; }
    શૂન્ય senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_ગાંઠ_Foreach VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
જાહેર:
    Ast_ગાંઠ_Foreach(VNType t, File_લીટી* fl, AstForeachHeader* headerp, Ast_ગાંઠ* bodyp)
        : Ast_ગાંઠ_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_ગાંઠ_Foreach;
    bool isGateOptimizable() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class Ast_ગાંઠ_If VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
સુરક્ષિત:
    Ast_ગાંઠ_If(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ* thensp, Ast_ગાંઠ* elsesp)
        : Ast_ગાંઠ_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_If;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isGateDedupable() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    શૂન્ય branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() અચલ { પાછા m_branchPred; }
    શૂન્ય isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() અચલ { પાછા m_isBoundsCheck; }
};
class Ast_ગાંઠ_Read_Write_Mem VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    અચલ bool m_isHex;  // readmemh, not readmemb
જાહેર:
    Ast_ગાંઠ_Read_Write_Mem(VNType t, File_લીટી* fl, bool hex, Ast_ગાંઠ_પદાવલિ* filenamep,
                        Ast_ગાંઠ_પદાવલિ* memp, Ast_ગાંઠ_પદાવલિ* lsbp, Ast_ગાંઠ_પદાવલિ* msbp)
        : Ast_ગાંઠ_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_ગાંઠ_Read_Write_Mem;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા isHex() == VN_DBG_AS(samep, ગાંઠ_Read_Write_Mem)->isHex();
    }
    bool isHex() અચલ { પાછા m_isHex; }
    આભાસી અચલ અક્ષર* cFuncPrefixp() અચલ = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : જાહેર Ast_ગાંઠ {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
જાહેર:
    AstCaseItem(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condsp, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() + INSTR_ગણતરી_BRANCH; }
    bool isDefault() અચલ { પાછા condsp() == nullptr; }
};

class AstForeachHeader final : જાહેર Ast_ગાંઠ {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
જાહેર:
    AstForeachHeader(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

જાહેર:
    AstAssertCtl(File_લીટી* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_ગાંઠ_પદાવલિ* levelp = nullptr, Ast_ગાંઠ_પદાવલિ* itemsp = nullptr);
    AstAssertCtl(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* controlTypep, Ast_ગાંઠ_પદાવલિ* assertTypesp = nullptr,
                 Ast_ગાંઠ_પદાવલિ* directiveTypep = nullptr, Ast_ગાંઠ_પદાવલિ* levelp = nullptr,
                 Ast_ગાંઠ_પદાવલિ* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() અચલ override { પાછા m_ctlType.ascii(); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    VAssertCtlType ctlType() અચલ { પાછા m_ctlType; }
    શૂન્ય ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() અચલ { પાછા m_assertTypes; }
    શૂન્ય assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() અચલ { પાછા m_directiveTypes; }
    શૂન્ય directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
};
class AstBreak final : જાહેર Ast_ગાંઠ_Stmt {
જાહેર:
    explicit AstBreak(File_લીટી* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() અચલ override { પાછા "break"; }
    bool isBrancher() અચલ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : જાહેર Ast_ગાંઠ_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
જાહેર:
    AstCAwait(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_Sen_વૃક્ષ* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isTimingControl() અચલ override { પાછા true; }
    Ast_Sen_વૃક્ષ* sentreep() અચલ { પાછા m_sentreep; }
    શૂન્ય clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : જાહેર Ast_ગાંઠ_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
જાહેર:
    AstCLocalScope(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : જાહેર Ast_ગાંઠ_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
જાહેર:
    AstCReturn(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstCStmt final : જાહેર Ast_ગાંઠ_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    અચલ VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    સ્થિર AstCStmt* profExecSection(File_લીટી* flp, અચલ std::string& section, bool ધકેલો) {
        // Compute the label
        std::string label;
        જો (v3Global.opt.hierChild()) {
            label += v3Global.opt.ટોચ_Module() + ":";
        } નહીંતર જો (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ધકેલો ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        જો (ધકેલો) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ધકેલો ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        પાછા new AstCStmt{flp, ધકેલો ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

જાહેર:
    explicit AstCStmt(File_લીટી* fl, અચલ std::string& text = "",
                      અચલ VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        જો (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    VCStmtType stmtType() અચલ { પાછા m_stmtType; }
    // Add some text, or a node to this statement
    શૂન્ય add(અચલ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    શૂન્ય add(Ast_ગાંઠ* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    સ્થિર AstCStmt* prof_Exec_Section_ધકેલો(File_લીટી* flp, અચલ std::string& section) {
        પાછા profExecSection(flp, section, true);
    }
    સ્થિર AstCStmt* prof_Exec_Section_કાઢો(File_લીટી* flp, અચલ std::string& section) {
        પાછા profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : જાહેર Ast_ગાંઠ_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    અચલ bool m_fromDollarC;  // Is from source '$c', emit decoration
જાહેર:
    explicit AstCStmtUser(File_લીટી* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    bool fromDollarC() અચલ { પાછા m_fromDollarC; }
    // Add some text, or a node to this statement
    શૂન્ય add(અચલ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    શૂન્ય add(Ast_ગાંઠ* nodep) { addNodesp(nodep); }
};
class AstCase final : જાહેર Ast_ગાંઠ_Stmt {
    // Case statement
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstCaseItem]
    // @astgen op3 := notParallelp : List[AstNode] // assertion code for non-full case's
    VCaseType m_caseType;  // 0=case, 1=casex, 2=casez
    bool m_fullPragma = false;  // Synthesis full_case
    bool m_parallelPragma = false;  // Synthesis parallel_case
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
જાહેર:
    AstCase(File_લીટી* fl, VCaseType caseType, Ast_ગાંઠ_પદાવલિ* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    string verilogKwd() અચલ override { પાછા casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() અચલ { પાછા m_caseType == VCaseType::CT_CASEX; }
    bool casez() અચલ { પાછા m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() અચલ { પાછા m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() અચલ { પાછા m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() અચલ { પાછા m_caseType == VCaseType::CT_CASE; }
    શૂન્ય caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    શૂન્ય caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() અચલ { પાછા m_fullPragma; }
    શૂન્ય fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() અચલ { પાછા m_parallelPragma; }
    શૂન્ય parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() અચલ { પાછા m_uniquePragma; }
    શૂન્ય uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() અચલ { પાછા m_unique0Pragma; }
    શૂન્ય unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() અચલ { પાછા m_priorityPragma; }
    શૂન્ય priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() અચલ;
};
class AstComment final : જાહેર Ast_ગાંઠ_Stmt {
    // Some comment to put into the output stream
    અચલ string m_name;  // Text of comment
    અચલ bool m_showAt;  // Show "at <fileline>"
જાહેર:
    AstComment(File_લીટી* fl, અચલ string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Text
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા true; }  // Ignore name in comments
    આભાસી bool showAt() અચલ { પાછા m_showAt; }
};
class AstConstraintBefore final : જાહેર Ast_ગાંઠ_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
જાહેર:
    AstConstraintBefore(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhssp, Ast_ગાંઠ_પદાવલિ* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class Ast_Constraint_પદાવલિ final : જાહેર Ast_ગાંઠ_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
જાહેર:
    Ast_Constraint_પદાવલિ(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_Constraint_પદાવલિ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_પદાવલિ;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isDisableSoft() અચલ { પાછા m_isDisableSoft; }
    શૂન્ય isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() અચલ { પાછા m_isSoft; }
    શૂન્ય isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : જાહેર Ast_ગાંઠ_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
જાહેર:
    AstConstraintUnique(File_લીટી* fl, Ast_ગાંઠ* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstContinue final : જાહેર Ast_ગાંઠ_Stmt {
જાહેર:
    explicit AstContinue(File_લીટી* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() અચલ override { પાછા "continue"; }
    bool isBrancher() અચલ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : જાહેર Ast_ગાંઠ_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

જાહેર:
    AstCoverInc(File_લીટી* fl, Ast_ગાંઠ_Cover_Decl* declp, અચલ bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 1 + 2 * INSTR_ગણતરી_LD; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstCoverInc* અચલ otherp = VN_DBG_AS(samep, CoverInc);
        પાછા declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isPure() override { પાછા false; }
    Ast_ગાંઠ_Cover_Decl* declp() અચલ { પાછા m_declp; }  // Where defined
    શૂન્ય declp(Ast_ગાંઠ_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() અચલ { પાછા m_isDuplicate; }
};
class AstDeassign final : જાહેર Ast_ગાંઠ_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
જાહેર:
    AstDeassign(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : જાહેર Ast_ગાંઠ_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    અચલ bool m_isCycleDelay;  // True if it is a cycle delay
જાહેર:
    AstDelay(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isTimingControl() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    bool isCycleDelay() અચલ { પાછા m_isCycleDelay; }
    bool isRangeDelay() અચલ { પાછા m_isCycleDelay && rhsp(); }
    bool isUnbounded() અચલ { પાછા isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    શૂન્ય fallDelay(Ast_ગાંઠ_પદાવલિ* અચલ fallDelayp) { rhsp(fallDelayp); }
    Ast_ગાંઠ_પદાવલિ* fallDelay() અચલ { પાછા m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
જાહેર:
    AstDisable(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    અચલ અક્ષર* broken() અચલ override;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય targetp(Ast_ગાંઠ* nodep) { m_targetp = nodep; }
    Ast_ગાંઠ* targetp() અચલ { પાછા m_targetp; }
    bool isBrancher() અચલ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : જાહેર Ast_ગાંઠ_Stmt {
    // A "disable fork" statement
જાહેર:
    explicit AstDisableFork(File_લીટી* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() અચલ override { પાછા "disable fork"; }
};
class AstDisplay final : જાહેર Ast_ગાંઠ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

જાહેર:
    AstDisplay(File_લીટી* fl, VDisplayType dispType, અચલ string& text, Ast_ગાંઠ_પદાવલિ* filep,
               Ast_ગાંઠ_પદાવલિ* exprsp, અક્ષર missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_લીટી* fl, VDisplayType dispType, Ast_ગાંઠ_પદાવલિ* filep, Ast_ગાંઠ_પદાવલિ* exprsp,
               અક્ષર missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* અચલ newp
            = new AstSFormatF{fl, AstSFormatF::પદાવલિ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!fmtp());
        પાછા nullptr;
    }
    string verilogKwd() અચલ override {
        પાછા (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: $display makes output
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    VDisplayType displayType() અચલ { પાછા m_displayType; }
    શૂન્ય displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() અચલ { પાછા displayType().addNewline(); }
};
class AstDumpCtl final : જાહેર Ast_ગાંઠ_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    અચલ VDumpCtlType m_ctlType;  // Type of operation
જાહેર:
    AstDumpCtl(File_લીટી* fl, VDumpCtlType ctlType, Ast_ગાંઠ_પદાવલિ* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() અચલ override { પાછા ctlType().ascii(); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    આભાસી bool cleanOut() અચલ { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    VDumpCtlType ctlType() અચલ { પાછા m_ctlType; }
};
class AstEventControl final : જાહેર Ast_ગાંઠ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
જાહેર:
    AstEventControl(File_લીટી* fl, Ast_Sen_વૃક્ષ* sentreep, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() અચલ override { પાછા "@(%l) %r"; }
    bool isTimingControl() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
};
class AstExecGraph final : જાહેર Ast_ગાંઠ_Stmt {
    // For parallel execution, this node contains a dependency graph. Each
    // vertex in the graph is an ExecMTask, which includes a function that
    // holds the sequential body of the mtask.
    //
    // The location where AstExecGraph appears as a procedural statement is
    // where the parallel graph will be executed. Execution proceeds after
    // the AstExecGraph when all threads have joined.
    //
    // The calls to the MTask functions in 'callsp' are in a topological
    // order, so they represent a valid sequential execution of the graph for
    // code analysis purposes. The emitted code passes a description of the
    // graph to the run-time library, which executes the graph.
    // @astgen op1 := callsp : List[AstCCall]  // Calls to MTask bodies
    // Trigger masks zipped with callsp, populated by V3ExecGraph::implement
    // @astgen op2 := maskps : List[AstVarRef]
    // @astgen op3 := triggerp : AstVarRef  // Trigger vector this graph is sensitive to
    // Static cost of the logic each trigger guards, populated by V3ExecGraph::implement
    // @astgen op4 := triggerCostsp : Optional[AstVarRef]
    // @astgen ptr := m_runfuncp : Optional[AstCFunc]  // Statically scheduled dispatch function
    V3Graph* અચલ m_depGraphp;  // contains ExecMTask vertices
    અચલ string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

જાહેર:
    AstExecGraph(File_લીટી* fl, અચલ string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    શૂન્ય cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    અચલ અક્ષર* broken() અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    V3Graph* depGraphp() { પાછા m_depGraphp; }
    અચલ V3Graph* depGraphp() અચલ { પાછા m_depGraphp; }
    AstCFunc* runfuncp() અચલ { પાછા m_runfuncp; }
    શૂન્ય runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : જાહેર Ast_ગાંઠ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
જાહેર:
    AstFClose(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() અચલ override { પાછા "$fclose"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstFFlush final : જાહેર Ast_ગાંઠ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
જાહેર:
    AstFFlush(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() અચલ override { પાછા "$fflush"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstFinish final : જાહેર Ast_ગાંઠ_Stmt {
જાહેર:
    explicit AstFinish(File_લીટી* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: $display makes output
    bool isUnlikely() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }  // Rarely executes
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા fileline() == samep->fileline(); }
};
class AstFinishFork final : જાહેર Ast_ગાંઠ_Stmt {
    // $finish in fork
જાહેર:
    explicit AstFinishFork(File_લીટી* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: $display makes output
    bool isUnlikely() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }  // Rarely executes
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા fileline() == samep->fileline(); }
};
class AstFireEvent final : જાહેર Ast_ગાંઠ_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    અચલ bool m_delayed;  // Delayed (->>) vs non-delayed (->)
જાહેર:
    AstFireEvent(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isDelayed() અચલ { પાછા m_delayed; }
};
class AstInitialAutomaticStmt final : જાહેર Ast_ગાંઠ_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
જાહેર:
    AstInitialAutomaticStmt(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool isPure() override { પાછા true; }
};
class AstInitialStaticStmt final : જાહેર Ast_ગાંઠ_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
જાહેર:
    AstInitialStaticStmt(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool isPure() override { પાછા true; }
};
class AstJumpBlock final : જાહેર Ast_ગાંઠ_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
જાહેર:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    bool isPure() override;

ખાનગી:
    bool getPurityRecurse() અચલ;
};
class AstJumpGo final : જાહેર Ast_ગાંઠ_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
જાહેર:
    AstJumpGo(File_લીટી* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    અચલ અક્ષર* broken() અચલ override;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isBrancher() અચલ override {
        પાછા true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() અચલ { પાછા m_blockp; }
};
class AstLoop final : જાહેર Ast_ગાંઠ_Stmt {
    // An inifinite loop, used to model all source level procedural loops.
    // Executes as:
    //  while (true) {
    //      stmtsp;
    //      // <- 'continue' inside 'stmtsp goes here
    //      contsp;
    //  }
    // 'contsp' is moved into 'stmtsp' in LinkJump when 'continue' statements are resovled.
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := contsp : List[AstNode] // Empty after LinkJump
    VOptionBool m_unroll;  // Full, none, or default unrolling
જાહેર:
    explicit AstLoop(File_લીટી* fl, Ast_ગાંઠ* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* thatp) અચલ override {
        પાછા m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    // ACCESSORS
    VOptionBool unroll() અચલ { પાછા m_unroll; }
    શૂન્ય unroll(અચલ VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : જાહેર Ast_ગાંઠ_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
જાહેર:
    AstLoopTest(File_લીટી* fl, AstLoop* loopp, Ast_ગાંઠ_પદાવલિ* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    અચલ અક્ષર* broken() અચલ override;
    શૂન્ય dump(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isBrancher() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    // ACCESSORS
    AstLoop* loopp() અચલ { પાછા m_loopp; }
};
class AstMonitorOff final : જાહેર Ast_ગાંઠ_Stmt {
    અચલ bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

જાહેર:
    AstMonitorOff(File_લીટી* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string verilogKwd() અચલ override { પાછા m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() અચલ override { પાછા false; }  // Though deleted before opt
    bool isPredictOptimizable() અચલ override { પાછા false; }  // Though deleted before opt
    bool isPure() override { પાછા false; }  // Though deleted before opt
    bool isOutputter() override { પાછા true; }  // Though deleted before opt
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() અચલ { પાછા m_off; }
};
class AstPExprClause final : જાહેર Ast_ગાંઠ_Stmt {
    અચલ bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    અચલ bool m_vacuous;  // Pass is vacuous

જાહેર:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_લીટી* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool pass() અચલ { પાછા m_pass; }
    bool vacuous() અચલ { પાછા m_vacuous; }
};
class AstPrintTimeScale final : જાહેર Ast_ગાંઠ_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
જાહેર:
    explicit AstPrintTimeScale(File_લીટી* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string verilogKwd() અચલ override { પાછા "$printtimescale"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
};
class AstRSBreak final : જાહેર Ast_ગાંઠ_Stmt {
    // randsequence break
જાહેર:
    explicit AstRSBreak(File_લીટી* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() અચલ override { પાછા "break"; }
    bool isBrancher() અચલ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : જાહેર Ast_ગાંઠ_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
જાહેર:
    AstRSProd(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
};
class AstRSProdItem final : જાહેર Ast_ગાંઠ_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
જાહેર:
    AstRSProdItem(File_લીટી* fl, અચલ string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    AstRSProd* prodp() અચલ { પાછા m_prodp; }
    શૂન્ય prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : જાહેર Ast_ગાંઠ_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
જાહેર:
    AstRSProdList(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* weightp, Ast_ગાંઠ* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    bool randJoin() અચલ { પાછા m_randJoin; }
    શૂન્ય randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : જાહેર Ast_ગાંઠ_Stmt {
    // randsequence return
જાહેર:
    explicit AstRSReturn(File_લીટી* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() અચલ override { પાછા "return"; }
    bool isBrancher() અચલ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : જાહેર Ast_ગાંઠ_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
જાહેર:
    AstRSRule(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* weightp, AstRSProdList* prodlistsp, Ast_ગાંઠ* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
};
class AstRandCase final : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
જાહેર:
    AstRandCase(File_લીટી* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
};
class AstRandSequence final : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
જાહેર:
    AstRandSequence(File_લીટી* fl, અચલ string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Block name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string start() અચલ VL_MT_STABLE { પાછા m_start; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    AstRSProd* prodp() અચલ { પાછા m_prodp; }
    શૂન્ય prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : જાહેર Ast_ગાંઠ_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
જાહેર:
    AstRelease(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
જાહેર:
    AstRepeat(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* countp, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() અચલ override { પાછા false; }  // Not relevant - converted to FOR
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstReturn final : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
જાહેર:
    explicit AstReturn(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() અચલ override { પાછા "return"; }
    bool isBrancher() અચલ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : જાહેર Ast_ગાંઠ_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
જાહેર:
    AstSFormat(File_લીટી* fl, bool optionalFormat, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* exprsp,
               અક્ષર missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* અચલ newp
            = new AstSFormatF{fl, AstSFormatF::પદાવલિ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_લીટી* fl, AstSFormatF* fmtp, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!fmtp());
        પાછા nullptr;
    }
    string verilogKwd() અચલ override { પાછા "$sformat"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા true; }
    bool isPure() override { પાછા true; }
    bool isOutputter() override { પાછા false; }
    આભાસી bool cleanOut() અચલ { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstSetuphold final : જાહેર Ast_ગાંઠ_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
જાહેર:
    AstSetuphold(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* refevp, Ast_ગાંઠ_પદાવલિ* dataevp,
                 Ast_ગાંઠ_પદાવલિ* delrefp = nullptr, Ast_ગાંઠ_પદાવલિ* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class Ast_ઢગલો_Trace_T final : જાહેર Ast_ગાંઠ_Stmt {
    // $stacktrace used as task
જાહેર:
    explicit Ast_ઢગલો_Trace_T(File_લીટી* fl)
        : ASTGEN_SUPER_ઢગલો_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_ઢગલો_Trace_T;
    string verilogKwd() અચલ override { પાછા "$stacktrace"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class Ast_Stmt_પદાવલિ final : જાહેર Ast_ગાંઠ_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
જાહેર:
    Ast_Stmt_પદાવલિ(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_Stmt_પદાવલિ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_પદાવલિ;
    // cppcheck-suppress uselessOverride
    bool isPure() override { પાછા exprp()->isPure(); }
};
class AstStmtPragma final : જાહેર Ast_ગાંઠ_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
જાહેર:
    AstStmtPragma(File_લીટી* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : જાહેર Ast_ગાંઠ_Stmt {
    અચલ bool m_isFatal;  // $fatal not $stop
જાહેર:
    AstStop(File_લીટી* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: $display makes output
    bool isUnlikely() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }  // Rarely executes
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા fileline() == samep->fileline(); }
    string emitVerilog() અચલ { પાછા m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() અચલ { પાછા m_isFatal; }
};
class AstSystemT final : જાહેર Ast_ગાંઠ_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
જાહેર:
    AstSystemT(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() અચલ override { પાછા "$system"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstTimeFormat final : જાહેર Ast_ગાંઠ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
જાહેર:
    AstTimeFormat(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* unitsp, Ast_ગાંઠ_પદાવલિ* precisionp, Ast_ગાંઠ_પદાવલિ* suffixp,
                  Ast_ગાંઠ_પદાવલિ* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() અચલ override { પાછા "$timeformat"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
};
class AstTraceDecl final : જાહેર Ast_ગાંઠ_Stmt {
    // Trace point declaration
    // Separate from AstTraceInc; as a declaration can't be deleted
    // Parents:  {statement list}
    // Expression being traced - Moved to AstTraceInc by V3Trace
    // @astgen op1 := valuep : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_dtypeCallp: Optional[AstCCall] // Type init function call
    // @astgen ptr := m_dtypeDeclp: Optional[AstTraceDecl] // CCall TraceDecl which replaces this
    uint32_t m_code{std::numeric_limits<uint32_t>::max()};  // Trace identifier code
    uint32_t m_fidx{0};  // Trace function index
    અચલ string m_showname;  // Name of variable
    અચલ VNumRange m_bitRange;  // Property of var the trace details
    અચલ VNumRange m_arrayRange;  // Property of var the trace details
    અચલ VVarType m_varType;  // Type of variable (for localparam vs. param)
    અચલ VDirection m_declDirection;  // Declared direction input/output etc
    અચલ bool m_inDtypeFunc;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    પૂર્ણાંક m_codeInc{0};  // Code increment for type
જાહેર:
    AstTraceDecl(File_લીટી* fl, અચલ string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_ગાંઠ_પદાવલિ* valuep, અચલ VNumRange& bitRange, અચલ VNumRange& arrayRange,
                 AstCCall* અચલ dtypeCallp, અચલ bool inDtypeFunc)
        : ASTGEN_SUPER_TraceDecl(fl)
        , m_showname{showname}
        , m_bitRange{bitRange}
        , m_arrayRange{arrayRange}
        , m_varType{varp->varType()}
        , m_declDirection{varp->declDirection()}
        , m_inDtypeFunc{inDtypeFunc} {
        dtypeFrom(valuep);
        this->valuep(valuep);
        this->dtypeCallp(dtypeCallp);
    }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() અચલ override VL_MT_STABLE { પાછા m_showname; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા true; }
    string showname() અચલ { પાછા m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() અચલ { પાછા m_code; }
    શૂન્ય code(uint32_t code) { m_code = code; }
    bool codeAssigned() અચલ { પાછા m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() અચલ { પાછા m_fidx; }
    શૂન્ય fidx(uint32_t fidx) { m_fidx = fidx; }
    શૂન્ય codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() અચલ {
        જો (m_codeInc) { પાછા m_codeInc; }
        પાછા (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_શબ્દો()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    અચલ VNumRange& bitRange() અચલ { પાછા m_bitRange; }
    અચલ VNumRange& arrayRange() અચલ { પાછા m_arrayRange; }
    VVarType varType() અચલ { પાછા m_varType; }
    VDirection declDirection() અચલ { પાછા m_declDirection; }
    AstCCall* dtypeCallp() અચલ { પાછા m_dtypeCallp; }
    શૂન્ય dtypeCallp(AstCCall* અચલ callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() અચલ { પાછા m_dtypeDeclp; }
    શૂન્ય dtypeDeclp(AstTraceDecl* અચલ declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() અચલ { પાછા m_inDtypeFunc; }
};
class AstTraceInc final : જાહેર Ast_ગાંઠ_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    અચલ uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    અચલ VTraceType m_traceType;  // Is this a const/full/incremental dump

જાહેર:
    AstTraceInc(File_લીટી* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_વૃક્ષ(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 10 + 2 * INSTR_ગણતરી_LD; }
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isPure() override { પાછા false; }
    AstTraceDecl* declp() અચલ { પાછા m_declp; }
    VTraceType traceType() અચલ { પાછા m_traceType; }
    uint32_t baseCode() અચલ { પાછા m_baseCode; }
};
class Ast_Trace_કાઢો_Prefix final : જાહેર Ast_ગાંઠ_Stmt {
જાહેર:
    explicit Ast_Trace_કાઢો_Prefix(File_લીટી* fl)
        : ASTGEN_SUPER_Trace_કાઢો_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_કાઢો_Prefix;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા false; }
};
class Ast_Trace_ધકેલો_Prefix final : જાહેર Ast_ગાંઠ_Stmt {
    અચલ string m_prefix;  // Prefix to add to signal names
    અચલ VTracePrefixType m_prefixType;  // Type of prefix being pushed
    અચલ પૂર્ણાંક m_ડાબું;  // Array left index, or struct/union member count
    અચલ પૂર્ણાંક m_જમણું;  // Array right index
    અચલ bool m_quotedPrefix;  // Quote prefix name
જાહેર:
    Ast_Trace_ધકેલો_Prefix(File_લીટી* fl, અચલ string& prefix, VTracePrefixType prefixType,
                       પૂર્ણાંક ડાબું = 0, પૂર્ણાંક જમણું = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_ધકેલો_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_ડાબું{ડાબું}
        , m_જમણું{જમણું}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_ધકેલો_Prefix;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા false; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string prefix() અચલ { પાછા m_prefix; }
    VTracePrefixType prefixType() અચલ { પાછા m_prefixType; }
    પૂર્ણાંક ડાબું() અચલ { પાછા m_ડાબું; }
    પૂર્ણાંક જમણું() અચલ { પાછા m_જમણું; }
    bool quotedPrefix() અચલ { પાછા m_quotedPrefix; }
};
class AstWait final : જાહેર Ast_ગાંઠ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
જાહેર:
    AstWait(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() અચલ override { પાછા true; }
};
class AstWaitFork final : જાહેર Ast_ગાંઠ_Stmt {
    // A "wait fork" statement
જાહેર:
    explicit AstWaitFork(File_લીટી* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() અચલ override { પાછા true; }
    string verilogKwd() અચલ override { પાછા "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : જાહેર Ast_ગાંઠ_Assign {
જાહેર:
    AstAssign(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp,
              Ast_ગાંઠ* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_ગાંઠ_Assign* cloneType(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) override {
        Ast_ગાંઠ* અચલ controlp = timingControlp() ? timingControlp()->clone_વૃક્ષ(false) : nullptr;
        પાછા new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : જાહેર Ast_ગાંઠ_Assign {
    // Compound assignments (+=, -=, *=, ...)
જાહેર:
    ગણના operation : uint8_t {
        Add,
        And,
        Div,
        ModDiv,
        Mul,
        Or,
        ShiftL,
        ShiftR,
        ShiftRS,
        Sub,
        Xor,
    };

ખાનગી:
    operation m_operation;

જાહેર:
    AstAssignCompound(AstAssignCompound::operation operation, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp,
                      Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_ગાંઠ_Assign* cloneType(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) override {
        Ast_ગાંઠ* અચલ controlp = timingControlp() ? timingControlp()->clone_વૃક્ષ(false) : nullptr;
        પાછા new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { પાછા m_operation; }
};
class AstAssignCont final : જાહેર Ast_ગાંઠ_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
જાહેર:
    AstAssignCont(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp,
                  Ast_ગાંઠ* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_ગાંઠ_Assign* cloneType(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) override {
        Ast_ગાંઠ* અચલ controlp = timingControlp() ? timingControlp()->clone_વૃક્ષ(false) : nullptr;
        પાછા new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : જાહેર Ast_ગાંઠ_Assign {
જાહેર:
    AstAssignDly(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp,
                 Ast_ગાંઠ* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_ગાંઠ_Assign* cloneType(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) override {
        Ast_ગાંઠ* અચલ controlp = timingControlp() ? timingControlp()->clone_વૃક્ષ(false) : nullptr;
        પાછા new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() અચલ override { પાછા false; }
    string verilogKwd() અચલ override { પાછા "<="; }
};
class AstAssignForce final : જાહેર Ast_ગાંઠ_Assign {
    // Procedural 'force' statement
જાહેર:
    AstAssignForce(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_ગાંઠ_Assign* cloneType(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) override {
        પાછા new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : જાહેર Ast_ગાંઠ_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
જાહેર:
    AstAssignW(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp,
               Ast_ગાંઠ* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_ગાંઠ_Assign* cloneType(Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp) override {
        Ast_ગાંઠ* અચલ controlp = timingControlp() ? timingControlp()->clone_વૃક્ષ(false) : nullptr;
        પાછા new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() અચલ override {
        પાછા timingControlp() || lhsp()->exists([](અચલ Ast_ગાંઠ_Var_Ref* refp) {
            પાછા refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() અચલ;
};

// === AstNodeBlock ===
class AstBegin final : જાહેર Ast_ગાંઠ_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    અચલ bool m_implied : 1;  // Not inserted by user
જાહેર:
    AstBegin(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    શૂન્ય setNeedProcess() { m_needProcess = true; }
    bool needProcess() અચલ { પાછા m_needProcess; }
    bool implied() અચલ { પાછા m_implied; }
};
class AstFork final : જાહેર Ast_ગાંઠ_Block {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    અચલ VJoinType m_joinType;  // Join keyword type
    bool m_immediateStart = false;  // Fork starts before its parent blocks or exits

જાહેર:
    AstFork(File_લીટી* fl, VJoinType joinType, અચલ string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstFork* અચલ asamep = VN_DBG_AS(samep, Fork);
        પાછા joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() અચલ override { પાછા !joinType().joinNone(); }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    VJoinType joinType() અચલ { પાછા m_joinType; }
    bool immediateStart() અચલ { પાછા m_immediateStart; }
    શૂન્ય immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : જાહેર Ast_ગાંઠ_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

જાહેર:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_લીટી* fl, Ast_ગાંઠ* propp, Ast_ગાંઠ* passsp, Ast_ગાંઠ* failsp, VAssertType type,
              VAssertDirectiveType directive, અચલ string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() અચલ override {
        પાછા directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : જાહેર Ast_ગાંઠ_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
જાહેર:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_લીટી* fl, Ast_ગાંઠ* propp, Ast_ગાંઠ* passsp, Ast_ગાંઠ* failsp,
                       અચલ string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() અચલ override { પાછા "assert"; }
};
class AstCover final : જાહેર Ast_ગાંઠ_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
જાહેર:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_લીટી* fl, Ast_ગાંઠ* propp, Ast_ગાંઠ* stmtsp, VAssertType type,
             અચલ string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() અચલ override { પાછા "cover"; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isCoverSeq() અચલ { પાછા m_isCoverSeq; }
    શૂન્ય isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() અચલ { પાછા m_isSeqEvent; }
    શૂન્ય isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : જાહેર Ast_ગાંઠ_Cover_Or_Assert {
જાહેર:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_લીટી* fl, Ast_ગાંઠ* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() અચલ override { પાછા "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : જાહેર Ast_ગાંઠ_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
જાહેર:
    AstConstraintForeach(File_લીટી* fl, AstForeachHeader* headerp, Ast_ગાંઠ* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() અચલ { પાછા m_soft; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstForeach final : જાહેર Ast_ગાંઠ_Foreach {
જાહેર:
    AstForeach(File_લીટી* fl, AstForeachHeader* headerp, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : જાહેર Ast_ગાંઠ_If {
જાહેર:
    AstConstraintIf(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ* thensp, Ast_ગાંઠ* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : જાહેર Ast_ગાંઠ_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
જાહેર:
    AstIf(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ* thensp = nullptr, Ast_ગાંઠ* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() અચલ { પાછા m_uniquePragma; }
    શૂન્ય uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() અચલ { પાછા m_unique0Pragma; }
    શૂન્ય unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() અચલ { પાછા m_priorityPragma; }
    શૂન્ય priorityPragma(bool flag) { m_priorityPragma = flag; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : જાહેર Ast_ગાંઠ_Read_Write_Mem {
જાહેર:
    AstReadMem(File_લીટી* fl, bool hex, Ast_ગાંઠ_પદાવલિ* filenamep, Ast_ગાંઠ_પદાવલિ* memp,
               Ast_ગાંઠ_પદાવલિ* lsbp, Ast_ગાંઠ_પદાવલિ* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() અચલ override { પાછા (isHex() ? "$readmemh" : "$readmemb"); }
    અચલ અક્ષર* cFuncPrefixp() અચલ override { પાછા "VL_READMEM_"; }
};
class AstWriteMem final : જાહેર Ast_ગાંઠ_Read_Write_Mem {
જાહેર:
    AstWriteMem(File_લીટી* fl, bool hex, Ast_ગાંઠ_પદાવલિ* filenamep, Ast_ગાંઠ_પદાવલિ* memp,
                Ast_ગાંઠ_પદાવલિ* lsbp, Ast_ગાંઠ_પદાવલિ* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() અચલ override { પાછા (isHex() ? "$writememh" : "$writememb"); }
    અચલ અક્ષર* cFuncPrefixp() અચલ override { પાછા "VL_WRITEMEM_"; }
};

#endif  // Guard
