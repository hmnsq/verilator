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

#अगर_अघोषित VERILATOR_V3ASTNODESTMT_H_
#घोषणा VERILATOR_V3ASTNODESTMT_H_

#अगर_अघोषित VERILATOR_V3AST_H_
#त्रुटि "Use V3Ast.h as the include"
#समावेश "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#घोषणा VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#पूर्ण_अगर

// === Abstract base node types (AstNode*) =====================================

class Ast_गाँठ_Stmt VL_NOT_FINAL : खुला Ast_गाँठ {
    // Procedural statement
रक्षित:
    Ast_गाँठ_Stmt(VNType t, File_पंक्ति* fl)
        : Ast_गाँठ{t, fl} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Stmt;
    // METHODS
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
};
class Ast_गाँठ_Assign VL_NOT_FINAL : खुला Ast_गाँठ_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
रक्षित:
    Ast_गाँठ_Assign(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp,
                  Ast_गाँठ* timingControlp = nullptr)
        : Ast_गाँठ_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Assign;
    // Clone single node, just get same type back.
    व्योम dump(std::ostream& str) स्थिर override;
    भव Ast_गाँठ_Assign* cloneType(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) = 0;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    भव bool cleanRhs() स्थिर { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    string verilogKwd() स्थिर override { वापस "="; }
    bool isTimingControl() स्थिर override { वापस timingControlp(); }
};
class Ast_गाँठ_Block VL_NOT_FINAL : खुला Ast_गाँठ_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
रक्षित:
    Ast_गाँठ_Block(VNType t, File_पंक्ति* fl, स्थिर string& name)
        : Ast_गाँठ_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Block;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Block name
    व्योम name(स्थिर string& name) override { m_name = name; }
    bool unnamed() स्थिर { वापस m_unnamed; }
};
class Ast_गाँठ_Cover_Or_Assert VL_NOT_FINAL : खुला Ast_गाँठ_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    स्थिर VAssertType m_userType;  // Assertion/cover type for user enable/disable
    स्थिर VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_immediate = false;  // Immediate assert (may differ from userType being immediate)

खुला:
    Ast_गाँठ_Cover_Or_Assert(VNType t, File_पंक्ति* fl, Ast_गाँठ* propp, Ast_गाँठ* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         स्थिर string& name = "")
        : Ast_गाँठ_Stmt{t, fl}
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
    ASTGEN_MEMBERS_Ast_गाँठ_Cover_Or_Assert;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस samep->name() == name(); }
    व्योम name(स्थिर string& name) override { m_name = name; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string type() { वापस ""; }
    VAssertType userType() स्थिर VL_MT_SAFE { वापस m_userType; }
    VAssertDirectiveType directive() स्थिर { वापस m_directive; }
    bool immediate() स्थिर { वापस m_immediate; }
    व्योम immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() स्थिर VL_MT_STABLE { वापस m_senFromAlways; }
    व्योम senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_गाँठ_Foreach VL_NOT_FINAL : खुला Ast_गाँठ_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
खुला:
    Ast_गाँठ_Foreach(VNType t, File_पंक्ति* fl, AstForeachHeader* headerp, Ast_गाँठ* bodyp)
        : Ast_गाँठ_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_गाँठ_Foreach;
    bool isGateOptimizable() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class Ast_गाँठ_If VL_NOT_FINAL : खुला Ast_गाँठ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
रक्षित:
    Ast_गाँठ_If(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ* thensp, Ast_गाँठ* elsesp)
        : Ast_गाँठ_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_If;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isGateDedupable() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    व्योम branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() स्थिर { वापस m_branchPred; }
    व्योम isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() स्थिर { वापस m_isBoundsCheck; }
};
class Ast_गाँठ_Read_Write_Mem VL_NOT_FINAL : खुला Ast_गाँठ_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    स्थिर bool m_isHex;  // readmemh, not readmemb
खुला:
    Ast_गाँठ_Read_Write_Mem(VNType t, File_पंक्ति* fl, bool hex, Ast_गाँठ_व्यंजक* filenamep,
                        Ast_गाँठ_व्यंजक* memp, Ast_गाँठ_व्यंजक* lsbp, Ast_गाँठ_व्यंजक* msbp)
        : Ast_गाँठ_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_गाँठ_Read_Write_Mem;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस isHex() == VN_DBG_AS(samep, गाँठ_Read_Write_Mem)->isHex();
    }
    bool isHex() स्थिर { वापस m_isHex; }
    भव स्थिर अक्षर* cFuncPrefixp() स्थिर = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : खुला Ast_गाँठ {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
खुला:
    AstCaseItem(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condsp, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() + INSTR_गिनती_BRANCH; }
    bool isDefault() स्थिर { वापस condsp() == nullptr; }
};

