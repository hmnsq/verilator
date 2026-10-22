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
#அந்தர்பாவ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_முனை_Stmt VL_NOT_FINAL : பொது Ast_முனை {
    // Procedural statement
பாதுகாக்கப்பட்ட:
    Ast_முனை_Stmt(VNType t, File_வரி* fl)
        : Ast_முனை{t, fl} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Stmt;
    // METHODS
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
};
class Ast_முனை_Assign VL_NOT_FINAL : பொது Ast_முனை_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
பாதுகாக்கப்பட்ட:
    Ast_முனை_Assign(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp,
                  Ast_முனை* timingControlp = nullptr)
        : Ast_முனை_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Assign;
    // Clone single node, just get same type back.
    வெறுமை dump(std::ostream& str) மாறிலி override;
    மெய்நிகர் Ast_முனை_Assign* cloneType(Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) = 0;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    மெய்நிகர் bool cleanRhs() மாறிலி { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    string verilogKwd() மாறிலி override { திரும்பு "="; }
    bool isTimingControl() மாறிலி override { திரும்பு timingControlp(); }
};
class Ast_முனை_Block VL_NOT_FINAL : பொது Ast_முனை_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
பாதுகாக்கப்பட்ட:
    Ast_முனை_Block(VNType t, File_வரி* fl, மாறிலி string& name)
        : Ast_முனை_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Block;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Block name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    bool unnamed() மாறிலி { திரும்பு m_unnamed; }
};
class Ast_முனை_Cover_Or_Assert VL_NOT_FINAL : பொது Ast_முனை_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    மாறிலி VAssertType m_userType;  // Assertion/cover type for user enable/disable
    மாறிலி VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_immediate = false;  // Immediate assert (may differ from userType being immediate)

பொது:
    Ast_முனை_Cover_Or_Assert(VNType t, File_வரி* fl, Ast_முனை* propp, Ast_முனை* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         மாறிலி string& name = "")
        : Ast_முனை_Stmt{t, fl}
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
    ASTGEN_MEMBERS_Ast_முனை_Cover_Or_Assert;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு samep->name() == name(); }
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string type() { திரும்பு ""; }
    VAssertType userType() மாறிலி VL_MT_SAFE { திரும்பு m_userType; }
    VAssertDirectiveType directive() மாறிலி { திரும்பு m_directive; }
    bool immediate() மாறிலி { திரும்பு m_immediate; }
    வெறுமை immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() மாறிலி VL_MT_STABLE { திரும்பு m_senFromAlways; }
    வெறுமை senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_முனை_Foreach VL_NOT_FINAL : பொது Ast_முனை_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
பொது:
    Ast_முனை_Foreach(VNType t, File_வரி* fl, AstForeachHeader* headerp, Ast_முனை* bodyp)
        : Ast_முனை_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_முனை_Foreach;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class Ast_முனை_If VL_NOT_FINAL : பொது Ast_முனை_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
பாதுகாக்கப்பட்ட:
    Ast_முனை_If(VNType t, File_வரி* fl, Ast_முனை_கோவை* condp, Ast_முனை* thensp, Ast_முனை* elsesp)
        : Ast_முனை_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_If;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isGateDedupable() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    வெறுமை branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() மாறிலி { திரும்பு m_branchPred; }
    வெறுமை isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() மாறிலி { திரும்பு m_isBoundsCheck; }
};
class Ast_முனை_Read_Write_Mem VL_NOT_FINAL : பொது Ast_முனை_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    மாறிலி bool m_isHex;  // readmemh, not readmemb
பொது:
    Ast_முனை_Read_Write_Mem(VNType t, File_வரி* fl, bool hex, Ast_முனை_கோவை* filenamep,
                        Ast_முனை_கோவை* memp, Ast_முனை_கோவை* lsbp, Ast_முனை_கோவை* msbp)
        : Ast_முனை_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_முனை_Read_Write_Mem;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு isHex() == VN_DBG_AS(samep, முனை_Read_Write_Mem)->isHex();
    }
    bool isHex() மாறிலி { திரும்பு m_isHex; }
    மெய்நிகர் மாறிலி எழுத்து* cFuncPrefixp() மாறிலி = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : பொது Ast_முனை {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
பொது:
    AstCaseItem(File_வரி* fl, Ast_முனை_கோவை* condsp, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() + INSTR_மொத்தம்_BRANCH; }
    bool isDefault() மாறிலி { திரும்பு condsp() == nullptr; }
};

