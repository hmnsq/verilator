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
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_ಗಂಟು_Stmt VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Procedural statement
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Stmt(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Stmt;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
};
class Ast_ಗಂಟು_Assign VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Assign(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
                  Ast_ಗಂಟು* timingControlp = nullptr)
        : Ast_ಗಂಟು_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Assign;
    // Clone single node, just get same type back.
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಆಭಾಸ Ast_ಗಂಟು_Assign* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) = 0;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಆಭಾಸ bool cleanRhs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "="; }
    bool isTimingControl() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ timingControlp(); }
};
class Ast_ಗಂಟು_Block VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Block(VNType t, File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : Ast_ಗಂಟು_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Block;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Block name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    bool unnamed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unnamed; }
};
class Ast_ಗಂಟು_Cover_Or_Assert VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    ಧ್ರುವ VAssertType m_userType;  // Assertion/cover type for user enable/disable
    ಧ್ರುವ VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_immediate = false;  // Immediate assert (may differ from userType being immediate)

ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Cover_Or_Assert(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ಧ್ರುವ string& name = "")
        : Ast_ಗಂಟು_Stmt{t, fl}
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
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Cover_Or_Assert;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ samep->name() == name(); }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string type() { ಪ್ರತ್ಯಾಗಮ ""; }
    VAssertType userType() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_userType; }
    VAssertDirectiveType directive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_directive; }
    bool immediate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_immediate; }
    ಶೂನ್ಯ immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_senFromAlways; }
    ಶೂನ್ಯ senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_ಗಂಟು_Foreach VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Foreach(VNType t, File_ಸಾಲು* fl, AstForeachHeader* headerp, Ast_ಗಂಟು* bodyp)
        : Ast_ಗಂಟು_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Foreach;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_ಗಂಟು_If VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_If(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* thensp, Ast_ಗಂಟು* elsesp)
        : Ast_ಗಂಟು_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_If;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isGateDedupable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_branchPred; }
    ಶೂನ್ಯ isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isBoundsCheck; }
};
class Ast_ಗಂಟು_Read_Write_Mem VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ಧ್ರುವ bool m_isHex;  // readmemh, not readmemb
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Read_Write_Mem(VNType t, File_ಸಾಲು* fl, bool hex, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep,
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* memp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* msbp)
        : Ast_ಗಂಟು_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Read_Write_Mem;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ isHex() == VN_DBG_AS(samep, ಗಂಟು_Read_Write_Mem)->isHex();
    }
    bool isHex() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isHex; }
    ಆಭಾಸ ಧ್ರುವ ವರ್ಣ* cFuncPrefixp() ಧ್ರುವ = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstCaseItem(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condsp, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
    bool isDefault() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ condsp() == nullptr; }
};

class AstForeachHeader final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
ಸಾರ್ವಜನಿಕ:
    AstForeachHeader(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

ಸಾರ್ವಜನಿಕ:
    AstAssertCtl(File_ಸಾಲು* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* levelp = nullptr, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemsp = nullptr);
    AstAssertCtl(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* controlTypep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* assertTypesp = nullptr,
                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* directiveTypep = nullptr, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* levelp = nullptr,
                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_ctlType.ascii(); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    VAssertCtlType ctlType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ctlType; }
    ಶೂನ್ಯ ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_assertTypes; }
    ಶೂನ್ಯ assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_directiveTypes; }
    ಶೂನ್ಯ directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
};
class AstBreak final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
ಸಾರ್ವಜನಿಕ:
    explicit AstBreak(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "break"; }
    bool isBrancher() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
