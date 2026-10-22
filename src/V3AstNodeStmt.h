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

#جے_بنا_تعریف VERILATOR_V3ASTNODESTMT_H_
#تعریف_کرو VERILATOR_V3ASTNODESTMT_H_

#جے_بنا_تعریف VERILATOR_V3AST_H_
#غلطی "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کرو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#جے_انت

// === Abstract base node types (AstNode*) =====================================

class Ast_گرہ_Stmt VL_NOT_FINAL : عام Ast_گرہ {
    // Procedural statement
محمي:
    Ast_گرہ_Stmt(VNType t, File_سطر* fl)
        : Ast_گرہ{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_گرہ_Stmt;
    // METHODS
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
};
class Ast_گرہ_سونپو VL_NOT_FINAL : عام Ast_گرہ_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
محمي:
    Ast_گرہ_سونپو(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
                  Ast_گرہ* timingControlp = nullptr)
        : Ast_گرہ_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_گرہ_سونپو;
    // Clone single node, just get same type back.
    فراغ dump(std::ostream& str) ثابت override;
    مجازی Ast_گرہ_سونپو* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) = 0;
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    مجازی bool cleanRhs() ثابت { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { أرجع true; }
    string verilogKwd() ثابت override { أرجع "="; }
    bool isTimingControl() ثابت override { أرجع timingControlp(); }
};
class Ast_گرہ_بلاک VL_NOT_FINAL : عام Ast_گرہ_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
محمي:
    Ast_گرہ_بلاک(VNType t, File_سطر* fl, ثابت string& name)
        : Ast_گرہ_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