class AstForeachHeader final : பொது Ast_முனை {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
பொது:
    AstForeachHeader(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : பொது Ast_முனை_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

பொது:
    AstAssertCtl(File_வரி* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_முனை_கோவை* levelp = nullptr, Ast_முனை_கோவை* itemsp = nullptr);
    AstAssertCtl(File_வரி* fl, Ast_முனை_கோவை* controlTypep, Ast_முனை_கோவை* assertTypesp = nullptr,
                 Ast_முனை_கோவை* directiveTypep = nullptr, Ast_முனை_கோவை* levelp = nullptr,
                 Ast_முனை_கோவை* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() மாறிலி override { திரும்பு m_ctlType.ascii(); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    VAssertCtlType ctlType() மாறிலி { திரும்பு m_ctlType; }
    வெறுமை ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() மாறிலி { திரும்பு m_assertTypes; }
    வெறுமை assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() மாறிலி { திரும்பு m_directiveTypes; }
    வெறுமை directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
};
class AstBreak final : பொது Ast_முனை_Stmt {
பொது:
    explicit AstBreak(File_வரி* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() மாறிலி override { திரும்பு "break"; }
    bool isBrancher() மாறிலி override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : பொது Ast_முனை_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
பொது:
    AstCAwait(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_Sen_மரம்* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isTimingControl() மாறிலி override { திரும்பு true; }
    Ast_Sen_மரம்* sentreep() மாறிலி { திரும்பு m_sentreep; }
    வெறுமை clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : பொது Ast_முனை_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
பொது:
    AstCLocalScope(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : பொது Ast_முனை_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
பொது:
    AstCReturn(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstCStmt final : பொது Ast_முனை_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    மாறிலி VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    நிலையான AstCStmt* profExecSection(File_வரி* flp, மாறிலி std::string& section, bool தள்ளு) {
        // Compute the label
        std::string label;
        எனில் (v3Global.opt.hierChild()) {
            label += v3Global.opt.உச்சி_Module() + ":";
        } இல்லையெனில் எனில் (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += தள்ளு ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        எனில் (தள்ளு) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += தள்ளு ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        திரும்பு new AstCStmt{flp, தள்ளு ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

பொது:
    explicit AstCStmt(File_வரி* fl, மாறிலி std::string& text = "",
                      மாறிலி VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        எனில் (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    VCStmtType stmtType() மாறிலி { திரும்பு m_stmtType; }
    // Add some text, or a node to this statement
    வெறுமை add(மாறிலி std::string& text) { addNodesp(new AstText{fileline(), text}); }
    வெறுமை add(Ast_முனை* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    நிலையான AstCStmt* prof_Exec_Section_தள்ளு(File_வரி* flp, மாறிலி std::string& section) {
        திரும்பு profExecSection(flp, section, true);
    }
    நிலையான AstCStmt* prof_Exec_Section_எடு(File_வரி* flp, மாறிலி std::string& section) {
        திரும்பு profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : பொது Ast_முனை_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    மாறிலி bool m_fromDollarC;  // Is from source '$c', emit decoration
பொது:
    explicit AstCStmtUser(File_வரி* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    bool fromDollarC() மாறிலி { திரும்பு m_fromDollarC; }
    // Add some text, or a node to this statement
    வெறுமை add(மாறிலி std::string& text) { addNodesp(new AstText{fileline(), text}); }
    வெறுமை add(Ast_முனை* nodep) { addNodesp(nodep); }
};
class AstCase final : பொது Ast_முனை_Stmt {
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
பொது:
    AstCase(File_வரி* fl, VCaseType caseType, Ast_முனை_கோவை* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    string verilogKwd() மாறிலி override { திரும்பு casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() மாறிலி { திரும்பு m_caseType == VCaseType::CT_CASEX; }
    bool casez() மாறிலி { திரும்பு m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() மாறிலி { திரும்பு m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() மாறிலி { திரும்பு m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() மாறிலி { திரும்பு m_caseType == VCaseType::CT_CASE; }
    வெறுமை caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    வெறுமை caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() மாறிலி { திரும்பு m_fullPragma; }
    வெறுமை fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() மாறிலி { திரும்பு m_parallelPragma; }
    வெறுமை parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() மாறிலி { திரும்பு m_uniquePragma; }
    வெறுமை uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() மாறிலி { திரும்பு m_unique0Pragma; }
    வெறுமை unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() மாறிலி { திரும்பு m_priorityPragma; }
    வெறுமை priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() மாறிலி;
};
class AstComment final : பொது Ast_முனை_Stmt {
    // Some comment to put into the output stream
    மாறிலி string m_name;  // Text of comment
    மாறிலி bool m_showAt;  // Show "at <fileline>"
பொது:
    AstComment(File_வரி* fl, மாறிலி string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Text
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு true; }  // Ignore name in comments
    மெய்நிகர் bool showAt() மாறிலி { திரும்பு m_showAt; }
};
class AstConstraintBefore final : பொது Ast_முனை_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
பொது:
    AstConstraintBefore(File_வரி* fl, Ast_முனை_கோவை* lhssp, Ast_முனை_கோவை* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class Ast_Constraint_கோவை final : பொது Ast_முனை_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
பொது:
    Ast_Constraint_கோவை(File_வரி* fl, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_Constraint_கோவை(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_கோவை;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isDisableSoft() மாறிலி { திரும்பு m_isDisableSoft; }
    வெறுமை isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() மாறிலி { திரும்பு m_isSoft; }
    வெறுமை isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : பொது Ast_முனை_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
பொது:
    AstConstraintUnique(File_வரி* fl, Ast_முனை* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstContinue final : பொது Ast_முனை_Stmt {
பொது:
    explicit AstContinue(File_வரி* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() மாறிலி override { திரும்பு "continue"; }
    bool isBrancher() மாறிலி override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : பொது Ast_முனை_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

பொது:
    AstCoverInc(File_வரி* fl, Ast_முனை_Cover_Decl* declp, மாறிலி bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 1 + 2 * INSTR_மொத்தம்_LD; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstCoverInc* மாறிலி otherp = VN_DBG_AS(samep, CoverInc);
        திரும்பு declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isPure() override { திரும்பு false; }
    Ast_முனை_Cover_Decl* declp() மாறிலி { திரும்பு m_declp; }  // Where defined
    வெறுமை declp(Ast_முனை_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() மாறிலி { திரும்பு m_isDuplicate; }
};
class AstDeassign final : பொது Ast_முனை_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
பொது:
    AstDeassign(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : பொது Ast_முனை_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    மாறிலி bool m_isCycleDelay;  // True if it is a cycle delay
பொது:
    AstDelay(File_வரி* fl, Ast_முனை_கோவை* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isTimingControl() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    bool isCycleDelay() மாறிலி { திரும்பு m_isCycleDelay; }
    bool isRangeDelay() மாறிலி { திரும்பு m_isCycleDelay && rhsp(); }
    bool isUnbounded() மாறிலி { திரும்பு isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    வெறுமை fallDelay(Ast_முனை_கோவை* மாறிலி fallDelayp) { rhsp(fallDelayp); }
    Ast_முனை_கோவை* fallDelay() மாறிலி { திரும்பு m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : பொது Ast_முனை_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
பொது:
    AstDisable(File_வரி* fl, Ast_முனை_கோவை* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    மாறிலி எழுத்து* broken() மாறிலி override;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை targetp(Ast_முனை* nodep) { m_targetp = nodep; }
    Ast_முனை* targetp() மாறிலி { திரும்பு m_targetp; }
    bool isBrancher() மாறிலி override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : பொது Ast_முனை_Stmt {
    // A "disable fork" statement
பொது:
    explicit AstDisableFork(File_வரி* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() மாறிலி override { திரும்பு "disable fork"; }
};
class AstDisplay final : பொது Ast_முனை_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

பொது:
    AstDisplay(File_வரி* fl, VDisplayType dispType, மாறிலி string& text, Ast_முனை_கோவை* filep,
               Ast_முனை_கோவை* exprsp, எழுத்து missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_வரி* fl, VDisplayType dispType, Ast_முனை_கோவை* filep, Ast_முனை_கோவை* exprsp,
               எழுத்து missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* மாறிலி newp
            = new AstSFormatF{fl, AstSFormatF::கோவை_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!fmtp());
        திரும்பு nullptr;
    }
    string verilogKwd() மாறிலி override {
        திரும்பு (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: $display makes output
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    VDisplayType displayType() மாறிலி { திரும்பு m_displayType; }
    வெறுமை displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() மாறிலி { திரும்பு displayType().addNewline(); }
};
class AstDumpCtl final : பொது Ast_முனை_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    மாறிலி VDumpCtlType m_ctlType;  // Type of operation
பொது:
    AstDumpCtl(File_வரி* fl, VDumpCtlType ctlType, Ast_முனை_கோவை* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() மாறிலி override { திரும்பு ctlType().ascii(); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    மெய்நிகர் bool cleanOut() மாறிலி { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    VDumpCtlType ctlType() மாறிலி { திரும்பு m_ctlType; }
};
class AstEventControl final : பொது Ast_முனை_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
பொது:
    AstEventControl(File_வரி* fl, Ast_Sen_மரம்* sentreep, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() மாறிலி override { திரும்பு "@(%l) %r"; }
    bool isTimingControl() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
};
class AstExecGraph final : பொது Ast_முனை_Stmt {
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
    V3Graph* மாறிலி m_depGraphp;  // contains ExecMTask vertices
    மாறிலி string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

பொது:
    AstExecGraph(File_வரி* fl, மாறிலி string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    வெறுமை cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    மாறிலி எழுத்து* broken() மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    V3Graph* depGraphp() { திரும்பு m_depGraphp; }
    மாறிலி V3Graph* depGraphp() மாறிலி { திரும்பு m_depGraphp; }
    AstCFunc* runfuncp() மாறிலி { திரும்பு m_runfuncp; }
    வெறுமை runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : பொது Ast_முனை_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
பொது:
    AstFClose(File_வரி* fl, Ast_முனை_கோவை* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() மாறிலி override { திரும்பு "$fclose"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstFFlush final : பொது Ast_முனை_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
பொது:
    AstFFlush(File_வரி* fl, Ast_முனை_கோவை* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() மாறிலி override { திரும்பு "$fflush"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstFinish final : பொது Ast_முனை_Stmt {
பொது:
    explicit AstFinish(File_வரி* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: $display makes output
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }  // Rarely executes
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு fileline() == samep->fileline(); }
};
class AstFinishFork final : பொது Ast_முனை_Stmt {
    // $finish in fork
பொது:
    explicit AstFinishFork(File_வரி* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: $display makes output
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }  // Rarely executes
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு fileline() == samep->fileline(); }
};
class AstFireEvent final : பொது Ast_முனை_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    மாறிலி bool m_delayed;  // Delayed (->>) vs non-delayed (->)
பொது:
    AstFireEvent(File_வரி* fl, Ast_முனை_கோவை* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isDelayed() மாறிலி { திரும்பு m_delayed; }
};
class AstInitialAutomaticStmt final : பொது Ast_முனை_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
பொது:
    AstInitialAutomaticStmt(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool isPure() override { திரும்பு true; }
};
class AstInitialStaticStmt final : பொது Ast_முனை_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
பொது:
    AstInitialStaticStmt(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool isPure() override { திரும்பு true; }
};
class AstJumpBlock final : பொது Ast_முனை_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
பொது:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool isPure() override;

தனிப்பட்ட:
    bool getPurityRecurse() மாறிலி;
};
class AstJumpGo final : பொது Ast_முனை_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
பொது:
    AstJumpGo(File_வரி* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    மாறிலி எழுத்து* broken() மாறிலி override;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isBrancher() மாறிலி override {
        திரும்பு true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() மாறிலி { திரும்பு m_blockp; }
};
class AstLoop final : பொது Ast_முனை_Stmt {
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
பொது:
    explicit AstLoop(File_வரி* fl, Ast_முனை* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* thatp) மாறிலி override {
        திரும்பு m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    // ACCESSORS
    VOptionBool unroll() மாறிலி { திரும்பு m_unroll; }
    வெறுமை unroll(மாறிலி VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : பொது Ast_முனை_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
பொது:
    AstLoopTest(File_வரி* fl, AstLoop* loopp, Ast_முனை_கோவை* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    மாறிலி எழுத்து* broken() மாறிலி override;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isBrancher() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    // ACCESSORS
    AstLoop* loopp() மாறிலி { திரும்பு m_loopp; }
};
class AstMonitorOff final : பொது Ast_முனை_Stmt {
    மாறிலி bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

பொது:
    AstMonitorOff(File_வரி* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string verilogKwd() மாறிலி override { திரும்பு m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }  // Though deleted before opt
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }  // Though deleted before opt
    bool isPure() override { திரும்பு false; }  // Though deleted before opt
    bool isOutputter() override { திரும்பு true; }  // Though deleted before opt
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() மாறிலி { திரும்பு m_off; }
};
class AstPExprClause final : பொது Ast_முனை_Stmt {
    மாறிலி bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    மாறிலி bool m_vacuous;  // Pass is vacuous

பொது:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_வரி* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool pass() மாறிலி { திரும்பு m_pass; }
    bool vacuous() மாறிலி { திரும்பு m_vacuous; }
};
class AstPrintTimeScale final : பொது Ast_முனை_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
பொது:
    explicit AstPrintTimeScale(File_வரி* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string verilogKwd() மாறிலி override { திரும்பு "$printtimescale"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
};
class AstRSBreak final : பொது Ast_முனை_Stmt {
    // randsequence break
பொது:
    explicit AstRSBreak(File_வரி* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() மாறிலி override { திரும்பு "break"; }
    bool isBrancher() மாறிலி override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : பொது Ast_முனை_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
பொது:
    AstRSProd(File_வரி* fl, மாறிலி string& name, Ast_முனை* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
};
class AstRSProdItem final : பொது Ast_முனை_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
பொது:
    AstRSProdItem(File_வரி* fl, மாறிலி string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    AstRSProd* prodp() மாறிலி { திரும்பு m_prodp; }
    வெறுமை prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : பொது Ast_முனை_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
பொது:
    AstRSProdList(File_வரி* fl, Ast_முனை_கோவை* weightp, Ast_முனை* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    bool randJoin() மாறிலி { திரும்பு m_randJoin; }
    வெறுமை randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : பொது Ast_முனை_Stmt {
    // randsequence return
பொது:
    explicit AstRSReturn(File_வரி* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() மாறிலி override { திரும்பு "return"; }
    bool isBrancher() மாறிலி override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : பொது Ast_முனை_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
பொது:
    AstRSRule(File_வரி* fl, Ast_முனை_கோவை* weightp, AstRSProdList* prodlistsp, Ast_முனை* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
};
class AstRandCase final : பொது Ast_முனை_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
பொது:
    AstRandCase(File_வரி* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
};
class AstRandSequence final : பொது Ast_முனை_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
பொது:
    AstRandSequence(File_வரி* fl, மாறிலி string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Block name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string start() மாறிலி VL_MT_STABLE { திரும்பு m_start; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    AstRSProd* prodp() மாறிலி { திரும்பு m_prodp; }
    வெறுமை prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : பொது Ast_முனை_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
பொது:
    AstRelease(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : பொது Ast_முனை_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
பொது:
    AstRepeat(File_வரி* fl, Ast_முனை_கோவை* countp, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }  // Not relevant - converted to FOR
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstReturn final : பொது Ast_முனை_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
பொது:
    explicit AstReturn(File_வரி* fl, Ast_முனை_கோவை* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() மாறிலி override { திரும்பு "return"; }
    bool isBrancher() மாறிலி override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : பொது Ast_முனை_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
பொது:
    AstSFormat(File_வரி* fl, bool optionalFormat, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* exprsp,
               எழுத்து missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* மாறிலி newp
            = new AstSFormatF{fl, AstSFormatF::கோவை_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_வரி* fl, AstSFormatF* fmtp, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!fmtp());
        திரும்பு nullptr;
    }
    string verilogKwd() மாறிலி override { திரும்பு "$sformat"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு true; }
    bool isPure() override { திரும்பு true; }
    bool isOutputter() override { திரும்பு false; }
    மெய்நிகர் bool cleanOut() மாறிலி { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstSetuphold final : பொது Ast_முனை_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
பொது:
    AstSetuphold(File_வரி* fl, Ast_முனை_கோவை* refevp, Ast_முனை_கோவை* dataevp,
                 Ast_முனை_கோவை* delrefp = nullptr, Ast_முனை_கோவை* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class Ast_அடுக்கு_Trace_T final : பொது Ast_முனை_Stmt {
    // $stacktrace used as task
பொது:
    explicit Ast_அடுக்கு_Trace_T(File_வரி* fl)
        : ASTGEN_SUPER_அடுக்கு_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_அடுக்கு_Trace_T;
    string verilogKwd() மாறிலி override { திரும்பு "$stacktrace"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class Ast_Stmt_கோவை final : பொது Ast_முனை_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
பொது:
    Ast_Stmt_கோவை(File_வரி* fl, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_Stmt_கோவை(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_கோவை;
    // cppcheck-suppress uselessOverride
    bool isPure() override { திரும்பு exprp()->isPure(); }
};
class AstStmtPragma final : பொது Ast_முனை_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
பொது:
    AstStmtPragma(File_வரி* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : பொது Ast_முனை_Stmt {
    மாறிலி bool m_isFatal;  // $fatal not $stop
பொது:
    AstStop(File_வரி* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: $display makes output
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }  // Rarely executes
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு fileline() == samep->fileline(); }
    string emitVerilog() மாறிலி { திரும்பு m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() மாறிலி { திரும்பு m_isFatal; }
};
class AstSystemT final : பொது Ast_முனை_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
பொது:
    AstSystemT(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() மாறிலி override { திரும்பு "$system"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstTimeFormat final : பொது Ast_முனை_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
பொது:
    AstTimeFormat(File_வரி* fl, Ast_முனை_கோவை* unitsp, Ast_முனை_கோவை* precisionp, Ast_முனை_கோவை* suffixp,
                  Ast_முனை_கோவை* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() மாறிலி override { திரும்பு "$timeformat"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
};
class AstTraceDecl final : பொது Ast_முனை_Stmt {
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
    மாறிலி string m_showname;  // Name of variable
    மாறிலி VNumRange m_bitRange;  // Property of var the trace details
    மாறிலி VNumRange m_arrayRange;  // Property of var the trace details
    மாறிலி VVarType m_varType;  // Type of variable (for localparam vs. param)
    மாறிலி VDirection m_declDirection;  // Declared direction input/output etc
    மாறிலி bool m_inDtypeFunc;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    முழு m_codeInc{0};  // Code increment for type
பொது:
    AstTraceDecl(File_வரி* fl, மாறிலி string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_முனை_கோவை* valuep, மாறிலி VNumRange& bitRange, மாறிலி VNumRange& arrayRange,
                 AstCCall* மாறிலி dtypeCallp, மாறிலி bool inDtypeFunc)
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
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_showname; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு true; }
    string showname() மாறிலி { திரும்பு m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() மாறிலி { திரும்பு m_code; }
    வெறுமை code(uint32_t code) { m_code = code; }
    bool codeAssigned() மாறிலி { திரும்பு m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() மாறிலி { திரும்பு m_fidx; }
    வெறுமை fidx(uint32_t fidx) { m_fidx = fidx; }
    வெறுமை codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() மாறிலி {
        எனில் (m_codeInc) { திரும்பு m_codeInc; }
        திரும்பு (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_சொற்கள்()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    மாறிலி VNumRange& bitRange() மாறிலி { திரும்பு m_bitRange; }
    மாறிலி VNumRange& arrayRange() மாறிலி { திரும்பு m_arrayRange; }
    VVarType varType() மாறிலி { திரும்பு m_varType; }
    VDirection declDirection() மாறிலி { திரும்பு m_declDirection; }
    AstCCall* dtypeCallp() மாறிலி { திரும்பு m_dtypeCallp; }
    வெறுமை dtypeCallp(AstCCall* மாறிலி callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() மாறிலி { திரும்பு m_dtypeDeclp; }
    வெறுமை dtypeDeclp(AstTraceDecl* மாறிலி declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() மாறிலி { திரும்பு m_inDtypeFunc; }
};
class AstTraceInc final : பொது Ast_முனை_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    மாறிலி uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    மாறிலி VTraceType m_traceType;  // Is this a const/full/incremental dump

பொது:
    AstTraceInc(File_வரி* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_மரம்(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 10 + 2 * INSTR_மொத்தம்_LD; }
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isPure() override { திரும்பு false; }
    AstTraceDecl* declp() மாறிலி { திரும்பு m_declp; }
    VTraceType traceType() மாறிலி { திரும்பு m_traceType; }
    uint32_t baseCode() மாறிலி { திரும்பு m_baseCode; }
};
class Ast_Trace_எடு_Prefix final : பொது Ast_முனை_Stmt {
பொது:
    explicit Ast_Trace_எடு_Prefix(File_வரி* fl)
        : ASTGEN_SUPER_Trace_எடு_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_எடு_Prefix;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு false; }
};
class Ast_Trace_தள்ளு_Prefix final : பொது Ast_முனை_Stmt {
    மாறிலி string m_prefix;  // Prefix to add to signal names
    மாறிலி VTracePrefixType m_prefixType;  // Type of prefix being pushed
    மாறிலி முழு m_இடது;  // Array left index, or struct/union member count
    மாறிலி முழு m_வலது;  // Array right index
    மாறிலி bool m_quotedPrefix;  // Quote prefix name
பொது:
    Ast_Trace_தள்ளு_Prefix(File_வரி* fl, மாறிலி string& prefix, VTracePrefixType prefixType,
                       முழு இடது = 0, முழு வலது = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_தள்ளு_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_இடது{இடது}
        , m_வலது{வலது}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_தள்ளு_Prefix;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு false; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string prefix() மாறிலி { திரும்பு m_prefix; }
    VTracePrefixType prefixType() மாறிலி { திரும்பு m_prefixType; }
    முழு இடது() மாறிலி { திரும்பு m_இடது; }
    முழு வலது() மாறிலி { திரும்பு m_வலது; }
    bool quotedPrefix() மாறிலி { திரும்பு m_quotedPrefix; }
};
class AstWait final : பொது Ast_முனை_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
பொது:
    AstWait(File_வரி* fl, Ast_முனை_கோவை* condp, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() மாறிலி override { திரும்பு true; }
};
class AstWaitFork final : பொது Ast_முனை_Stmt {
    // A "wait fork" statement
பொது:
    explicit AstWaitFork(File_வரி* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() மாறிலி override { திரும்பு true; }
    string verilogKwd() மாறிலி override { திரும்பு "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : பொது Ast_முனை_Assign {
பொது:
    AstAssign(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp,
              Ast_முனை* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_முனை_Assign* cloneType(Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) override {
        Ast_முனை* மாறிலி controlp = timingControlp() ? timingControlp()->clone_மரம்(false) : nullptr;
        திரும்பு new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : பொது Ast_முனை_Assign {
    // Compound assignments (+=, -=, *=, ...)
பொது:
    எண்ணிக்கை operation : uint8_t {
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

தனிப்பட்ட:
    operation m_operation;

பொது:
    AstAssignCompound(AstAssignCompound::operation operation, File_வரி* fl, Ast_முனை_கோவை* lhsp,
                      Ast_முனை_கோவை* rhsp, Ast_முனை* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_முனை_Assign* cloneType(Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) override {
        Ast_முனை* மாறிலி controlp = timingControlp() ? timingControlp()->clone_மரம்(false) : nullptr;
        திரும்பு new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { திரும்பு m_operation; }
};
class AstAssignCont final : பொது Ast_முனை_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
பொது:
    AstAssignCont(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp,
                  Ast_முனை* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_முனை_Assign* cloneType(Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) override {
        Ast_முனை* மாறிலி controlp = timingControlp() ? timingControlp()->clone_மரம்(false) : nullptr;
        திரும்பு new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : பொது Ast_முனை_Assign {
பொது:
    AstAssignDly(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp,
                 Ast_முனை* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_முனை_Assign* cloneType(Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) override {
        Ast_முனை* மாறிலி controlp = timingControlp() ? timingControlp()->clone_மரம்(false) : nullptr;
        திரும்பு new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    string verilogKwd() மாறிலி override { திரும்பு "<="; }
};
class AstAssignForce final : பொது Ast_முனை_Assign {
    // Procedural 'force' statement
பொது:
    AstAssignForce(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_முனை_Assign* cloneType(Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) override {
        திரும்பு new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : பொது Ast_முனை_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
பொது:
    AstAssignW(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp,
               Ast_முனை* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_முனை_Assign* cloneType(Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp) override {
        Ast_முனை* மாறிலி controlp = timingControlp() ? timingControlp()->clone_மரம்(false) : nullptr;
        திரும்பு new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() மாறிலி override {
        திரும்பு timingControlp() || lhsp()->exists([](மாறிலி Ast_முனை_Var_Ref* refp) {
            திரும்பு refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() மாறிலி;
};

// === AstNodeBlock ===
class AstBegin final : பொது Ast_முனை_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    மாறிலி bool m_implied : 1;  // Not inserted by user
பொது:
    AstBegin(File_வரி* fl, மாறிலி string& name, Ast_முனை* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    வெறுமை setNeedProcess() { m_needProcess = true; }
    bool needProcess() மாறிலி { திரும்பு m_needProcess; }
    bool implied() மாறிலி { திரும்பு m_implied; }
};
class AstFork final : பொது Ast_முனை_Block {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    மாறிலி VJoinType m_joinType;  // Join keyword type
    bool m_immediateStart = false;  // Fork starts before its parent blocks or exits

பொது:
    AstFork(File_வரி* fl, VJoinType joinType, மாறிலி string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstFork* மாறிலி asamep = VN_DBG_AS(samep, Fork);
        திரும்பு joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() மாறிலி override { திரும்பு !joinType().joinNone(); }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    VJoinType joinType() மாறிலி { திரும்பு m_joinType; }
    bool immediateStart() மாறிலி { திரும்பு m_immediateStart; }
    வெறுமை immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : பொது Ast_முனை_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

பொது:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_வரி* fl, Ast_முனை* propp, Ast_முனை* passsp, Ast_முனை* failsp, VAssertType type,
              VAssertDirectiveType directive, மாறிலி string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() மாறிலி override {
        திரும்பு directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : பொது Ast_முனை_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
பொது:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_வரி* fl, Ast_முனை* propp, Ast_முனை* passsp, Ast_முனை* failsp,
                       மாறிலி string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() மாறிலி override { திரும்பு "assert"; }
};
class AstCover final : பொது Ast_முனை_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
பொது:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_வரி* fl, Ast_முனை* propp, Ast_முனை* stmtsp, VAssertType type,
             மாறிலி string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() மாறிலி override { திரும்பு "cover"; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isCoverSeq() மாறிலி { திரும்பு m_isCoverSeq; }
    வெறுமை isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() மாறிலி { திரும்பு m_isSeqEvent; }
    வெறுமை isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : பொது Ast_முனை_Cover_Or_Assert {
பொது:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_வரி* fl, Ast_முனை* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() மாறிலி override { திரும்பு "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : பொது Ast_முனை_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
பொது:
    AstConstraintForeach(File_வரி* fl, AstForeachHeader* headerp, Ast_முனை* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() மாறிலி { திரும்பு m_soft; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstForeach final : பொது Ast_முனை_Foreach {
பொது:
    AstForeach(File_வரி* fl, AstForeachHeader* headerp, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : பொது Ast_முனை_If {
பொது:
    AstConstraintIf(File_வரி* fl, Ast_முனை_கோவை* condp, Ast_முனை* thensp, Ast_முனை* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : பொது Ast_முனை_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
பொது:
    AstIf(File_வரி* fl, Ast_முனை_கோவை* condp, Ast_முனை* thensp = nullptr, Ast_முனை* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() மாறிலி { திரும்பு m_uniquePragma; }
    வெறுமை uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() மாறிலி { திரும்பு m_unique0Pragma; }
    வெறுமை unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() மாறிலி { திரும்பு m_priorityPragma; }
    வெறுமை priorityPragma(bool flag) { m_priorityPragma = flag; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : பொது Ast_முனை_Read_Write_Mem {
பொது:
    AstReadMem(File_வரி* fl, bool hex, Ast_முனை_கோவை* filenamep, Ast_முனை_கோவை* memp,
               Ast_முனை_கோவை* lsbp, Ast_முனை_கோவை* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() மாறிலி override { திரும்பு (isHex() ? "$readmemh" : "$readmemb"); }
    மாறிலி எழுத்து* cFuncPrefixp() மாறிலி override { திரும்பு "VL_READMEM_"; }
};
class AstWriteMem final : பொது Ast_முனை_Read_Write_Mem {
பொது:
    AstWriteMem(File_வரி* fl, bool hex, Ast_முனை_கோவை* filenamep, Ast_முனை_கோவை* memp,
                Ast_முனை_கோவை* lsbp, Ast_முனை_கோவை* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() மாறிலி override { திரும்பு (isHex() ? "$writememh" : "$writememb"); }
    மாறிலி எழுத்து* cFuncPrefixp() மாறிலி override { திரும்பு "VL_WRITEMEM_"; }
};

#endif  // Guard