ಸಾರ್ವಜನಿಕ:
    AstCAwait(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Sen_ವೃಕ್ಷ* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isTimingControl() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_Sen_ವೃಕ್ಷ* sentreep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sentreep; }
    ಶೂನ್ಯ clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstCLocalScope(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstCReturn(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCStmt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ಧ್ರುವ VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    ಸ್ಥಿರ AstCStmt* profExecSection(File_ಸಾಲು* flp, ಧ್ರುವ std::string& section, bool ತಳ್ಳು) {
        // Compute the label
        std::string label;
        ಯದಿ (v3Global.opt.hierChild()) {
            label += v3Global.opt.ತುದಿ_Module() + ":";
        } ಅನ್ಯಥಾ ಯದಿ (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ತಳ್ಳು ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        ಯದಿ (ತಳ್ಳು) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ತಳ್ಳು ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        ಪ್ರತ್ಯಾಗಮ new AstCStmt{flp, ತಳ್ಳು ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

ಸಾರ್ವಜನಿಕ:
    explicit AstCStmt(File_ಸಾಲು* fl, ಧ್ರುವ std::string& text = "",
                      ಧ್ರುವ VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        ಯದಿ (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    VCStmtType stmtType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stmtType; }
    // Add some text, or a node to this statement
    ಶೂನ್ಯ add(ಧ್ರುವ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ಶೂನ್ಯ add(Ast_ಗಂಟು* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    ಸ್ಥಿರ AstCStmt* prof_Exec_Section_ತಳ್ಳು(File_ಸಾಲು* flp, ಧ್ರುವ std::string& section) {
        ಪ್ರತ್ಯಾಗಮ profExecSection(flp, section, true);
    }
    ಸ್ಥಿರ AstCStmt* prof_Exec_Section_ತೆಗೆ(File_ಸಾಲು* flp, ಧ್ರುವ std::string& section) {
        ಪ್ರತ್ಯಾಗಮ profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ಧ್ರುವ bool m_fromDollarC;  // Is from source '$c', emit decoration
ಸಾರ್ವಜನಿಕ:
    explicit AstCStmtUser(File_ಸಾಲು* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool fromDollarC() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fromDollarC; }
    // Add some text, or a node to this statement
    ಶೂನ್ಯ add(ಧ್ರುವ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ಶೂನ್ಯ add(Ast_ಗಂಟು* nodep) { addNodesp(nodep); }
};
class AstCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
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
ಸಾರ್ವಜನಿಕ:
    AstCase(File_ಸಾಲು* fl, VCaseType caseType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_caseType == VCaseType::CT_CASEX; }
    bool casez() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_caseType == VCaseType::CT_CASE; }
    ಶೂನ್ಯ caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    ಶೂನ್ಯ caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fullPragma; }
    ಶೂನ್ಯ fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_parallelPragma; }
    ಶೂನ್ಯ parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_uniquePragma; }
    ಶೂನ್ಯ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unique0Pragma; }
    ಶೂನ್ಯ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_priorityPragma; }
    ಶೂನ್ಯ priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() ಧ್ರುವ;
};
class AstComment final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Some comment to put into the output stream
    ಧ್ರುವ string m_name;  // Text of comment
    ಧ್ರುವ bool m_showAt;  // Show "at <fileline>"
ಸಾರ್ವಜನಿಕ:
    AstComment(File_ಸಾಲು* fl, ಧ್ರುವ string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Text
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }  // Ignore name in comments
    ಆಭಾಸ bool showAt() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_showAt; }
};
class AstConstraintBefore final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstConstraintBefore(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhssp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
ಸಾರ್ವಜನಿಕ:
    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Constraint_ಅಭಿವ್ಯಕ್ತಿ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isDisableSoft() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isDisableSoft; }
    ಶೂನ್ಯ isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isSoft; }
    ಶೂನ್ಯ isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstConstraintUnique(File_ಸಾಲು* fl, Ast_ಗಂಟು* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstContinue final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
ಸಾರ್ವಜನಿಕ:
    explicit AstContinue(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "continue"; }
    bool isBrancher() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

ಸಾರ್ವಜನಿಕ:
    AstCoverInc(File_ಸಾಲು* fl, Ast_ಗಂಟು_Cover_Decl* declp, ಧ್ರುವ bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1 + 2 * INSTR_ಎಣಿಕೆ_LD; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstCoverInc* ಧ್ರುವ otherp = VN_DBG_AS(samep, CoverInc);
        ಪ್ರತ್ಯಾಗಮ declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    Ast_ಗಂಟು_Cover_Decl* declp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_declp; }  // Where defined
    ಶೂನ್ಯ declp(Ast_ಗಂಟು_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isDuplicate; }
};
class AstDeassign final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstDeassign(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ಧ್ರುವ bool m_isCycleDelay;  // True if it is a cycle delay
ಸಾರ್ವಜನಿಕ:
    AstDelay(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isTimingControl() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    bool isCycleDelay() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isCycleDelay; }
    bool isRangeDelay() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isCycleDelay && rhsp(); }
    bool isUnbounded() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    ಶೂನ್ಯ fallDelay(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fallDelayp) { rhsp(fallDelayp); }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fallDelay() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
