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
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_گره_Stmt VL_NOT_FINAL : عمومی Ast_گره {
    // Procedural statement
محافظتشده:
    Ast_گره_Stmt(VNType t, File_سطر* fl)
        : Ast_گره{t, fl} {}

عمومی:
    ASTGEN_MEMBERS_Ast_گره_Stmt;
    // METHODS
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
};
class Ast_گره_Assign VL_NOT_FINAL : عمومی Ast_گره_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
محافظتشده:
    Ast_گره_Assign(VNType t, File_سطر* fl, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp,
                  Ast_گره* timingControlp = nullptr)
        : Ast_گره_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

عمومی:
    ASTGEN_MEMBERS_Ast_گره_Assign;
    // Clone single node, just get same type back.
    تهی dump(std::ostream& str) ثابت override;
    مجازی Ast_گره_Assign* cloneType(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) = 0;
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    مجازی bool cleanRhs() ثابت { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت widthInstrs(); }
    bool same_گره(ثابت Ast_گره*) ثابت override { بازگشت true; }
    string verilogKwd() ثابت override { بازگشت "="; }
    bool isTimingControl() ثابت override { بازگشت timingControlp(); }
};
class Ast_گره_Block VL_NOT_FINAL : عمومی Ast_گره_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
محافظتشده:
    Ast_گره_Block(VNType t, File_سطر* fl, ثابت string& name)
        : Ast_گره_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

عمومی:
    ASTGEN_MEMBERS_Ast_گره_Block;
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }  // * = Block name
    تهی name(ثابت string& name) override { m_name = name; }
    bool unnamed() ثابت { بازگشت m_unnamed; }
};
class Ast_گره_Cover_Or_Assert VL_NOT_FINAL : عمومی Ast_گره_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    ثابت VAssertType m_userType;  // Assertion/cover type for user enable/disable
    ثابت VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_immediate = false;  // Immediate assert (may differ from userType being immediate)

عمومی:
    Ast_گره_Cover_Or_Assert(VNType t, File_سطر* fl, Ast_گره* propp, Ast_گره* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ثابت string& name = "")
        : Ast_گره_Stmt{t, fl}
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
    ASTGEN_MEMBERS_Ast_گره_Cover_Or_Assert;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }  // * = Var name
    bool same_گره(ثابت Ast_گره* samep) ثابت override { بازگشت samep->name() == name(); }
    تهی name(ثابت string& name) override { m_name = name; }
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
    string type() { بازگشت ""; }
    VAssertType userType() ثابت VL_MT_SAFE { بازگشت m_userType; }
    VAssertDirectiveType directive() ثابت { بازگشت m_directive; }
    bool immediate() ثابت { بازگشت m_immediate; }
    تهی immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() ثابت VL_MT_STABLE { بازگشت m_senFromAlways; }
    تهی senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_گره_Foreach VL_NOT_FINAL : عمومی Ast_گره_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
عمومی:
    Ast_گره_Foreach(VNType t, File_سطر* fl, AstForeachHeader* headerp, Ast_گره* bodyp)
        : Ast_گره_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_گره_Foreach;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class Ast_گره_If VL_NOT_FINAL : عمومی Ast_گره_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
محافظتشده:
    Ast_گره_If(VNType t, File_سطر* fl, Ast_گره_عبارت* condp, Ast_گره* thensp, Ast_گره* elsesp)
        : Ast_گره_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

