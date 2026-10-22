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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ASTNODESTMT_H_
#تعریف_کرو VERILATOR_V3ASTNODESTMT_H_

#اگر_غیر_تعریف_شدہ VERILATOR_V3AST_H_
#غلطی "Use V3Ast.h as the include"
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کرو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#اگر_ختم

// === Abstract base node types (AstNode*) =====================================

class Ast_گرہ_Stmt VL_NOT_FINAL : عوامی Ast_گرہ {
    // Procedural statement
محفوظ:
    Ast_گرہ_Stmt(VNType t, File_سطر* fl)
        : Ast_گرہ{t, fl} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Stmt;
    // METHODS
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
};
class Ast_گرہ_تفویض VL_NOT_FINAL : عوامی Ast_گرہ_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
محفوظ:
    Ast_گرہ_تفویض(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
                  Ast_گرہ* timingControlp = nullptr)
        : Ast_گرہ_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_تفویض;
    // Clone single node, just get same type back.
    خالی dump(std::ostream& str) ثابت override;
    مجازی Ast_گرہ_تفویض* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) = 0;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    مجازی bool cleanRhs() ثابت { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    string verilogKwd() ثابت override { واپس "="; }
    bool isTimingControl() ثابت override { واپس timingControlp(); }
};
class Ast_گرہ_بلاک VL_NOT_FINAL : عوامی Ast_گرہ_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
محفوظ:
    Ast_گرہ_بلاک(VNType t, File_سطر* fl, ثابت string& name)
        : Ast_گرہ_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_بلاک;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Block name
    خالی name(ثابت string& name) override { m_name = name; }
    bool unnamed() ثابت { واپس m_unnamed; }
};
class Ast_گرہ_Cover_Or_Assert VL_NOT_FINAL : عوامی Ast_گرہ_Stmt {
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
    bool m_فوری = false;  // Immediate assert (may differ from userType being immediate)

عوامی:
    Ast_گرہ_Cover_Or_Assert(VNType t, File_سطر* fl, Ast_گرہ* propp, Ast_گرہ* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ثابت string& name = "")
        : Ast_گرہ_Stmt{t, fl}
        , m_name{name}
        , m_userType{userType}
        , m_directive{directive} {
        this->propp(propp);
        جوڑو_Passsp(passsp);
        m_فوری = m_userType.شامل_ہے_Any(VAssertType::SIMPLE_فوری
                                             | VAssertType::OBSERVED_DEFERRED_فوری
                                             | VAssertType::FINAL_DEFERRED_فوری)
                      || m_userType == VAssertType::اندرونی_2;
    }
    ASTGEN_MEMBERS_Ast_گرہ_Cover_Or_Assert;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس samep->name() == name(); }
    خالی name(ثابت string& name) override { m_name = name; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string type() { واپس ""; }
    VAssertType userType() ثابت VL_MT_محفوظ { واپس m_userType; }
    VAssertDirectiveType directive() ثابت { واپس m_directive; }
    bool فوری() ثابت { واپس m_فوری; }
    خالی فوری(bool flag) { m_فوری = flag; }
    bool senFromAlways() ثابت VL_MT_STABLE { واپس m_senFromAlways; }
    خالی senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_گرہ_Foreach VL_NOT_FINAL : عوامی Ast_گرہ_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
عوامی:
    Ast_گرہ_Foreach(VNType t, File_سطر* fl, Ast_Foreach_سرخی* headerp, Ast_گرہ* bodyp)
        : Ast_گرہ_Stmt(t, fl) {
        this->headerp(headerp);
        جوڑو_Bodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_گرہ_Foreach;
    bool isGateOptimizable() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_گرہ_If VL_NOT_FINAL : عوامی Ast_گرہ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
محفوظ:
    Ast_گرہ_If(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* thensp, Ast_گرہ* elsesp)
        : Ast_گرہ_Stmt{t, fl} {
        this->condp(condp);
        جوڑو_Thensp(thensp);
        جوڑو_Elsesp(elsesp);
        isBoundsCheck(false);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_If;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isGateDedupable() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    خالی branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ثابت { واپس m_branchPred; }
    خالی isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ثابت { واپس m_isBoundsCheck; }
};
class Ast_گرہ_Read_Write_Mem VL_NOT_FINAL : عوامی Ast_گرہ_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ثابت bool m_isHex;  // readmemh, not readmemb
عوامی:
    Ast_گرہ_Read_Write_Mem(VNType t, File_سطر* fl, bool hex, Ast_گرہ_اظہاریہ* filenamep,
                        Ast_گرہ_اظہاریہ* memp, Ast_گرہ_اظہاریہ* lsbp, Ast_گرہ_اظہاریہ* msbp)
        : Ast_گرہ_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_گرہ_Read_Write_Mem;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس isHex() == VN_DBG_AS(samep, گرہ_Read_Write_Mem)->isHex();
    }
    bool isHex() ثابت { واپس m_isHex; }
    مجازی ثابت حرف* cFuncPrefixp() ثابت = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Case_شے final : عوامی Ast_گرہ {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
عوامی:
    Ast_Case_شے(File_سطر* fl, Ast_گرہ_اظہاریہ* condsp, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Case_شے(fl) {
        جوڑو_Condsp(condsp);
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Case_شے;
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
    bool isDefault() ثابت { واپس condsp() == nullptr; }
};

class Ast_Foreach_سرخی final : عوامی Ast_گرہ {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
عوامی:
    Ast_Foreach_سرخی(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ* elementsp)
        : ASTGEN_SUPER_Foreach_سرخی(fl) {
        this->fromp(fromp);
        جوڑو_Elementsp(elementsp);
    }
    ASTGEN_MEMBERS_Ast_Foreach_سرخی;
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : عوامی Ast_گرہ_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

عوامی:
    AstAssertCtl(File_سطر* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_گرہ_اظہاریہ* levelp = nullptr, Ast_گرہ_اظہاریہ* itemsp = nullptr);
    AstAssertCtl(File_سطر* fl, Ast_گرہ_اظہاریہ* controlTypep, Ast_گرہ_اظہاریہ* assertTypesp = nullptr,
                 Ast_گرہ_اظہاریہ* directiveTypep = nullptr, Ast_گرہ_اظہاریہ* levelp = nullptr,
                 Ast_گرہ_اظہاریہ* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ثابت override { واپس m_ctlType.ascii(); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    VAssertCtlType ctlType() ثابت { واپس m_ctlType; }
    خالی ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ثابت { واپس m_assertTypes; }
    خالی assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ثابت { واپس m_directiveTypes; }
    خالی directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstBreak final : عوامی Ast_گرہ_Stmt {
عوامی:
    explicit AstBreak(File_سطر* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ثابت override { واپس "break"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : عوامی Ast_گرہ_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
عوامی:
    AstCAwait(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_Sen_درخت* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isTimingControl() ثابت override { واپس true; }
    Ast_Sen_درخت* sentreep() ثابت { واپس m_sentreep; }
    خالی clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : عوامی Ast_گرہ_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
عوامی:
    AstCLocalScope(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : عوامی Ast_گرہ_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
عوامی:
    AstCReturn(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstCStmt final : عوامی Ast_گرہ_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ثابت VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    ساکن AstCStmt* profExecSection(File_سطر* flp, ثابت std::string& section, bool ڈالو) {
        // Compute the label
        std::string label;
        اگر (v3Global.opt.hier_بچہ()) {
            label += v3Global.opt.چوٹی_Module() + ":";
        } ورنہ اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ڈالو ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        اگر (ڈالو) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ڈالو ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        واپس new AstCStmt{flp, ڈالو ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

عوامی:
    explicit AstCStmt(File_سطر* fl, ثابت std::string& متن = "",
                      ثابت VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        اگر (!متن.خالی_خانے()) جوڑو(متن);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    VCStmtType stmtType() ثابت { واپس m_stmtType; }
    // Add some text, or a node to this statement
    خالی جوڑو(ثابت std::string& متن) { جوڑو_Nodesp(new Ast_متن{fileline(), متن}); }
    خالی جوڑو(Ast_گرہ* nodep) { جوڑو_Nodesp(nodep); }
    // Static factory methods for specific cases
    ساکن AstCStmt* prof_Exec_Section_ڈالو(File_سطر* flp, ثابت std::string& section) {
        واپس profExecSection(flp, section, true);
    }
    ساکن AstCStmt* prof_Exec_Section_نکالو(File_سطر* flp, ثابت std::string& section) {
        واپس profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : عوامی Ast_گرہ_Stmt {
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
عوامی:
    explicit AstCStmtUser(File_سطر* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    bool fromDollarC() ثابت { واپس m_fromDollarC; }
    // Add some text, or a node to this statement
    خالی جوڑو(ثابت std::string& متن) { جوڑو_Nodesp(new Ast_متن{fileline(), متن}); }
    خالی جوڑو(Ast_گرہ* nodep) { جوڑو_Nodesp(nodep); }
};
class AstCase final : عوامی Ast_گرہ_Stmt {
    // Case statement
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstCaseItem]
    // @astgen op3 := notParallelp : List[AstNode] // assertion code for non-full case's
    VCaseType m_caseType;  // 0=case, 1=casex, 2=casez
    bool m_بھرا_Pragma = false;  // Synthesis full_case
    bool m_parallelPragma = false;  // Synthesis parallel_case
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ترجیح_Pragma = false;  // priority case
عوامی:
    AstCase(File_سطر* fl, VCaseType caseType, Ast_گرہ_اظہاریہ* exprp, Ast_Case_شے* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    string verilogKwd() ثابت override { واپس casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ثابت { واپس m_caseType == VCaseType::CT_CASEX; }
    bool casez() ثابت { واپس m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ثابت { واپس m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ثابت { واپس m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ثابت { واپس m_caseType == VCaseType::CT_CASE; }
    خالی case_Inside_سیٹ() { m_caseType = VCaseType::CT_CASEINSIDE; }
    خالی case_Matches_سیٹ() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool بھرا_Pragma() ثابت { واپس m_بھرا_Pragma; }
    خالی بھرا_Pragma(bool flag) { m_بھرا_Pragma = flag; }
    bool parallelPragma() ثابت { واپس m_parallelPragma; }
    خالی parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ثابت { واپس m_uniquePragma; }
    خالی uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ثابت { واپس m_unique0Pragma; }
    خالی unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ترجیح_Pragma() ثابت { واپس m_ترجیح_Pragma; }
    خالی ترجیح_Pragma(bool flag) { m_ترجیح_Pragma = flag; }
    string pragmaString() ثابت;
};
class AstComment final : عوامی Ast_گرہ_Stmt {
    // Some comment to put into the output stream
    ثابت string m_name;  // Text of comment
    ثابت bool m_showAt;  // Show "at <fileline>"
عوامی:
    AstComment(File_سطر* fl, ثابت string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Text
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس true; }  // Ignore name in comments
    مجازی bool showAt() ثابت { واپس m_showAt; }
};
class AstConstraintBefore final : عوامی Ast_گرہ_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
عوامی:
    AstConstraintBefore(File_سطر* fl, Ast_گرہ_اظہاریہ* lhssp, Ast_گرہ_اظہاریہ* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        جوڑو_Lhssp(lhssp);
        جوڑو_Rhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_Constraint_اظہاریہ final : عوامی Ast_گرہ_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
عوامی:
    Ast_Constraint_اظہاریہ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Constraint_اظہاریہ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_اظہاریہ;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isDisableSoft() ثابت { واپس m_isDisableSoft; }
    خالی isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ثابت { واپس m_isSoft; }
    خالی isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : عوامی Ast_گرہ_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
عوامی:
    AstConstraintUnique(File_سطر* fl, Ast_گرہ* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        جوڑو_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstContinue final : عوامی Ast_گرہ_Stmt {
عوامی:
    explicit AstContinue(File_سطر* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ثابت override { واپس "continue"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : عوامی Ast_گرہ_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

عوامی:
    AstCoverInc(File_سطر* fl, Ast_گرہ_Cover_Decl* declp, ثابت bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس 1 + 2 * INSTR_گنتی_LD; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstCoverInc* ثابت otherp = VN_DBG_AS(samep, CoverInc);
        واپس declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool is_خالص() override { واپس false; }
    Ast_گرہ_Cover_Decl* declp() ثابت { واپس m_declp; }  // Where defined
    خالی declp(Ast_گرہ_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ثابت { واپس m_isDuplicate; }
};
class AstDeassign final : عوامی Ast_گرہ_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
عوامی:
    AstDeassign(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : عوامی Ast_گرہ_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ثابت bool m_is_چکر_Delay;  // True if it is a cycle delay
عوامی:
    AstDelay(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, bool is_چکر_Delay)
        : ASTGEN_SUPER_Delay(fl)
        , m_is_چکر_Delay{is_چکر_Delay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isTimingControl() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    bool is_چکر_Delay() ثابت { واپس m_is_چکر_Delay; }
    bool isRangeDelay() ثابت { واپس m_is_چکر_Delay && rhsp(); }
    bool isUnbounded() ثابت { واپس isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    خالی fallDelay(Ast_گرہ_اظہاریہ* ثابت fallDelayp) { rhsp(fallDelayp); }
    Ast_گرہ_اظہاریہ* fallDelay() ثابت { واپس m_is_چکر_Delay ? nullptr : rhsp(); }
};
class AstDisable final : عوامی Ast_گرہ_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
عوامی:
    AstDisable(File_سطر* fl, Ast_گرہ_اظہاریہ* ہدف_Refp)
        : ASTGEN_SUPER_Disable(fl) {
        this->ہدف_Refp(ہدف_Refp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ثابت حرف* broken() ثابت override;
    خالی dump(std::ostream& str) ثابت override;
    خالی targetp(Ast_گرہ* nodep) { m_targetp = nodep; }
    Ast_گرہ* targetp() ثابت { واپس m_targetp; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : عوامی Ast_گرہ_Stmt {
    // A "disable fork" statement
عوامی:
    explicit AstDisableFork(File_سطر* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ثابت override { واپس "disable fork"; }
};
class AstDisplay final : عوامی Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

عوامی:
    AstDisplay(File_سطر* fl, VDisplayType dispType, ثابت string& متن, Ast_گرہ_اظہاریہ* filep,
               Ast_گرہ_اظہاریہ* exprsp, حرف missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, متن, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_سطر* fl, VDisplayType dispType, Ast_گرہ_اظہاریہ* filep, Ast_گرہ_اظہاریہ* exprsp,
               حرف missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ثابت newp
            = new AstSFormatF{fl, AstSFormatF::اظہاریہ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!fmtp());
        واپس nullptr;
    }
    string verilogKwd() ثابت override {
        واپس (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    VDisplayType displayType() ثابت { واپس m_displayType; }
    خالی displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool جوڑو_Newline() ثابت { واپس displayType().جوڑو_Newline(); }
};
class AstDumpCtl final : عوامی Ast_گرہ_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ثابت VDumpCtlType m_ctlType;  // Type of operation
عوامی:
    AstDumpCtl(File_سطر* fl, VDumpCtlType ctlType, Ast_گرہ_اظہاریہ* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ثابت override { واپس ctlType().ascii(); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    مجازی bool clean_برآمد() ثابت { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    VDumpCtlType ctlType() ثابت { واپس m_ctlType; }
};
class AstEventControl final : عوامی Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
عوامی:
    AstEventControl(File_سطر* fl, Ast_Sen_درخت* sentreep, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ثابت override { واپس "@(%l) %r"; }
    bool isTimingControl() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس 0; }
};
class Ast_Exec_گراف final : عوامی Ast_گرہ_Stmt {
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
    V3_گراف* ثابت m_depGraphp;  // contains ExecMTask vertices
    ثابت string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

عوامی:
    Ast_Exec_گراف(File_سطر* fl, ثابت string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~Ast_Exec_گراف() override;
    ASTGEN_MEMBERS_Ast_Exec_گراف;
    خالی cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ثابت حرف* broken() ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    V3_گراف* depGraphp() { واپس m_depGraphp; }
    ثابت V3_گراف* depGraphp() ثابت { واپس m_depGraphp; }
    AstCFunc* runfuncp() ثابت { واپس m_runfuncp; }
    خالی runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : عوامی Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
عوامی:
    AstFClose(File_سطر* fl, Ast_گرہ_اظہاریہ* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ثابت override { واپس "$fclose"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstFFlush final : عوامی Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
عوامی:
    AstFFlush(File_سطر* fl, Ast_گرہ_اظہاریہ* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ثابت override { واپس "$fflush"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_اختتام final : عوامی Ast_گرہ_Stmt {
عوامی:
    explicit Ast_اختتام(File_سطر* fl)
        : ASTGEN_SUPER_اختتام(fl) {}
    ASTGEN_MEMBERS_Ast_اختتام;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس 0; }  // Rarely executes
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس fileline() == samep->fileline(); }
};
class Ast_اختتام_Fork final : عوامی Ast_گرہ_Stmt {
    // $finish in fork
عوامی:
    explicit Ast_اختتام_Fork(File_سطر* fl)
        : ASTGEN_SUPER_اختتام_Fork(fl) {}
    ASTGEN_MEMBERS_Ast_اختتام_Fork;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس 0; }  // Rarely executes
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس fileline() == samep->fileline(); }
};
class AstFireEvent final : عوامی Ast_گرہ_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ثابت bool m_delayed;  // Delayed (->>) vs non-delayed (->)
عوامی:
    AstFireEvent(File_سطر* fl, Ast_گرہ_اظہاریہ* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isDelayed() ثابت { واپس m_delayed; }
};
class AstInitialAutomaticStmt final : عوامی Ast_گرہ_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
عوامی:
    AstInitialAutomaticStmt(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool is_خالص() override { واپس true; }
};
class AstInitialStaticStmt final : عوامی Ast_گرہ_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
عوامی:
    AstInitialStaticStmt(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool is_خالص() override { واپس true; }
};
class Ast_Jump_بلاک final : عوامی Ast_گرہ_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
عوامی:
    // After construction must call ->labelp to associate with appropriate label
    Ast_Jump_بلاک(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Jump_بلاک(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Jump_بلاک;
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool is_خالص() override;

نجی:
    bool getPurityRecurse() ثابت;
};
class AstJumpGo final : عوامی Ast_گرہ_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
عوامی:
    AstJumpGo(File_سطر* fl, Ast_Jump_بلاک* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ثابت حرف* broken() ثابت override;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isBrancher() ثابت override {
        واپس true;  // SPECIAL: We don't process code after breaks
    }
    Ast_Jump_بلاک* blockp() ثابت { واپس m_blockp; }
};
class AstLoop final : عوامی Ast_گرہ_Stmt {
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
عوامی:
    explicit AstLoop(File_سطر* fl, Ast_گرہ* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* thatp) ثابت override {
        واپس m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    // ACCESSORS
    VOptionBool unroll() ثابت { واپس m_unroll; }
    خالی unroll(ثابت VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : عوامی Ast_گرہ_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
عوامی:
    AstLoopTest(File_سطر* fl, AstLoop* loopp, Ast_گرہ_اظہاریہ* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ثابت حرف* broken() ثابت override;
    خالی dump(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isBrancher() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس 0; }
    // ACCESSORS
    AstLoop* loopp() ثابت { واپس m_loopp; }
};
class AstMonitorOff final : عوامی Ast_گرہ_Stmt {
    ثابت bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

عوامی:
    AstMonitorOff(File_سطر* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { واپس m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ثابت override { واپس false; }  // Though deleted before opt
    bool isPredictOptimizable() ثابت override { واپس false; }  // Though deleted before opt
    bool is_خالص() override { واپس false; }  // Though deleted before opt
    bool isOutputter() override { واپس true; }  // Though deleted before opt
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ثابت { واپس m_off; }
};
class AstPExprClause final : عوامی Ast_گرہ_Stmt {
    ثابت bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ثابت bool m_vacuous;  // Pass is vacuous

عوامی:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_سطر* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool pass() ثابت { واپس m_pass; }
    bool vacuous() ثابت { واپس m_vacuous; }
};
class Ast_Print_وقت_Scale final : عوامی Ast_گرہ_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
عوامی:
    explicit Ast_Print_وقت_Scale(File_سطر* fl)
        : ASTGEN_SUPER_Print_وقت_Scale(fl) {}
    ASTGEN_MEMBERS_Ast_Print_وقت_Scale;
    خالی name(ثابت string& name) override { m_name = name; }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { واپس "$printtimescale"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ثابت { واپس m_timeunit; }
};
class AstRSBreak final : عوامی Ast_گرہ_Stmt {
    // randsequence break
عوامی:
    explicit AstRSBreak(File_سطر* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ثابت override { واپس "break"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : عوامی Ast_گرہ_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
عوامی:
    AstRSProd(File_سطر* fl, ثابت string& name, Ast_گرہ* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        جوڑو_Portsp(portsp);
        جوڑو_Rulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
};
class Ast_RSProd_شے final : عوامی Ast_گرہ_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
عوامی:
    Ast_RSProd_شے(File_سطر* fl, ثابت string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProd_شے(fl)
        , m_name{name} {
        جوڑو_Argsp(argsp);
    }
    ASTGEN_MEMBERS_Ast_RSProd_شے;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    AstRSProd* prodp() ثابت { واپس m_prodp; }
    خالی prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : عوامی Ast_گرہ_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
عوامی:
    AstRSProdList(File_سطر* fl, Ast_گرہ_اظہاریہ* weightp, Ast_گرہ* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        جوڑو_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    bool randJoin() ثابت { واپس m_randJoin; }
    خالی randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : عوامی Ast_گرہ_Stmt {
    // randsequence return
عوامی:
    explicit AstRSReturn(File_سطر* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ثابت override { واپس "return"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : عوامی Ast_گرہ_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
عوامی:
    AstRSRule(File_سطر* fl, Ast_گرہ_اظہاریہ* weightp, AstRSProdList* prodlistsp, Ast_گرہ* وزن_Stmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        جوڑو_Prodlistsp(prodlistsp);
        جوڑو_وزن_Stmtsp(وزن_Stmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
};
class AstRandCase final : عوامی Ast_گرہ_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
عوامی:
    AstRandCase(File_سطر* fl, Ast_Case_شے* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        جوڑو_Itemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
};
class AstRandSequence final : عوامی Ast_گرہ_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_آغاز;  // Name of start production, or "" to use first production
عوامی:
    AstRandSequence(File_سطر* fl, ثابت string& آغاز, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_آغاز{آغاز} {
        جوڑو_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Block name
    خالی name(ثابت string& name) override { m_name = name; }
    string آغاز() ثابت VL_MT_STABLE { واپس m_آغاز; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    AstRSProd* prodp() ثابت { واپس m_prodp; }
    خالی prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : عوامی Ast_گرہ_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
عوامی:
    AstRelease(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : عوامی Ast_گرہ_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
عوامی:
    AstRepeat(File_سطر* fl, Ast_گرہ_اظہاریہ* countp, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ثابت override { واپس false; }  // Not relevant - converted to FOR
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstReturn final : عوامی Ast_گرہ_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
عوامی:
    explicit AstReturn(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ثابت override { واپس "return"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : عوامی Ast_گرہ_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
عوامی:
    AstSFormat(File_سطر* fl, bool optionalFormat, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* exprsp,
               حرف missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ثابت newp
            = new AstSFormatF{fl, AstSFormatF::اظہاریہ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_سطر* fl, AstSFormatF* fmtp, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!fmtp());
        واپس nullptr;
    }
    string verilogKwd() ثابت override { واپس "$sformat"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس true; }
    bool is_خالص() override { واپس true; }
    bool isOutputter() override { واپس false; }
    مجازی bool clean_برآمد() ثابت { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstSetuphold final : عوامی Ast_گرہ_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
عوامی:
    AstSetuphold(File_سطر* fl, Ast_گرہ_اظہاریہ* refevp, Ast_گرہ_اظہاریہ* dataevp,
                 Ast_گرہ_اظہاریہ* delrefp = nullptr, Ast_گرہ_اظہاریہ* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_ڈھیر_Trace_T final : عوامی Ast_گرہ_Stmt {
    // $stacktrace used as task
عوامی:
    explicit Ast_ڈھیر_Trace_T(File_سطر* fl)
        : ASTGEN_SUPER_ڈھیر_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_ڈھیر_Trace_T;
    string verilogKwd() ثابت override { واپس "$stacktrace"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_Stmt_اظہاریہ final : عوامی Ast_گرہ_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
عوامی:
    Ast_Stmt_اظہاریہ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Stmt_اظہاریہ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_اظہاریہ;
    // cppcheck-suppress uselessOverride
    bool is_خالص() override { واپس exprp()->is_خالص(); }
};
class AstStmtPragma final : عوامی Ast_گرہ_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
عوامی:
    AstStmtPragma(File_سطر* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : عوامی Ast_گرہ_Stmt {
    ثابت bool m_isFatal;  // $fatal not $stop
عوامی:
    AstStop(File_سطر* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس 0; }  // Rarely executes
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس fileline() == samep->fileline(); }
    string emitVerilog() ثابت { واپس m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ثابت { واپس m_isFatal; }
};
class AstSystemT final : عوامی Ast_گرہ_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
عوامی:
    AstSystemT(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ثابت override { واپس "$system"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_وقت_Format final : عوامی Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
عوامی:
    Ast_وقت_Format(File_سطر* fl, Ast_گرہ_اظہاریہ* unitsp, Ast_گرہ_اظہاریہ* precisionp, Ast_گرہ_اظہاریہ* suffixp,
                  Ast_گرہ_اظہاریہ* widthp)
        : ASTGEN_SUPER_وقت_Format(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_Ast_وقت_Format;
    string verilogKwd() ثابت override { واپس "$timeformat"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
};
class AstTraceDecl final : عوامی Ast_گرہ_Stmt {
    // Trace point declaration
    // Separate from AstTraceInc; as a declaration can't be deleted
    // Parents:  {statement list}
    // Expression being traced - Moved to AstTraceInc by V3Trace
    // @astgen op1 := valuep : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_dtypeCallp: Optional[AstCCall] // Type init function call
    // @astgen ptr := m_dtypeDeclp: Optional[AstTraceDecl] // CCall TraceDecl which replaces this
    uint32_t m_رمز{std::numeric_limits<uint32_t>::max()};  // Trace identifier code
    uint32_t m_fidx{0};  // Trace function index
    ثابت string m_showname;  // Name of variable
    ثابت VNumRange m_ثنائی_ہندسہ_Range;  // Property of var the trace details
    ثابت VNumRange m_arrayRange;  // Property of var the trace details
    ثابت VVarType m_varType;  // Type of variable (for localparam vs. param)
    ثابت VDirection m_declDirection;  // Declared direction input/output etc
    ثابت bool m_اندر_Dtype_Func;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    صحیح m_رمز_Inc{0};  // Code increment for type
عوامی:
    AstTraceDecl(File_سطر* fl, ثابت string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_گرہ_اظہاریہ* valuep, ثابت VNumRange& ثنائی_ہندسہ_Range, ثابت VNumRange& arrayRange,
                 AstCCall* ثابت dtypeCallp, ثابت bool اندر_Dtype_Func)
        : ASTGEN_SUPER_TraceDecl(fl)
        , m_showname{showname}
        , m_ثنائی_ہندسہ_Range{ثنائی_ہندسہ_Range}
        , m_arrayRange{arrayRange}
        , m_varType{varp->varType()}
        , m_declDirection{varp->declDirection()}
        , m_اندر_Dtype_Func{اندر_Dtype_Func} {
        dtypeFrom(valuep);
        this->valuep(valuep);
        this->dtypeCallp(dtypeCallp);
    }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ثابت override VL_MT_STABLE { واپس m_showname; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس true; }
    string showname() ثابت { واپس m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t رمز() ثابت { واپس m_رمز; }
    خالی رمز(uint32_t رمز) { m_رمز = رمز; }
    bool رمز_Assigned() ثابت { واپس m_رمز != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ثابت { واپس m_fidx; }
    خالی fidx(uint32_t fidx) { m_fidx = fidx; }
    خالی رمز_Inc(uint32_t رمز_Inc) { m_رمز_Inc = رمز_Inc; }
    uint32_t رمز_Inc() ثابت {
        اگر (m_رمز_Inc) { واپس m_رمز_Inc; }
        واپس (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->چوڑائی_الفاظ()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ثابت VNumRange& ثنائی_ہندسہ_Range() ثابت { واپس m_ثنائی_ہندسہ_Range; }
    ثابت VNumRange& arrayRange() ثابت { واپس m_arrayRange; }
    VVarType varType() ثابت { واپس m_varType; }
    VDirection declDirection() ثابت { واپس m_declDirection; }
    AstCCall* dtypeCallp() ثابت { واپس m_dtypeCallp; }
    خالی dtypeCallp(AstCCall* ثابت callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ثابت { واپس m_dtypeDeclp; }
    خالی dtypeDeclp(AstTraceDecl* ثابت declp) { m_dtypeDeclp = declp; }
    bool اندر_Dtype_Func() ثابت { واپس m_اندر_Dtype_Func; }
};
class AstTraceInc final : عوامی Ast_گرہ_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ثابت uint32_t m_بنیاد_رمز;  // Trace code base value in function containing this AstTraceInc
    ثابت VTraceType m_traceType;  // Is this a const/full/incremental dump

عوامی:
    AstTraceInc(File_سطر* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t بنیاد_رمز = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_بنیاد_رمز{بنیاد_رمز}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_درخت(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس 10 + 2 * INSTR_گنتی_LD; }
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool is_خالص() override { واپس false; }
    AstTraceDecl* declp() ثابت { واپس m_declp; }
    VTraceType traceType() ثابت { واپس m_traceType; }
    uint32_t بنیاد_رمز() ثابت { واپس m_بنیاد_رمز; }
};
class Ast_Trace_نکالو_سابقہ final : عوامی Ast_گرہ_Stmt {
عوامی:
    explicit Ast_Trace_نکالو_سابقہ(File_سطر* fl)
        : ASTGEN_SUPER_Trace_نکالو_سابقہ(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_نکالو_سابقہ;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس false; }
};
class Ast_Trace_ڈالو_سابقہ final : عوامی Ast_گرہ_Stmt {
    ثابت string m_سابقہ;  // Prefix to add to signal names
    ثابت VTrace_سابقہ_Type m_سابقہ_Type;  // Type of prefix being pushed
    ثابت صحیح m_بایاں;  // Array left index, or struct/union member count
    ثابت صحیح m_دایاں;  // Array right index
    ثابت bool m_quoted_سابقہ;  // Quote prefix name
عوامی:
    Ast_Trace_ڈالو_سابقہ(File_سطر* fl, ثابت string& سابقہ, VTrace_سابقہ_Type سابقہ_Type,
                       صحیح بایاں = 0, صحیح دایاں = 0, bool quoted_سابقہ = true)
        : ASTGEN_SUPER_Trace_ڈالو_سابقہ(fl)
        , m_سابقہ{سابقہ}
        , m_سابقہ_Type{سابقہ_Type}
        , m_بایاں{بایاں}
        , m_دایاں{دایاں}
        , m_quoted_سابقہ{quoted_سابقہ} {}
    ASTGEN_MEMBERS_Ast_Trace_ڈالو_سابقہ;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس false; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string سابقہ() ثابت { واپس m_سابقہ; }
    VTrace_سابقہ_Type سابقہ_Type() ثابت { واپس m_سابقہ_Type; }
    صحیح بایاں() ثابت { واپس m_بایاں; }
    صحیح دایاں() ثابت { واپس m_دایاں; }
    bool quoted_سابقہ() ثابت { واپس m_quoted_سابقہ; }
};
class AstWait final : عوامی Ast_گرہ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
عوامی:
    AstWait(File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ثابت override { واپس true; }
};
class AstWaitFork final : عوامی Ast_گرہ_Stmt {
    // A "wait fork" statement
عوامی:
    explicit AstWaitFork(File_سطر* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ثابت override { واپس true; }
    string verilogKwd() ثابت override { واپس "wait fork"; }
};

// === AstNodeAssign ===
class Ast_تفویض final : عوامی Ast_گرہ_تفویض {
عوامی:
    Ast_تفویض(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
              Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_تفویض(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_تفویض;
    Ast_گرہ_تفویض* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        واپس new Ast_تفویض{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_تفویض_Compound final : عوامی Ast_گرہ_تفویض {
    // Compound assignments (+=, -=, *=, ...)
عوامی:
    شمار operation : uint8_t {
        جوڑو_2,
        And,
        Div,
        ModDiv,
        Mul,
        Or,
        سرکاؤ_L,
        سرکاؤ_R,
        سرکاؤ_RS,
        Sub,
        Xor,
    };

نجی:
    operation m_operation;

عوامی:
    Ast_تفویض_Compound(Ast_تفویض_Compound::operation operation, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp,
                      Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_تفویض_Compound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_تفویض_Compound;
    Ast_گرہ_تفویض* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        واپس new Ast_تفویض_Compound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { واپس m_operation; }
};
class Ast_تفویض_Cont final : عوامی Ast_گرہ_تفویض {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
عوامی:
    Ast_تفویض_Cont(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
                  Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_تفویض_Cont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_تفویض_Cont;
    Ast_گرہ_تفویض* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        واپس new Ast_تفویض_Cont{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_تفویض_Dly final : عوامی Ast_گرہ_تفویض {
عوامی:
    Ast_تفویض_Dly(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
                 Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_تفویض_Dly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_تفویض_Dly;
    Ast_گرہ_تفویض* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        واپس new Ast_تفویض_Dly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ثابت override { واپس false; }
    string verilogKwd() ثابت override { واپس "<="; }
};
class Ast_تفویض_Force final : عوامی Ast_گرہ_تفویض {
    // Procedural 'force' statement
عوامی:
    Ast_تفویض_Force(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_تفویض_Force(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_تفویض_Force;
    Ast_گرہ_تفویض* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        واپس new Ast_تفویض_Force{fileline(), lhsp, rhsp};
    }
};
class Ast_تفویض_W final : عوامی Ast_گرہ_تفویض {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
عوامی:
    Ast_تفویض_W(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
               Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_تفویض_W(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_تفویض_W;
    Ast_گرہ_تفویض* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        واپس new Ast_تفویض_W{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ثابت override {
        واپس timingControlp() || lhsp()->exists([](ثابت Ast_گرہ_Var_Ref* refp) {
            واپس refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ثابت;
};

// === AstNodeBlock ===
class AstBegin final : عوامی Ast_گرہ_بلاک {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ثابت bool m_implied : 1;  // Not inserted by user
عوامی:
    AstBegin(File_سطر* fl, ثابت string& name, Ast_گرہ* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    خالی سیٹ_Need_Process() { m_needProcess = true; }
    bool needProcess() ثابت { واپس m_needProcess; }
    bool implied() ثابت { واپس m_implied; }
};
class AstFork final : عوامی Ast_گرہ_بلاک {
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
    bool m_فوری_آغاز = false;  // Fork starts before its parent blocks or exits

عوامی:
    AstFork(File_سطر* fl, VJoinType joinType, ثابت string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstFork* ثابت asamep = VN_DBG_AS(samep, Fork);
        واپس joinType() == asamep->joinType() && فوری_آغاز() == asamep->فوری_آغاز();
    }
    bool isTimingControl() ثابت override { واپس !joinType().joinNone(); }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    VJoinType joinType() ثابت { واپس m_joinType; }
    bool فوری_آغاز() ثابت { واپس m_فوری_آغاز; }
    خالی فوری_آغاز(bool flag) { m_فوری_آغاز = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : عوامی Ast_گرہ_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

عوامی:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_سطر* fl, Ast_گرہ* propp, Ast_گرہ* passsp, Ast_گرہ* failsp, VAssertType type,
              VAssertDirectiveType directive, ثابت string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        جوڑو_Failsp(failsp);
    }
    string verilogKwd() ثابت override {
        واپس directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : عوامی Ast_گرہ_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
عوامی:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_سطر* fl, Ast_گرہ* propp, Ast_گرہ* passsp, Ast_گرہ* failsp,
                       ثابت string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::اندرونی_2,
                                       VAssertDirectiveType::INTRINSIC, name) {
        جوڑو_Failsp(failsp);
    }
    string verilogKwd() ثابت override { واپس "assert"; }
};
class AstCover final : عوامی Ast_گرہ_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
عوامی:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_سطر* fl, Ast_گرہ* propp, Ast_گرہ* stmtsp, VAssertType type,
             ثابت string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ثابت override { واپس "cover"; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isCoverSeq() ثابت { واپس m_isCoverSeq; }
    خالی isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ثابت { واپس m_isSeqEvent; }
    خالی isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : عوامی Ast_گرہ_Cover_Or_Assert {
عوامی:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_سطر* fl, Ast_گرہ* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::اندرونی_2,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ثابت override { واپس "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : عوامی Ast_گرہ_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
عوامی:
    AstConstraintForeach(File_سطر* fl, Ast_Foreach_سرخی* headerp, Ast_گرہ* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ثابت { واپس m_soft; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class AstForeach final : عوامی Ast_گرہ_Foreach {
عوامی:
    AstForeach(File_سطر* fl, Ast_Foreach_سرخی* headerp, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : عوامی Ast_گرہ_If {
عوامی:
    AstConstraintIf(File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* thensp, Ast_گرہ* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : عوامی Ast_گرہ_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ترجیح_Pragma = false;  // priority case
عوامی:
    AstIf(File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* thensp = nullptr, Ast_گرہ* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ثابت { واپس m_uniquePragma; }
    خالی uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ثابت { واپس m_unique0Pragma; }
    خالی unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ترجیح_Pragma() ثابت { واپس m_ترجیح_Pragma; }
    خالی ترجیح_Pragma(bool flag) { m_ترجیح_Pragma = flag; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : عوامی Ast_گرہ_Read_Write_Mem {
عوامی:
    AstReadMem(File_سطر* fl, bool hex, Ast_گرہ_اظہاریہ* filenamep, Ast_گرہ_اظہاریہ* memp,
               Ast_گرہ_اظہاریہ* lsbp, Ast_گرہ_اظہاریہ* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ثابت override { واپس (isHex() ? "$readmemh" : "$readmemb"); }
    ثابت حرف* cFuncPrefixp() ثابت override { واپس "VL_READMEM_"; }
};
class AstWriteMem final : عوامی Ast_گرہ_Read_Write_Mem {
عوامی:
    AstWriteMem(File_سطر* fl, bool hex, Ast_گرہ_اظہاریہ* filenamep, Ast_گرہ_اظہاریہ* memp,
                Ast_گرہ_اظہاریہ* lsbp, Ast_گرہ_اظہاریہ* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ثابت override { واپس (isHex() ? "$writememh" : "$writememb"); }
    ثابت حرف* cFuncPrefixp() ثابت override { واپس "VL_WRITEMEM_"; }
};

#اگر_ختم  // Guard