ಸಾರ್ವಜನಿಕ:
    AstDisable(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ targetp(Ast_ಗಂಟು* nodep) { m_targetp = nodep; }
    Ast_ಗಂಟು* targetp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_targetp; }
    bool isBrancher() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // A "disable fork" statement
ಸಾರ್ವಜನಿಕ:
    explicit AstDisableFork(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "disable fork"; }
};
class AstDisplay final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

ಸಾರ್ವಜನಿಕ:
    AstDisplay(File_ಸಾಲು* fl, VDisplayType dispType, ಧ್ರುವ string& text, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep,
               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp, ವರ್ಣ missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_ಸಾಲು* fl, VDisplayType dispType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp,
               ವರ್ಣ missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ಧ್ರುವ newp
            = new AstSFormatF{fl, AstSFormatF::ಅಭಿವ್ಯಕ್ತಿ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!fmtp());
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    string verilogKwd() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    VDisplayType displayType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_displayType; }
    ಶೂನ್ಯ displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ displayType().addNewline(); }
};
class AstDumpCtl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ಧ್ರುವ VDumpCtlType m_ctlType;  // Type of operation
ಸಾರ್ವಜನಿಕ:
    AstDumpCtl(File_ಸಾಲು* fl, VDumpCtlType ctlType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ctlType().ascii(); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    ಆಭಾಸ bool cleanOut() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    VDumpCtlType ctlType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ctlType; }
};
class AstEventControl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstEventControl(File_ಸಾಲು* fl, Ast_Sen_ವೃಕ್ಷ* sentreep, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "@(%l) %r"; }
    bool isTimingControl() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
};
class AstExecGraph final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
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
    V3Graph* ಧ್ರುವ m_depGraphp;  // contains ExecMTask vertices
    ಧ್ರುವ string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

ಸಾರ್ವಜನಿಕ:
    AstExecGraph(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    ಶೂನ್ಯ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    V3Graph* depGraphp() { ಪ್ರತ್ಯಾಗಮ m_depGraphp; }
    ಧ್ರುವ V3Graph* depGraphp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_depGraphp; }
    AstCFunc* runfuncp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_runfuncp; }
    ಶೂನ್ಯ runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
ಸಾರ್ವಜನಿಕ:
    AstFClose(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$fclose"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFFlush final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
ಸಾರ್ವಜನಿಕ:
    AstFFlush(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$fflush"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFinish final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
ಸಾರ್ವಜನಿಕ:
    explicit AstFinish(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }  // Rarely executes
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ fileline() == samep->fileline(); }
};
class AstFinishFork final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // $finish in fork
ಸಾರ್ವಜನಿಕ:
    explicit AstFinishFork(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }  // Rarely executes
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ fileline() == samep->fileline(); }
};
class AstFireEvent final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ಧ್ರುವ bool m_delayed;  // Delayed (->>) vs non-delayed (->)
ಸಾರ್ವಜನಿಕ:
    AstFireEvent(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isDelayed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_delayed; }
};
class AstInitialAutomaticStmt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstInitialAutomaticStmt(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstInitialStaticStmt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstInitialStaticStmt(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstJumpBlock final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
ಸಾರ್ವಜನಿಕ:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override;

ಗುಪ್ತ:
    bool getPurityRecurse() ಧ್ರುವ;
};
class AstJumpGo final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
ಸಾರ್ವಜನಿಕ:
    AstJumpGo(File_ಸಾಲು* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isBrancher() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_blockp; }
};
class AstLoop final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
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
ಸಾರ್ವಜನಿಕ:
    explicit AstLoop(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* thatp) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    // ACCESSORS
    VOptionBool unroll() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unroll; }
    ಶೂನ್ಯ unroll(ಧ್ರುವ VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
ಸಾರ್ವಜನಿಕ:
    AstLoopTest(File_ಸಾಲು* fl, AstLoop* loopp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isBrancher() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    // ACCESSORS
    AstLoop* loopp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_loopp; }
};
class AstMonitorOff final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    ಧ್ರುವ bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