عمومی:
    ASTGEN_MEMBERS_Ast_گره_If;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isGateDedupable() ثابت override { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
    تهی branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ثابت { بازگشت m_branchPred; }
    تهی isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ثابت { بازگشت m_isBoundsCheck; }
};
class Ast_گره_Read_Write_Mem VL_NOT_FINAL : عمومی Ast_گره_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ثابت bool m_isHex;  // readmemh, not readmemb
عمومی:
    Ast_گره_Read_Write_Mem(VNType t, File_سطر* fl, bool hex, Ast_گره_عبارت* filenamep,
                        Ast_گره_عبارت* memp, Ast_گره_عبارت* lsbp, Ast_گره_عبارت* msbp)
        : Ast_گره_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_گره_Read_Write_Mem;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isUnlikely() ثابت override { بازگشت true; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        بازگشت isHex() == VN_DBG_AS(samep, گره_Read_Write_Mem)->isHex();
    }
    bool isHex() ثابت { بازگشت m_isHex; }
    مجازی ثابت نویسه* cFuncPrefixp() ثابت = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : عمومی Ast_گره {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
عمومی:
    AstCaseItem(File_سطر* fl, Ast_گره_عبارت* condsp, Ast_گره* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    صحیح instr_شمار() ثابت override { بازگشت widthInstrs() + INSTR_شمار_BRANCH; }
    bool isDefault() ثابت { بازگشت condsp() == nullptr; }
};

class AstForeachHeader final : عمومی Ast_گره {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
عمومی:
    AstForeachHeader(File_سطر* fl, Ast_گره_عبارت* fromp, Ast_گره* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : عمومی Ast_گره_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

عمومی:
    AstAssertCtl(File_سطر* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_گره_عبارت* levelp = nullptr, Ast_گره_عبارت* itemsp = nullptr);
    AstAssertCtl(File_سطر* fl, Ast_گره_عبارت* controlTypep, Ast_گره_عبارت* assertTypesp = nullptr,
                 Ast_گره_عبارت* directiveTypep = nullptr, Ast_گره_عبارت* levelp = nullptr,
                 Ast_گره_عبارت* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ثابت override { بازگشت m_ctlType.ascii(); }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    VAssertCtlType ctlType() ثابت { بازگشت m_ctlType; }
    تهی ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ثابت { بازگشت m_assertTypes; }
    تهی assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ثابت { بازگشت m_directiveTypes; }
    تهی directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstBreak final : عمومی Ast_گره_Stmt {
عمومی:
    explicit AstBreak(File_سطر* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ثابت override { بازگشت "break"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : عمومی Ast_گره_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
عمومی:
    AstCAwait(File_سطر* fl, Ast_گره_عبارت* exprp, Ast_Sen_درخت* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isTimingControl() ثابت override { بازگشت true; }
    Ast_Sen_درخت* sentreep() ثابت { بازگشت m_sentreep; }
    تهی clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : عمومی Ast_گره_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
عمومی:
    AstCLocalScope(File_سطر* fl, Ast_گره* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : عمومی Ast_گره_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
عمومی:
    AstCReturn(File_سطر* fl, Ast_گره_عبارت* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    صحیح instr_شمار() ثابت override { بازگشت widthInstrs(); }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class AstCStmt final : عمومی Ast_گره_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ثابت VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    ایستا AstCStmt* profExecSection(File_سطر* flp, ثابت std::string& section, bool افزودن) {
        // Compute the label
        std::string label;
        اگر (v3Global.opt.hierChild()) {
            label += v3Global.opt.بالاترین_Module() + ":";
        } وگرنه اگر (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += افزودن ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        اگر (افزودن) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += افزودن ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        بازگشت new AstCStmt{flp, افزودن ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

عمومی:
    explicit AstCStmt(File_سطر* fl, ثابت std::string& text = "",
                      ثابت VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        اگر (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool same_گره(ثابت Ast_گره*) ثابت override { بازگشت true; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    VCStmtType stmtType() ثابت { بازگشت m_stmtType; }
    // Add some text, or a node to this statement
    تهی add(ثابت std::string& text) { addNodesp(new AstText{fileline(), text}); }
    تهی add(Ast_گره* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    ایستا AstCStmt* prof_Exec_Section_افزودن(File_سطر* flp, ثابت std::string& section) {
        بازگشت profExecSection(flp, section, true);
    }
    ایستا AstCStmt* prof_Exec_Section_برداشت(File_سطر* flp, ثابت std::string& section) {
        بازگشت profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : عمومی Ast_گره_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ثابت bool m_fromDollarC;  // Is from source '$c', emit decoration
عمومی:
    explicit AstCStmtUser(File_سطر* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool same_گره(ثابت Ast_گره*) ثابت override { بازگشت true; }
    bool fromDollarC() ثابت { بازگشت m_fromDollarC; }
    // Add some text, or a node to this statement
    تهی add(ثابت std::string& text) { addNodesp(new AstText{fileline(), text}); }
    تهی add(Ast_گره* nodep) { addNodesp(nodep); }
};
class AstCase final : عمومی Ast_گره_Stmt {
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
عمومی:
    AstCase(File_سطر* fl, VCaseType caseType, Ast_گره_عبارت* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    string verilogKwd() ثابت override { بازگشت casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        بازگشت m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ثابت { بازگشت m_caseType == VCaseType::CT_CASEX; }
    bool casez() ثابت { بازگشت m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ثابت { بازگشت m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ثابت { بازگشت m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ثابت { بازگشت m_caseType == VCaseType::CT_CASE; }
    تهی caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    تهی caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() ثابت { بازگشت m_fullPragma; }
    تهی fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() ثابت { بازگشت m_parallelPragma; }
    تهی parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ثابت { بازگشت m_uniquePragma; }
    تهی uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ثابت { بازگشت m_unique0Pragma; }
    تهی unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ثابت { بازگشت m_priorityPragma; }
    تهی priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() ثابت;
};
class AstComment final : عمومی Ast_گره_Stmt {
    // Some comment to put into the output stream
    ثابت string m_name;  // Text of comment
    ثابت bool m_showAt;  // Show "at <fileline>"
عمومی:
    AstComment(File_سطر* fl, ثابت string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }  // * = Text
    bool same_گره(ثابت Ast_گره* samep) ثابت override { بازگشت true; }  // Ignore name in comments
    مجازی bool showAt() ثابت { بازگشت m_showAt; }
};
class AstConstraintBefore final : عمومی Ast_گره_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
عمومی:
    AstConstraintBefore(File_سطر* fl, Ast_گره_عبارت* lhssp, Ast_گره_عبارت* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class Ast_Constraint_عبارت final : عمومی Ast_گره_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
عمومی:
    Ast_Constraint_عبارت(File_سطر* fl, Ast_گره_عبارت* exprp)
        : ASTGEN_SUPER_Constraint_عبارت(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_عبارت;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
    bool isDisableSoft() ثابت { بازگشت m_isDisableSoft; }
    تهی isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ثابت { بازگشت m_isSoft; }
    تهی isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : عمومی Ast_گره_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
عمومی:
    AstConstraintUnique(File_سطر* fl, Ast_گره* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class AstContinue final : عمومی Ast_گره_Stmt {
عمومی:
    explicit AstContinue(File_سطر* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ثابت override { بازگشت "continue"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : عمومی Ast_گره_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

عمومی:
    AstCoverInc(File_سطر* fl, Ast_گره_Cover_Decl* declp, ثابت bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_شمار() ثابت override { بازگشت 1 + 2 * INSTR_شمار_LD; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstCoverInc* ثابت otherp = VN_DBG_AS(samep, CoverInc);
        بازگشت declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isPure() override { بازگشت false; }
    Ast_گره_Cover_Decl* declp() ثابت { بازگشت m_declp; }  // Where defined
    تهی declp(Ast_گره_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ثابت { بازگشت m_isDuplicate; }
};
class AstDeassign final : عمومی Ast_گره_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
عمومی:
    AstDeassign(File_سطر* fl, Ast_گره_عبارت* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : عمومی Ast_گره_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ثابت bool m_isCycleDelay;  // True if it is a cycle delay
عمومی:
    AstDelay(File_سطر* fl, Ast_گره_عبارت* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isTimingControl() ثابت override { بازگشت true; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
    تهی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ثابت { بازگشت m_timeunit; }
    bool isCycleDelay() ثابت { بازگشت m_isCycleDelay; }
    bool isRangeDelay() ثابت { بازگشت m_isCycleDelay && rhsp(); }
    bool isUnbounded() ثابت { بازگشت isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    تهی fallDelay(Ast_گره_عبارت* ثابت fallDelayp) { rhsp(fallDelayp); }
    Ast_گره_عبارت* fallDelay() ثابت { بازگشت m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : عمومی Ast_گره_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
عمومی:
    AstDisable(File_سطر* fl, Ast_گره_عبارت* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ثابت نویسه* broken() ثابت override;
    تهی dump(std::ostream& str) ثابت override;
    تهی targetp(Ast_گره* nodep) { m_targetp = nodep; }
    Ast_گره* targetp() ثابت { بازگشت m_targetp; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : عمومی Ast_گره_Stmt {
    // A "disable fork" statement
عمومی:
    explicit AstDisableFork(File_سطر* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ثابت override { بازگشت "disable fork"; }
};
class AstDisplay final : عمومی Ast_گره_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

عمومی:
    AstDisplay(File_سطر* fl, VDisplayType dispType, ثابت string& text, Ast_گره_عبارت* filep,
               Ast_گره_عبارت* exprsp, نویسه missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_سطر* fl, VDisplayType dispType, Ast_گره_عبارت* filep, Ast_گره_عبارت* exprsp,
               نویسه missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ثابت newp
            = new AstSFormatF{fl, AstSFormatF::عبارت_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!fmtp());
        بازگشت nullptr;
    }
    string verilogKwd() ثابت override {
        بازگشت (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { بازگشت true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { بازگشت true; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        بازگشت displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_PLI; }
    VDisplayType displayType() ثابت { بازگشت m_displayType; }
    تهی displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() ثابت { بازگشت displayType().addNewline(); }
};
class AstDumpCtl final : عمومی Ast_گره_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ثابت VDumpCtlType m_ctlType;  // Type of operation
عمومی:
    AstDumpCtl(File_سطر* fl, VDumpCtlType ctlType, Ast_گره_عبارت* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ثابت override { بازگشت ctlType().ascii(); }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    مجازی bool cleanOut() ثابت { بازگشت true; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    VDumpCtlType ctlType() ثابت { بازگشت m_ctlType; }
};
class AstEventControl final : عمومی Ast_گره_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
عمومی:
    AstEventControl(File_سطر* fl, Ast_Sen_درخت* sentreep, Ast_گره* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ثابت override { بازگشت "@(%l) %r"; }
    bool isTimingControl() ثابت override { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت 0; }
};
class AstExecGraph final : عمومی Ast_گره_Stmt {
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
    V3Graph* ثابت m_depGraphp;  // contains ExecMTask vertices
    ثابت string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

عمومی:
    AstExecGraph(File_سطر* fl, ثابت string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    تهی cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ثابت نویسه* broken() ثابت override;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    V3Graph* depGraphp() { بازگشت m_depGraphp; }
    ثابت V3Graph* depGraphp() ثابت { بازگشت m_depGraphp; }
    AstCFunc* runfuncp() ثابت { بازگشت m_runfuncp; }
    تهی runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : عمومی Ast_گره_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
عمومی:
    AstFClose(File_سطر* fl, Ast_گره_عبارت* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ثابت override { بازگشت "$fclose"; }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isUnlikely() ثابت override { بازگشت true; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class AstFFlush final : عمومی Ast_گره_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
عمومی:
    AstFFlush(File_سطر* fl, Ast_گره_عبارت* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ثابت override { بازگشت "$fflush"; }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isUnlikely() ثابت override { بازگشت true; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class AstFinish final : عمومی Ast_گره_Stmt {
عمومی:
    explicit AstFinish(File_سطر* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { بازگشت true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت 0; }  // Rarely executes
    bool same_گره(ثابت Ast_گره* samep) ثابت override { بازگشت fileline() == samep->fileline(); }
};
class AstFinishFork final : عمومی Ast_گره_Stmt {
    // $finish in fork
عمومی:
    explicit AstFinishFork(File_سطر* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { بازگشت true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت 0; }  // Rarely executes
    bool same_گره(ثابت Ast_گره* samep) ثابت override { بازگشت fileline() == samep->fileline(); }
};
class AstFireEvent final : عمومی Ast_گره_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ثابت bool m_delayed;  // Delayed (->>) vs non-delayed (->)
عمومی:
    AstFireEvent(File_سطر* fl, Ast_گره_عبارت* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isDelayed() ثابت { بازگشت m_delayed; }
};
class AstInitialAutomaticStmt final : عمومی Ast_گره_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
عمومی:
    AstInitialAutomaticStmt(File_سطر* fl, Ast_گره* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    صحیح instr_شمار() ثابت override { بازگشت 0; }
    bool isPure() override { بازگشت true; }
};
class AstInitialStaticStmt final : عمومی Ast_گره_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
عمومی:
    AstInitialStaticStmt(File_سطر* fl, Ast_گره* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    صحیح instr_شمار() ثابت override { بازگشت 0; }
    bool isPure() override { بازگشت true; }
};
class AstJumpBlock final : عمومی Ast_گره_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
عمومی:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_سطر* fl, Ast_گره* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    صحیح instr_شمار() ثابت override { بازگشت 0; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool isPure() override;

خصوصی:
    bool getPurityRecurse() ثابت;
};
class AstJumpGo final : عمومی Ast_گره_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
عمومی:
    AstJumpGo(File_سطر* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ثابت نویسه* broken() ثابت override;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        بازگشت blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isBrancher() ثابت override {
        بازگشت true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() ثابت { بازگشت m_blockp; }
};
class AstLoop final : عمومی Ast_گره_Stmt {
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
عمومی:
    explicit AstLoop(File_سطر* fl, Ast_گره* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool same_گره(ثابت Ast_گره* thatp) ثابت override {
        بازگشت m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    // ACCESSORS
    VOptionBool unroll() ثابت { بازگشت m_unroll; }
    تهی unroll(ثابت VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : عمومی Ast_گره_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
عمومی:
    AstLoopTest(File_سطر* fl, AstLoop* loopp, Ast_گره_عبارت* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ثابت نویسه* broken() ثابت override;
    تهی dump(std::ostream& str) ثابت override;
    bool same_گره(ثابت Ast_گره*) ثابت override { بازگشت true; }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isBrancher() ثابت override { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت 0; }
    // ACCESSORS
    AstLoop* loopp() ثابت { بازگشت m_loopp; }
};
class AstMonitorOff final : عمومی Ast_گره_Stmt {
    ثابت bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

عمومی:
    AstMonitorOff(File_سطر* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { بازگشت m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ثابت override { بازگشت false; }  // Though deleted before opt
    bool isPredictOptimizable() ثابت override { بازگشت false; }  // Though deleted before opt
    bool isPure() override { بازگشت false; }  // Though deleted before opt
    bool isOutputter() override { بازگشت true; }  // Though deleted before opt
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_PLI; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        بازگشت m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ثابت { بازگشت m_off; }
};
class AstPExprClause final : عمومی Ast_گره_Stmt {
    ثابت bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ثابت bool m_vacuous;  // Pass is vacuous

عمومی:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_سطر* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool pass() ثابت { بازگشت m_pass; }
    bool vacuous() ثابت { بازگشت m_vacuous; }
};
class AstPrintTimeScale final : عمومی Ast_گره_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
عمومی:
    explicit AstPrintTimeScale(File_سطر* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    تهی name(ثابت string& name) override { m_name = name; }
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }  // * = Var name
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { بازگشت "$printtimescale"; }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_PLI; }
    تهی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ثابت { بازگشت m_timeunit; }
};
class AstRSBreak final : عمومی Ast_گره_Stmt {
    // randsequence break
عمومی:
    explicit AstRSBreak(File_سطر* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ثابت override { بازگشت "break"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : عمومی Ast_گره_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
عمومی:
    AstRSProd(File_سطر* fl, ثابت string& name, Ast_گره* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
};
class AstRSProdItem final : عمومی Ast_گره_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
عمومی:
    AstRSProdItem(File_سطر* fl, ثابت string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    AstRSProd* prodp() ثابت { بازگشت m_prodp; }
    تهی prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : عمومی Ast_گره_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
عمومی:
    AstRSProdList(File_سطر* fl, Ast_گره_عبارت* weightp, Ast_گره* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    bool randJoin() ثابت { بازگشت m_randJoin; }
    تهی randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : عمومی Ast_گره_Stmt {
    // randsequence return
عمومی:
    explicit AstRSReturn(File_سطر* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ثابت override { بازگشت "return"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : عمومی Ast_گره_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
عمومی:
    AstRSRule(File_سطر* fl, Ast_گره_عبارت* weightp, AstRSProdList* prodlistsp, Ast_گره* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
};
class AstRandCase final : عمومی Ast_گره_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
عمومی:
    AstRandCase(File_سطر* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
};
class AstRandSequence final : عمومی Ast_گره_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
عمومی:
    AstRandSequence(File_سطر* fl, ثابت string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }  // * = Block name
    تهی name(ثابت string& name) override { m_name = name; }
    string start() ثابت VL_MT_STABLE { بازگشت m_start; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    AstRSProd* prodp() ثابت { بازگشت m_prodp; }
    تهی prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : عمومی Ast_گره_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
عمومی:
    AstRelease(File_سطر* fl, Ast_گره_عبارت* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : عمومی Ast_گره_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
عمومی:
    AstRepeat(File_سطر* fl, Ast_گره_عبارت* countp, Ast_گره* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ثابت override { بازگشت false; }  // Not relevant - converted to FOR
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_BRANCH; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class AstReturn final : عمومی Ast_گره_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
عمومی:
    explicit AstReturn(File_سطر* fl, Ast_گره_عبارت* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ثابت override { بازگشت "return"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : عمومی Ast_گره_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
عمومی:
    AstSFormat(File_سطر* fl, bool optionalFormat, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* exprsp,
               نویسه missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ثابت newp
            = new AstSFormatF{fl, AstSFormatF::عبارت_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_سطر* fl, AstSFormatF* fmtp, Ast_گره_عبارت* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!fmtp());
        بازگشت nullptr;
    }
    string verilogKwd() ثابت override { بازگشت "$sformat"; }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت true; }
    bool isPure() override { بازگشت true; }
    bool isOutputter() override { بازگشت false; }
    مجازی bool cleanOut() ثابت { بازگشت false; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_PLI; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class AstSetuphold final : عمومی Ast_گره_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
عمومی:
    AstSetuphold(File_سطر* fl, Ast_گره_عبارت* refevp, Ast_گره_عبارت* dataevp,
                 Ast_گره_عبارت* delrefp = nullptr, Ast_گره_عبارت* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class Ast_پشته_Trace_T final : عمومی Ast_گره_Stmt {
    // $stacktrace used as task
عمومی:
    explicit Ast_پشته_Trace_T(File_سطر* fl)
        : ASTGEN_SUPER_پشته_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_پشته_Trace_T;
    string verilogKwd() ثابت override { بازگشت "$stacktrace"; }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isUnlikely() ثابت override { بازگشت true; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class Ast_Stmt_عبارت final : عمومی Ast_گره_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
عمومی:
    Ast_Stmt_عبارت(File_سطر* fl, Ast_گره_عبارت* exprp)
        : ASTGEN_SUPER_Stmt_عبارت(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_عبارت;
    // cppcheck-suppress uselessOverride
    bool isPure() override { بازگشت exprp()->isPure(); }
};
class AstStmtPragma final : عمومی Ast_گره_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
عمومی:
    AstStmtPragma(File_سطر* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : عمومی Ast_گره_Stmt {
    ثابت bool m_isFatal;  // $fatal not $stop
عمومی:
    AstStop(File_سطر* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { بازگشت true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت 0; }  // Rarely executes
    bool same_گره(ثابت Ast_گره* samep) ثابت override { بازگشت fileline() == samep->fileline(); }
    string emitVerilog() ثابت { بازگشت m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ثابت { بازگشت m_isFatal; }
};
class AstSystemT final : عمومی Ast_گره_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
عمومی:
    AstSystemT(File_سطر* fl, Ast_گره_عبارت* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ثابت override { بازگشت "$system"; }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isUnlikely() ثابت override { بازگشت true; }
    bool same_گره(ثابت Ast_گره* /*samep*/) ثابت override { بازگشت true; }
};
class AstTimeFormat final : عمومی Ast_گره_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
عمومی:
    AstTimeFormat(File_سطر* fl, Ast_گره_عبارت* unitsp, Ast_گره_عبارت* precisionp, Ast_گره_عبارت* suffixp,
                  Ast_گره_عبارت* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() ثابت override { بازگشت "$timeformat"; }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isPure() override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    صحیح instr_شمار() ثابت override { بازگشت INSTR_شمار_PLI; }
};
class AstTraceDecl final : عمومی Ast_گره_Stmt {
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
    ثابت string m_showname;  // Name of variable
    ثابت VNumRange m_bitRange;  // Property of var the trace details
    ثابت VNumRange m_arrayRange;  // Property of var the trace details
    ثابت VVarType m_varType;  // Type of variable (for localparam vs. param)
    ثابت VDirection m_declDirection;  // Declared direction input/output etc
    ثابت bool m_inDtypeFunc;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    صحیح m_codeInc{0};  // Code increment for type
عمومی:
    AstTraceDecl(File_سطر* fl, ثابت string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_گره_عبارت* valuep, ثابت VNumRange& bitRange, ثابت VNumRange& arrayRange,
                 AstCCall* ثابت dtypeCallp, ثابت bool inDtypeFunc)
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
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_شمار() ثابت override { بازگشت 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ثابت override VL_MT_STABLE { بازگشت m_showname; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override { بازگشت true; }
    string showname() ثابت { بازگشت m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() ثابت { بازگشت m_code; }
    تهی code(uint32_t code) { m_code = code; }
    bool codeAssigned() ثابت { بازگشت m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ثابت { بازگشت m_fidx; }
    تهی fidx(uint32_t fidx) { m_fidx = fidx; }
    تهی codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() ثابت {
        اگر (m_codeInc) { بازگشت m_codeInc; }
        بازگشت (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_واژگان()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ثابت VNumRange& bitRange() ثابت { بازگشت m_bitRange; }
    ثابت VNumRange& arrayRange() ثابت { بازگشت m_arrayRange; }
    VVarType varType() ثابت { بازگشت m_varType; }
    VDirection declDirection() ثابت { بازگشت m_declDirection; }
    AstCCall* dtypeCallp() ثابت { بازگشت m_dtypeCallp; }
    تهی dtypeCallp(AstCCall* ثابت callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ثابت { بازگشت m_dtypeDeclp; }
    تهی dtypeDeclp(AstTraceDecl* ثابت declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() ثابت { بازگشت m_inDtypeFunc; }
};
class AstTraceInc final : عمومی Ast_گره_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ثابت uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    ثابت VTraceType m_traceType;  // Is this a const/full/incremental dump

عمومی:
    AstTraceInc(File_سطر* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_درخت(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_شمار() ثابت override { بازگشت 10 + 2 * INSTR_شمار_LD; }
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        بازگشت declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    bool isPredictOptimizable() ثابت override { بازگشت false; }
    bool isOutputter() override { بازگشت true; }
    bool isPure() override { بازگشت false; }
    AstTraceDecl* declp() ثابت { بازگشت m_declp; }
    VTraceType traceType() ثابت { بازگشت m_traceType; }
    uint32_t baseCode() ثابت { بازگشت m_baseCode; }
};
class Ast_Trace_برداشت_Prefix final : عمومی Ast_گره_Stmt {
عمومی:
    explicit Ast_Trace_برداشت_Prefix(File_سطر* fl)
        : ASTGEN_SUPER_Trace_برداشت_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_برداشت_Prefix;
    bool same_گره(ثابت Ast_گره* samep) ثابت override { بازگشت false; }
};
class Ast_Trace_افزودن_Prefix final : عمومی Ast_گره_Stmt {
    ثابت string m_prefix;  // Prefix to add to signal names
    ثابت VTracePrefixType m_prefixType;  // Type of prefix being pushed
    ثابت صحیح m_چپ;  // Array left index, or struct/union member count
    ثابت صحیح m_راست;  // Array right index
    ثابت bool m_quotedPrefix;  // Quote prefix name
عمومی:
    Ast_Trace_افزودن_Prefix(File_سطر* fl, ثابت string& prefix, VTracePrefixType prefixType,
                       صحیح چپ = 0, صحیح راست = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_افزودن_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_چپ{چپ}
        , m_راست{راست}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_افزودن_Prefix;
    bool same_گره(ثابت Ast_گره* samep) ثابت override { بازگشت false; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    string prefix() ثابت { بازگشت m_prefix; }
    VTracePrefixType prefixType() ثابت { بازگشت m_prefixType; }
    صحیح چپ() ثابت { بازگشت m_چپ; }
    صحیح راست() ثابت { بازگشت m_راست; }
    bool quotedPrefix() ثابت { بازگشت m_quotedPrefix; }
};
class AstWait final : عمومی Ast_گره_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
عمومی:
    AstWait(File_سطر* fl, Ast_گره_عبارت* condp, Ast_گره* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ثابت override { بازگشت true; }
};
class AstWaitFork final : عمومی Ast_گره_Stmt {
    // A "wait fork" statement
عمومی:
    explicit AstWaitFork(File_سطر* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ثابت override { بازگشت true; }
    string verilogKwd() ثابت override { بازگشت "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : عمومی Ast_گره_Assign {
عمومی:
    AstAssign(File_سطر* fl, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp,
              Ast_گره* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_گره_Assign* cloneType(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) override {
        Ast_گره* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        بازگشت new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : عمومی Ast_گره_Assign {
    // Compound assignments (+=, -=, *=, ...)
عمومی:
    شمارش operation : uint8_t {
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

خصوصی:
    operation m_operation;

عمومی:
    AstAssignCompound(AstAssignCompound::operation operation, File_سطر* fl, Ast_گره_عبارت* lhsp,
                      Ast_گره_عبارت* rhsp, Ast_گره* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_گره_Assign* cloneType(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) override {
        Ast_گره* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        بازگشت new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { بازگشت m_operation; }
};
class AstAssignCont final : عمومی Ast_گره_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
عمومی:
    AstAssignCont(File_سطر* fl, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp,
                  Ast_گره* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_گره_Assign* cloneType(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) override {
        Ast_گره* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        بازگشت new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : عمومی Ast_گره_Assign {
عمومی:
    AstAssignDly(File_سطر* fl, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp,
                 Ast_گره* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_گره_Assign* cloneType(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) override {
        Ast_گره* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        بازگشت new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ثابت override { بازگشت false; }
    string verilogKwd() ثابت override { بازگشت "<="; }
};
class AstAssignForce final : عمومی Ast_گره_Assign {
    // Procedural 'force' statement
عمومی:
    AstAssignForce(File_سطر* fl, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_گره_Assign* cloneType(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) override {
        بازگشت new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : عمومی Ast_گره_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
عمومی:
    AstAssignW(File_سطر* fl, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp,
               Ast_گره* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_گره_Assign* cloneType(Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) override {
        Ast_گره* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        بازگشت new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ثابت override {
        بازگشت timingControlp() || lhsp()->exists([](ثابت Ast_گره_Var_Ref* refp) {
            بازگشت refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ثابت;
};

// === AstNodeBlock ===
class AstBegin final : عمومی Ast_گره_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ثابت bool m_implied : 1;  // Not inserted by user
عمومی:
    AstBegin(File_سطر* fl, ثابت string& name, Ast_گره* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    تهی setNeedProcess() { m_needProcess = true; }
    bool needProcess() ثابت { بازگشت m_needProcess; }
    bool implied() ثابت { بازگشت m_implied; }
};
class AstFork final : عمومی Ast_گره_Block {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    ثابت VJoinType m_joinType;  // Join keyword type
    bool m_immediateStart = false;  // Fork starts before its parent blocks or exits

عمومی:
    AstFork(File_سطر* fl, VJoinType joinType, ثابت string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstFork* ثابت asamep = VN_DBG_AS(samep, Fork);
        بازگشت joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() ثابت override { بازگشت !joinType().joinNone(); }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    VJoinType joinType() ثابت { بازگشت m_joinType; }
    bool immediateStart() ثابت { بازگشت m_immediateStart; }
    تهی immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : عمومی Ast_گره_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

عمومی:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_سطر* fl, Ast_گره* propp, Ast_گره* passsp, Ast_گره* failsp, VAssertType type,
              VAssertDirectiveType directive, ثابت string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ثابت override {
        بازگشت directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : عمومی Ast_گره_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
عمومی:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_سطر* fl, Ast_گره* propp, Ast_گره* passsp, Ast_گره* failsp,
                       ثابت string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ثابت override { بازگشت "assert"; }
};
class AstCover final : عمومی Ast_گره_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
عمومی:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_سطر* fl, Ast_گره* propp, Ast_گره* stmtsp, VAssertType type,
             ثابت string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ثابت override { بازگشت "cover"; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    bool isCoverSeq() ثابت { بازگشت m_isCoverSeq; }
    تهی isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ثابت { بازگشت m_isSeqEvent; }
    تهی isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : عمومی Ast_گره_Cover_Or_Assert {
عمومی:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_سطر* fl, Ast_گره* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ثابت override { بازگشت "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : عمومی Ast_گره_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
عمومی:
    AstConstraintForeach(File_سطر* fl, AstForeachHeader* headerp, Ast_گره* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ثابت { بازگشت m_soft; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
};
class AstForeach final : عمومی Ast_گره_Foreach {
عمومی:
    AstForeach(File_سطر* fl, AstForeachHeader* headerp, Ast_گره* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : عمومی Ast_گره_If {
عمومی:
    AstConstraintIf(File_سطر* fl, Ast_گره_عبارت* condp, Ast_گره* thensp, Ast_گره* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : عمومی Ast_گره_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
عمومی:
    AstIf(File_سطر* fl, Ast_گره_عبارت* condp, Ast_گره* thensp = nullptr, Ast_گره* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ثابت { بازگشت m_uniquePragma; }
    تهی uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ثابت { بازگشت m_unique0Pragma; }
    تهی unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ثابت { بازگشت m_priorityPragma; }
    تهی priorityPragma(bool flag) { m_priorityPragma = flag; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : عمومی Ast_گره_Read_Write_Mem {
عمومی:
    AstReadMem(File_سطر* fl, bool hex, Ast_گره_عبارت* filenamep, Ast_گره_عبارت* memp,
               Ast_گره_عبارت* lsbp, Ast_گره_عبارت* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ثابت override { بازگشت (isHex() ? "$readmemh" : "$readmemb"); }
    ثابت نویسه* cFuncPrefixp() ثابت override { بازگشت "VL_READMEM_"; }
};
class AstWriteMem final : عمومی Ast_گره_Read_Write_Mem {
عمومی:
    AstWriteMem(File_سطر* fl, bool hex, Ast_گره_عبارت* filenamep, Ast_گره_عبارت* memp,
                Ast_گره_عبارت* lsbp, Ast_گره_عبارت* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ثابت override { بازگشت (isHex() ? "$writememh" : "$writememb"); }
    ثابت نویسه* cFuncPrefixp() ثابت override { بازگشت "VL_WRITEMEM_"; }
};

#endif  // Guard