class AstForeachHeader final : खुला Ast_गाँठ {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
खुला:
    AstForeachHeader(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : खुला Ast_गाँठ_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

खुला:
    AstAssertCtl(File_पंक्ति* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_गाँठ_व्यंजक* levelp = nullptr, Ast_गाँठ_व्यंजक* itemsp = nullptr);
    AstAssertCtl(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* controlTypep, Ast_गाँठ_व्यंजक* assertTypesp = nullptr,
                 Ast_गाँठ_व्यंजक* directiveTypep = nullptr, Ast_गाँठ_व्यंजक* levelp = nullptr,
                 Ast_गाँठ_व्यंजक* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() स्थिर override { वापस m_ctlType.ascii(); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    VAssertCtlType ctlType() स्थिर { वापस m_ctlType; }
    व्योम ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() स्थिर { वापस m_assertTypes; }
    व्योम assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() स्थिर { वापस m_directiveTypes; }
    व्योम directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
};
class AstBreak final : खुला Ast_गाँठ_Stmt {
खुला:
    explicit AstBreak(File_पंक्ति* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() स्थिर override { वापस "break"; }
    bool isBrancher() स्थिर override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : खुला Ast_गाँठ_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
खुला:
    AstCAwait(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_Sen_वृक्ष* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isTimingControl() स्थिर override { वापस true; }
    Ast_Sen_वृक्ष* sentreep() स्थिर { वापस m_sentreep; }
    व्योम clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : खुला Ast_गाँठ_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
खुला:
    AstCLocalScope(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : खुला Ast_गाँठ_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
खुला:
    AstCReturn(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstCStmt final : खुला Ast_गाँठ_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    स्थिर VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    अटल AstCStmt* profExecSection(File_पंक्ति* flp, स्थिर std::string& section, bool धकेलो) {
        // Compute the label
        std::string label;
        अगर (v3Global.opt.hierChild()) {
            label += v3Global.opt.शीर्ष_Module() + ":";
        } अन्यथा अगर (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += धकेलो ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        अगर (धकेलो) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += धकेलो ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        वापस new AstCStmt{flp, धकेलो ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

खुला:
    explicit AstCStmt(File_पंक्ति* fl, स्थिर std::string& text = "",
                      स्थिर VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        अगर (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    VCStmtType stmtType() स्थिर { वापस m_stmtType; }
    // Add some text, or a node to this statement
    व्योम add(स्थिर std::string& text) { addNodesp(new AstText{fileline(), text}); }
    व्योम add(Ast_गाँठ* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    अटल AstCStmt* prof_Exec_Section_धकेलो(File_पंक्ति* flp, स्थिर std::string& section) {
        वापस profExecSection(flp, section, true);
    }
    अटल AstCStmt* prof_Exec_Section_निकालो(File_पंक्ति* flp, स्थिर std::string& section) {
        वापस profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : खुला Ast_गाँठ_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    स्थिर bool m_fromDollarC;  // Is from source '$c', emit decoration
खुला:
    explicit AstCStmtUser(File_पंक्ति* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    bool fromDollarC() स्थिर { वापस m_fromDollarC; }
    // Add some text, or a node to this statement
    व्योम add(स्थिर std::string& text) { addNodesp(new AstText{fileline(), text}); }
    व्योम add(Ast_गाँठ* nodep) { addNodesp(nodep); }
};
class AstCase final : खुला Ast_गाँठ_Stmt {
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
खुला:
    AstCase(File_पंक्ति* fl, VCaseType caseType, Ast_गाँठ_व्यंजक* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    string verilogKwd() स्थिर override { वापस casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() स्थिर { वापस m_caseType == VCaseType::CT_CASEX; }
    bool casez() स्थिर { वापस m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() स्थिर { वापस m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() स्थिर { वापस m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() स्थिर { वापस m_caseType == VCaseType::CT_CASE; }
    व्योम caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    व्योम caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() स्थिर { वापस m_fullPragma; }
    व्योम fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() स्थिर { वापस m_parallelPragma; }
    व्योम parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() स्थिर { वापस m_uniquePragma; }
    व्योम uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() स्थिर { वापस m_unique0Pragma; }
    व्योम unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() स्थिर { वापस m_priorityPragma; }
    व्योम priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() स्थिर;
};
class AstComment final : खुला Ast_गाँठ_Stmt {
    // Some comment to put into the output stream
    स्थिर string m_name;  // Text of comment
    स्थिर bool m_showAt;  // Show "at <fileline>"
खुला:
    AstComment(File_पंक्ति* fl, स्थिर string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Text
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस true; }  // Ignore name in comments
    भव bool showAt() स्थिर { वापस m_showAt; }
};
class AstConstraintBefore final : खुला Ast_गाँठ_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
खुला:
    AstConstraintBefore(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhssp, Ast_गाँठ_व्यंजक* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class Ast_Constraint_व्यंजक final : खुला Ast_गाँठ_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
खुला:
    Ast_Constraint_व्यंजक(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_Constraint_व्यंजक(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_व्यंजक;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isDisableSoft() स्थिर { वापस m_isDisableSoft; }
    व्योम isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() स्थिर { वापस m_isSoft; }
    व्योम isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : खुला Ast_गाँठ_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
खुला:
    AstConstraintUnique(File_पंक्ति* fl, Ast_गाँठ* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstContinue final : खुला Ast_गाँठ_Stmt {
खुला:
    explicit AstContinue(File_पंक्ति* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() स्थिर override { वापस "continue"; }
    bool isBrancher() स्थिर override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : खुला Ast_गाँठ_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

खुला:
    AstCoverInc(File_पंक्ति* fl, Ast_गाँठ_Cover_Decl* declp, स्थिर bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस 1 + 2 * INSTR_गिनती_LD; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstCoverInc* स्थिर otherp = VN_DBG_AS(samep, CoverInc);
        वापस declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isPure() override { वापस false; }
    Ast_गाँठ_Cover_Decl* declp() स्थिर { वापस m_declp; }  // Where defined
    व्योम declp(Ast_गाँठ_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() स्थिर { वापस m_isDuplicate; }
};
class AstDeassign final : खुला Ast_गाँठ_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
खुला:
    AstDeassign(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : खुला Ast_गाँठ_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    स्थिर bool m_isCycleDelay;  // True if it is a cycle delay
खुला:
    AstDelay(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isTimingControl() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    bool isCycleDelay() स्थिर { वापस m_isCycleDelay; }
    bool isRangeDelay() स्थिर { वापस m_isCycleDelay && rhsp(); }
    bool isUnbounded() स्थिर { वापस isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    व्योम fallDelay(Ast_गाँठ_व्यंजक* स्थिर fallDelayp) { rhsp(fallDelayp); }
    Ast_गाँठ_व्यंजक* fallDelay() स्थिर { वापस m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : खुला Ast_गाँठ_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
खुला:
    AstDisable(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    स्थिर अक्षर* broken() स्थिर override;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम targetp(Ast_गाँठ* nodep) { m_targetp = nodep; }
    Ast_गाँठ* targetp() स्थिर { वापस m_targetp; }
    bool isBrancher() स्थिर override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : खुला Ast_गाँठ_Stmt {
    // A "disable fork" statement
खुला:
    explicit AstDisableFork(File_पंक्ति* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() स्थिर override { वापस "disable fork"; }
};
class AstDisplay final : खुला Ast_गाँठ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

खुला:
    AstDisplay(File_पंक्ति* fl, VDisplayType dispType, स्थिर string& text, Ast_गाँठ_व्यंजक* filep,
               Ast_गाँठ_व्यंजक* exprsp, अक्षर missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_पंक्ति* fl, VDisplayType dispType, Ast_गाँठ_व्यंजक* filep, Ast_गाँठ_व्यंजक* exprsp,
               अक्षर missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* स्थिर newp
            = new AstSFormatF{fl, AstSFormatF::व्यंजक_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!fmtp());
        वापस nullptr;
    }
    string verilogKwd() स्थिर override {
        वापस (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: $display makes output
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    VDisplayType displayType() स्थिर { वापस m_displayType; }
    व्योम displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() स्थिर { वापस displayType().addNewline(); }
};
class AstDumpCtl final : खुला Ast_गाँठ_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    स्थिर VDumpCtlType m_ctlType;  // Type of operation
खुला:
    AstDumpCtl(File_पंक्ति* fl, VDumpCtlType ctlType, Ast_गाँठ_व्यंजक* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() स्थिर override { वापस ctlType().ascii(); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    भव bool cleanOut() स्थिर { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    VDumpCtlType ctlType() स्थिर { वापस m_ctlType; }
};
class AstEventControl final : खुला Ast_गाँठ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
खुला:
    AstEventControl(File_पंक्ति* fl, Ast_Sen_वृक्ष* sentreep, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() स्थिर override { वापस "@(%l) %r"; }
    bool isTimingControl() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
};
class AstExecGraph final : खुला Ast_गाँठ_Stmt {
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
    V3Graph* स्थिर m_depGraphp;  // contains ExecMTask vertices
    स्थिर string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

खुला:
    AstExecGraph(File_पंक्ति* fl, स्थिर string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    व्योम cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    स्थिर अक्षर* broken() स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    V3Graph* depGraphp() { वापस m_depGraphp; }
    स्थिर V3Graph* depGraphp() स्थिर { वापस m_depGraphp; }
    AstCFunc* runfuncp() स्थिर { वापस m_runfuncp; }
    व्योम runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : खुला Ast_गाँठ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
खुला:
    AstFClose(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() स्थिर override { वापस "$fclose"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstFFlush final : खुला Ast_गाँठ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
खुला:
    AstFFlush(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() स्थिर override { वापस "$fflush"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstFinish final : खुला Ast_गाँठ_Stmt {
खुला:
    explicit AstFinish(File_पंक्ति* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: $display makes output
    bool isUnlikely() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }  // Rarely executes
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस fileline() == samep->fileline(); }
};
class AstFinishFork final : खुला Ast_गाँठ_Stmt {
    // $finish in fork
खुला:
    explicit AstFinishFork(File_पंक्ति* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: $display makes output
    bool isUnlikely() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }  // Rarely executes
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस fileline() == samep->fileline(); }
};
class AstFireEvent final : खुला Ast_गाँठ_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    स्थिर bool m_delayed;  // Delayed (->>) vs non-delayed (->)
खुला:
    AstFireEvent(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isDelayed() स्थिर { वापस m_delayed; }
};
class AstInitialAutomaticStmt final : खुला Ast_गाँठ_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
खुला:
    AstInitialAutomaticStmt(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool isPure() override { वापस true; }
};
class AstInitialStaticStmt final : खुला Ast_गाँठ_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
खुला:
    AstInitialStaticStmt(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool isPure() override { वापस true; }
};
class AstJumpBlock final : खुला Ast_गाँठ_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
खुला:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool isPure() override;

निजी:
    bool getPurityRecurse() स्थिर;
};
class AstJumpGo final : खुला Ast_गाँठ_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
खुला:
    AstJumpGo(File_पंक्ति* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    स्थिर अक्षर* broken() स्थिर override;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isBrancher() स्थिर override {
        वापस true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() स्थिर { वापस m_blockp; }
};
class AstLoop final : खुला Ast_गाँठ_Stmt {
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
खुला:
    explicit AstLoop(File_पंक्ति* fl, Ast_गाँठ* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* thatp) स्थिर override {
        वापस m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    // ACCESSORS
    VOptionBool unroll() स्थिर { वापस m_unroll; }
    व्योम unroll(स्थिर VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : खुला Ast_गाँठ_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
खुला:
    AstLoopTest(File_पंक्ति* fl, AstLoop* loopp, Ast_गाँठ_व्यंजक* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    स्थिर अक्षर* broken() स्थिर override;
    व्योम dump(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isBrancher() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    // ACCESSORS
    AstLoop* loopp() स्थिर { वापस m_loopp; }
};
class AstMonitorOff final : खुला Ast_गाँठ_Stmt {
    स्थिर bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

खुला:
    AstMonitorOff(File_पंक्ति* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string verilogKwd() स्थिर override { वापस m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() स्थिर override { वापस false; }  // Though deleted before opt
    bool isPredictOptimizable() स्थिर override { वापस false; }  // Though deleted before opt
    bool isPure() override { वापस false; }  // Though deleted before opt
    bool isOutputter() override { वापस true; }  // Though deleted before opt
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() स्थिर { वापस m_off; }
};
class AstPExprClause final : खुला Ast_गाँठ_Stmt {
    स्थिर bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    स्थिर bool m_vacuous;  // Pass is vacuous

खुला:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_पंक्ति* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool pass() स्थिर { वापस m_pass; }
    bool vacuous() स्थिर { वापस m_vacuous; }
};
class AstPrintTimeScale final : खुला Ast_गाँठ_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
खुला:
    explicit AstPrintTimeScale(File_पंक्ति* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    व्योम name(स्थिर string& name) override { m_name = name; }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string verilogKwd() स्थिर override { वापस "$printtimescale"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
};
class AstRSBreak final : खुला Ast_गाँठ_Stmt {
    // randsequence break
खुला:
    explicit AstRSBreak(File_पंक्ति* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() स्थिर override { वापस "break"; }
    bool isBrancher() स्थिर override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : खुला Ast_गाँठ_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
खुला:
    AstRSProd(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
};
class AstRSProdItem final : खुला Ast_गाँठ_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
खुला:
    AstRSProdItem(File_पंक्ति* fl, स्थिर string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    AstRSProd* prodp() स्थिर { वापस m_prodp; }
    व्योम prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : खुला Ast_गाँठ_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
खुला:
    AstRSProdList(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* weightp, Ast_गाँठ* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    bool randJoin() स्थिर { वापस m_randJoin; }
    व्योम randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : खुला Ast_गाँठ_Stmt {
    // randsequence return
खुला:
    explicit AstRSReturn(File_पंक्ति* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() स्थिर override { वापस "return"; }
    bool isBrancher() स्थिर override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : खुला Ast_गाँठ_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
खुला:
    AstRSRule(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* weightp, AstRSProdList* prodlistsp, Ast_गाँठ* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
};
class AstRandCase final : खुला Ast_गाँठ_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
खुला:
    AstRandCase(File_पंक्ति* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
};
class AstRandSequence final : खुला Ast_गाँठ_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
खुला:
    AstRandSequence(File_पंक्ति* fl, स्थिर string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Block name
    व्योम name(स्थिर string& name) override { m_name = name; }
    string start() स्थिर VL_MT_STABLE { वापस m_start; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    AstRSProd* prodp() स्थिर { वापस m_prodp; }
    व्योम prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : खुला Ast_गाँठ_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
खुला:
    AstRelease(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : खुला Ast_गाँठ_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
खुला:
    AstRepeat(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* countp, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() स्थिर override { वापस false; }  // Not relevant - converted to FOR
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstReturn final : खुला Ast_गाँठ_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
खुला:
    explicit AstReturn(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() स्थिर override { वापस "return"; }
    bool isBrancher() स्थिर override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : खुला Ast_गाँठ_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
खुला:
    AstSFormat(File_पंक्ति* fl, bool optionalFormat, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* exprsp,
               अक्षर missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* स्थिर newp
            = new AstSFormatF{fl, AstSFormatF::व्यंजक_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_पंक्ति* fl, AstSFormatF* fmtp, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!fmtp());
        वापस nullptr;
    }
    string verilogKwd() स्थिर override { वापस "$sformat"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस true; }
    bool isPure() override { वापस true; }
    bool isOutputter() override { वापस false; }
    भव bool cleanOut() स्थिर { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstSetuphold final : खुला Ast_गाँठ_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
खुला:
    AstSetuphold(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* refevp, Ast_गाँठ_व्यंजक* dataevp,
                 Ast_गाँठ_व्यंजक* delrefp = nullptr, Ast_गाँठ_व्यंजक* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class Ast_ढेर_Trace_T final : खुला Ast_गाँठ_Stmt {
    // $stacktrace used as task
खुला:
    explicit Ast_ढेर_Trace_T(File_पंक्ति* fl)
        : ASTGEN_SUPER_ढेर_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_ढेर_Trace_T;
    string verilogKwd() स्थिर override { वापस "$stacktrace"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class Ast_Stmt_व्यंजक final : खुला Ast_गाँठ_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
खुला:
    Ast_Stmt_व्यंजक(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_Stmt_व्यंजक(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_व्यंजक;
    // cppcheck-suppress uselessOverride
    bool isPure() override { वापस exprp()->isPure(); }
};
class AstStmtPragma final : खुला Ast_गाँठ_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
खुला:
    AstStmtPragma(File_पंक्ति* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : खुला Ast_गाँठ_Stmt {
    स्थिर bool m_isFatal;  // $fatal not $stop
खुला:
    AstStop(File_पंक्ति* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: $display makes output
    bool isUnlikely() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }  // Rarely executes
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस fileline() == samep->fileline(); }
    string emitVerilog() स्थिर { वापस m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() स्थिर { वापस m_isFatal; }
};
class AstSystemT final : खुला Ast_गाँठ_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
खुला:
    AstSystemT(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() स्थिर override { वापस "$system"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstTimeFormat final : खुला Ast_गाँठ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
खुला:
    AstTimeFormat(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* unitsp, Ast_गाँठ_व्यंजक* precisionp, Ast_गाँठ_व्यंजक* suffixp,
                  Ast_गाँठ_व्यंजक* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() स्थिर override { वापस "$timeformat"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
};
class AstTraceDecl final : खुला Ast_गाँठ_Stmt {
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
    स्थिर string m_showname;  // Name of variable
    स्थिर VNumRange m_bitRange;  // Property of var the trace details
    स्थिर VNumRange m_arrayRange;  // Property of var the trace details
    स्थिर VVarType m_varType;  // Type of variable (for localparam vs. param)
    स्थिर VDirection m_declDirection;  // Declared direction input/output etc
    स्थिर bool m_inDtypeFunc;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    पूर्णांक m_codeInc{0};  // Code increment for type
खुला:
    AstTraceDecl(File_पंक्ति* fl, स्थिर string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_गाँठ_व्यंजक* valuep, स्थिर VNumRange& bitRange, स्थिर VNumRange& arrayRange,
                 AstCCall* स्थिर dtypeCallp, स्थिर bool inDtypeFunc)
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
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() स्थिर override VL_MT_STABLE { वापस m_showname; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस true; }
    string showname() स्थिर { वापस m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() स्थिर { वापस m_code; }
    व्योम code(uint32_t code) { m_code = code; }
    bool codeAssigned() स्थिर { वापस m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() स्थिर { वापस m_fidx; }
    व्योम fidx(uint32_t fidx) { m_fidx = fidx; }
    व्योम codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() स्थिर {
        अगर (m_codeInc) { वापस m_codeInc; }
        वापस (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_शब्दगण()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    स्थिर VNumRange& bitRange() स्थिर { वापस m_bitRange; }
    स्थिर VNumRange& arrayRange() स्थिर { वापस m_arrayRange; }
    VVarType varType() स्थिर { वापस m_varType; }
    VDirection declDirection() स्थिर { वापस m_declDirection; }
    AstCCall* dtypeCallp() स्थिर { वापस m_dtypeCallp; }
    व्योम dtypeCallp(AstCCall* स्थिर callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() स्थिर { वापस m_dtypeDeclp; }
    व्योम dtypeDeclp(AstTraceDecl* स्थिर declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() स्थिर { वापस m_inDtypeFunc; }
};
class AstTraceInc final : खुला Ast_गाँठ_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    स्थिर uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    स्थिर VTraceType m_traceType;  // Is this a const/full/incremental dump

खुला:
    AstTraceInc(File_पंक्ति* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_वृक्ष(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस 10 + 2 * INSTR_गिनती_LD; }
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isPure() override { वापस false; }
    AstTraceDecl* declp() स्थिर { वापस m_declp; }
    VTraceType traceType() स्थिर { वापस m_traceType; }
    uint32_t baseCode() स्थिर { वापस m_baseCode; }
};
class Ast_Trace_निकालो_Prefix final : खुला Ast_गाँठ_Stmt {
खुला:
    explicit Ast_Trace_निकालो_Prefix(File_पंक्ति* fl)
        : ASTGEN_SUPER_Trace_निकालो_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_निकालो_Prefix;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस false; }
};
class Ast_Trace_धकेलो_Prefix final : खुला Ast_गाँठ_Stmt {
    स्थिर string m_prefix;  // Prefix to add to signal names
    स्थिर VTracePrefixType m_prefixType;  // Type of prefix being pushed
    स्थिर पूर्णांक m_बायाँ;  // Array left index, or struct/union member count
    स्थिर पूर्णांक m_दायाँ;  // Array right index
    स्थिर bool m_quotedPrefix;  // Quote prefix name
खुला:
    Ast_Trace_धकेलो_Prefix(File_पंक्ति* fl, स्थिर string& prefix, VTracePrefixType prefixType,
                       पूर्णांक बायाँ = 0, पूर्णांक दायाँ = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_धकेलो_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_बायाँ{बायाँ}
        , m_दायाँ{दायाँ}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_धकेलो_Prefix;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस false; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string prefix() स्थिर { वापस m_prefix; }
    VTracePrefixType prefixType() स्थिर { वापस m_prefixType; }
    पूर्णांक बायाँ() स्थिर { वापस m_बायाँ; }
    पूर्णांक दायाँ() स्थिर { वापस m_दायाँ; }
    bool quotedPrefix() स्थिर { वापस m_quotedPrefix; }
};
class AstWait final : खुला Ast_गाँठ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
खुला:
    AstWait(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() स्थिर override { वापस true; }
};
class AstWaitFork final : खुला Ast_गाँठ_Stmt {
    // A "wait fork" statement
खुला:
    explicit AstWaitFork(File_पंक्ति* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() स्थिर override { वापस true; }
    string verilogKwd() स्थिर override { वापस "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : खुला Ast_गाँठ_Assign {
खुला:
    AstAssign(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp,
              Ast_गाँठ* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_गाँठ_Assign* cloneType(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) override {
        Ast_गाँठ* स्थिर controlp = timingControlp() ? timingControlp()->clone_वृक्ष(false) : nullptr;
        वापस new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : खुला Ast_गाँठ_Assign {
    // Compound assignments (+=, -=, *=, ...)
खुला:
    क्रमागत operation : uint8_t {
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

निजी:
    operation m_operation;

खुला:
    AstAssignCompound(AstAssignCompound::operation operation, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp,
                      Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_गाँठ_Assign* cloneType(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) override {
        Ast_गाँठ* स्थिर controlp = timingControlp() ? timingControlp()->clone_वृक्ष(false) : nullptr;
        वापस new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { वापस m_operation; }
};
class AstAssignCont final : खुला Ast_गाँठ_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
खुला:
    AstAssignCont(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp,
                  Ast_गाँठ* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_गाँठ_Assign* cloneType(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) override {
        Ast_गाँठ* स्थिर controlp = timingControlp() ? timingControlp()->clone_वृक्ष(false) : nullptr;
        वापस new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : खुला Ast_गाँठ_Assign {
खुला:
    AstAssignDly(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp,
                 Ast_गाँठ* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_गाँठ_Assign* cloneType(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) override {
        Ast_गाँठ* स्थिर controlp = timingControlp() ? timingControlp()->clone_वृक्ष(false) : nullptr;
        वापस new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() स्थिर override { वापस false; }
    string verilogKwd() स्थिर override { वापस "<="; }
};
class AstAssignForce final : खुला Ast_गाँठ_Assign {
    // Procedural 'force' statement
खुला:
    AstAssignForce(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_गाँठ_Assign* cloneType(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) override {
        वापस new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : खुला Ast_गाँठ_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
खुला:
    AstAssignW(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp,
               Ast_गाँठ* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_गाँठ_Assign* cloneType(Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp) override {
        Ast_गाँठ* स्थिर controlp = timingControlp() ? timingControlp()->clone_वृक्ष(false) : nullptr;
        वापस new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() स्थिर override {
        वापस timingControlp() || lhsp()->exists([](स्थिर Ast_गाँठ_Var_Ref* refp) {
            वापस refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() स्थिर;
};

// === AstNodeBlock ===
class AstBegin final : खुला Ast_गाँठ_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    स्थिर bool m_implied : 1;  // Not inserted by user
खुला:
    AstBegin(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    व्योम setNeedProcess() { m_needProcess = true; }
    bool needProcess() स्थिर { वापस m_needProcess; }
    bool implied() स्थिर { वापस m_implied; }
};
class AstFork final : खुला Ast_गाँठ_Block {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    स्थिर VJoinType m_joinType;  // Join keyword type
    bool m_immediateStart = false;  // Fork starts before its parent blocks or exits

खुला:
    AstFork(File_पंक्ति* fl, VJoinType joinType, स्थिर string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstFork* स्थिर asamep = VN_DBG_AS(samep, Fork);
        वापस joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() स्थिर override { वापस !joinType().joinNone(); }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    VJoinType joinType() स्थिर { वापस m_joinType; }
    bool immediateStart() स्थिर { वापस m_immediateStart; }
    व्योम immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : खुला Ast_गाँठ_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

खुला:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_पंक्ति* fl, Ast_गाँठ* propp, Ast_गाँठ* passsp, Ast_गाँठ* failsp, VAssertType type,
              VAssertDirectiveType directive, स्थिर string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() स्थिर override {
        वापस directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : खुला Ast_गाँठ_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
खुला:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_पंक्ति* fl, Ast_गाँठ* propp, Ast_गाँठ* passsp, Ast_गाँठ* failsp,
                       स्थिर string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() स्थिर override { वापस "assert"; }
};
class AstCover final : खुला Ast_गाँठ_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
खुला:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_पंक्ति* fl, Ast_गाँठ* propp, Ast_गाँठ* stmtsp, VAssertType type,
             स्थिर string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() स्थिर override { वापस "cover"; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isCoverSeq() स्थिर { वापस m_isCoverSeq; }
    व्योम isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() स्थिर { वापस m_isSeqEvent; }
    व्योम isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : खुला Ast_गाँठ_Cover_Or_Assert {
खुला:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_पंक्ति* fl, Ast_गाँठ* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() स्थिर override { वापस "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : खुला Ast_गाँठ_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
खुला:
    AstConstraintForeach(File_पंक्ति* fl, AstForeachHeader* headerp, Ast_गाँठ* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() स्थिर { वापस m_soft; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstForeach final : खुला Ast_गाँठ_Foreach {
खुला:
    AstForeach(File_पंक्ति* fl, AstForeachHeader* headerp, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : खुला Ast_गाँठ_If {
खुला:
    AstConstraintIf(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ* thensp, Ast_गाँठ* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : खुला Ast_गाँठ_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
खुला:
    AstIf(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ* thensp = nullptr, Ast_गाँठ* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() स्थिर { वापस m_uniquePragma; }
    व्योम uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() स्थिर { वापस m_unique0Pragma; }
    व्योम unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() स्थिर { वापस m_priorityPragma; }
    व्योम priorityPragma(bool flag) { m_priorityPragma = flag; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : खुला Ast_गाँठ_Read_Write_Mem {
खुला:
    AstReadMem(File_पंक्ति* fl, bool hex, Ast_गाँठ_व्यंजक* filenamep, Ast_गाँठ_व्यंजक* memp,
               Ast_गाँठ_व्यंजक* lsbp, Ast_गाँठ_व्यंजक* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() स्थिर override { वापस (isHex() ? "$readmemh" : "$readmemb"); }
    स्थिर अक्षर* cFuncPrefixp() स्थिर override { वापस "VL_READMEM_"; }
};
class AstWriteMem final : खुला Ast_गाँठ_Read_Write_Mem {
खुला:
    AstWriteMem(File_पंक्ति* fl, bool hex, Ast_गाँठ_व्यंजक* filenamep, Ast_गाँठ_व्यंजक* memp,
                Ast_गाँठ_व्यंजक* lsbp, Ast_गाँठ_व्यंजक* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() स्थिर override { वापस (isHex() ? "$writememh" : "$writememb"); }
    स्थिर अक्षर* cFuncPrefixp() स्थिर override { वापस "VL_WRITEMEM_"; }
};

#पूर्ण_अगर  // Guard
