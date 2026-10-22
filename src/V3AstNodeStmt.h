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
#ଅନ୍ତର୍ଭାଵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_ଗଣ୍ଠି_Stmt VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Procedural statement
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Stmt(VNType t, File_ଧାଡ଼ି* fl)
        : Ast_ଗଣ୍ଠି{t, fl} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Stmt;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
};
class Ast_ଗଣ୍ଠି_Assign VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Assign(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp,
                  Ast_ଗଣ୍ଠି* timingControlp = nullptr)
        : Ast_ଗଣ୍ଠି_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Assign;
    // Clone single node, just get same type back.
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଆଭାସୀ Ast_ଗଣ୍ଠି_Assign* cloneType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) = 0;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଆଭାସୀ bool cleanRhs() ଅଚଳ { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "="; }
    bool isTimingControl() ଅଚଳ override { ଫେରନ୍ତୁ timingControlp(); }
};
class Ast_ଗଣ୍ଠି_Block VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Block(VNType t, File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : Ast_ଗଣ୍ଠି_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Block;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Block name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    bool unnamed() ଅଚଳ { ଫେରନ୍ତୁ m_unnamed; }
};
class Ast_ଗଣ୍ଠି_Cover_Or_Assert VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    ଅଚଳ VAssertType m_userType;  // Assertion/cover type for user enable/disable
    ଅଚଳ VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_immediate = false;  // Immediate assert (may differ from userType being immediate)

ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_Cover_Or_Assert(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* propp, Ast_ଗଣ୍ଠି* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ଅଚଳ string& name = "")
        : Ast_ଗଣ୍ଠି_Stmt{t, fl}
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
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Cover_Or_Assert;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ samep->name() == name(); }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string type() { ଫେରନ୍ତୁ ""; }
    VAssertType userType() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_userType; }
    VAssertDirectiveType directive() ଅଚଳ { ଫେରନ୍ତୁ m_directive; }
    bool immediate() ଅଚଳ { ଫେରନ୍ତୁ m_immediate; }
    ଶୂନ୍ୟ immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_senFromAlways; }
    ଶୂନ୍ୟ senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_ଗଣ୍ଠି_Foreach VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_Foreach(VNType t, File_ଧାଡ଼ି* fl, AstForeachHeader* headerp, Ast_ଗଣ୍ଠି* bodyp)
        : Ast_ଗଣ୍ଠି_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Foreach;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_ଗଣ୍ଠି_If VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_If(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି* thensp, Ast_ଗଣ୍ଠି* elsesp)
        : Ast_ଗଣ୍ଠି_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_If;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isGateDedupable() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ଅଚଳ { ଫେରନ୍ତୁ m_branchPred; }
    ଶୂନ୍ୟ isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ଅଚଳ { ଫେରନ୍ତୁ m_isBoundsCheck; }
};
class Ast_ଗଣ୍ଠି_Read_Write_Mem VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ଅଚଳ bool m_isHex;  // readmemh, not readmemb
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_Read_Write_Mem(VNType t, File_ଧାଡ଼ି* fl, bool hex, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filenamep,
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* memp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* msbp)
        : Ast_ଗଣ୍ଠି_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Read_Write_Mem;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ isHex() == VN_DBG_AS(samep, ଗଣ୍ଠି_Read_Write_Mem)->isHex();
    }
    bool isHex() ଅଚଳ { ଫେରନ୍ତୁ m_isHex; }
    ଆଭାସୀ ଅଚଳ ଅକ୍ଷର* cFuncPrefixp() ଅଚଳ = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstCaseItem(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condsp, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() + INSTR_ଗଣନା_BRANCH; }
    bool isDefault() ଅଚଳ { ଫେରନ୍ତୁ condsp() == nullptr; }
};

