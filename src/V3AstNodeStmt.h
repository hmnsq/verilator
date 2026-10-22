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
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_عقدة_Stmt VL_NOT_FINAL : عام Ast_عقدة {
    // Procedural statement
محمي:
    Ast_عقدة_Stmt(VNType t, File_سطر* fl)
        : Ast_عقدة{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Stmt;
    // METHODS
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
};
class Ast_عقدة_Assign VL_NOT_FINAL : عام Ast_عقدة_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
محمي:
    Ast_عقدة_Assign(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp,
                  Ast_عقدة* timingControlp = nullptr)
        : Ast_عقدة_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Assign;
    // Clone single node, just get same type back.
    فراغ dump(std::ostream& str) ثابت override;
    virtual Ast_عقدة_Assign* cloneType(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp) = 0;
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    virtual bool cleanRhs() ثابت { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    string verilogKwd() ثابت override { أرجع "="; }
    bool isTimingControl() ثابت override { أرجع timingControlp(); }
};
class Ast_عقدة_Block VL_NOT_FINAL : عام Ast_عقدة_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
محمي:
    Ast_عقدة_Block(VNType t, File_سطر* fl, ثابت string& name)
        : Ast_عقدة_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Block;
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Block name
    فراغ name(ثابت string& name) override { m_name = name; }
    bool unnamed() ثابت { أرجع m_unnamed; }
};
class Ast_عقدة_Cover_Or_Assert VL_NOT_FINAL : عام Ast_عقدة_Stmt {
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

عام:
    Ast_عقدة_Cover_Or_Assert(VNType t, File_سطر* fl, Ast_عقدة* propp, Ast_عقدة* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ثابت string& name = "")
        : Ast_عقدة_Stmt{t, fl}
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
    ASTGEN_MEMBERS_Ast_عقدة_Cover_Or_Assert;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع samep->name() == name(); }
    فراغ name(ثابت string& name) override { m_name = name; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string type() { أرجع ""; }
    VAssertType userType() ثابت VL_MT_SAFE { أرجع m_userType; }
    VAssertDirectiveType directive() ثابت { أرجع m_directive; }
    bool immediate() ثابت { أرجع m_immediate; }
    فراغ immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() ثابت VL_MT_STABLE { أرجع m_senFromAlways; }
    فراغ senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_عقدة_Foreach VL_NOT_FINAL : عام Ast_عقدة_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
عام:
    Ast_عقدة_Foreach(VNType t, File_سطر* fl, AstForeachHeader* headerp, Ast_عقدة* bodyp)
        : Ast_عقدة_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_عقدة_Foreach;
    bool isGateOptimizable() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class Ast_عقدة_If VL_NOT_FINAL : عام Ast_عقدة_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
محمي:
    Ast_عقدة_If(VNType t, File_سطر* fl, Ast_عقدة_تعبير* condp, Ast_عقدة* thensp, Ast_عقدة* elsesp)
        : Ast_عقدة_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_If;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isGateDedupable() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    فراغ branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ثابت { أرجع m_branchPred; }
    فراغ isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ثابت { أرجع m_isBoundsCheck; }
};
class Ast_عقدة_Read_Write_Mem VL_NOT_FINAL : عام Ast_عقدة_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ثابت bool m_isHex;  // readmemh, not readmemb
عام:
    Ast_عقدة_Read_Write_Mem(VNType t, File_سطر* fl, bool hex, Ast_عقدة_تعبير* filenamep,
                        Ast_عقدة_تعبير* memp, Ast_عقدة_تعبير* lsbp, Ast_عقدة_تعبير* msbp)
        : Ast_عقدة_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_عقدة_Read_Write_Mem;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع isHex() == VN_DBG_AS(samep, عقدة_Read_Write_Mem)->isHex();
    }
    bool isHex() ثابت { أرجع m_isHex; }
    virtual ثابت حرف* cFuncPrefixp() ثابت = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : عام Ast_عقدة {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
عام:
    AstCaseItem(File_سطر* fl, Ast_عقدة_تعبير* condsp, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() + INSTR_عدد_BRANCH; }
    bool isDefault() ثابت { أرجع condsp() == nullptr; }
};

