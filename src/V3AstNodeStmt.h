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

#यदि_अपरिभाषितम् VERILATOR_V3ASTNODESTMT_H_
#परिभाषय VERILATOR_V3ASTNODESTMT_H_

#यदि_अपरिभाषितम् VERILATOR_V3AST_H_
#दोषः "Use V3Ast.h as the include"
#अन्तर्भाव "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#परिभाषय VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#यदि_अन्तः

// === Abstract base node types (AstNode*) =====================================

class Ast_ग्रन्थिः_Stmt VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // Procedural statement
रक्षित:
    Ast_ग्रन्थिः_Stmt(VNType t, File_पङ्क्तिः* fl)
        : Ast_ग्रन्थिः{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Stmt;
    // METHODS
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
};
class Ast_ग्रन्थिः_नियोजय VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
रक्षित:
    Ast_ग्रन्थिः_नियोजय(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp,
                  Ast_ग्रन्थिः* timingControlp = nullptr)
        : Ast_ग्रन्थिः_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_नियोजय;
    // Clone single node, just get same type back.
    शून्य dump(std::ostream& str) ध्रुव override;
    आभास Ast_ग्रन्थिः_नियोजय* cloneType(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) = 0;
    bool hasDType() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
    आभास bool cleanRhs() ध्रुव { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    string verilogKwd() ध्रुव override { प्रत्यागम "="; }
    bool isTimingControl() ध्रुव override { प्रत्यागम timingControlp(); }
};
class Ast_ग्रन्थिः_खण्डम् VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
रक्षित:
    Ast_ग्रन्थिः_खण्डम्(VNType t, File_पङ्क्तिः* fl, ध्रुव string& name)
        : Ast_ग्रन्थिः_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_खण्डम्;
    bool maybe_Pointed_गन्तव्यम्() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Block name
    शून्य name(ध्रुव string& name) override { m_name = name; }
    bool unnamed() ध्रुव { प्रत्यागम m_unnamed; }
};
class Ast_ग्रन्थिः_Cover_Or_Assert VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    ध्रुव VAssertType m_userType;  // Assertion/cover type for user enable/disable
    ध्रुव VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_तात्कालिकम् = false;  // Immediate assert (may differ from userType being immediate)