ಸಾರ್ವಜನಿಕ:
    AstMonitorOff(File_ಸಾಲು* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Though deleted before opt
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Though deleted before opt
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // Though deleted before opt
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // Though deleted before opt
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_off; }
};
class AstPExprClause final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    ಧ್ರುವ bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ಧ್ರುವ bool m_vacuous;  // Pass is vacuous

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_ಸಾಲು* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool pass() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pass; }
    bool vacuous() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vacuous; }
};
class AstPrintTimeScale final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    explicit AstPrintTimeScale(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$printtimescale"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
};
class AstRSBreak final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randsequence break
ಸಾರ್ವಜನಿಕ:
    explicit AstRSBreak(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "break"; }
    bool isBrancher() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
ಸಾರ್ವಜನಿಕ:
    AstRSProd(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstRSProdItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
ಸಾರ್ವಜನಿಕ:
    AstRSProdItem(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    AstRSProd* prodp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_prodp; }
    ಶೂನ್ಯ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
ಸಾರ್ವಜನಿಕ:
    AstRSProdList(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp, Ast_ಗಂಟು* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool randJoin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_randJoin; }
    ಶೂನ್ಯ randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randsequence return
ಸಾರ್ವಜನಿಕ:
    explicit AstRSReturn(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "return"; }
    bool isBrancher() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstRSRule(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp, AstRSProdList* prodlistsp, Ast_ಗಂಟು* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstRandCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
ಸಾರ್ವಜನಿಕ:
    AstRandCase(File_ಸಾಲು* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstRandSequence final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
ಸಾರ್ವಜನಿಕ:
    AstRandSequence(File_ಸಾಲು* fl, ಧ್ರುವ string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Block name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string start() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_start; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    AstRSProd* prodp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_prodp; }
    ಶೂನ್ಯ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstRelease(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstRepeat(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Not relevant - converted to FOR
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstReturn final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    explicit AstReturn(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "return"; }
    bool isBrancher() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstSFormat(File_ಸಾಲು* fl, bool optionalFormat, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp,
               ವರ್ಣ missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ಧ್ರುವ newp
            = new AstSFormatF{fl, AstSFormatF::ಅಭಿವ್ಯಕ್ತಿ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_ಸಾಲು* fl, AstSFormatF* fmtp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!fmtp());
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$sformat"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ false; }
    ಆಭಾಸ bool cleanOut() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSetuphold final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstSetuphold(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* refevp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* dataevp,
                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delrefp = nullptr, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_ರಾಶಿ_Trace_T final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // $stacktrace used as task
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ರಾಶಿ_Trace_T(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ರಾಶಿ_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_ರಾಶಿ_Trace_T;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$stacktrace"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Stmt_ಅಭಿವ್ಯಕ್ತಿ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ;
    // cppcheck-suppress uselessOverride
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ exprp()->isPure(); }
};
class AstStmtPragma final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
ಸಾರ್ವಜನಿಕ:
    AstStmtPragma(File_ಸಾಲು* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    ಧ್ರುವ bool m_isFatal;  // $fatal not $stop
ಸಾರ್ವಜನಿಕ:
    AstStop(File_ಸಾಲು* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }  // Rarely executes
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ fileline() == samep->fileline(); }
    string emitVerilog() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isFatal; }
};
class AstSystemT final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstSystemT(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$system"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstTimeFormat final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstTimeFormat(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* unitsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* precisionp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* suffixp,
                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$timeformat"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
};
class AstTraceDecl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
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
    ಧ್ರುವ string m_showname;  // Name of variable
    ಧ್ರುವ VNumRange m_bitRange;  // Property of var the trace details
    ಧ್ರುವ VNumRange m_arrayRange;  // Property of var the trace details
    ಧ್ರುವ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ಧ್ರುವ VDirection m_declDirection;  // Declared direction input/output etc
    ಧ್ರುವ bool m_inDtypeFunc;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    ಪೂರ್ಣಾಙ್ಕ m_codeInc{0};  // Code increment for type
ಸಾರ್ವಜನಿಕ:
    AstTraceDecl(File_ಸಾಲು* fl, ಧ್ರುವ string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, ಧ್ರುವ VNumRange& bitRange, ಧ್ರುವ VNumRange& arrayRange,
                 AstCCall* ಧ್ರುವ dtypeCallp, ಧ್ರುವ bool inDtypeFunc)
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
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_showname; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    string showname() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_code; }
    ಶೂನ್ಯ code(uint32_t code) { m_code = code; }
    bool codeAssigned() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fidx; }
    ಶೂನ್ಯ fidx(uint32_t fidx) { m_fidx = fidx; }
    ಶೂನ್ಯ codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() ಧ್ರುವ {
        ಯದಿ (m_codeInc) { ಪ್ರತ್ಯಾಗಮ m_codeInc; }
        ಪ್ರತ್ಯಾಗಮ (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_ಪದಗಳು()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ಧ್ರುವ VNumRange& bitRange() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bitRange; }
    ಧ್ರುವ VNumRange& arrayRange() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_arrayRange; }
    VVarType varType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_varType; }
    VDirection declDirection() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_declDirection; }
    AstCCall* dtypeCallp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dtypeCallp; }
    ಶೂನ್ಯ dtypeCallp(AstCCall* ಧ್ರುವ callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dtypeDeclp; }
    ಶೂನ್ಯ dtypeDeclp(AstTraceDecl* ಧ್ರುವ declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inDtypeFunc; }
};
class AstTraceInc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ಧ್ರುವ uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    ಧ್ರುವ VTraceType m_traceType;  // Is this a const/full/incremental dump