class AstForeachHeader final : عام Ast_عقدة {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
عام:
    AstForeachHeader(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : عام Ast_عقدة_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

عام:
    AstAssertCtl(File_سطر* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_عقدة_تعبير* levelp = nullptr, Ast_عقدة_تعبير* itemsp = nullptr);
    AstAssertCtl(File_سطر* fl, Ast_عقدة_تعبير* controlTypep, Ast_عقدة_تعبير* assertTypesp = nullptr,
                 Ast_عقدة_تعبير* directiveTypep = nullptr, Ast_عقدة_تعبير* levelp = nullptr,
                 Ast_عقدة_تعبير* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ثابت override { أرجع m_ctlType.ascii(); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    VAssertCtlType ctlType() ثابت { أرجع m_ctlType; }
    فراغ ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ثابت { أرجع m_assertTypes; }
    فراغ assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ثابت { أرجع m_directiveTypes; }
    فراغ directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstBreak final : عام Ast_عقدة_Stmt {
عام:
    explicit AstBreak(File_سطر* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ثابت override { أرجع "break"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : عام Ast_عقدة_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
عام:
    AstCAwait(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_Sen_شجرة* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isTimingControl() ثابت override { أرجع true; }
    Ast_Sen_شجرة* sentreep() ثابت { أرجع m_sentreep; }
    فراغ clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : عام Ast_عقدة_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
عام:
    AstCLocalScope(File_سطر* fl, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : عام Ast_عقدة_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstCReturn(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstCStmt final : عام Ast_عقدة_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ثابت VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    ساكن AstCStmt* profExecSection(File_سطر* flp, ثابت std::string& section, bool ادفع) {
        // Compute the label
        std::string label;
        إذا (v3Global.opt.hierChild()) {
            label += v3Global.opt.قمة_Module() + ":";
        } وإلا إذا (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ادفع ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        إذا (ادفع) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ادفع ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        أرجع new AstCStmt{flp, ادفع ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

عام:
    explicit AstCStmt(File_سطر* fl, ثابت std::string& text = "",
                      ثابت VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        إذا (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VCStmtType stmtType() ثابت { أرجع m_stmtType; }
    // Add some text, or a node to this statement
    فراغ add(ثابت std::string& text) { addNodesp(new AstText{fileline(), text}); }
    فراغ add(Ast_عقدة* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    ساكن AstCStmt* prof_Exec_Section_ادفع(File_سطر* flp, ثابت std::string& section) {
        أرجع profExecSection(flp, section, true);
    }
    ساكن AstCStmt* prof_Exec_Section_اسحب(File_سطر* flp, ثابت std::string& section) {
        أرجع profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : عام Ast_عقدة_Stmt {
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
عام:
    explicit AstCStmtUser(File_سطر* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    bool fromDollarC() ثابت { أرجع m_fromDollarC; }
    // Add some text, or a node to this statement
    فراغ add(ثابت std::string& text) { addNodesp(new AstText{fileline(), text}); }
    فراغ add(Ast_عقدة* nodep) { addNodesp(nodep); }
};
class AstCase final : عام Ast_عقدة_Stmt {
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
عام:
    AstCase(File_سطر* fl, VCaseType caseType, Ast_عقدة_تعبير* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    string verilogKwd() ثابت override { أرجع casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ثابت { أرجع m_caseType == VCaseType::CT_CASEX; }
    bool casez() ثابت { أرجع m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ثابت { أرجع m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ثابت { أرجع m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ثابت { أرجع m_caseType == VCaseType::CT_CASE; }
    فراغ caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    فراغ caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() ثابت { أرجع m_fullPragma; }
    فراغ fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() ثابت { أرجع m_parallelPragma; }
    فراغ parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ثابت { أرجع m_uniquePragma; }
    فراغ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ثابت { أرجع m_unique0Pragma; }
    فراغ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ثابت { أرجع m_priorityPragma; }
    فراغ priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() ثابت;
};
class AstComment final : عام Ast_عقدة_Stmt {
    // Some comment to put into the output stream
    ثابت string m_name;  // Text of comment
    ثابت bool m_showAt;  // Show "at <fileline>"
عام:
    AstComment(File_سطر* fl, ثابت string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Text
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع true; }  // Ignore name in comments
    virtual bool showAt() ثابت { أرجع m_showAt; }
};
class AstConstraintBefore final : عام Ast_عقدة_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
عام:
    AstConstraintBefore(File_سطر* fl, Ast_عقدة_تعبير* lhssp, Ast_عقدة_تعبير* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class Ast_Constraint_تعبير final : عام Ast_عقدة_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
عام:
    Ast_Constraint_تعبير(File_سطر* fl, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_Constraint_تعبير(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_تعبير;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isDisableSoft() ثابت { أرجع m_isDisableSoft; }
    فراغ isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ثابت { أرجع m_isSoft; }
    فراغ isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : عام Ast_عقدة_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
عام:
    AstConstraintUnique(File_سطر* fl, Ast_عقدة* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstContinue final : عام Ast_عقدة_Stmt {
عام:
    explicit AstContinue(File_سطر* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ثابت override { أرجع "continue"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : عام Ast_عقدة_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

عام:
    AstCoverInc(File_سطر* fl, Ast_عقدة_Cover_Decl* declp, ثابت bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع 1 + 2 * INSTR_عدد_LD; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstCoverInc* ثابت otherp = VN_DBG_AS(samep, CoverInc);
        أرجع declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isPure() override { أرجع false; }
    Ast_عقدة_Cover_Decl* declp() ثابت { أرجع m_declp; }  // Where defined
    فراغ declp(Ast_عقدة_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ثابت { أرجع m_isDuplicate; }
};
class AstDeassign final : عام Ast_عقدة_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstDeassign(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : عام Ast_عقدة_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ثابت bool m_isCycleDelay;  // True if it is a cycle delay
عام:
    AstDelay(File_سطر* fl, Ast_عقدة_تعبير* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isTimingControl() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    bool isCycleDelay() ثابت { أرجع m_isCycleDelay; }
    bool isRangeDelay() ثابت { أرجع m_isCycleDelay && rhsp(); }
    bool isUnbounded() ثابت { أرجع isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    فراغ fallDelay(Ast_عقدة_تعبير* ثابت fallDelayp) { rhsp(fallDelayp); }
    Ast_عقدة_تعبير* fallDelay() ثابت { أرجع m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : عام Ast_عقدة_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
عام:
    AstDisable(File_سطر* fl, Ast_عقدة_تعبير* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ثابت حرف* broken() ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ targetp(Ast_عقدة* nodep) { m_targetp = nodep; }
    Ast_عقدة* targetp() ثابت { أرجع m_targetp; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : عام Ast_عقدة_Stmt {
    // A "disable fork" statement
عام:
    explicit AstDisableFork(File_سطر* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ثابت override { أرجع "disable fork"; }
};
class AstDisplay final : عام Ast_عقدة_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

عام:
    AstDisplay(File_سطر* fl, VDisplayType dispType, ثابت string& text, Ast_عقدة_تعبير* filep,
               Ast_عقدة_تعبير* exprsp, حرف missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_سطر* fl, VDisplayType dispType, Ast_عقدة_تعبير* filep, Ast_عقدة_تعبير* exprsp,
               حرف missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ثابت newp
            = new AstSFormatF{fl, AstSFormatF::تعبير_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!fmtp());
        أرجع nullptr;
    }
    string verilogKwd() ثابت override {
        أرجع (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
    VDisplayType displayType() ثابت { أرجع m_displayType; }
    فراغ displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() ثابت { أرجع displayType().addNewline(); }
};
class AstDumpCtl final : عام Ast_عقدة_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ثابت VDumpCtlType m_ctlType;  // Type of operation
عام:
    AstDumpCtl(File_سطر* fl, VDumpCtlType ctlType, Ast_عقدة_تعبير* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ثابت override { أرجع ctlType().ascii(); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    virtual bool cleanOut() ثابت { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VDumpCtlType ctlType() ثابت { أرجع m_ctlType; }
};
class AstEventControl final : عام Ast_عقدة_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
عام:
    AstEventControl(File_سطر* fl, Ast_Sen_شجرة* sentreep, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ثابت override { أرجع "@(%l) %r"; }
    bool isTimingControl() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع 0; }
};
class AstExecGraph final : عام Ast_عقدة_Stmt {
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

عام:
    AstExecGraph(File_سطر* fl, ثابت string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    فراغ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ثابت حرف* broken() ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    V3Graph* depGraphp() { أرجع m_depGraphp; }
    ثابت V3Graph* depGraphp() ثابت { أرجع m_depGraphp; }
    AstCFunc* runfuncp() ثابت { أرجع m_runfuncp; }
    فراغ runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : عام Ast_عقدة_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
عام:
    AstFClose(File_سطر* fl, Ast_عقدة_تعبير* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ثابت override { أرجع "$fclose"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstFFlush final : عام Ast_عقدة_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
عام:
    AstFFlush(File_سطر* fl, Ast_عقدة_تعبير* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ثابت override { أرجع "$fflush"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstFinish final : عام Ast_عقدة_Stmt {
عام:
    explicit AstFinish(File_سطر* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع 0; }  // Rarely executes
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع fileline() == samep->fileline(); }
};
class AstFinishFork final : عام Ast_عقدة_Stmt {
    // $finish in fork
عام:
    explicit AstFinishFork(File_سطر* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع 0; }  // Rarely executes
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع fileline() == samep->fileline(); }
};
class AstFireEvent final : عام Ast_عقدة_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ثابت bool m_delayed;  // Delayed (->>) vs non-delayed (->)
عام:
    AstFireEvent(File_سطر* fl, Ast_عقدة_تعبير* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isDelayed() ثابت { أرجع m_delayed; }
};
class AstInitialAutomaticStmt final : عام Ast_عقدة_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
عام:
    AstInitialAutomaticStmt(File_سطر* fl, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool isPure() override { أرجع true; }
};
class AstInitialStaticStmt final : عام Ast_عقدة_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
عام:
    AstInitialStaticStmt(File_سطر* fl, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool isPure() override { أرجع true; }
};
class AstJumpBlock final : عام Ast_عقدة_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
عام:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_سطر* fl, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool isPure() override;

خاص:
    bool getPurityRecurse() ثابت;
};
class AstJumpGo final : عام Ast_عقدة_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
عام:
    AstJumpGo(File_سطر* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ثابت حرف* broken() ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isBrancher() ثابت override {
        أرجع true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() ثابت { أرجع m_blockp; }
};
class AstLoop final : عام Ast_عقدة_Stmt {
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
عام:
    explicit AstLoop(File_سطر* fl, Ast_عقدة* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_عقدة(ثابت Ast_عقدة* thatp) ثابت override {
        أرجع m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    // ACCESSORS
    VOptionBool unroll() ثابت { أرجع m_unroll; }
    فراغ unroll(ثابت VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : عام Ast_عقدة_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
عام:
    AstLoopTest(File_سطر* fl, AstLoop* loopp, Ast_عقدة_تعبير* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ثابت حرف* broken() ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isBrancher() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع 0; }
    // ACCESSORS
    AstLoop* loopp() ثابت { أرجع m_loopp; }
};
class AstMonitorOff final : عام Ast_عقدة_Stmt {
    ثابت bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

عام:
    AstMonitorOff(File_سطر* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { أرجع m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ثابت override { أرجع false; }  // Though deleted before opt
    bool isPredictOptimizable() ثابت override { أرجع false; }  // Though deleted before opt
    bool isPure() override { أرجع false; }  // Though deleted before opt
    bool isOutputter() override { أرجع true; }  // Though deleted before opt
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ثابت { أرجع m_off; }
};
class AstPExprClause final : عام Ast_عقدة_Stmt {
    ثابت bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ثابت bool m_vacuous;  // Pass is vacuous

عام:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_سطر* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool pass() ثابت { أرجع m_pass; }
    bool vacuous() ثابت { أرجع m_vacuous; }
};
class Ast_اطبع_Time_Scale final : عام Ast_عقدة_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
عام:
    explicit Ast_اطبع_Time_Scale(File_سطر* fl)
        : ASTGEN_SUPER_اطبع_Time_Scale(fl) {}
    ASTGEN_MEMBERS_Ast_اطبع_Time_Scale;
    فراغ name(ثابت string& name) override { m_name = name; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { أرجع "$printtimescale"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
};
class AstRSBreak final : عام Ast_عقدة_Stmt {
    // randsequence break
عام:
    explicit AstRSBreak(File_سطر* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ثابت override { أرجع "break"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : عام Ast_عقدة_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
عام:
    AstRSProd(File_سطر* fl, ثابت string& name, Ast_عقدة* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
};
class AstRSProdItem final : عام Ast_عقدة_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
عام:
    AstRSProdItem(File_سطر* fl, ثابت string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    AstRSProd* prodp() ثابت { أرجع m_prodp; }
    فراغ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : عام Ast_عقدة_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
عام:
    AstRSProdList(File_سطر* fl, Ast_عقدة_تعبير* weightp, Ast_عقدة* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    bool randJoin() ثابت { أرجع m_randJoin; }
    فراغ randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : عام Ast_عقدة_Stmt {
    // randsequence return
عام:
    explicit AstRSReturn(File_سطر* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ثابت override { أرجع "return"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : عام Ast_عقدة_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
عام:
    AstRSRule(File_سطر* fl, Ast_عقدة_تعبير* weightp, AstRSProdList* prodlistsp, Ast_عقدة* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
};
class AstRandCase final : عام Ast_عقدة_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
عام:
    AstRandCase(File_سطر* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
};
class AstRandSequence final : عام Ast_عقدة_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
عام:
    AstRandSequence(File_سطر* fl, ثابت string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Block name
    فراغ name(ثابت string& name) override { m_name = name; }
    string start() ثابت VL_MT_STABLE { أرجع m_start; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    AstRSProd* prodp() ثابت { أرجع m_prodp; }
    فراغ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : عام Ast_عقدة_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstRelease(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : عام Ast_عقدة_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
عام:
    AstRepeat(File_سطر* fl, Ast_عقدة_تعبير* countp, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ثابت override { أرجع false; }  // Not relevant - converted to FOR
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstReturn final : عام Ast_عقدة_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
عام:
    explicit AstReturn(File_سطر* fl, Ast_عقدة_تعبير* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ثابت override { أرجع "return"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : عام Ast_عقدة_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
عام:
    AstSFormat(File_سطر* fl, bool optionalFormat, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* exprsp,
               حرف missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ثابت newp
            = new AstSFormatF{fl, AstSFormatF::تعبير_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_سطر* fl, AstSFormatF* fmtp, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!fmtp());
        أرجع nullptr;
    }
    string verilogKwd() ثابت override { أرجع "$sformat"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع true; }
    bool isPure() override { أرجع true; }
    bool isOutputter() override { أرجع false; }
    virtual bool cleanOut() ثابت { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstSetuphold final : عام Ast_عقدة_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
عام:
    AstSetuphold(File_سطر* fl, Ast_عقدة_تعبير* refevp, Ast_عقدة_تعبير* dataevp,
                 Ast_عقدة_تعبير* delrefp = nullptr, Ast_عقدة_تعبير* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class Ast_مكدس_Trace_T final : عام Ast_عقدة_Stmt {
    // $stacktrace used as task
عام:
    explicit Ast_مكدس_Trace_T(File_سطر* fl)
        : ASTGEN_SUPER_مكدس_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_مكدس_Trace_T;
    string verilogKwd() ثابت override { أرجع "$stacktrace"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class Ast_Stmt_تعبير final : عام Ast_عقدة_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
عام:
    Ast_Stmt_تعبير(File_سطر* fl, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_Stmt_تعبير(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_تعبير;
    // cppcheck-suppress uselessOverride
    bool isPure() override { أرجع exprp()->isPure(); }
};
class AstStmtPragma final : عام Ast_عقدة_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
عام:
    AstStmtPragma(File_سطر* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : عام Ast_عقدة_Stmt {
    ثابت bool m_isFatal;  // $fatal not $stop
عام:
    AstStop(File_سطر* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع 0; }  // Rarely executes
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع fileline() == samep->fileline(); }
    string emitVerilog() ثابت { أرجع m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ثابت { أرجع m_isFatal; }
};
class AstSystemT final : عام Ast_عقدة_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstSystemT(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ثابت override { أرجع "$system"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstTimeFormat final : عام Ast_عقدة_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
عام:
    AstTimeFormat(File_سطر* fl, Ast_عقدة_تعبير* unitsp, Ast_عقدة_تعبير* precisionp, Ast_عقدة_تعبير* suffixp,
                  Ast_عقدة_تعبير* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() ثابت override { أرجع "$timeformat"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
};
class AstTraceDecl final : عام Ast_عقدة_Stmt {
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
    صحيح m_codeInc{0};  // Code increment for type
عام:
    AstTraceDecl(File_سطر* fl, ثابت string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_عقدة_تعبير* valuep, ثابت VNumRange& bitRange, ثابت VNumRange& arrayRange,
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
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ثابت override VL_MT_STABLE { أرجع m_showname; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع true; }
    string showname() ثابت { أرجع m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() ثابت { أرجع m_code; }
    فراغ code(uint32_t code) { m_code = code; }
    bool codeAssigned() ثابت { أرجع m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ثابت { أرجع m_fidx; }
    فراغ fidx(uint32_t fidx) { m_fidx = fidx; }
    فراغ codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() ثابت {
        إذا (m_codeInc) { أرجع m_codeInc; }
        أرجع (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_كلمات()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ثابت VNumRange& bitRange() ثابت { أرجع m_bitRange; }
    ثابت VNumRange& arrayRange() ثابت { أرجع m_arrayRange; }
    VVarType varType() ثابت { أرجع m_varType; }
    VDirection declDirection() ثابت { أرجع m_declDirection; }
    AstCCall* dtypeCallp() ثابت { أرجع m_dtypeCallp; }
    فراغ dtypeCallp(AstCCall* ثابت callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ثابت { أرجع m_dtypeDeclp; }
    فراغ dtypeDeclp(AstTraceDecl* ثابت declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() ثابت { أرجع m_inDtypeFunc; }
};
class AstTraceInc final : عام Ast_عقدة_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ثابت uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    ثابت VTraceType m_traceType;  // Is this a const/full/incremental dump

عام:
    AstTraceInc(File_سطر* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_شجرة(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع 10 + 2 * INSTR_عدد_LD; }
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isPure() override { أرجع false; }
    AstTraceDecl* declp() ثابت { أرجع m_declp; }
    VTraceType traceType() ثابت { أرجع m_traceType; }
    uint32_t baseCode() ثابت { أرجع m_baseCode; }
};
class Ast_Trace_اسحب_Prefix final : عام Ast_عقدة_Stmt {
عام:
    explicit Ast_Trace_اسحب_Prefix(File_سطر* fl)
        : ASTGEN_SUPER_Trace_اسحب_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_اسحب_Prefix;
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع false; }
};
class Ast_Trace_ادفع_Prefix final : عام Ast_عقدة_Stmt {
    ثابت string m_prefix;  // Prefix to add to signal names
    ثابت VTracePrefixType m_prefixType;  // Type of prefix being pushed
    ثابت صحيح m_يسار;  // Array left index, or struct/union member count
    ثابت صحيح m_يمين;  // Array right index
    ثابت bool m_quotedPrefix;  // Quote prefix name
عام:
    Ast_Trace_ادفع_Prefix(File_سطر* fl, ثابت string& prefix, VTracePrefixType prefixType,
                       صحيح يسار = 0, صحيح يمين = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_ادفع_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_يسار{يسار}
        , m_يمين{يمين}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_ادفع_Prefix;
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع false; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string prefix() ثابت { أرجع m_prefix; }
    VTracePrefixType prefixType() ثابت { أرجع m_prefixType; }
    صحيح يسار() ثابت { أرجع m_يسار; }
    صحيح يمين() ثابت { أرجع m_يمين; }
    bool quotedPrefix() ثابت { أرجع m_quotedPrefix; }
};
class AstWait final : عام Ast_عقدة_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
عام:
    AstWait(File_سطر* fl, Ast_عقدة_تعبير* condp, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ثابت override { أرجع true; }
};
class AstWaitFork final : عام Ast_عقدة_Stmt {
    // A "wait fork" statement
عام:
    explicit AstWaitFork(File_سطر* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ثابت override { أرجع true; }
    string verilogKwd() ثابت override { أرجع "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : عام Ast_عقدة_Assign {
عام:
    AstAssign(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp,
              Ast_عقدة* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_عقدة_Assign* cloneType(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp) override {
        Ast_عقدة* ثابت controlp = timingControlp() ? timingControlp()->clone_شجرة(false) : nullptr;
        أرجع new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : عام Ast_عقدة_Assign {
    // Compound assignments (+=, -=, *=, ...)
عام:
    تعداد operation : uint8_t {
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

خاص:
    operation m_operation;

عام:
    AstAssignCompound(AstAssignCompound::operation operation, File_سطر* fl, Ast_عقدة_تعبير* lhsp,
                      Ast_عقدة_تعبير* rhsp, Ast_عقدة* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_عقدة_Assign* cloneType(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp) override {
        Ast_عقدة* ثابت controlp = timingControlp() ? timingControlp()->clone_شجرة(false) : nullptr;
        أرجع new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { أرجع m_operation; }
};
class AstAssignCont final : عام Ast_عقدة_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
عام:
    AstAssignCont(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp,
                  Ast_عقدة* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_عقدة_Assign* cloneType(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp) override {
        Ast_عقدة* ثابت controlp = timingControlp() ? timingControlp()->clone_شجرة(false) : nullptr;
        أرجع new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : عام Ast_عقدة_Assign {
عام:
    AstAssignDly(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp,
                 Ast_عقدة* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_عقدة_Assign* cloneType(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp) override {
        Ast_عقدة* ثابت controlp = timingControlp() ? timingControlp()->clone_شجرة(false) : nullptr;
        أرجع new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    string verilogKwd() ثابت override { أرجع "<="; }
};
class AstAssignForce final : عام Ast_عقدة_Assign {
    // Procedural 'force' statement
عام:
    AstAssignForce(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_عقدة_Assign* cloneType(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp) override {
        أرجع new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : عام Ast_عقدة_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
عام:
    AstAssignW(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp,
               Ast_عقدة* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_عقدة_Assign* cloneType(Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp) override {
        Ast_عقدة* ثابت controlp = timingControlp() ? timingControlp()->clone_شجرة(false) : nullptr;
        أرجع new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ثابت override {
        أرجع timingControlp() || lhsp()->exists([](ثابت Ast_عقدة_Var_Ref* refp) {
            أرجع refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ثابت;
};

// === AstNodeBlock ===
class AstBegin final : عام Ast_عقدة_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ثابت bool m_implied : 1;  // Not inserted by user
عام:
    AstBegin(File_سطر* fl, ثابت string& name, Ast_عقدة* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ setNeedProcess() { m_needProcess = true; }
    bool needProcess() ثابت { أرجع m_needProcess; }
    bool implied() ثابت { أرجع m_implied; }
};
class AstFork final : عام Ast_عقدة_Block {
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

عام:
    AstFork(File_سطر* fl, VJoinType joinType, ثابت string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstFork* ثابت asamep = VN_DBG_AS(samep, Fork);
        أرجع joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() ثابت override { أرجع !joinType().joinNone(); }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VJoinType joinType() ثابت { أرجع m_joinType; }
    bool immediateStart() ثابت { أرجع m_immediateStart; }
    فراغ immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : عام Ast_عقدة_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

عام:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_سطر* fl, Ast_عقدة* propp, Ast_عقدة* passsp, Ast_عقدة* failsp, VAssertType type,
              VAssertDirectiveType directive, ثابت string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ثابت override {
        أرجع directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : عام Ast_عقدة_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
عام:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_سطر* fl, Ast_عقدة* propp, Ast_عقدة* passsp, Ast_عقدة* failsp,
                       ثابت string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ثابت override { أرجع "assert"; }
};
class AstCover final : عام Ast_عقدة_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
عام:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_سطر* fl, Ast_عقدة* propp, Ast_عقدة* stmtsp, VAssertType type,
             ثابت string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ثابت override { أرجع "cover"; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isCoverSeq() ثابت { أرجع m_isCoverSeq; }
    فراغ isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ثابت { أرجع m_isSeqEvent; }
    فراغ isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : عام Ast_عقدة_Cover_Or_Assert {
عام:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_سطر* fl, Ast_عقدة* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ثابت override { أرجع "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : عام Ast_عقدة_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
عام:
    AstConstraintForeach(File_سطر* fl, AstForeachHeader* headerp, Ast_عقدة* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ثابت { أرجع m_soft; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstForeach final : عام Ast_عقدة_Foreach {
عام:
    AstForeach(File_سطر* fl, AstForeachHeader* headerp, Ast_عقدة* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : عام Ast_عقدة_If {
عام:
    AstConstraintIf(File_سطر* fl, Ast_عقدة_تعبير* condp, Ast_عقدة* thensp, Ast_عقدة* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : عام Ast_عقدة_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
عام:
    AstIf(File_سطر* fl, Ast_عقدة_تعبير* condp, Ast_عقدة* thensp = nullptr, Ast_عقدة* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ثابت { أرجع m_uniquePragma; }
    فراغ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ثابت { أرجع m_unique0Pragma; }
    فراغ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ثابت { أرجع m_priorityPragma; }
    فراغ priorityPragma(bool flag) { m_priorityPragma = flag; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : عام Ast_عقدة_Read_Write_Mem {
عام:
    AstReadMem(File_سطر* fl, bool hex, Ast_عقدة_تعبير* filenamep, Ast_عقدة_تعبير* memp,
               Ast_عقدة_تعبير* lsbp, Ast_عقدة_تعبير* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ثابت override { أرجع (isHex() ? "$readmemh" : "$readmemb"); }
    ثابت حرف* cFuncPrefixp() ثابت override { أرجع "VL_READMEM_"; }
};
class AstWriteMem final : عام Ast_عقدة_Read_Write_Mem {
عام:
    AstWriteMem(File_سطر* fl, bool hex, Ast_عقدة_تعبير* filenamep, Ast_عقدة_تعبير* memp,
                Ast_عقدة_تعبير* lsbp, Ast_عقدة_تعبير* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ثابت override { أرجع (isHex() ? "$writememh" : "$writememb"); }
    ثابت حرف* cFuncPrefixp() ثابت override { أرجع "VL_WRITEMEM_"; }
};

#endif  // Guard