सार्वजनिक:
    Ast_ग्रन्थिः_Cover_Or_Assert(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः* propp, Ast_ग्रन्थिः* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ध्रुव string& name = "")
        : Ast_ग्रन्थिः_Stmt{t, fl}
        , m_name{name}
        , m_userType{userType}
        , m_directive{directive} {
        this->propp(propp);
        सङ्कलय_Passsp(passsp);
        m_तात्कालिकम् = m_userType.अन्तर्भवति_Any(VAssertType::SIMPLE_तात्कालिकम्
                                             | VAssertType::OBSERVED_DEFERRED_तात्कालिकम्
                                             | VAssertType::FINAL_DEFERRED_तात्कालिकम्)
                      || m_userType == VAssertType::आन्तरिकम्;
    }
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Cover_Or_Assert;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Var name
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम samep->name() == name(); }
    शून्य name(ध्रुव string& name) override { m_name = name; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string type() { प्रत्यागम ""; }
    VAssertType userType() ध्रुव VL_MT_SAFE { प्रत्यागम m_userType; }
    VAssertDirectiveType directive() ध्रुव { प्रत्यागम m_directive; }
    bool तात्कालिकम्() ध्रुव { प्रत्यागम m_तात्कालिकम्; }
    शून्य तात्कालिकम्(bool flag) { m_तात्कालिकम् = flag; }
    bool senFromAlways() ध्रुव VL_MT_STABLE { प्रत्यागम m_senFromAlways; }
    शून्य senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_ग्रन्थिः_Foreach VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
सार्वजनिक:
    Ast_ग्रन्थिः_Foreach(VNType t, File_पङ्क्तिः* fl, Ast_Foreach_शीर्षकम्* headerp, Ast_ग्रन्थिः* bodyp)
        : Ast_ग्रन्थिः_Stmt(t, fl) {
        this->headerp(headerp);
        सङ्कलय_Bodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Foreach;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class Ast_ग्रन्थिः_If VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
रक्षित:
    Ast_ग्रन्थिः_If(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः* thensp, Ast_ग्रन्थिः* elsesp)
        : Ast_ग्रन्थिः_Stmt{t, fl} {
        this->condp(condp);
        सङ्कलय_Thensp(thensp);
        सङ्कलय_Elsesp(elsesp);
        isBoundsCheck(false);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_If;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isGateDedupable() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    शून्य branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ध्रुव { प्रत्यागम m_branchPred; }
    शून्य isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ध्रुव { प्रत्यागम m_isBoundsCheck; }
};
class Ast_ग्रन्थिः_Read_Write_Mem VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ध्रुव bool m_isHex;  // readmemh, not readmemb
सार्वजनिक:
    Ast_ग्रन्थिः_Read_Write_Mem(VNType t, File_पङ्क्तिः* fl, bool hex, Ast_ग्रन्थिः_व्यञ्जकम्* filenamep,
                        Ast_ग्रन्थिः_व्यञ्जकम्* memp, Ast_ग्रन्थिः_व्यञ्जकम्* lsbp, Ast_ग्रन्थिः_व्यञ्जकम्* msbp)
        : Ast_ग्रन्थिः_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Read_Write_Mem;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम isHex() == VN_DBG_AS(samep, ग्रन्थिः_Read_Write_Mem)->isHex();
    }
    bool isHex() ध्रुव { प्रत्यागम m_isHex; }
    आभास ध्रुव वर्ण* cFuncPrefixp() ध्रुव = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : सार्वजनिक Ast_ग्रन्थिः {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
सार्वजनिक:
    AstCaseItem(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condsp, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        सङ्कलय_Condsp(condsp);
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() + INSTR_गणनम्_BRANCH; }
    bool isDefault() ध्रुव { प्रत्यागम condsp() == nullptr; }
};

class Ast_Foreach_शीर्षकम् final : सार्वजनिक Ast_ग्रन्थिः {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
सार्वजनिक:
    Ast_Foreach_शीर्षकम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः* elementsp)
        : ASTGEN_SUPER_Foreach_शीर्षकम्(fl) {
        this->fromp(fromp);
        सङ्कलय_Elementsp(elementsp);
    }
    ASTGEN_MEMBERS_Ast_Foreach_शीर्षकम्;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool maybe_Pointed_गन्तव्यम्() ध्रुव override VL_MT_SAFE { प्रत्यागम false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

सार्वजनिक:
    AstAssertCtl(File_पङ्क्तिः* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_ग्रन्थिः_व्यञ्जकम्* levelp = nullptr, Ast_ग्रन्थिः_व्यञ्जकम्* itemsp = nullptr);
    AstAssertCtl(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* controlTypep, Ast_ग्रन्थिः_व्यञ्जकम्* assertTypesp = nullptr,
                 Ast_ग्रन्थिः_व्यञ्जकम्* directiveTypep = nullptr, Ast_ग्रन्थिः_व्यञ्जकम्* levelp = nullptr,
                 Ast_ग्रन्थिः_व्यञ्जकम्* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ध्रुव override { प्रत्यागम m_ctlType.ascii(); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    VAssertCtlType ctlType() ध्रुव { प्रत्यागम m_ctlType; }
    शून्य ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ध्रुव { प्रत्यागम m_assertTypes; }
    शून्य assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ध्रुव { प्रत्यागम m_directiveTypes; }
    शून्य directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
};
class AstBreak final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
सार्वजनिक:
    explicit AstBreak(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ध्रुव override { प्रत्यागम "break"; }
    bool isBrancher() ध्रुव override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
सार्वजनिक:
    AstCAwait(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_Sen_वृक्षः* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isTimingControl() ध्रुव override { प्रत्यागम true; }
    Ast_Sen_वृक्षः* sentreep() ध्रुव { प्रत्यागम m_sentreep; }
    शून्य clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
सार्वजनिक:
    AstCLocalScope(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
सार्वजनिक:
    AstCReturn(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstCStmt final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ध्रुव VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    स्थिर AstCStmt* profExecSection(File_पङ्क्तिः* flp, ध्रुव std::string& section, bool प्रक्षिप) {
        // Compute the label
        std::string label;
        यदि (v3Global.opt.hierChild()) {
            label += v3Global.opt.शिखरम्_Module() + ":";
        } अन्यथा यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += प्रक्षिप ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        यदि (प्रक्षिप) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += प्रक्षिप ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        प्रत्यागम new AstCStmt{flp, प्रक्षिप ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

सार्वजनिक:
    explicit AstCStmt(File_पङ्क्तिः* fl, ध्रुव std::string& text = "",
                      ध्रुव VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        यदि (!text.रिक्तानि()) सङ्कलय(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    VCStmtType stmtType() ध्रुव { प्रत्यागम m_stmtType; }
    // Add some text, or a node to this statement
    शून्य सङ्कलय(ध्रुव std::string& text) { सङ्कलय_Nodesp(new AstText{fileline(), text}); }
    शून्य सङ्कलय(Ast_ग्रन्थिः* nodep) { सङ्कलय_Nodesp(nodep); }
    // Static factory methods for specific cases
    स्थिर AstCStmt* prof_Exec_Section_प्रक्षिप(File_पङ्क्तिः* flp, ध्रुव std::string& section) {
        प्रत्यागम profExecSection(flp, section, true);
    }
    स्थिर AstCStmt* prof_Exec_Section_निष्कासय(File_पङ्क्तिः* flp, ध्रुव std::string& section) {
        प्रत्यागम profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ध्रुव bool m_fromDollarC;  // Is from source '$c', emit decoration
सार्वजनिक:
    explicit AstCStmtUser(File_पङ्क्तिः* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    bool fromDollarC() ध्रुव { प्रत्यागम m_fromDollarC; }
    // Add some text, or a node to this statement
    शून्य सङ्कलय(ध्रुव std::string& text) { सङ्कलय_Nodesp(new AstText{fileline(), text}); }
    शून्य सङ्कलय(Ast_ग्रन्थिः* nodep) { सङ्कलय_Nodesp(nodep); }
};
class AstCase final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Case statement
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstCaseItem]
    // @astgen op3 := notParallelp : List[AstNode] // assertion code for non-full case's
    VCaseType m_caseType;  // 0=case, 1=casex, 2=casez
    bool m_पूर्णम्_Pragma = false;  // Synthesis full_case
    bool m_parallelPragma = false;  // Synthesis parallel_case
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_प्राथमिकता_Pragma = false;  // priority case
सार्वजनिक:
    AstCase(File_पङ्क्तिः* fl, VCaseType caseType, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        सङ्कलय_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    string verilogKwd() ध्रुव override { प्रत्यागम casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ध्रुव { प्रत्यागम m_caseType == VCaseType::CT_CASEX; }
    bool casez() ध्रुव { प्रत्यागम m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ध्रुव { प्रत्यागम m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ध्रुव { प्रत्यागम m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ध्रुव { प्रत्यागम m_caseType == VCaseType::CT_CASE; }
    शून्य case_Inside_समुच्चयः() { m_caseType = VCaseType::CT_CASEINSIDE; }
    शून्य case_Matches_समुच्चयः() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool पूर्णम्_Pragma() ध्रुव { प्रत्यागम m_पूर्णम्_Pragma; }
    शून्य पूर्णम्_Pragma(bool flag) { m_पूर्णम्_Pragma = flag; }
    bool parallelPragma() ध्रुव { प्रत्यागम m_parallelPragma; }
    शून्य parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ध्रुव { प्रत्यागम m_uniquePragma; }
    शून्य uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ध्रुव { प्रत्यागम m_unique0Pragma; }
    शून्य unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool प्राथमिकता_Pragma() ध्रुव { प्रत्यागम m_प्राथमिकता_Pragma; }
    शून्य प्राथमिकता_Pragma(bool flag) { m_प्राथमिकता_Pragma = flag; }
    string pragmaString() ध्रुव;
};
class AstComment final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Some comment to put into the output stream
    ध्रुव string m_name;  // Text of comment
    ध्रुव bool m_showAt;  // Show "at <fileline>"
सार्वजनिक:
    AstComment(File_पङ्क्तिः* fl, ध्रुव string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Text
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम true; }  // Ignore name in comments
    आभास bool showAt() ध्रुव { प्रत्यागम m_showAt; }
};
class AstConstraintBefore final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
सार्वजनिक:
    AstConstraintBefore(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhssp, Ast_ग्रन्थिः_व्यञ्जकम्* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        सङ्कलय_Lhssp(lhssp);
        सङ्कलय_Rhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class Ast_Constraint_व्यञ्जकम् final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
सार्वजनिक:
    Ast_Constraint_व्यञ्जकम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_Constraint_व्यञ्जकम्(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_व्यञ्जकम्;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isDisableSoft() ध्रुव { प्रत्यागम m_isDisableSoft; }
    शून्य isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ध्रुव { प्रत्यागम m_isSoft; }
    शून्य isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
सार्वजनिक:
    AstConstraintUnique(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        सङ्कलय_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstContinue final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
सार्वजनिक:
    explicit AstContinue(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ध्रुव override { प्रत्यागम "continue"; }
    bool isBrancher() ध्रुव override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

सार्वजनिक:
    AstCoverInc(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_Cover_Decl* declp, ध्रुव bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 1 + 2 * INSTR_गणनम्_LD; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstCoverInc* ध्रुव otherp = VN_DBG_AS(samep, CoverInc);
        प्रत्यागम declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    Ast_ग्रन्थिः_Cover_Decl* declp() ध्रुव { प्रत्यागम m_declp; }  // Where defined
    शून्य declp(Ast_ग्रन्थिः_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ध्रुव { प्रत्यागम m_isDuplicate; }
};
class AstDeassign final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
सार्वजनिक:
    AstDeassign(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ध्रुव bool m_is_आवृत्तिः_Delay;  // True if it is a cycle delay
सार्वजनिक:
    AstDelay(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, bool is_आवृत्तिः_Delay)
        : ASTGEN_SUPER_Delay(fl)
        , m_is_आवृत्तिः_Delay{is_आवृत्तिः_Delay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isTimingControl() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ध्रुव { प्रत्यागम m_timeunit; }
    bool is_आवृत्तिः_Delay() ध्रुव { प्रत्यागम m_is_आवृत्तिः_Delay; }
    bool isRangeDelay() ध्रुव { प्रत्यागम m_is_आवृत्तिः_Delay && rhsp(); }
    bool isUnbounded() ध्रुव { प्रत्यागम isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    शून्य fallDelay(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fallDelayp) { rhsp(fallDelayp); }
    Ast_ग्रन्थिः_व्यञ्जकम्* fallDelay() ध्रुव { प्रत्यागम m_is_आवृत्तिः_Delay ? nullptr : rhsp(); }
};
class AstDisable final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
सार्वजनिक:
    AstDisable(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ध्रुव वर्ण* broken() ध्रुव override;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य targetp(Ast_ग्रन्थिः* nodep) { m_targetp = nodep; }
    Ast_ग्रन्थिः* targetp() ध्रुव { प्रत्यागम m_targetp; }
    bool isBrancher() ध्रुव override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // A "disable fork" statement
सार्वजनिक:
    explicit AstDisableFork(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ध्रुव override { प्रत्यागम "disable fork"; }
};
class AstDisplay final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

सार्वजनिक:
    AstDisplay(File_पङ्क्तिः* fl, VDisplayType dispType, ध्रुव string& text, Ast_ग्रन्थिः_व्यञ्जकम्* filep,
               Ast_ग्रन्थिः_व्यञ्जकम्* exprsp, वर्ण missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_पङ्क्तिः* fl, VDisplayType dispType, Ast_ग्रन्थिः_व्यञ्जकम्* filep, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp,
               वर्ण missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ध्रुव newp
            = new AstSFormatF{fl, AstSFormatF::व्यञ्जकम्_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!fmtp());
        प्रत्यागम nullptr;
    }
    string verilogKwd() ध्रुव override {
        प्रत्यागम (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { प्रत्यागम true; }  // SPECIAL: $display makes output
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
    VDisplayType displayType() ध्रुव { प्रत्यागम m_displayType; }
    शून्य displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool सङ्कलय_Newline() ध्रुव { प्रत्यागम displayType().सङ्कलय_Newline(); }
};
class AstDumpCtl final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ध्रुव VDumpCtlType m_ctlType;  // Type of operation
सार्वजनिक:
    AstDumpCtl(File_पङ्क्तिः* fl, VDumpCtlType ctlType, Ast_ग्रन्थिः_व्यञ्जकम्* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ध्रुव override { प्रत्यागम ctlType().ascii(); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    आभास bool clean_बहिर्गमनम्() ध्रुव { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    VDumpCtlType ctlType() ध्रुव { प्रत्यागम m_ctlType; }
};
class AstEventControl final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
सार्वजनिक:
    AstEventControl(File_पङ्क्तिः* fl, Ast_Sen_वृक्षः* sentreep, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ध्रुव override { प्रत्यागम "@(%l) %r"; }
    bool isTimingControl() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
};
class AstExecGraph final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
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
    V3Graph* ध्रुव m_depGraphp;  // contains ExecMTask vertices
    ध्रुव string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

सार्वजनिक:
    AstExecGraph(File_पङ्क्तिः* fl, ध्रुव string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    शून्य cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ध्रुव वर्ण* broken() ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
    V3Graph* depGraphp() { प्रत्यागम m_depGraphp; }
    ध्रुव V3Graph* depGraphp() ध्रुव { प्रत्यागम m_depGraphp; }
    AstCFunc* runfuncp() ध्रुव { प्रत्यागम m_runfuncp; }
    शून्य runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
सार्वजनिक:
    AstFClose(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ध्रुव override { प्रत्यागम "$fclose"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstFFlush final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
सार्वजनिक:
    AstFFlush(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ध्रुव override { प्रत्यागम "$fflush"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstFinish final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
सार्वजनिक:
    explicit AstFinish(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { प्रत्यागम true; }  // SPECIAL: $display makes output
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }  // Rarely executes
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम fileline() == samep->fileline(); }
};
class AstFinishFork final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // $finish in fork
सार्वजनिक:
    explicit AstFinishFork(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { प्रत्यागम true; }  // SPECIAL: $display makes output
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }  // Rarely executes
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम fileline() == samep->fileline(); }
};
class AstFireEvent final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ध्रुव bool m_delayed;  // Delayed (->>) vs non-delayed (->)
सार्वजनिक:
    AstFireEvent(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isDelayed() ध्रुव { प्रत्यागम m_delayed; }
};
class AstInitialAutomaticStmt final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
सार्वजनिक:
    AstInitialAutomaticStmt(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool is_विशुद्धम्() override { प्रत्यागम true; }
};
class AstInitialStaticStmt final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
सार्वजनिक:
    AstInitialStaticStmt(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool is_विशुद्धम्() override { प्रत्यागम true; }
};
class Ast_Jump_खण्डम् final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
सार्वजनिक:
    // After construction must call ->labelp to associate with appropriate label
    Ast_Jump_खण्डम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_Jump_खण्डम्(fl) {
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Jump_खण्डम्;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool maybe_Pointed_गन्तव्यम्() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
    bool is_विशुद्धम्() override;

गुप्त:
    bool getPurityRecurse() ध्रुव;
};
class AstJumpGo final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
सार्वजनिक:
    AstJumpGo(File_पङ्क्तिः* fl, Ast_Jump_खण्डम्* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ध्रुव वर्ण* broken() ध्रुव override;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isBrancher() ध्रुव override {
        प्रत्यागम true;  // SPECIAL: We don't process code after breaks
    }
    Ast_Jump_खण्डम्* blockp() ध्रुव { प्रत्यागम m_blockp; }
};
class AstLoop final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
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
सार्वजनिक:
    explicit AstLoop(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* thatp) ध्रुव override {
        प्रत्यागम m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    bool maybe_Pointed_गन्तव्यम्() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
    // ACCESSORS
    VOptionBool unroll() ध्रुव { प्रत्यागम m_unroll; }
    शून्य unroll(ध्रुव VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
सार्वजनिक:
    AstLoopTest(File_पङ्क्तिः* fl, AstLoop* loopp, Ast_ग्रन्थिः_व्यञ्जकम्* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ध्रुव वर्ण* broken() ध्रुव override;
    शून्य dump(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isBrancher() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    // ACCESSORS
    AstLoop* loopp() ध्रुव { प्रत्यागम m_loopp; }
};
class AstMonitorOff final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    ध्रुव bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

सार्वजनिक:
    AstMonitorOff(File_पङ्क्तिः* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string verilogKwd() ध्रुव override { प्रत्यागम m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }  // Though deleted before opt
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }  // Though deleted before opt
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // Though deleted before opt
    bool isOutputter() override { प्रत्यागम true; }  // Though deleted before opt
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ध्रुव { प्रत्यागम m_off; }
};
class AstPExprClause final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    ध्रुव bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ध्रुव bool m_vacuous;  // Pass is vacuous

सार्वजनिक:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_पङ्क्तिः* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool pass() ध्रुव { प्रत्यागम m_pass; }
    bool vacuous() ध्रुव { प्रत्यागम m_vacuous; }
};
class AstPrintTimeScale final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    explicit AstPrintTimeScale(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Var name
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string verilogKwd() ध्रुव override { प्रत्यागम "$printtimescale"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ध्रुव { प्रत्यागम m_timeunit; }
};
class AstRSBreak final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // randsequence break
सार्वजनिक:
    explicit AstRSBreak(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ध्रुव override { प्रत्यागम "break"; }
    bool isBrancher() ध्रुव override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
सार्वजनिक:
    AstRSProd(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        सङ्कलय_Portsp(portsp);
        सङ्कलय_Rulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybe_Pointed_गन्तव्यम्() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
};
class AstRSProdItem final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
सार्वजनिक:
    AstRSProdItem(File_पङ्क्तिः* fl, ध्रुव string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        सङ्कलय_Argsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    AstRSProd* prodp() ध्रुव { प्रत्यागम m_prodp; }
    शून्य prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
सार्वजनिक:
    AstRSProdList(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* weightp, Ast_ग्रन्थिः* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        सङ्कलय_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    bool randJoin() ध्रुव { प्रत्यागम m_randJoin; }
    शून्य randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // randsequence return
सार्वजनिक:
    explicit AstRSReturn(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ध्रुव override { प्रत्यागम "return"; }
    bool isBrancher() ध्रुव override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
सार्वजनिक:
    AstRSRule(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* weightp, AstRSProdList* prodlistsp, Ast_ग्रन्थिः* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        सङ्कलय_Prodlistsp(prodlistsp);
        सङ्कलय_Weight_Stmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
};
class AstRandCase final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
सार्वजनिक:
    AstRandCase(File_पङ्क्तिः* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        सङ्कलय_Itemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
};
class AstRandSequence final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
सार्वजनिक:
    AstRandSequence(File_पङ्क्तिः* fl, ध्रुव string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        सङ्कलय_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Block name
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string start() ध्रुव VL_MT_STABLE { प्रत्यागम m_start; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    AstRSProd* prodp() ध्रुव { प्रत्यागम m_prodp; }
    शून्य prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
सार्वजनिक:
    AstRelease(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
सार्वजनिक:
    AstRepeat(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* countp, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }  // Not relevant - converted to FOR
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstReturn final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
सार्वजनिक:
    explicit AstReturn(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ध्रुव override { प्रत्यागम "return"; }
    bool isBrancher() ध्रुव override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
सार्वजनिक:
    AstSFormat(File_पङ्क्तिः* fl, bool optionalFormat, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp,
               वर्ण missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ध्रुव newp
            = new AstSFormatF{fl, AstSFormatF::व्यञ्जकम्_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_पङ्क्तिः* fl, AstSFormatF* fmtp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!fmtp());
        प्रत्यागम nullptr;
    }
    string verilogKwd() ध्रुव override { प्रत्यागम "$sformat"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम true; }
    bool is_विशुद्धम्() override { प्रत्यागम true; }
    bool isOutputter() override { प्रत्यागम false; }
    आभास bool clean_बहिर्गमनम्() ध्रुव { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstSetuphold final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
सार्वजनिक:
    AstSetuphold(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* refevp, Ast_ग्रन्थिः_व्यञ्जकम्* dataevp,
                 Ast_ग्रन्थिः_व्यञ्जकम्* delrefp = nullptr, Ast_ग्रन्थिः_व्यञ्जकम्* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class Ast_राशिः_Trace_T final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // $stacktrace used as task
सार्वजनिक:
    explicit Ast_राशिः_Trace_T(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_राशिः_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_राशिः_Trace_T;
    string verilogKwd() ध्रुव override { प्रत्यागम "$stacktrace"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class Ast_Stmt_व्यञ्जकम् final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    Ast_Stmt_व्यञ्जकम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_Stmt_व्यञ्जकम्(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_व्यञ्जकम्;
    // cppcheck-suppress uselessOverride
    bool is_विशुद्धम्() override { प्रत्यागम exprp()->is_विशुद्धम्(); }
};
class AstStmtPragma final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
सार्वजनिक:
    AstStmtPragma(File_पङ्क्तिः* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    ध्रुव bool m_isFatal;  // $fatal not $stop
सार्वजनिक:
    AstStop(File_पङ्क्तिः* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { प्रत्यागम true; }  // SPECIAL: $display makes output
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }  // Rarely executes
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम fileline() == samep->fileline(); }
    string emitVerilog() ध्रुव { प्रत्यागम m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ध्रुव { प्रत्यागम m_isFatal; }
};
class AstSystemT final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
सार्वजनिक:
    AstSystemT(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ध्रुव override { प्रत्यागम "$system"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstTimeFormat final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
सार्वजनिक:
    AstTimeFormat(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* unitsp, Ast_ग्रन्थिः_व्यञ्जकम्* precisionp, Ast_ग्रन्थिः_व्यञ्जकम्* suffixp,
                  Ast_ग्रन्थिः_व्यञ्जकम्* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() ध्रुव override { प्रत्यागम "$timeformat"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
};
class AstTraceDecl final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Trace point declaration
    // Separate from AstTraceInc; as a declaration can't be deleted
    // Parents:  {statement list}
    // Expression being traced - Moved to AstTraceInc by V3Trace
    // @astgen op1 := valuep : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_dtypeCallp: Optional[AstCCall] // Type init function call
    // @astgen ptr := m_dtypeDeclp: Optional[AstTraceDecl] // CCall TraceDecl which replaces this
    uint32_t m_संहिता{std::numeric_limits<uint32_t>::max()};  // Trace identifier code
    uint32_t m_fidx{0};  // Trace function index
    ध्रुव string m_showname;  // Name of variable
    ध्रुव VNumRange m_bitRange;  // Property of var the trace details
    ध्रुव VNumRange m_arrayRange;  // Property of var the trace details
    ध्रुव VVarType m_varType;  // Type of variable (for localparam vs. param)
    ध्रुव VDirection m_declDirection;  // Declared direction input/output etc
    ध्रुव bool m_अन्तर्_Dtype_Func;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    पूर्णाङ्क m_संहिता_Inc{0};  // Code increment for type
सार्वजनिक:
    AstTraceDecl(File_पङ्क्तिः* fl, ध्रुव string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_ग्रन्थिः_व्यञ्जकम्* valuep, ध्रुव VNumRange& bitRange, ध्रुव VNumRange& arrayRange,
                 AstCCall* ध्रुव dtypeCallp, ध्रुव bool अन्तर्_Dtype_Func)
        : ASTGEN_SUPER_TraceDecl(fl)
        , m_showname{showname}
        , m_bitRange{bitRange}
        , m_arrayRange{arrayRange}
        , m_varType{varp->varType()}
        , m_declDirection{varp->declDirection()}
        , m_अन्तर्_Dtype_Func{अन्तर्_Dtype_Func} {
        dtypeFrom(valuep);
        this->valuep(valuep);
        this->dtypeCallp(dtypeCallp);
    }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_showname; }
    bool maybe_Pointed_गन्तव्यम्() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
    bool hasDType() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम true; }
    string showname() ध्रुव { प्रत्यागम m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t संहिता() ध्रुव { प्रत्यागम m_संहिता; }
    शून्य संहिता(uint32_t संहिता) { m_संहिता = संहिता; }
    bool संहिता_Assigned() ध्रुव { प्रत्यागम m_संहिता != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ध्रुव { प्रत्यागम m_fidx; }
    शून्य fidx(uint32_t fidx) { m_fidx = fidx; }
    शून्य संहिता_Inc(uint32_t संहिता_Inc) { m_संहिता_Inc = संहिता_Inc; }
    uint32_t संहिता_Inc() ध्रुव {
        यदि (m_संहिता_Inc) { प्रत्यागम m_संहिता_Inc; }
        प्रत्यागम (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->विस्तारः_शब्दाः()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ध्रुव VNumRange& bitRange() ध्रुव { प्रत्यागम m_bitRange; }
    ध्रुव VNumRange& arrayRange() ध्रुव { प्रत्यागम m_arrayRange; }
    VVarType varType() ध्रुव { प्रत्यागम m_varType; }
    VDirection declDirection() ध्रुव { प्रत्यागम m_declDirection; }
    AstCCall* dtypeCallp() ध्रुव { प्रत्यागम m_dtypeCallp; }
    शून्य dtypeCallp(AstCCall* ध्रुव callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ध्रुव { प्रत्यागम m_dtypeDeclp; }
    शून्य dtypeDeclp(AstTraceDecl* ध्रुव declp) { m_dtypeDeclp = declp; }
    bool अन्तर्_Dtype_Func() ध्रुव { प्रत्यागम m_अन्तर्_Dtype_Func; }
};
class AstTraceInc final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ध्रुव uint32_t m_आधारः_संहिता;  // Trace code base value in function containing this AstTraceInc
    ध्रुव VTraceType m_traceType;  // Is this a const/full/incremental dump

सार्वजनिक:
    AstTraceInc(File_पङ्क्तिः* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t आधारः_संहिता = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_आधारः_संहिता{आधारः_संहिता}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_वृक्षः(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 10 + 2 * INSTR_गणनम्_LD; }
    bool hasDType() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    AstTraceDecl* declp() ध्रुव { प्रत्यागम m_declp; }
    VTraceType traceType() ध्रुव { प्रत्यागम m_traceType; }
    uint32_t आधारः_संहिता() ध्रुव { प्रत्यागम m_आधारः_संहिता; }
};
class Ast_Trace_निष्कासय_उपसर्गः final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
सार्वजनिक:
    explicit Ast_Trace_निष्कासय_उपसर्गः(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_Trace_निष्कासय_उपसर्गः(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_निष्कासय_उपसर्गः;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम false; }
};
class Ast_Trace_प्रक्षिप_उपसर्गः final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    ध्रुव string m_उपसर्गः;  // Prefix to add to signal names
    ध्रुव VTrace_उपसर्गः_Type m_उपसर्गः_Type;  // Type of prefix being pushed
    ध्रुव पूर्णाङ्क m_वामः;  // Array left index, or struct/union member count
    ध्रुव पूर्णाङ्क m_दक्षिणः;  // Array right index
    ध्रुव bool m_quoted_उपसर्गः;  // Quote prefix name
सार्वजनिक:
    Ast_Trace_प्रक्षिप_उपसर्गः(File_पङ्क्तिः* fl, ध्रुव string& उपसर्गः, VTrace_उपसर्गः_Type उपसर्गः_Type,
                       पूर्णाङ्क वामः = 0, पूर्णाङ्क दक्षिणः = 0, bool quoted_उपसर्गः = true)
        : ASTGEN_SUPER_Trace_प्रक्षिप_उपसर्गः(fl)
        , m_उपसर्गः{उपसर्गः}
        , m_उपसर्गः_Type{उपसर्गः_Type}
        , m_वामः{वामः}
        , m_दक्षिणः{दक्षिणः}
        , m_quoted_उपसर्गः{quoted_उपसर्गः} {}
    ASTGEN_MEMBERS_Ast_Trace_प्रक्षिप_उपसर्गः;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम false; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string उपसर्गः() ध्रुव { प्रत्यागम m_उपसर्गः; }
    VTrace_उपसर्गः_Type उपसर्गः_Type() ध्रुव { प्रत्यागम m_उपसर्गः_Type; }
    पूर्णाङ्क वामः() ध्रुव { प्रत्यागम m_वामः; }
    पूर्णाङ्क दक्षिणः() ध्रुव { प्रत्यागम m_दक्षिणः; }
    bool quoted_उपसर्गः() ध्रुव { प्रत्यागम m_quoted_उपसर्गः; }
};
class AstWait final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
सार्वजनिक:
    AstWait(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ध्रुव override { प्रत्यागम true; }
};
class AstWaitFork final : सार्वजनिक Ast_ग्रन्थिः_Stmt {
    // A "wait fork" statement
सार्वजनिक:
    explicit AstWaitFork(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ध्रुव override { प्रत्यागम true; }
    string verilogKwd() ध्रुव override { प्रत्यागम "wait fork"; }
};

// === AstNodeAssign ===
class Ast_नियोजय final : सार्वजनिक Ast_ग्रन्थिः_नियोजय {
सार्वजनिक:
    Ast_नियोजय(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp,
              Ast_ग्रन्थिः* timingControlp = nullptr)
        : ASTGEN_SUPER_नियोजय(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_नियोजय;
    Ast_ग्रन्थिः_नियोजय* cloneType(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) override {
        Ast_ग्रन्थिः* ध्रुव controlp = timingControlp() ? timingControlp()->clone_वृक्षः(false) : nullptr;
        प्रत्यागम new Ast_नियोजय{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_नियोजय_Compound final : सार्वजनिक Ast_ग्रन्थिः_नियोजय {
    // Compound assignments (+=, -=, *=, ...)
सार्वजनिक:
    गणना operation : uint8_t {
        सङ्कलय_2,
        And,
        Div,
        ModDiv,
        Mul,
        Or,
        सरणम्_L,
        सरणम्_R,
        सरणम्_RS,
        Sub,
        Xor,
    };

गुप्त:
    operation m_operation;

सार्वजनिक:
    Ast_नियोजय_Compound(Ast_नियोजय_Compound::operation operation, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp,
                      Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः* timingControlp = nullptr)
        : ASTGEN_SUPER_नियोजय_Compound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_नियोजय_Compound;
    Ast_ग्रन्थिः_नियोजय* cloneType(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) override {
        Ast_ग्रन्थिः* ध्रुव controlp = timingControlp() ? timingControlp()->clone_वृक्षः(false) : nullptr;
        प्रत्यागम new Ast_नियोजय_Compound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { प्रत्यागम m_operation; }
};
class Ast_नियोजय_Cont final : सार्वजनिक Ast_ग्रन्थिः_नियोजय {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
सार्वजनिक:
    Ast_नियोजय_Cont(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp,
                  Ast_ग्रन्थिः* timingControlp = nullptr)
        : ASTGEN_SUPER_नियोजय_Cont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_नियोजय_Cont;
    Ast_ग्रन्थिः_नियोजय* cloneType(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) override {
        Ast_ग्रन्थिः* ध्रुव controlp = timingControlp() ? timingControlp()->clone_वृक्षः(false) : nullptr;
        प्रत्यागम new Ast_नियोजय_Cont{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_नियोजय_Dly final : सार्वजनिक Ast_ग्रन्थिः_नियोजय {
सार्वजनिक:
    Ast_नियोजय_Dly(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp,
                 Ast_ग्रन्थिः* timingControlp = nullptr)
        : ASTGEN_SUPER_नियोजय_Dly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_नियोजय_Dly;
    Ast_ग्रन्थिः_नियोजय* cloneType(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) override {
        Ast_ग्रन्थिः* ध्रुव controlp = timingControlp() ? timingControlp()->clone_वृक्षः(false) : nullptr;
        प्रत्यागम new Ast_नियोजय_Dly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    string verilogKwd() ध्रुव override { प्रत्यागम "<="; }
};
class Ast_नियोजय_Force final : सार्वजनिक Ast_ग्रन्थिः_नियोजय {
    // Procedural 'force' statement
सार्वजनिक:
    Ast_नियोजय_Force(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_नियोजय_Force(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_नियोजय_Force;
    Ast_ग्रन्थिः_नियोजय* cloneType(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) override {
        प्रत्यागम new Ast_नियोजय_Force{fileline(), lhsp, rhsp};
    }
};
class Ast_नियोजय_W final : सार्वजनिक Ast_ग्रन्थिः_नियोजय {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
सार्वजनिक:
    Ast_नियोजय_W(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp,
               Ast_ग्रन्थिः* timingControlp = nullptr)
        : ASTGEN_SUPER_नियोजय_W(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_नियोजय_W;
    Ast_ग्रन्थिः_नियोजय* cloneType(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) override {
        Ast_ग्रन्थिः* ध्रुव controlp = timingControlp() ? timingControlp()->clone_वृक्षः(false) : nullptr;
        प्रत्यागम new Ast_नियोजय_W{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ध्रुव override {
        प्रत्यागम timingControlp() || lhsp()->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* refp) {
            प्रत्यागम refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ध्रुव;
};

// === AstNodeBlock ===
class AstBegin final : सार्वजनिक Ast_ग्रन्थिः_खण्डम् {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ध्रुव bool m_implied : 1;  // Not inserted by user
सार्वजनिक:
    AstBegin(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        सङ्कलय_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    शून्य समुच्चयः_Need_Process() { m_needProcess = true; }
    bool needProcess() ध्रुव { प्रत्यागम m_needProcess; }
    bool implied() ध्रुव { प्रत्यागम m_implied; }
};
class AstFork final : सार्वजनिक Ast_ग्रन्थिः_खण्डम् {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    ध्रुव VJoinType m_joinType;  // Join keyword type
    bool m_तात्कालिकम्_Start = false;  // Fork starts before its parent blocks or exits

सार्वजनिक:
    AstFork(File_पङ्क्तिः* fl, VJoinType joinType, ध्रुव string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstFork* ध्रुव asamep = VN_DBG_AS(samep, Fork);
        प्रत्यागम joinType() == asamep->joinType() && तात्कालिकम्_Start() == asamep->तात्कालिकम्_Start();
    }
    bool isTimingControl() ध्रुव override { प्रत्यागम !joinType().joinNone(); }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    VJoinType joinType() ध्रुव { प्रत्यागम m_joinType; }
    bool तात्कालिकम्_Start() ध्रुव { प्रत्यागम m_तात्कालिकम्_Start; }
    शून्य तात्कालिकम्_Start(bool flag) { m_तात्कालिकम्_Start = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : सार्वजनिक Ast_ग्रन्थिः_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

सार्वजनिक:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* propp, Ast_ग्रन्थिः* passsp, Ast_ग्रन्थिः* failsp, VAssertType type,
              VAssertDirectiveType directive, ध्रुव string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        सङ्कलय_Failsp(failsp);
    }
    string verilogKwd() ध्रुव override {
        प्रत्यागम directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : सार्वजनिक Ast_ग्रन्थिः_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
सार्वजनिक:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* propp, Ast_ग्रन्थिः* passsp, Ast_ग्रन्थिः* failsp,
                       ध्रुव string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::आन्तरिकम्,
                                       VAssertDirectiveType::INTRINSIC, name) {
        सङ्कलय_Failsp(failsp);
    }
    string verilogKwd() ध्रुव override { प्रत्यागम "assert"; }
};
class AstCover final : सार्वजनिक Ast_ग्रन्थिः_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
सार्वजनिक:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* propp, Ast_ग्रन्थिः* stmtsp, VAssertType type,
             ध्रुव string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ध्रुव override { प्रत्यागम "cover"; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isCoverSeq() ध्रुव { प्रत्यागम m_isCoverSeq; }
    शून्य isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ध्रुव { प्रत्यागम m_isSeqEvent; }
    शून्य isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : सार्वजनिक Ast_ग्रन्थिः_Cover_Or_Assert {
सार्वजनिक:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::आन्तरिकम्,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ध्रुव override { प्रत्यागम "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : सार्वजनिक Ast_ग्रन्थिः_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
सार्वजनिक:
    AstConstraintForeach(File_पङ्क्तिः* fl, Ast_Foreach_शीर्षकम्* headerp, Ast_ग्रन्थिः* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ध्रुव { प्रत्यागम m_soft; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstForeach final : सार्वजनिक Ast_ग्रन्थिः_Foreach {
सार्वजनिक:
    AstForeach(File_पङ्क्तिः* fl, Ast_Foreach_शीर्षकम्* headerp, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : सार्वजनिक Ast_ग्रन्थिः_If {
सार्वजनिक:
    AstConstraintIf(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः* thensp, Ast_ग्रन्थिः* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : सार्वजनिक Ast_ग्रन्थिः_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_प्राथमिकता_Pragma = false;  // priority case
सार्वजनिक:
    AstIf(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः* thensp = nullptr, Ast_ग्रन्थिः* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ध्रुव { प्रत्यागम m_uniquePragma; }
    शून्य uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ध्रुव { प्रत्यागम m_unique0Pragma; }
    शून्य unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool प्राथमिकता_Pragma() ध्रुव { प्रत्यागम m_प्राथमिकता_Pragma; }
    शून्य प्राथमिकता_Pragma(bool flag) { m_प्राथमिकता_Pragma = flag; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : सार्वजनिक Ast_ग्रन्थिः_Read_Write_Mem {
सार्वजनिक:
    AstReadMem(File_पङ्क्तिः* fl, bool hex, Ast_ग्रन्थिः_व्यञ्जकम्* filenamep, Ast_ग्रन्थिः_व्यञ्जकम्* memp,
               Ast_ग्रन्थिः_व्यञ्जकम्* lsbp, Ast_ग्रन्थिः_व्यञ्जकम्* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ध्रुव override { प्रत्यागम (isHex() ? "$readmemh" : "$readmemb"); }
    ध्रुव वर्ण* cFuncPrefixp() ध्रुव override { प्रत्यागम "VL_READMEM_"; }
};
class AstWriteMem final : सार्वजनिक Ast_ग्रन्थिः_Read_Write_Mem {
सार्वजनिक:
    AstWriteMem(File_पङ्क्तिः* fl, bool hex, Ast_ग्रन्थिः_व्यञ्जकम्* filenamep, Ast_ग्रन्थिः_व्यञ्जकम्* memp,
                Ast_ग्रन्थिः_व्यञ्जकम्* lsbp, Ast_ग्रन्थिः_व्यञ्जकम्* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ध्रुव override { प्रत्यागम (isHex() ? "$writememh" : "$writememb"); }
    ध्रुव वर्ण* cFuncPrefixp() ध्रुव override { प्रत्यागम "VL_WRITEMEM_"; }
};

#यदि_अन्तः  // Guard