عام:
    ASTGEN_MEMBERS_Ast_گرہ_بلاک;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Block name
    فراغ name(ثابت string& name) override { m_name = name; }
    bool unnamed() ثابت { أرجع m_unnamed; }
};
class Ast_گرہ_Cover_Or_Assert VL_NOT_FINAL : عام Ast_گرہ_Stmt {
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

عام:
    Ast_گرہ_Cover_Or_Assert(VNType t, File_سطر* fl, Ast_گرہ* propp, Ast_گرہ* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ثابت string& name = "")
        : Ast_گرہ_Stmt{t, fl}
        , m_name{name}
        , m_userType{userType}
        , m_directive{directive} {
        this->propp(propp);
        جوڑو_Passsp(passsp);
        m_فوری = m_userType.شامل_اے_Any(VAssertType::SIMPLE_فوری
                                             | VAssertType::OBSERVED_DEFERRED_فوری
                                             | VAssertType::FINAL_DEFERRED_فوری)
                      || m_userType == VAssertType::اندرونی;
    }
    ASTGEN_MEMBERS_Ast_گرہ_Cover_Or_Assert;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { أرجع samep->name() == name(); }
    فراغ name(ثابت string& name) override { m_name = name; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string type() { أرجع ""; }
    VAssertType userType() ثابت VL_MT_محفوظ { أرجع m_userType; }
    VAssertDirectiveType directive() ثابت { أرجع m_directive; }
    bool فوری() ثابت { أرجع m_فوری; }
    فراغ فوری(bool flag) { m_فوری = flag; }
    bool senFromAlways() ثابت VL_MT_STABLE { أرجع m_senFromAlways; }
    فراغ senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_گرہ_Foreach VL_NOT_FINAL : عام Ast_گرہ_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
عام:
    Ast_گرہ_Foreach(VNType t, File_سطر* fl, Ast_Foreach_سرخی* headerp, Ast_گرہ* bodyp)
        : Ast_گرہ_Stmt(t, fl) {
        this->headerp(headerp);
        جوڑو_Bodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_گرہ_Foreach;
    bool isGateOptimizable() ثابت override { أرجع false; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_گرہ_If VL_NOT_FINAL : عام Ast_گرہ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
محمي:
    Ast_گرہ_If(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* thensp, Ast_گرہ* elsesp)
        : Ast_گرہ_Stmt{t, fl} {
        this->condp(condp);
        جوڑو_Thensp(thensp);
        جوڑو_Elsesp(elsesp);
        isBoundsCheck(false);
    }

عام:
    ASTGEN_MEMBERS_Ast_گرہ_If;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isGateDedupable() ثابت override { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
    فراغ branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ثابت { أرجع m_branchPred; }
    فراغ isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ثابت { أرجع m_isBoundsCheck; }
};
class Ast_گرہ_Read_Write_Mem VL_NOT_FINAL : عام Ast_گرہ_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ثابت bool m_isHex;  // readmemh, not readmemb
عام:
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
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        أرجع isHex() == VN_DBG_AS(samep, گرہ_Read_Write_Mem)->isHex();
    }
    bool isHex() ثابت { أرجع m_isHex; }
    مجازی ثابت حرف* cFuncPrefixp() ثابت = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Case_شے final : عام Ast_گرہ {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
عام:
    Ast_Case_شے(File_سطر* fl, Ast_گرہ_اظہاریہ* condsp, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Case_شے(fl) {
        جوڑو_Condsp(condsp);
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Case_شے;
    صحيح instr_گنتی() ثابت override { أرجع چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
    bool isDefault() ثابت { أرجع condsp() == nullptr; }
};

class Ast_Foreach_سرخی final : عام Ast_گرہ {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
عام:
    Ast_Foreach_سرخی(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ* elementsp)
        : ASTGEN_SUPER_Foreach_سرخی(fl) {
        this->fromp(fromp);
        جوڑو_Elementsp(elementsp);
    }
    ASTGEN_MEMBERS_Ast_Foreach_سرخی;
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : عام Ast_گرہ_Stmt {
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
                 Ast_گرہ_اظہاریہ* levelp = nullptr, Ast_گرہ_اظہاریہ* itemsp = nullptr);
    AstAssertCtl(File_سطر* fl, Ast_گرہ_اظہاریہ* controlTypep, Ast_گرہ_اظہاریہ* assertTypesp = nullptr,
                 Ast_گرہ_اظہاریہ* directiveTypep = nullptr, Ast_گرہ_اظہاریہ* levelp = nullptr,
                 Ast_گرہ_اظہاریہ* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ثابت override { أرجع m_ctlType.ascii(); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
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
class AstBreak final : عام Ast_گرہ_Stmt {
عام:
    explicit AstBreak(File_سطر* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ثابت override { أرجع "break"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : عام Ast_گرہ_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
عام:
    AstCAwait(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_Sen_درخت* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isTimingControl() ثابت override { أرجع true; }
    Ast_Sen_درخت* sentreep() ثابت { أرجع m_sentreep; }
    فراغ clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : عام Ast_گرہ_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
عام:
    AstCLocalScope(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : عام Ast_گرہ_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstCReturn(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    صحيح instr_گنتی() ثابت override { أرجع چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class AstCStmt final : عام Ast_گرہ_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ثابت VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    ساكن AstCStmt* profExecSection(File_سطر* flp, ثابت std::string& section, bool ڈالو) {
        // Compute the label
        std::string label;
        إذا (v3Global.opt.hier_بچہ()) {
            label += v3Global.opt.چوٹی_Module() + ":";
        } وإلا إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ڈالو ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        إذا (ڈالو) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ڈالو ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        أرجع new AstCStmt{flp, ڈالو ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

عام:
    explicit AstCStmt(File_سطر* fl, ثابت std::string& متن = "",
                      ثابت VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        إذا (!متن.خالی_تھاواں()) جوڑو(متن);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VCStmtType stmtType() ثابت { أرجع m_stmtType; }
    // Add some text, or a node to this statement
    فراغ جوڑو(ثابت std::string& متن) { جوڑو_Nodesp(new Ast_متن{fileline(), متن}); }
    فراغ جوڑو(Ast_گرہ* nodep) { جوڑو_Nodesp(nodep); }
    // Static factory methods for specific cases
    ساكن AstCStmt* prof_Exec_Section_ڈالو(File_سطر* flp, ثابت std::string& section) {
        أرجع profExecSection(flp, section, true);
    }
    ساكن AstCStmt* prof_Exec_Section_نکالو(File_سطر* flp, ثابت std::string& section) {
        أرجع profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : عام Ast_گرہ_Stmt {
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
    bool is_خالص() override { أرجع false; }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { أرجع true; }
    bool fromDollarC() ثابت { أرجع m_fromDollarC; }
    // Add some text, or a node to this statement
    فراغ جوڑو(ثابت std::string& متن) { جوڑو_Nodesp(new Ast_متن{fileline(), متن}); }
    فراغ جوڑو(Ast_گرہ* nodep) { جوڑو_Nodesp(nodep); }
};
class AstCase final : عام Ast_گرہ_Stmt {
    // Case statement
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstCaseItem]
    // @astgen op3 := notParallelp : List[AstNode] // assertion code for non-full case's
    VCaseType m_caseType;  // 0=case, 1=casex, 2=casez
    bool m_بھریا_Pragma = false;  // Synthesis full_case
    bool m_parallelPragma = false;  // Synthesis parallel_case
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ترجیح_Pragma = false;  // priority case
عام:
    AstCase(File_سطر* fl, VCaseType caseType, Ast_گرہ_اظہاریہ* exprp, Ast_Case_شے* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    string verilogKwd() ثابت override { أرجع casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        أرجع m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ثابت { أرجع m_caseType == VCaseType::CT_CASEX; }
    bool casez() ثابت { أرجع m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ثابت { أرجع m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ثابت { أرجع m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ثابت { أرجع m_caseType == VCaseType::CT_CASE; }
    فراغ case_Inside_سیٹ() { m_caseType = VCaseType::CT_CASEINSIDE; }
    فراغ case_Matches_سیٹ() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool بھریا_Pragma() ثابت { أرجع m_بھریا_Pragma; }
    فراغ بھریا_Pragma(bool flag) { m_بھریا_Pragma = flag; }
    bool parallelPragma() ثابت { أرجع m_parallelPragma; }
    فراغ parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ثابت { أرجع m_uniquePragma; }
    فراغ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ثابت { أرجع m_unique0Pragma; }
    فراغ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ترجیح_Pragma() ثابت { أرجع m_ترجیح_Pragma; }
    فراغ ترجیح_Pragma(bool flag) { m_ترجیح_Pragma = flag; }
    string pragmaString() ثابت;
};
class AstComment final : عام Ast_گرہ_Stmt {
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
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { أرجع true; }  // Ignore name in comments
    مجازی bool showAt() ثابت { أرجع m_showAt; }
};
class AstConstraintBefore final : عام Ast_گرہ_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
عام:
    AstConstraintBefore(File_سطر* fl, Ast_گرہ_اظہاریہ* lhssp, Ast_گرہ_اظہاریہ* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        جوڑو_Lhssp(lhssp);
        جوڑو_Rhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_Constraint_اظہاریہ final : عام Ast_گرہ_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
عام:
    Ast_Constraint_اظہاریہ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Constraint_اظہاریہ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_اظہاریہ;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
    bool isDisableSoft() ثابت { أرجع m_isDisableSoft; }
    فراغ isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ثابت { أرجع m_isSoft; }
    فراغ isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : عام Ast_گرہ_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
عام:
    AstConstraintUnique(File_سطر* fl, Ast_گرہ* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        جوڑو_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class AstContinue final : عام Ast_گرہ_Stmt {
عام:
    explicit AstContinue(File_سطر* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ثابت override { أرجع "continue"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : عام Ast_گرہ_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

عام:
    AstCoverInc(File_سطر* fl, Ast_گرہ_Cover_Decl* declp, ثابت bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_گنتی() ثابت override { أرجع 1 + 2 * INSTR_گنتی_LD; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstCoverInc* ثابت otherp = VN_DBG_AS(samep, CoverInc);
        أرجع declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool is_خالص() override { أرجع false; }
    Ast_گرہ_Cover_Decl* declp() ثابت { أرجع m_declp; }  // Where defined
    فراغ declp(Ast_گرہ_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ثابت { أرجع m_isDuplicate; }
};
class AstDeassign final : عام Ast_گرہ_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstDeassign(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : عام Ast_گرہ_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ثابت bool m_is_چکر_Delay;  // True if it is a cycle delay
عام:
    AstDelay(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, bool is_چکر_Delay)
        : ASTGEN_SUPER_Delay(fl)
        , m_is_چکر_Delay{is_چکر_Delay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isTimingControl() ثابت override { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    bool is_چکر_Delay() ثابت { أرجع m_is_چکر_Delay; }
    bool isRangeDelay() ثابت { أرجع m_is_چکر_Delay && rhsp(); }
    bool isUnbounded() ثابت { أرجع isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    فراغ fallDelay(Ast_گرہ_اظہاریہ* ثابت fallDelayp) { rhsp(fallDelayp); }
    Ast_گرہ_اظہاریہ* fallDelay() ثابت { أرجع m_is_چکر_Delay ? nullptr : rhsp(); }
};
class AstDisable final : عام Ast_گرہ_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
عام:
    AstDisable(File_سطر* fl, Ast_گرہ_اظہاریہ* نشانہ_Refp)
        : ASTGEN_SUPER_Disable(fl) {
        this->نشانہ_Refp(نشانہ_Refp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ثابت حرف* broken() ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ targetp(Ast_گرہ* nodep) { m_targetp = nodep; }
    Ast_گرہ* targetp() ثابت { أرجع m_targetp; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : عام Ast_گرہ_Stmt {
    // A "disable fork" statement
عام:
    explicit AstDisableFork(File_سطر* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ثابت override { أرجع "disable fork"; }
};
class AstDisplay final : عام Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

عام:
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
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        أرجع displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_PLI; }
    VDisplayType displayType() ثابت { أرجع m_displayType; }
    فراغ displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool جوڑو_Newline() ثابت { أرجع displayType().جوڑو_Newline(); }
};
class AstDumpCtl final : عام Ast_گرہ_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ثابت VDumpCtlType m_ctlType;  // Type of operation
عام:
    AstDumpCtl(File_سطر* fl, VDumpCtlType ctlType, Ast_گرہ_اظہاریہ* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ثابت override { أرجع ctlType().ascii(); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    مجازی bool clean_باہرلا() ثابت { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VDumpCtlType ctlType() ثابت { أرجع m_ctlType; }
};
class AstEventControl final : عام Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
عام:
    AstEventControl(File_سطر* fl, Ast_Sen_درخت* sentreep, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ثابت override { أرجع "@(%l) %r"; }
    bool isTimingControl() ثابت override { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع 0; }
};
class Ast_Exec_گراف final : عام Ast_گرہ_Stmt {
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

عام:
    Ast_Exec_گراف(File_سطر* fl, ثابت string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~Ast_Exec_گراف() override;
    ASTGEN_MEMBERS_Ast_Exec_گراف;
    فراغ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ثابت حرف* broken() ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    V3_گراف* depGraphp() { أرجع m_depGraphp; }
    ثابت V3_گراف* depGraphp() ثابت { أرجع m_depGraphp; }
    AstCFunc* runfuncp() ثابت { أرجع m_runfuncp; }
    فراغ runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : عام Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
عام:
    AstFClose(File_سطر* fl, Ast_گرہ_اظہاریہ* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ثابت override { أرجع "$fclose"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class AstFFlush final : عام Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
عام:
    AstFFlush(File_سطر* fl, Ast_گرہ_اظہاریہ* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ثابت override { أرجع "$fflush"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_انت final : عام Ast_گرہ_Stmt {
عام:
    explicit Ast_انت(File_سطر* fl)
        : ASTGEN_SUPER_انت(fl) {}
    ASTGEN_MEMBERS_Ast_انت;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع 0; }  // Rarely executes
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { أرجع fileline() == samep->fileline(); }
};
class Ast_انت_Fork final : عام Ast_گرہ_Stmt {
    // $finish in fork
عام:
    explicit Ast_انت_Fork(File_سطر* fl)
        : ASTGEN_SUPER_انت_Fork(fl) {}
    ASTGEN_MEMBERS_Ast_انت_Fork;
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع 0; }  // Rarely executes
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { أرجع fileline() == samep->fileline(); }
};
class AstFireEvent final : عام Ast_گرہ_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ثابت bool m_delayed;  // Delayed (->>) vs non-delayed (->)
عام:
    AstFireEvent(File_سطر* fl, Ast_گرہ_اظہاریہ* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isDelayed() ثابت { أرجع m_delayed; }
};
class AstInitialAutomaticStmt final : عام Ast_گرہ_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
عام:
    AstInitialAutomaticStmt(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    صحيح instr_گنتی() ثابت override { أرجع 0; }
    bool is_خالص() override { أرجع true; }
};
class AstInitialStaticStmt final : عام Ast_گرہ_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
عام:
    AstInitialStaticStmt(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    صحيح instr_گنتی() ثابت override { أرجع 0; }
    bool is_خالص() override { أرجع true; }
};
class Ast_Jump_بلاک final : عام Ast_گرہ_Stmt {
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
    Ast_Jump_بلاک(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Jump_بلاک(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Jump_بلاک;
    صحيح instr_گنتی() ثابت override { أرجع 0; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    bool is_خالص() override;

خاص:
    bool getPurityRecurse() ثابت;
};
class AstJumpGo final : عام Ast_گرہ_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
عام:
    AstJumpGo(File_سطر* fl, Ast_Jump_بلاک* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ثابت حرف* broken() ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        أرجع blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isBrancher() ثابت override {
        أرجع true;  // SPECIAL: We don't process code after breaks
    }
    Ast_Jump_بلاک* blockp() ثابت { أرجع m_blockp; }
};
class AstLoop final : عام Ast_گرہ_Stmt {
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
    explicit AstLoop(File_سطر* fl, Ast_گرہ* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* thatp) ثابت override {
        أرجع m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    // ACCESSORS
    VOptionBool unroll() ثابت { أرجع m_unroll; }
    فراغ unroll(ثابت VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : عام Ast_گرہ_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
عام:
    AstLoopTest(File_سطر* fl, AstLoop* loopp, Ast_گرہ_اظہاریہ* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ثابت حرف* broken() ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isBrancher() ثابت override { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع 0; }
    // ACCESSORS
    AstLoop* loopp() ثابت { أرجع m_loopp; }
};
class AstMonitorOff final : عام Ast_گرہ_Stmt {
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
    bool is_خالص() override { أرجع false; }  // Though deleted before opt
    bool isOutputter() override { أرجع true; }  // Though deleted before opt
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_PLI; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        أرجع m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ثابت { أرجع m_off; }
};
class AstPExprClause final : عام Ast_گرہ_Stmt {
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
class Ast_Print_ویلا_Scale final : عام Ast_گرہ_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
عام:
    explicit Ast_Print_ویلا_Scale(File_سطر* fl)
        : ASTGEN_SUPER_Print_ویلا_Scale(fl) {}
    ASTGEN_MEMBERS_Ast_Print_ویلا_Scale;
    فراغ name(ثابت string& name) override { m_name = name; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { أرجع "$printtimescale"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_PLI; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
};
class AstRSBreak final : عام Ast_گرہ_Stmt {
    // randsequence break
عام:
    explicit AstRSBreak(File_سطر* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ثابت override { أرجع "break"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : عام Ast_گرہ_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
عام:
    AstRSProd(File_سطر* fl, ثابت string& name, Ast_گرہ* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        جوڑو_Portsp(portsp);
        جوڑو_Rulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
};
class Ast_RSProd_شے final : عام Ast_گرہ_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
عام:
    Ast_RSProd_شے(File_سطر* fl, ثابت string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProd_شے(fl)
        , m_name{name} {
        جوڑو_Argsp(argsp);
    }
    ASTGEN_MEMBERS_Ast_RSProd_شے;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    AstRSProd* prodp() ثابت { أرجع m_prodp; }
    فراغ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : عام Ast_گرہ_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
عام:
    AstRSProdList(File_سطر* fl, Ast_گرہ_اظہاریہ* weightp, Ast_گرہ* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        جوڑو_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    bool randJoin() ثابت { أرجع m_randJoin; }
    فراغ randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : عام Ast_گرہ_Stmt {
    // randsequence return
عام:
    explicit AstRSReturn(File_سطر* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ثابت override { أرجع "return"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : عام Ast_گرہ_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
عام:
    AstRSRule(File_سطر* fl, Ast_گرہ_اظہاریہ* weightp, AstRSProdList* prodlistsp, Ast_گرہ* بھار_Stmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        جوڑو_Prodlistsp(prodlistsp);
        جوڑو_بھار_Stmtsp(بھار_Stmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
};
class AstRandCase final : عام Ast_گرہ_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
عام:
    AstRandCase(File_سطر* fl, Ast_Case_شے* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        جوڑو_Itemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
};
class AstRandSequence final : عام Ast_گرہ_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_شروع;  // Name of start production, or "" to use first production
عام:
    AstRandSequence(File_سطر* fl, ثابت string& شروع, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_شروع{شروع} {
        جوڑو_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Block name
    فراغ name(ثابت string& name) override { m_name = name; }
    string شروع() ثابت VL_MT_STABLE { أرجع m_شروع; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    AstRSProd* prodp() ثابت { أرجع m_prodp; }
    فراغ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : عام Ast_گرہ_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstRelease(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : عام Ast_گرہ_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
عام:
    AstRepeat(File_سطر* fl, Ast_گرہ_اظہاریہ* countp, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ثابت override { أرجع false; }  // Not relevant - converted to FOR
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_BRANCH; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class AstReturn final : عام Ast_گرہ_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
عام:
    explicit AstReturn(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ثابت override { أرجع "return"; }
    bool isBrancher() ثابت override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : عام Ast_گرہ_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
عام:
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
        أرجع nullptr;
    }
    string verilogKwd() ثابت override { أرجع "$sformat"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع true; }
    bool is_خالص() override { أرجع true; }
    bool isOutputter() override { أرجع false; }
    مجازی bool clean_باہرلا() ثابت { أرجع false; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_PLI; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class AstSetuphold final : عام Ast_گرہ_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
عام:
    AstSetuphold(File_سطر* fl, Ast_گرہ_اظہاریہ* refevp, Ast_گرہ_اظہاریہ* dataevp,
                 Ast_گرہ_اظہاریہ* delrefp = nullptr, Ast_گرہ_اظہاریہ* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_ڈھیر_Trace_T final : عام Ast_گرہ_Stmt {
    // $stacktrace used as task
عام:
    explicit Ast_ڈھیر_Trace_T(File_سطر* fl)
        : ASTGEN_SUPER_ڈھیر_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_ڈھیر_Trace_T;
    string verilogKwd() ثابت override { أرجع "$stacktrace"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_Stmt_اظہاریہ final : عام Ast_گرہ_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
عام:
    Ast_Stmt_اظہاریہ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Stmt_اظہاریہ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_اظہاریہ;
    // cppcheck-suppress uselessOverride
    bool is_خالص() override { أرجع exprp()->is_خالص(); }
};
class AstStmtPragma final : عام Ast_گرہ_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
عام:
    AstStmtPragma(File_سطر* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : عام Ast_گرہ_Stmt {
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
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع 0; }  // Rarely executes
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { أرجع fileline() == samep->fileline(); }
    string emitVerilog() ثابت { أرجع m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ثابت { أرجع m_isFatal; }
};
class AstSystemT final : عام Ast_گرہ_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstSystemT(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ثابت override { أرجع "$system"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_ویلا_Format final : عام Ast_گرہ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
عام:
    Ast_ویلا_Format(File_سطر* fl, Ast_گرہ_اظہاریہ* unitsp, Ast_گرہ_اظہاریہ* precisionp, Ast_گرہ_اظہاریہ* suffixp,
                  Ast_گرہ_اظہاریہ* widthp)
        : ASTGEN_SUPER_ویلا_Format(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_Ast_ویلا_Format;
    string verilogKwd() ثابت override { أرجع "$timeformat"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    صحيح instr_گنتی() ثابت override { أرجع INSTR_گنتی_PLI; }
};
class AstTraceDecl final : عام Ast_گرہ_Stmt {
    // Trace point declaration
    // Separate from AstTraceInc; as a declaration can't be deleted
    // Parents:  {statement list}
    // Expression being traced - Moved to AstTraceInc by V3Trace
    // @astgen op1 := valuep : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_dtypeCallp: Optional[AstCCall] // Type init function call
    // @astgen ptr := m_dtypeDeclp: Optional[AstTraceDecl] // CCall TraceDecl which replaces this
    uint32_t m_کوڈ{std::numeric_limits<uint32_t>::max()};  // Trace identifier code
    uint32_t m_fidx{0};  // Trace function index
    ثابت string m_showname;  // Name of variable
    ثابت VNumRange m_ثنائی_ہندسہ_Range;  // Property of var the trace details
    ثابت VNumRange m_arrayRange;  // Property of var the trace details
    ثابت VVarType m_varType;  // Type of variable (for localparam vs. param)
    ثابت VDirection m_declDirection;  // Declared direction input/output etc
    ثابت bool m_وچ_Dtype_Func;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    صحيح m_کوڈ_Inc{0};  // Code increment for type
عام:
    AstTraceDecl(File_سطر* fl, ثابت string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_گرہ_اظہاریہ* valuep, ثابت VNumRange& ثنائی_ہندسہ_Range, ثابت VNumRange& arrayRange,
                 AstCCall* ثابت dtypeCallp, ثابت bool وچ_Dtype_Func)
        : ASTGEN_SUPER_TraceDecl(fl)
        , m_showname{showname}
        , m_ثنائی_ہندسہ_Range{ثنائی_ہندسہ_Range}
        , m_arrayRange{arrayRange}
        , m_varType{varp->varType()}
        , m_declDirection{varp->declDirection()}
        , m_وچ_Dtype_Func{وچ_Dtype_Func} {
        dtypeFrom(valuep);
        this->valuep(valuep);
        this->dtypeCallp(dtypeCallp);
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_گنتی() ثابت override { أرجع 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ثابت override VL_MT_STABLE { أرجع m_showname; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { أرجع true; }
    string showname() ثابت { أرجع m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t کوڈ() ثابت { أرجع m_کوڈ; }
    فراغ کوڈ(uint32_t کوڈ) { m_کوڈ = کوڈ; }
    bool کوڈ_Assigned() ثابت { أرجع m_کوڈ != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ثابت { أرجع m_fidx; }
    فراغ fidx(uint32_t fidx) { m_fidx = fidx; }
    فراغ کوڈ_Inc(uint32_t کوڈ_Inc) { m_کوڈ_Inc = کوڈ_Inc; }
    uint32_t کوڈ_Inc() ثابت {
        إذا (m_کوڈ_Inc) { أرجع m_کوڈ_Inc; }
        أرجع (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->چوڑائی_الفاظ()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ثابت VNumRange& ثنائی_ہندسہ_Range() ثابت { أرجع m_ثنائی_ہندسہ_Range; }
    ثابت VNumRange& arrayRange() ثابت { أرجع m_arrayRange; }
    VVarType varType() ثابت { أرجع m_varType; }
    VDirection declDirection() ثابت { أرجع m_declDirection; }
    AstCCall* dtypeCallp() ثابت { أرجع m_dtypeCallp; }
    فراغ dtypeCallp(AstCCall* ثابت callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ثابت { أرجع m_dtypeDeclp; }
    فراغ dtypeDeclp(AstTraceDecl* ثابت declp) { m_dtypeDeclp = declp; }
    bool وچ_Dtype_Func() ثابت { أرجع m_وچ_Dtype_Func; }
};
class AstTraceInc final : عام Ast_گرہ_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ثابت uint32_t m_ادھار_کوڈ;  // Trace code base value in function containing this AstTraceInc
    ثابت VTraceType m_traceType;  // Is this a const/full/incremental dump

عام:
    AstTraceInc(File_سطر* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t ادھار_کوڈ = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_ادھار_کوڈ{ادھار_کوڈ}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_درخت(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_گنتی() ثابت override { أرجع 10 + 2 * INSTR_گنتی_LD; }
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        أرجع declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool is_خالص() override { أرجع false; }
    AstTraceDecl* declp() ثابت { أرجع m_declp; }
    VTraceType traceType() ثابت { أرجع m_traceType; }
    uint32_t ادھار_کوڈ() ثابت { أرجع m_ادھار_کوڈ; }
};
class Ast_Trace_نکالو_اگیتر final : عام Ast_گرہ_Stmt {
عام:
    explicit Ast_Trace_نکالو_اگیتر(File_سطر* fl)
        : ASTGEN_SUPER_Trace_نکالو_اگیتر(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_نکالو_اگیتر;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { أرجع false; }
};
class Ast_Trace_ڈالو_اگیتر final : عام Ast_گرہ_Stmt {
    ثابت string m_اگیتر;  // Prefix to add to signal names
    ثابت VTrace_اگیتر_Type m_اگیتر_Type;  // Type of prefix being pushed
    ثابت صحيح m_بایاں;  // Array left index, or struct/union member count
    ثابت صحيح m_دایاں;  // Array right index
    ثابت bool m_quoted_اگیتر;  // Quote prefix name
عام:
    Ast_Trace_ڈالو_اگیتر(File_سطر* fl, ثابت string& اگیتر, VTrace_اگیتر_Type اگیتر_Type,
                       صحيح بایاں = 0, صحيح دایاں = 0, bool quoted_اگیتر = true)
        : ASTGEN_SUPER_Trace_ڈالو_اگیتر(fl)
        , m_اگیتر{اگیتر}
        , m_اگیتر_Type{اگیتر_Type}
        , m_بایاں{بایاں}
        , m_دایاں{دایاں}
        , m_quoted_اگیتر{quoted_اگیتر} {}
    ASTGEN_MEMBERS_Ast_Trace_ڈالو_اگیتر;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { أرجع false; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string اگیتر() ثابت { أرجع m_اگیتر; }
    VTrace_اگیتر_Type اگیتر_Type() ثابت { أرجع m_اگیتر_Type; }
    صحيح بایاں() ثابت { أرجع m_بایاں; }
    صحيح دایاں() ثابت { أرجع m_دایاں; }
    bool quoted_اگیتر() ثابت { أرجع m_quoted_اگیتر; }
};
class AstWait final : عام Ast_گرہ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
عام:
    AstWait(File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ثابت override { أرجع true; }
};
class AstWaitFork final : عام Ast_گرہ_Stmt {
    // A "wait fork" statement
عام:
    explicit AstWaitFork(File_سطر* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ثابت override { أرجع true; }
    string verilogKwd() ثابت override { أرجع "wait fork"; }
};

// === AstNodeAssign ===
class Ast_سونپو final : عام Ast_گرہ_سونپو {
عام:
    Ast_سونپو(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
              Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_سونپو(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_سونپو;
    Ast_گرہ_سونپو* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        أرجع new Ast_سونپو{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_سونپو_Compound final : عام Ast_گرہ_سونپو {
    // Compound assignments (+=, -=, *=, ...)
عام:
    تعداد operation : uint8_t {
        جوڑو_2,
        And,
        Div,
        ModDiv,
        Mul,
        Or,
        کھسکاؤ_L,
        کھسکاؤ_R,
        کھسکاؤ_RS,
        Sub,
        Xor,
    };

خاص:
    operation m_operation;

عام:
    Ast_سونپو_Compound(Ast_سونپو_Compound::operation operation, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp,
                      Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_سونپو_Compound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_سونپو_Compound;
    Ast_گرہ_سونپو* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        أرجع new Ast_سونپو_Compound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { أرجع m_operation; }
};
class Ast_سونپو_Cont final : عام Ast_گرہ_سونپو {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
عام:
    Ast_سونپو_Cont(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
                  Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_سونپو_Cont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_سونپو_Cont;
    Ast_گرہ_سونپو* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        أرجع new Ast_سونپو_Cont{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_سونپو_Dly final : عام Ast_گرہ_سونپو {
عام:
    Ast_سونپو_Dly(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
                 Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_سونپو_Dly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_سونپو_Dly;
    Ast_گرہ_سونپو* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        أرجع new Ast_سونپو_Dly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    string verilogKwd() ثابت override { أرجع "<="; }
};
class Ast_سونپو_Force final : عام Ast_گرہ_سونپو {
    // Procedural 'force' statement
عام:
    Ast_سونپو_Force(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_سونپو_Force(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_سونپو_Force;
    Ast_گرہ_سونپو* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        أرجع new Ast_سونپو_Force{fileline(), lhsp, rhsp};
    }
};
class Ast_سونپو_W final : عام Ast_گرہ_سونپو {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
عام:
    Ast_سونپو_W(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
               Ast_گرہ* timingControlp = nullptr)
        : ASTGEN_SUPER_سونپو_W(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_سونپو_W;
    Ast_گرہ_سونپو* cloneType(Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) override {
        Ast_گرہ* ثابت controlp = timingControlp() ? timingControlp()->clone_درخت(false) : nullptr;
        أرجع new Ast_سونپو_W{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ثابت override {
        أرجع timingControlp() || lhsp()->exists([](ثابت Ast_گرہ_Var_Ref* refp) {
            أرجع refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ثابت;
};

// === AstNodeBlock ===
class AstBegin final : عام Ast_گرہ_بلاک {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ثابت bool m_implied : 1;  // Not inserted by user
عام:
    AstBegin(File_سطر* fl, ثابت string& name, Ast_گرہ* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        جوڑو_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ سیٹ_Need_Process() { m_needProcess = true; }
    bool needProcess() ثابت { أرجع m_needProcess; }
    bool implied() ثابت { أرجع m_implied; }
};
class AstFork final : عام Ast_گرہ_بلاک {
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
    bool m_فوری_شروع = false;  // Fork starts before its parent blocks or exits

عام:
    AstFork(File_سطر* fl, VJoinType joinType, ثابت string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstFork* ثابت asamep = VN_DBG_AS(samep, Fork);
        أرجع joinType() == asamep->joinType() && فوری_شروع() == asamep->فوری_شروع();
    }
    bool isTimingControl() ثابت override { أرجع !joinType().joinNone(); }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VJoinType joinType() ثابت { أرجع m_joinType; }
    bool فوری_شروع() ثابت { أرجع m_فوری_شروع; }
    فراغ فوری_شروع(bool flag) { m_فوری_شروع = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : عام Ast_گرہ_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

عام:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_سطر* fl, Ast_گرہ* propp, Ast_گرہ* passsp, Ast_گرہ* failsp, VAssertType type,
              VAssertDirectiveType directive, ثابت string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        جوڑو_Failsp(failsp);
    }
    string verilogKwd() ثابت override {
        أرجع directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : عام Ast_گرہ_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
عام:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_سطر* fl, Ast_گرہ* propp, Ast_گرہ* passsp, Ast_گرہ* failsp,
                       ثابت string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::اندرونی,
                                       VAssertDirectiveType::INTRINSIC, name) {
        جوڑو_Failsp(failsp);
    }
    string verilogKwd() ثابت override { أرجع "assert"; }
};
class AstCover final : عام Ast_گرہ_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
عام:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_سطر* fl, Ast_گرہ* propp, Ast_گرہ* stmtsp, VAssertType type,
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
class AstRestrict final : عام Ast_گرہ_Cover_Or_Assert {
عام:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_سطر* fl, Ast_گرہ* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::اندرونی,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ثابت override { أرجع "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : عام Ast_گرہ_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
عام:
    AstConstraintForeach(File_سطر* fl, Ast_Foreach_سرخی* headerp, Ast_گرہ* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ثابت { أرجع m_soft; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstForeach final : عام Ast_گرہ_Foreach {
عام:
    AstForeach(File_سطر* fl, Ast_Foreach_سرخی* headerp, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : عام Ast_گرہ_If {
عام:
    AstConstraintIf(File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* thensp, Ast_گرہ* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : عام Ast_گرہ_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ترجیح_Pragma = false;  // priority case
عام:
    AstIf(File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* thensp = nullptr, Ast_گرہ* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ثابت { أرجع m_uniquePragma; }
    فراغ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ثابت { أرجع m_unique0Pragma; }
    فراغ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ترجیح_Pragma() ثابت { أرجع m_ترجیح_Pragma; }
    فراغ ترجیح_Pragma(bool flag) { m_ترجیح_Pragma = flag; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : عام Ast_گرہ_Read_Write_Mem {
عام:
    AstReadMem(File_سطر* fl, bool hex, Ast_گرہ_اظہاریہ* filenamep, Ast_گرہ_اظہاریہ* memp,
               Ast_گرہ_اظہاریہ* lsbp, Ast_گرہ_اظہاریہ* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ثابت override { أرجع (isHex() ? "$readmemh" : "$readmemb"); }
    ثابت حرف* cFuncPrefixp() ثابت override { أرجع "VL_READMEM_"; }
};
class AstWriteMem final : عام Ast_گرہ_Read_Write_Mem {
عام:
    AstWriteMem(File_سطر* fl, bool hex, Ast_گرہ_اظہاریہ* filenamep, Ast_گرہ_اظہاریہ* memp,
                Ast_گرہ_اظہاریہ* lsbp, Ast_گرہ_اظہاریہ* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ثابت override { أرجع (isHex() ? "$writememh" : "$writememb"); }
    ثابت حرف* cFuncPrefixp() ثابت override { أرجع "VL_WRITEMEM_"; }
};

#جے_انت  // Guard