ಸಾರ್ವಜನಿಕ:
    AstTraceInc(File_ಸಾಲು* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_ವೃಕ್ಷ(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 10 + 2 * INSTR_ಎಣಿಕೆ_LD; }
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    AstTraceDecl* declp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_declp; }
    VTraceType traceType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceType; }
    uint32_t baseCode() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_baseCode; }
};
class Ast_Trace_ತೆಗೆ_Prefix final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
ಸಾರ್ವಜನಿಕ:
    explicit Ast_Trace_ತೆಗೆ_Prefix(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Trace_ತೆಗೆ_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_ತೆಗೆ_Prefix;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class Ast_Trace_ತಳ್ಳು_Prefix final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    ಧ್ರುವ string m_prefix;  // Prefix to add to signal names
    ಧ್ರುವ VTracePrefixType m_prefixType;  // Type of prefix being pushed
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_ಎಡ;  // Array left index, or struct/union member count
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_ಬಲ;  // Array right index
    ಧ್ರುವ bool m_quotedPrefix;  // Quote prefix name
ಸಾರ್ವಜನಿಕ:
    Ast_Trace_ತಳ್ಳು_Prefix(File_ಸಾಲು* fl, ಧ್ರುವ string& prefix, VTracePrefixType prefixType,
                       ಪೂರ್ಣಾಙ್ಕ ಎಡ = 0, ಪೂರ್ಣಾಙ್ಕ ಬಲ = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_ತಳ್ಳು_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_ಎಡ{ಎಡ}
        , m_ಬಲ{ಬಲ}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_ತಳ್ಳು_Prefix;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string prefix() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_prefix; }
    VTracePrefixType prefixType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_prefixType; }
    ಪೂರ್ಣಾಙ್ಕ ಎಡ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಎಡ; }
    ಪೂರ್ಣಾಙ್ಕ ಬಲ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಬಲ; }
    bool quotedPrefix() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_quotedPrefix; }
};
class AstWait final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstWait(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstWaitFork final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // A "wait fork" statement
ಸಾರ್ವಜನಿಕ:
    explicit AstWaitFork(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Assign {
ಸಾರ್ವಜನಿಕ:
    AstAssign(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
              Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_ಗಂಟು_Assign* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಧ್ರುವ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಪ್ರತ್ಯಾಗಮ new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Assign {
    // Compound assignments (+=, -=, *=, ...)
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ operation : uint8_t {
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

ಗುಪ್ತ:
    operation m_operation;

ಸಾರ್ವಜನಿಕ:
    AstAssignCompound(AstAssignCompound::operation operation, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp,
                      Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_ಗಂಟು_Assign* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಧ್ರುವ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಪ್ರತ್ಯಾಗಮ new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { ಪ್ರತ್ಯಾಗಮ m_operation; }
};
class AstAssignCont final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
ಸಾರ್ವಜನಿಕ:
    AstAssignCont(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
                  Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_ಗಂಟು_Assign* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಧ್ರುವ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಪ್ರತ್ಯಾಗಮ new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Assign {
ಸಾರ್ವಜನಿಕ:
    AstAssignDly(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
                 Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_ಗಂಟು_Assign* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಧ್ರುವ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಪ್ರತ್ಯಾಗಮ new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "<="; }
};
class AstAssignForce final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Assign {
    // Procedural 'force' statement
ಸಾರ್ವಜನಿಕ:
    AstAssignForce(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_ಗಂಟು_Assign* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        ಪ್ರತ್ಯಾಗಮ new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
ಸಾರ್ವಜನಿಕ:
    AstAssignW(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
               Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_ಗಂಟು_Assign* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಧ್ರುವ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಪ್ರತ್ಯಾಗಮ new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ timingControlp() || lhsp()->exists([](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* refp) {
            ಪ್ರತ್ಯಾಗಮ refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ಧ್ರುವ;
};

// === AstNodeBlock ===
class AstBegin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ಧ್ರುವ bool m_implied : 1;  // Not inserted by user
ಸಾರ್ವಜನಿಕ:
    AstBegin(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ setNeedProcess() { m_needProcess = true; }
    bool needProcess() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needProcess; }
    bool implied() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_implied; }
};
class AstFork final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Block {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    ಧ್ರುವ VJoinType m_joinType;  // Join keyword type
    bool m_immediateStart = false;  // Fork starts before its parent blocks or exits

ಸಾರ್ವಜನಿಕ:
    AstFork(File_ಸಾಲು* fl, VJoinType joinType, ಧ್ರುವ string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstFork* ಧ್ರುವ asamep = VN_DBG_AS(samep, Fork);
        ಪ್ರತ್ಯಾಗಮ joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ !joinType().joinNone(); }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    VJoinType joinType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_joinType; }
    bool immediateStart() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_immediateStart; }
    ಶೂನ್ಯ immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_ಸಾಲು* fl, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* passsp, Ast_ಗಂಟು* failsp, VAssertType type,
              VAssertDirectiveType directive, ಧ್ರುವ string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_ಸಾಲು* fl, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* passsp, Ast_ಗಂಟು* failsp,
                       ಧ್ರುವ string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "assert"; }
};
class AstCover final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_ಸಾಲು* fl, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* stmtsp, VAssertType type,
             ಧ್ರುವ string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "cover"; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isCoverSeq() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isCoverSeq; }
    ಶೂನ್ಯ isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isSeqEvent; }
    ಶೂನ್ಯ isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Or_Assert {
ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_ಸಾಲು* fl, Ast_ಗಂಟು* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
ಸಾರ್ವಜನಿಕ:
    AstConstraintForeach(File_ಸಾಲು* fl, AstForeachHeader* headerp, Ast_ಗಂಟು* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_soft; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class AstForeach final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Foreach {
ಸಾರ್ವಜನಿಕ:
    AstForeach(File_ಸಾಲು* fl, AstForeachHeader* headerp, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_If {
ಸಾರ್ವಜನಿಕ:
    AstConstraintIf(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* thensp, Ast_ಗಂಟು* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
ಸಾರ್ವಜನಿಕ:
    AstIf(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* thensp = nullptr, Ast_ಗಂಟು* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_uniquePragma; }
    ಶೂನ್ಯ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unique0Pragma; }
    ಶೂನ್ಯ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_priorityPragma; }
    ಶೂನ್ಯ priorityPragma(bool flag) { m_priorityPragma = flag; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Read_Write_Mem {
ಸಾರ್ವಜನಿಕ:
    AstReadMem(File_ಸಾಲು* fl, bool hex, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* memp,
               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ (isHex() ? "$readmemh" : "$readmemb"); }
    ಧ್ರುವ ವರ್ಣ* cFuncPrefixp() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "VL_READMEM_"; }
};
class AstWriteMem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Read_Write_Mem {
ಸಾರ್ವಜನಿಕ:
    AstWriteMem(File_ಸಾಲು* fl, bool hex, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* memp,
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ (isHex() ? "$writememh" : "$writememb"); }
    ಧ್ರುವ ವರ್ಣ* cFuncPrefixp() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "VL_WRITEMEM_"; }
};

#endif  // Guard