class AstForeachHeader final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
ସର୍ବସାଧାରଣ:
    AstForeachHeader(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

ସର୍ବସାଧାରଣ:
    AstAssertCtl(File_ଧାଡ଼ି* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* levelp = nullptr, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemsp = nullptr);
    AstAssertCtl(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* controlTypep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* assertTypesp = nullptr,
                 Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* directiveTypep = nullptr, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* levelp = nullptr,
                 Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ m_ctlType.ascii(); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    VAssertCtlType ctlType() ଅଚଳ { ଫେରନ୍ତୁ m_ctlType; }
    ଶୂନ୍ୟ ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ଅଚଳ { ଫେରନ୍ତୁ m_assertTypes; }
    ଶୂନ୍ୟ assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ଅଚଳ { ଫେରନ୍ତୁ m_directiveTypes; }
    ଶୂନ୍ୟ directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
};
class AstBreak final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
ସର୍ବସାଧାରଣ:
    explicit AstBreak(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "break"; }
    bool isBrancher() ଅଚଳ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
ସର୍ବସାଧାରଣ:
    AstCAwait(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_Sen_ବୃକ୍ଷ* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isTimingControl() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_Sen_ବୃକ୍ଷ* sentreep() ଅଚଳ { ଫେରନ୍ତୁ m_sentreep; }
    ଶୂନ୍ୟ clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstCLocalScope(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstCReturn(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCStmt final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ଅଚଳ VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    ସ୍ଥିର AstCStmt* profExecSection(File_ଧାଡ଼ି* flp, ଅଚଳ std::string& section, bool ଠେଲ) {
        // Compute the label
        std::string label;
        ଯଦି (v3Global.opt.hierChild()) {
            label += v3Global.opt.ଶୀର୍ଷ_Module() + ":";
        } ଅନ୍ୟଥା ଯଦି (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ଠେଲ ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        ଯଦି (ଠେଲ) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ଠେଲ ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        ଫେରନ୍ତୁ new AstCStmt{flp, ଠେଲ ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

ସର୍ବସାଧାରଣ:
    explicit AstCStmt(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& text = "",
                      ଅଚଳ VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        ଯଦି (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    VCStmtType stmtType() ଅଚଳ { ଫେରନ୍ତୁ m_stmtType; }
    // Add some text, or a node to this statement
    ଶୂନ୍ୟ add(ଅଚଳ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ଶୂନ୍ୟ add(Ast_ଗଣ୍ଠି* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    ସ୍ଥିର AstCStmt* prof_Exec_Section_ଠେଲ(File_ଧାଡ଼ି* flp, ଅଚଳ std::string& section) {
        ଫେରନ୍ତୁ profExecSection(flp, section, true);
    }
    ସ୍ଥିର AstCStmt* prof_Exec_Section_କାଢ଼(File_ଧାଡ଼ି* flp, ଅଚଳ std::string& section) {
        ଫେରନ୍ତୁ profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ଅଚଳ bool m_fromDollarC;  // Is from source '$c', emit decoration
ସର୍ବସାଧାରଣ:
    explicit AstCStmtUser(File_ଧାଡ଼ି* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool fromDollarC() ଅଚଳ { ଫେରନ୍ତୁ m_fromDollarC; }
    // Add some text, or a node to this statement
    ଶୂନ୍ୟ add(ଅଚଳ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ଶୂନ୍ୟ add(Ast_ଗଣ୍ଠି* nodep) { addNodesp(nodep); }
};
class AstCase final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
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
ସର୍ବସାଧାରଣ:
    AstCase(File_ଧାଡ଼ି* fl, VCaseType caseType, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ଅଚଳ { ଫେରନ୍ତୁ m_caseType == VCaseType::CT_CASEX; }
    bool casez() ଅଚଳ { ଫେରନ୍ତୁ m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ଅଚଳ { ଫେରନ୍ତୁ m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ଅଚଳ { ଫେରନ୍ତୁ m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ଅଚଳ { ଫେରନ୍ତୁ m_caseType == VCaseType::CT_CASE; }
    ଶୂନ୍ୟ caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    ଶୂନ୍ୟ caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() ଅଚଳ { ଫେରନ୍ତୁ m_fullPragma; }
    ଶୂନ୍ୟ fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() ଅଚଳ { ଫେରନ୍ତୁ m_parallelPragma; }
    ଶୂନ୍ୟ parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ଅଚଳ { ଫେରନ୍ତୁ m_uniquePragma; }
    ଶୂନ୍ୟ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ଅଚଳ { ଫେରନ୍ତୁ m_unique0Pragma; }
    ଶୂନ୍ୟ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ଅଚଳ { ଫେରନ୍ତୁ m_priorityPragma; }
    ଶୂନ୍ୟ priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() ଅଚଳ;
};
class AstComment final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Some comment to put into the output stream
    ଅଚଳ string m_name;  // Text of comment
    ଅଚଳ bool m_showAt;  // Show "at <fileline>"
ସର୍ବସାଧାରଣ:
    AstComment(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Text
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ true; }  // Ignore name in comments
    ଆଭାସୀ bool showAt() ଅଚଳ { ଫେରନ୍ତୁ m_showAt; }
};
class AstConstraintBefore final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    AstConstraintBefore(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhssp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_Constraint_ପଦସମଷ୍ଟି final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
ସର୍ବସାଧାରଣ:
    Ast_Constraint_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_Constraint_ପଦସମଷ୍ଟି(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_ପଦସମଷ୍ଟି;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isDisableSoft() ଅଚଳ { ଫେରନ୍ତୁ m_isDisableSoft; }
    ଶୂନ୍ୟ isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ଅଚଳ { ଫେରନ୍ତୁ m_isSoft; }
    ଶୂନ୍ୟ isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstConstraintUnique(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstContinue final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
ସର୍ବସାଧାରଣ:
    explicit AstContinue(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "continue"; }
    bool isBrancher() ଅଚଳ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

ସର୍ବସାଧାରଣ:
    AstCoverInc(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_Cover_Decl* declp, ଅଚଳ bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 1 + 2 * INSTR_ଗଣନା_LD; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstCoverInc* ଅଚଳ otherp = VN_DBG_AS(samep, CoverInc);
        ଫେରନ୍ତୁ declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    Ast_ଗଣ୍ଠି_Cover_Decl* declp() ଅଚଳ { ଫେରନ୍ତୁ m_declp; }  // Where defined
    ଶୂନ୍ୟ declp(Ast_ଗଣ୍ଠି_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ଅଚଳ { ଫେରନ୍ତୁ m_isDuplicate; }
};
class AstDeassign final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstDeassign(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ଅଚଳ bool m_isCycleDelay;  // True if it is a cycle delay
ସର୍ବସାଧାରଣ:
    AstDelay(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isTimingControl() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    bool isCycleDelay() ଅଚଳ { ଫେରନ୍ତୁ m_isCycleDelay; }
    bool isRangeDelay() ଅଚଳ { ଫେରନ୍ତୁ m_isCycleDelay && rhsp(); }
    bool isUnbounded() ଅଚଳ { ଫେରନ୍ତୁ isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    ଶୂନ୍ୟ fallDelay(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fallDelayp) { rhsp(fallDelayp); }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fallDelay() ଅଚଳ { ଫେରନ୍ତୁ m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
ସର୍ବସାଧାରଣ:
    AstDisable(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ targetp(Ast_ଗଣ୍ଠି* nodep) { m_targetp = nodep; }
    Ast_ଗଣ୍ଠି* targetp() ଅଚଳ { ଫେରନ୍ତୁ m_targetp; }
    bool isBrancher() ଅଚଳ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // A "disable fork" statement
ସର୍ବସାଧାରଣ:
    explicit AstDisableFork(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "disable fork"; }
};
class AstDisplay final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

ସର୍ବସାଧାରଣ:
    AstDisplay(File_ଧାଡ଼ି* fl, VDisplayType dispType, ଅଚଳ string& text, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep,
               Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp, ଅକ୍ଷର missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_ଧାଡ଼ି* fl, VDisplayType dispType, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp,
               ଅକ୍ଷର missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ଅଚଳ newp
            = new AstSFormatF{fl, AstSFormatF::ପଦସମଷ୍ଟି_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!fmtp());
        ଫେରନ୍ତୁ nullptr;
    }
    string verilogKwd() ଅଚଳ override {
        ଫେରନ୍ତୁ (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    VDisplayType displayType() ଅଚଳ { ଫେରନ୍ତୁ m_displayType; }
    ଶୂନ୍ୟ displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() ଅଚଳ { ଫେରନ୍ତୁ displayType().addNewline(); }
};
class AstDumpCtl final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ଅଚଳ VDumpCtlType m_ctlType;  // Type of operation
ସର୍ବସାଧାରଣ:
    AstDumpCtl(File_ଧାଡ଼ି* fl, VDumpCtlType ctlType, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ ctlType().ascii(); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    ଆଭାସୀ bool cleanOut() ଅଚଳ { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    VDumpCtlType ctlType() ଅଚଳ { ଫେରନ୍ତୁ m_ctlType; }
};
class AstEventControl final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstEventControl(File_ଧାଡ଼ି* fl, Ast_Sen_ବୃକ୍ଷ* sentreep, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "@(%l) %r"; }
    bool isTimingControl() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
};
class AstExecGraph final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
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
    V3Graph* ଅଚଳ m_depGraphp;  // contains ExecMTask vertices
    ଅଚଳ string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

ସର୍ବସାଧାରଣ:
    AstExecGraph(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    ଶୂନ୍ୟ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    V3Graph* depGraphp() { ଫେରନ୍ତୁ m_depGraphp; }
    ଅଚଳ V3Graph* depGraphp() ଅଚଳ { ଫେରନ୍ତୁ m_depGraphp; }
    AstCFunc* runfuncp() ଅଚଳ { ଫେରନ୍ତୁ m_runfuncp; }
    ଶୂନ୍ୟ runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
ସର୍ବସାଧାରଣ:
    AstFClose(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$fclose"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFFlush final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
ସର୍ବସାଧାରଣ:
    AstFFlush(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$fflush"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFinish final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
ସର୍ବସାଧାରଣ:
    explicit AstFinish(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }  // Rarely executes
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ fileline() == samep->fileline(); }
};
class AstFinishFork final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // $finish in fork
ସର୍ବସାଧାରଣ:
    explicit AstFinishFork(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }  // Rarely executes
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ fileline() == samep->fileline(); }
};
class AstFireEvent final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ଅଚଳ bool m_delayed;  // Delayed (->>) vs non-delayed (->)
ସର୍ବସାଧାରଣ:
    AstFireEvent(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isDelayed() ଅଚଳ { ଫେରନ୍ତୁ m_delayed; }
};
class AstInitialAutomaticStmt final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstInitialAutomaticStmt(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool isPure() override { ଫେରନ୍ତୁ true; }
};
class AstInitialStaticStmt final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstInitialStaticStmt(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool isPure() override { ଫେରନ୍ତୁ true; }
};
class AstJumpBlock final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
ସର୍ବସାଧାରଣ:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool isPure() override;

ବ୍ୟକ୍ତିଗତ:
    bool getPurityRecurse() ଅଚଳ;
};
class AstJumpGo final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
ସର୍ବସାଧାରଣ:
    AstJumpGo(File_ଧାଡ଼ି* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isBrancher() ଅଚଳ override {
        ଫେରନ୍ତୁ true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() ଅଚଳ { ଫେରନ୍ତୁ m_blockp; }
};
class AstLoop final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
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
ସର୍ବସାଧାରଣ:
    explicit AstLoop(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* thatp) ଅଚଳ override {
        ଫେରନ୍ତୁ m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    // ACCESSORS
    VOptionBool unroll() ଅଚଳ { ଫେରନ୍ତୁ m_unroll; }
    ଶୂନ୍ୟ unroll(ଅଚଳ VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
ସର୍ବସାଧାରଣ:
    AstLoopTest(File_ଧାଡ଼ି* fl, AstLoop* loopp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isBrancher() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    // ACCESSORS
    AstLoop* loopp() ଅଚଳ { ଫେରନ୍ତୁ m_loopp; }
};
class AstMonitorOff final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    ଅଚଳ bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

ସର୍ବସାଧାରଣ:
    AstMonitorOff(File_ଧାଡ଼ି* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Though deleted before opt
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Though deleted before opt
    bool isPure() override { ଫେରନ୍ତୁ false; }  // Though deleted before opt
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // Though deleted before opt
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ଅଚଳ { ଫେରନ୍ତୁ m_off; }
};
class AstPExprClause final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    ଅଚଳ bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ଅଚଳ bool m_vacuous;  // Pass is vacuous

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_ଧାଡ଼ି* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool pass() ଅଚଳ { ଫେରନ୍ତୁ m_pass; }
    bool vacuous() ଅଚଳ { ଫେରନ୍ତୁ m_vacuous; }
};
class AstPrintTimeScale final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
ସର୍ବସାଧାରଣ:
    explicit AstPrintTimeScale(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$printtimescale"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
};
class AstRSBreak final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // randsequence break
ସର୍ବସାଧାରଣ:
    explicit AstRSBreak(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "break"; }
    bool isBrancher() ଅଚଳ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
ସର୍ବସାଧାରଣ:
    AstRSProd(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
};
class AstRSProdItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
ସର୍ବସାଧାରଣ:
    AstRSProdItem(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    AstRSProd* prodp() ଅଚଳ { ଫେରନ୍ତୁ m_prodp; }
    ଶୂନ୍ୟ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
ସର୍ବସାଧାରଣ:
    AstRSProdList(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* weightp, Ast_ଗଣ୍ଠି* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    bool randJoin() ଅଚଳ { ଫେରନ୍ତୁ m_randJoin; }
    ଶୂନ୍ୟ randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // randsequence return
ସର୍ବସାଧାରଣ:
    explicit AstRSReturn(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "return"; }
    bool isBrancher() ଅଚଳ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstRSRule(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* weightp, AstRSProdList* prodlistsp, Ast_ଗଣ୍ଠି* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
};
class AstRandCase final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
ସର୍ବସାଧାରଣ:
    AstRandCase(File_ଧାଡ଼ି* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
};
class AstRandSequence final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
ସର୍ବସାଧାରଣ:
    AstRandSequence(File_ଧାଡ଼ି* fl, ଅଚଳ string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Block name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string start() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_start; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    AstRSProd* prodp() ଅଚଳ { ଫେରନ୍ତୁ m_prodp; }
    ଶୂନ୍ୟ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstRelease(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstRepeat(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Not relevant - converted to FOR
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstReturn final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    explicit AstReturn(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "return"; }
    bool isBrancher() ଅଚଳ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstSFormat(File_ଧାଡ଼ି* fl, bool optionalFormat, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp,
               ଅକ୍ଷର missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ଅଚଳ newp
            = new AstSFormatF{fl, AstSFormatF::ପଦସମଷ୍ଟି_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_ଧାଡ଼ି* fl, AstSFormatF* fmtp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!fmtp());
        ଫେରନ୍ତୁ nullptr;
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$sformat"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isPure() override { ଫେରନ୍ତୁ true; }
    bool isOutputter() override { ଫେରନ୍ତୁ false; }
    ଆଭାସୀ bool cleanOut() ଅଚଳ { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSetuphold final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    AstSetuphold(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* refevp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* dataevp,
                 Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* delrefp = nullptr, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_ଗଦା_Trace_T final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // $stacktrace used as task
ସର୍ବସାଧାରଣ:
    explicit Ast_ଗଦା_Trace_T(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_ଗଦା_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_ଗଦା_Trace_T;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$stacktrace"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_Stmt_ପଦସମଷ୍ଟି final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    Ast_Stmt_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_Stmt_ପଦସମଷ୍ଟି(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_ପଦସମଷ୍ଟି;
    // cppcheck-suppress uselessOverride
    bool isPure() override { ଫେରନ୍ତୁ exprp()->isPure(); }
};
class AstStmtPragma final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
ସର୍ବସାଧାରଣ:
    AstStmtPragma(File_ଧାଡ଼ି* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    ଅଚଳ bool m_isFatal;  // $fatal not $stop
ସର୍ବସାଧାରଣ:
    AstStop(File_ଧାଡ଼ି* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }  // Rarely executes
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ fileline() == samep->fileline(); }
    string emitVerilog() ଅଚଳ { ଫେରନ୍ତୁ m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ଅଚଳ { ଫେରନ୍ତୁ m_isFatal; }
};
class AstSystemT final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstSystemT(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$system"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstTimeFormat final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    AstTimeFormat(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* unitsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* precisionp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* suffixp,
                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$timeformat"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
};
class AstTraceDecl final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
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
    ଅଚଳ string m_showname;  // Name of variable
    ଅଚଳ VNumRange m_bitRange;  // Property of var the trace details
    ଅଚଳ VNumRange m_arrayRange;  // Property of var the trace details
    ଅଚଳ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ଅଚଳ VDirection m_declDirection;  // Declared direction input/output etc
    ଅଚଳ bool m_inDtypeFunc;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_codeInc{0};  // Code increment for type
ସର୍ବସାଧାରଣ:
    AstTraceDecl(File_ଧାଡ଼ି* fl, ଅଚଳ string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, ଅଚଳ VNumRange& bitRange, ଅଚଳ VNumRange& arrayRange,
                 AstCCall* ଅଚଳ dtypeCallp, ଅଚଳ bool inDtypeFunc)
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
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_showname; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    string showname() ଅଚଳ { ଫେରନ୍ତୁ m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() ଅଚଳ { ଫେରନ୍ତୁ m_code; }
    ଶୂନ୍ୟ code(uint32_t code) { m_code = code; }
    bool codeAssigned() ଅଚଳ { ଫେରନ୍ତୁ m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ଅଚଳ { ଫେରନ୍ତୁ m_fidx; }
    ଶୂନ୍ୟ fidx(uint32_t fidx) { m_fidx = fidx; }
    ଶୂନ୍ୟ codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() ଅଚଳ {
        ଯଦି (m_codeInc) { ଫେରନ୍ତୁ m_codeInc; }
        ଫେରନ୍ତୁ (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_ଶବ୍ଦଗୁଡ଼ିକ()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ଅଚଳ VNumRange& bitRange() ଅଚଳ { ଫେରନ୍ତୁ m_bitRange; }
    ଅଚଳ VNumRange& arrayRange() ଅଚଳ { ଫେରନ୍ତୁ m_arrayRange; }
    VVarType varType() ଅଚଳ { ଫେରନ୍ତୁ m_varType; }
    VDirection declDirection() ଅଚଳ { ଫେରନ୍ତୁ m_declDirection; }
    AstCCall* dtypeCallp() ଅଚଳ { ଫେରନ୍ତୁ m_dtypeCallp; }
    ଶୂନ୍ୟ dtypeCallp(AstCCall* ଅଚଳ callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ଅଚଳ { ଫେରନ୍ତୁ m_dtypeDeclp; }
    ଶୂନ୍ୟ dtypeDeclp(AstTraceDecl* ଅଚଳ declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() ଅଚଳ { ଫେରନ୍ତୁ m_inDtypeFunc; }
};
class AstTraceInc final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ଅଚଳ uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    ଅଚଳ VTraceType m_traceType;  // Is this a const/full/incremental dump

ସର୍ବସାଧାରଣ:
    AstTraceInc(File_ଧାଡ଼ି* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_ବୃକ୍ଷ(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 10 + 2 * INSTR_ଗଣନା_LD; }
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    AstTraceDecl* declp() ଅଚଳ { ଫେରନ୍ତୁ m_declp; }
    VTraceType traceType() ଅଚଳ { ଫେରନ୍ତୁ m_traceType; }
    uint32_t baseCode() ଅଚଳ { ଫେରନ୍ତୁ m_baseCode; }
};
class Ast_Trace_କାଢ଼_Prefix final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
ସର୍ବସାଧାରଣ:
    explicit Ast_Trace_କାଢ଼_Prefix(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_Trace_କାଢ଼_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_କାଢ଼_Prefix;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class Ast_Trace_ଠେଲ_Prefix final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    ଅଚଳ string m_prefix;  // Prefix to add to signal names
    ଅଚଳ VTracePrefixType m_prefixType;  // Type of prefix being pushed
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ବାମ;  // Array left index, or struct/union member count
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ଡାହାଣ;  // Array right index
    ଅଚଳ bool m_quotedPrefix;  // Quote prefix name
ସର୍ବସାଧାରଣ:
    Ast_Trace_ଠେଲ_Prefix(File_ଧାଡ଼ି* fl, ଅଚଳ string& prefix, VTracePrefixType prefixType,
                       ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ = 0, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_ଠେଲ_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_ବାମ{ବାମ}
        , m_ଡାହାଣ{ଡାହାଣ}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_ଠେଲ_Prefix;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string prefix() ଅଚଳ { ଫେରନ୍ତୁ m_prefix; }
    VTracePrefixType prefixType() ଅଚଳ { ଫେରନ୍ତୁ m_prefixType; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ() ଅଚଳ { ଫେରନ୍ତୁ m_ବାମ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ() ଅଚଳ { ଫେରନ୍ତୁ m_ଡାହାଣ; }
    bool quotedPrefix() ଅଚଳ { ଫେରନ୍ତୁ m_quotedPrefix; }
};
class AstWait final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstWait(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstWaitFork final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stmt {
    // A "wait fork" statement
ସର୍ବସାଧାରଣ:
    explicit AstWaitFork(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Assign {
ସର୍ବସାଧାରଣ:
    AstAssign(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp,
              Ast_ଗଣ୍ଠି* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_ଗଣ୍ଠି_Assign* cloneType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) override {
        Ast_ଗଣ୍ଠି* ଅଚଳ controlp = timingControlp() ? timingControlp()->clone_ବୃକ୍ଷ(false) : nullptr;
        ଫେରନ୍ତୁ new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Assign {
    // Compound assignments (+=, -=, *=, ...)
ସର୍ବସାଧାରଣ:
    ଗଣନା operation : uint8_t {
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

ବ୍ୟକ୍ତିଗତ:
    operation m_operation;

ସର୍ବସାଧାରଣ:
    AstAssignCompound(AstAssignCompound::operation operation, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp,
                      Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_ଗଣ୍ଠି_Assign* cloneType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) override {
        Ast_ଗଣ୍ଠି* ଅଚଳ controlp = timingControlp() ? timingControlp()->clone_ବୃକ୍ଷ(false) : nullptr;
        ଫେରନ୍ତୁ new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { ଫେରନ୍ତୁ m_operation; }
};
class AstAssignCont final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
ସର୍ବସାଧାରଣ:
    AstAssignCont(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp,
                  Ast_ଗଣ୍ଠି* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_ଗଣ୍ଠି_Assign* cloneType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) override {
        Ast_ଗଣ୍ଠି* ଅଚଳ controlp = timingControlp() ? timingControlp()->clone_ବୃକ୍ଷ(false) : nullptr;
        ଫେରନ୍ତୁ new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Assign {
ସର୍ବସାଧାରଣ:
    AstAssignDly(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp,
                 Ast_ଗଣ୍ଠି* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_ଗଣ୍ଠି_Assign* cloneType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) override {
        Ast_ଗଣ୍ଠି* ଅଚଳ controlp = timingControlp() ? timingControlp()->clone_ବୃକ୍ଷ(false) : nullptr;
        ଫେରନ୍ତୁ new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "<="; }
};
class AstAssignForce final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Assign {
    // Procedural 'force' statement
ସର୍ବସାଧାରଣ:
    AstAssignForce(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_ଗଣ୍ଠି_Assign* cloneType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) override {
        ଫେରନ୍ତୁ new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
ସର୍ବସାଧାରଣ:
    AstAssignW(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp,
               Ast_ଗଣ୍ଠି* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_ଗଣ୍ଠି_Assign* cloneType(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) override {
        Ast_ଗଣ୍ଠି* ଅଚଳ controlp = timingControlp() ? timingControlp()->clone_ବୃକ୍ଷ(false) : nullptr;
        ଫେରନ୍ତୁ new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ଅଚଳ override {
        ଫେରନ୍ତୁ timingControlp() || lhsp()->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* refp) {
            ଫେରନ୍ତୁ refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ଅଚଳ;
};

// === AstNodeBlock ===
class AstBegin final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ଅଚଳ bool m_implied : 1;  // Not inserted by user
ସର୍ବସାଧାରଣ:
    AstBegin(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ setNeedProcess() { m_needProcess = true; }
    bool needProcess() ଅଚଳ { ଫେରନ୍ତୁ m_needProcess; }
    bool implied() ଅଚଳ { ଫେରନ୍ତୁ m_implied; }
};
class AstFork final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Block {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    ଅଚଳ VJoinType m_joinType;  // Join keyword type
    bool m_immediateStart = false;  // Fork starts before its parent blocks or exits

ସର୍ବସାଧାରଣ:
    AstFork(File_ଧାଡ଼ି* fl, VJoinType joinType, ଅଚଳ string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstFork* ଅଚଳ asamep = VN_DBG_AS(samep, Fork);
        ଫେରନ୍ତୁ joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() ଅଚଳ override { ଫେରନ୍ତୁ !joinType().joinNone(); }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    VJoinType joinType() ଅଚଳ { ଫେରନ୍ତୁ m_joinType; }
    bool immediateStart() ଅଚଳ { ଫେରନ୍ତୁ m_immediateStart; }
    ଶୂନ୍ୟ immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* propp, Ast_ଗଣ୍ଠି* passsp, Ast_ଗଣ୍ଠି* failsp, VAssertType type,
              VAssertDirectiveType directive, ଅଚଳ string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ଅଚଳ override {
        ଫେରନ୍ତୁ directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* propp, Ast_ଗଣ୍ଠି* passsp, Ast_ଗଣ୍ଠି* failsp,
                       ଅଚଳ string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "assert"; }
};
class AstCover final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* propp, Ast_ଗଣ୍ଠି* stmtsp, VAssertType type,
             ଅଚଳ string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "cover"; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isCoverSeq() ଅଚଳ { ଫେରନ୍ତୁ m_isCoverSeq; }
    ଶୂନ୍ୟ isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ଅଚଳ { ଫେରନ୍ତୁ m_isSeqEvent; }
    ଶୂନ୍ୟ isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cover_Or_Assert {
ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
ସର୍ବସାଧାରଣ:
    AstConstraintForeach(File_ଧାଡ଼ି* fl, AstForeachHeader* headerp, Ast_ଗଣ୍ଠି* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ଅଚଳ { ଫେରନ୍ତୁ m_soft; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstForeach final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Foreach {
ସର୍ବସାଧାରଣ:
    AstForeach(File_ଧାଡ଼ି* fl, AstForeachHeader* headerp, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_If {
ସର୍ବସାଧାରଣ:
    AstConstraintIf(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି* thensp, Ast_ଗଣ୍ଠି* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
ସର୍ବସାଧାରଣ:
    AstIf(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି* thensp = nullptr, Ast_ଗଣ୍ଠି* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ଅଚଳ { ଫେରନ୍ତୁ m_uniquePragma; }
    ଶୂନ୍ୟ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ଅଚଳ { ଫେରନ୍ତୁ m_unique0Pragma; }
    ଶୂନ୍ୟ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ଅଚଳ { ଫେରନ୍ତୁ m_priorityPragma; }
    ଶୂନ୍ୟ priorityPragma(bool flag) { m_priorityPragma = flag; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Read_Write_Mem {
ସର୍ବସାଧାରଣ:
    AstReadMem(File_ଧାଡ଼ି* fl, bool hex, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filenamep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* memp,
               Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ (isHex() ? "$readmemh" : "$readmemb"); }
    ଅଚଳ ଅକ୍ଷର* cFuncPrefixp() ଅଚଳ override { ଫେରନ୍ତୁ "VL_READMEM_"; }
};
class AstWriteMem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Read_Write_Mem {
ସର୍ବସାଧାରଣ:
    AstWriteMem(File_ଧାଡ଼ି* fl, bool hex, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filenamep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* memp,
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ (isHex() ? "$writememh" : "$writememb"); }
    ଅଚଳ ଅକ୍ଷର* cFuncPrefixp() ଅଚଳ override { ଫେରନ୍ତୁ "VL_WRITEMEM_"; }
};

#endif  // Guard
