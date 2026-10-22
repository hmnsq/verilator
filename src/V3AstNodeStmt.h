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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ASTNODESTMT_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ASTNODESTMT_H_

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3AST_H_
#ದೋಷ "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ವ್ಯಾಖ್ಯಾನಿಸು VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ಆದರೆ_ಅಂತ್ಯ

// === Abstract base node types (AstNode*) =====================================

class Ast_ಗಂಟು_Stmt VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Procedural statement
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Stmt(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Stmt;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
};
class Ast_ಗಂಟು_ನಿಯೋಜಿಸು VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
                  Ast_ಗಂಟು* timingControlp = nullptr)
        : Ast_ಗಂಟು_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_ನಿಯೋಜಿಸು;
    // Clone single node, just get same type back.
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) = 0;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ವಾಸ್ತವಿಕ bool cleanRhs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "="; }
    bool isTimingControl() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ timingControlp(); }
};
class Ast_ಗಂಟು_ಖಂಡ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_ಖಂಡ(VNType t, File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : Ast_ಗಂಟು_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_ಖಂಡ;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Block name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    bool unnamed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unnamed; }
};
class Ast_ಗಂಟು_Cover_Or_Assert VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    ಸ್ಥಿರಾಂಕ VAssertType m_userType;  // Assertion/cover type for user enable/disable
    ಸ್ಥಿರಾಂಕ VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_ತಕ್ಷಣ = false;  // Immediate assert (may differ from userType being immediate)

ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Cover_Or_Assert(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ಸ್ಥಿರಾಂಕ string& name = "")
        : Ast_ಗಂಟು_Stmt{t, fl}
        , m_name{name}
        , m_userType{userType}
        , m_directive{directive} {
        this->propp(propp);
        ಕೂಡಿಸು_Passsp(passsp);
        m_ತಕ್ಷಣ = m_userType.ಒಳಗೊಂಡಿದೆ_Any(VAssertType::SIMPLE_ತಕ್ಷಣ
                                             | VAssertType::OBSERVED_DEFERRED_ತಕ್ಷಣ
                                             | VAssertType::FINAL_DEFERRED_ತಕ್ಷಣ)
                      || m_userType == VAssertType::ಆಂತರಿಕ;
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Cover_Or_Assert;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ samep->name() == name(); }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string type() { ಹಿಂತಿರುಗಿ ""; }
    VAssertType userType() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_userType; }
    VAssertDirectiveType directive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_directive; }
    bool ತಕ್ಷಣ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಕ್ಷಣ; }
    ಶೂನ್ಯ ತಕ್ಷಣ(bool flag) { m_ತಕ್ಷಣ = flag; }
    bool senFromAlways() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_senFromAlways; }
    ಶೂನ್ಯ senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_ಗಂಟು_Foreach VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Foreach(VNType t, File_ಸಾಲು* fl, Ast_Foreach_ಶೀರ್ಷಿಕೆ* headerp, Ast_ಗಂಟು* bodyp)
        : Ast_ಗಂಟು_Stmt(t, fl) {
        this->headerp(headerp);
        ಕೂಡಿಸು_Bodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Foreach;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಗಂಟು_If VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_If(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* thensp, Ast_ಗಂಟು* elsesp)
        : Ast_ಗಂಟು_Stmt{t, fl} {
        this->condp(condp);
        ಕೂಡಿಸು_Thensp(thensp);
        ಕೂಡಿಸು_Elsesp(elsesp);
        isBoundsCheck(false);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_If;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isGateDedupable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_branchPred; }
    ಶೂನ್ಯ isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isBoundsCheck; }
};
class Ast_ಗಂಟು_Read_Write_Mem VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ಸ್ಥಿರಾಂಕ bool m_isHex;  // readmemh, not readmemb
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
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ isHex() == VN_DBG_AS(samep, ಗಂಟು_Read_Write_Mem)->isHex();
    }
    bool isHex() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isHex; }
    ವಾಸ್ತವಿಕ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cFuncPrefixp() ಸ್ಥಿರಾಂಕ = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Case_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    Ast_Case_ಅಂಶ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condsp, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_Case_ಅಂಶ(fl) {
        ಕೂಡಿಸು_Condsp(condsp);
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Case_ಅಂಶ;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
    bool isDefault() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ condsp() == nullptr; }
};

class Ast_Foreach_ಶೀರ್ಷಿಕೆ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
ಸಾರ್ವಜನಿಕ:
    Ast_Foreach_ಶೀರ್ಷಿಕೆ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು* elementsp)
        : ASTGEN_SUPER_Foreach_ಶೀರ್ಷಿಕೆ(fl) {
        this->fromp(fromp);
        ಕೂಡಿಸು_Elementsp(elementsp);
    }
    ASTGEN_MEMBERS_Ast_Foreach_ಶೀರ್ಷಿಕೆ;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ false; }
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
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ctlType.ascii(); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    VAssertCtlType ctlType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ctlType; }
    ಶೂನ್ಯ ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_assertTypes; }
    ಶೂನ್ಯ assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_directiveTypes; }
    ಶೂನ್ಯ directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
};
class AstBreak final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
ಸಾರ್ವಜನಿಕ:
    explicit AstBreak(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "break"; }
    bool isBrancher() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }  // Node removed early
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
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isTimingControl() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_Sen_ವೃಕ್ಷ* sentreep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sentreep; }
    ಶೂನ್ಯ clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstCLocalScope(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        ಕೂಡಿಸು_Stmtsp(stmtsp);
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
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstCStmt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ಸ್ಥಿರಾಂಕ VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    ಸ್ಥಿರ AstCStmt* profExecSection(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ std::string& section, bool ತಳ್ಳು) {
        // Compute the label
        std::string label;
        ಆದರೆ (v3Global.opt.hier_ಮಗು()) {
            label += v3Global.opt.ತುದಿ_Module() + ":";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ತಳ್ಳು ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        ಆದರೆ (ತಳ್ಳು) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ತಳ್ಳು ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        ಹಿಂತಿರುಗಿ new AstCStmt{flp, ತಳ್ಳು ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

ಸಾರ್ವಜನಿಕ:
    explicit AstCStmt(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ = "",
                      ಸ್ಥಿರಾಂಕ VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        ಆದರೆ (!ಪಠ್ಯ.ಖಾಲಿಗಳು()) ಕೂಡಿಸು(ಪಠ್ಯ);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    VCStmtType stmtType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stmtType; }
    // Add some text, or a node to this statement
    ಶೂನ್ಯ ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) { ಕೂಡಿಸು_Nodesp(new Ast_ಪಠ್ಯ{fileline(), ಪಠ್ಯ}); }
    ಶೂನ್ಯ ಕೂಡಿಸು(Ast_ಗಂಟು* nodep) { ಕೂಡಿಸು_Nodesp(nodep); }
    // Static factory methods for specific cases
    ಸ್ಥಿರ AstCStmt* prof_Exec_Section_ತಳ್ಳು(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ std::string& section) {
        ಹಿಂತಿರುಗಿ profExecSection(flp, section, true);
    }
    ಸ್ಥಿರ AstCStmt* prof_Exec_Section_ತೆಗೆ(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ std::string& section) {
        ಹಿಂತಿರುಗಿ profExecSection(flp, section, false);
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
    ಸ್ಥಿರಾಂಕ bool m_fromDollarC;  // Is from source '$c', emit decoration
ಸಾರ್ವಜನಿಕ:
    explicit AstCStmtUser(File_ಸಾಲು* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool fromDollarC() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fromDollarC; }
    // Add some text, or a node to this statement
    ಶೂನ್ಯ ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) { ಕೂಡಿಸು_Nodesp(new Ast_ಪಠ್ಯ{fileline(), ಪಠ್ಯ}); }
    ಶೂನ್ಯ ಕೂಡಿಸು(Ast_ಗಂಟು* nodep) { ಕೂಡಿಸು_Nodesp(nodep); }
};
class AstCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Case statement
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstCaseItem]
    // @astgen op3 := notParallelp : List[AstNode] // assertion code for non-full case's
    VCaseType m_caseType;  // 0=case, 1=casex, 2=casez
    bool m_ತುಂಬಿದ_Pragma = false;  // Synthesis full_case
    bool m_parallelPragma = false;  // Synthesis parallel_case
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ಆದ್ಯತೆ_Pragma = false;  // priority case
ಸಾರ್ವಜನಿಕ:
    AstCase(File_ಸಾಲು* fl, VCaseType caseType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Case_ಅಂಶ* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_caseType == VCaseType::CT_CASEX; }
    bool casez() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_caseType == VCaseType::CT_CASE; }
    ಶೂನ್ಯ case_Inside_ಗಣ() { m_caseType = VCaseType::CT_CASEINSIDE; }
    ಶೂನ್ಯ case_Matches_ಗಣ() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool ತುಂಬಿದ_Pragma() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತುಂಬಿದ_Pragma; }
    ಶೂನ್ಯ ತುಂಬಿದ_Pragma(bool flag) { m_ತುಂಬಿದ_Pragma = flag; }
    bool parallelPragma() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_parallelPragma; }
    ಶೂನ್ಯ parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_uniquePragma; }
    ಶೂನ್ಯ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unique0Pragma; }
    ಶೂನ್ಯ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ಆದ್ಯತೆ_Pragma() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಆದ್ಯತೆ_Pragma; }
    ಶೂನ್ಯ ಆದ್ಯತೆ_Pragma(bool flag) { m_ಆದ್ಯತೆ_Pragma = flag; }
    string pragmaString() ಸ್ಥಿರಾಂಕ;
};
class AstComment final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Some comment to put into the output stream
    ಸ್ಥಿರಾಂಕ string m_name;  // Text of comment
    ಸ್ಥಿರಾಂಕ bool m_showAt;  // Show "at <fileline>"
ಸಾರ್ವಜನಿಕ:
    AstComment(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Text
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }  // Ignore name in comments
    ವಾಸ್ತವಿಕ bool showAt() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_showAt; }
};
class AstConstraintBefore final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstConstraintBefore(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhssp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        ಕೂಡಿಸು_Lhssp(lhssp);
        ಕೂಡಿಸು_Rhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isDisableSoft() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isDisableSoft; }
    ಶೂನ್ಯ isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isSoft; }
    ಶೂನ್ಯ isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstConstraintUnique(File_ಸಾಲು* fl, Ast_ಗಂಟು* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        ಕೂಡಿಸು_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstContinue final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
ಸಾರ್ವಜನಿಕ:
    explicit AstContinue(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "continue"; }
    bool isBrancher() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }  // Node removed early
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
    AstCoverInc(File_ಸಾಲು* fl, Ast_ಗಂಟು_Cover_Decl* declp, ಸ್ಥಿರಾಂಕ bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1 + 2 * INSTR_ಎಣಿಕೆ_LD; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstCoverInc* ಸ್ಥಿರಾಂಕ otherp = VN_DBG_AS(samep, CoverInc);
        ಹಿಂತಿರುಗಿ declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    Ast_ಗಂಟು_Cover_Decl* declp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_declp; }  // Where defined
    ಶೂನ್ಯ declp(Ast_ಗಂಟು_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isDuplicate; }
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
    ಸ್ಥಿರಾಂಕ bool m_is_ಚಕ್ರ_Delay;  // True if it is a cycle delay
ಸಾರ್ವಜನಿಕ:
    AstDelay(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, bool is_ಚಕ್ರ_Delay)
        : ASTGEN_SUPER_Delay(fl)
        , m_is_ಚಕ್ರ_Delay{is_ಚಕ್ರ_Delay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isTimingControl() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    bool is_ಚಕ್ರ_Delay() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_ಚಕ್ರ_Delay; }
    bool isRangeDelay() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_ಚಕ್ರ_Delay && rhsp(); }
    bool isUnbounded() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    ಶೂನ್ಯ fallDelay(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fallDelayp) { rhsp(fallDelayp); }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fallDelay() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_ಚಕ್ರ_Delay ? nullptr : rhsp(); }
};
class AstDisable final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
ಸಾರ್ವಜನಿಕ:
    AstDisable(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಗುರಿ_Refp)
        : ASTGEN_SUPER_Disable(fl) {
        this->ಗುರಿ_Refp(ಗುರಿ_Refp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ targetp(Ast_ಗಂಟು* nodep) { m_targetp = nodep; }
    Ast_ಗಂಟು* targetp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_targetp; }
    bool isBrancher() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // A "disable fork" statement
ಸಾರ್ವಜನಿಕ:
    explicit AstDisableFork(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "disable fork"; }
};
class AstDisplay final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

ಸಾರ್ವಜನಿಕ:
    AstDisplay(File_ಸಾಲು* fl, VDisplayType dispType, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep,
               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp, ಅಕ್ಷರ missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, ಪಠ್ಯ, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_ಸಾಲು* fl, VDisplayType dispType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp,
               ಅಕ್ಷರ missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ಸ್ಥಿರಾಂಕ newp
            = new AstSFormatF{fl, AstSFormatF::ಅಭಿವ್ಯಕ್ತಿ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!fmtp());
        ಹಿಂತಿರುಗಿ nullptr;
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    VDisplayType displayType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_displayType; }
    ಶೂನ್ಯ displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool ಕೂಡಿಸು_Newline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ displayType().ಕೂಡಿಸು_Newline(); }
};
class AstDumpCtl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ಸ್ಥಿರಾಂಕ VDumpCtlType m_ctlType;  // Type of operation
ಸಾರ್ವಜನಿಕ:
    AstDumpCtl(File_ಸಾಲು* fl, VDumpCtlType ctlType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ctlType().ascii(); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    ವಾಸ್ತವಿಕ bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    VDumpCtlType ctlType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ctlType; }
};
class AstEventControl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstEventControl(File_ಸಾಲು* fl, Ast_Sen_ವೃಕ್ಷ* sentreep, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "@(%l) %r"; }
    bool isTimingControl() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
};
class Ast_Exec_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
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
    V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_depGraphp;  // contains ExecMTask vertices
    ಸ್ಥಿರಾಂಕ string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

ಸಾರ್ವಜನಿಕ:
    Ast_Exec_ನಕ್ಷೆ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~Ast_Exec_ನಕ್ಷೆ() override;
    ASTGEN_MEMBERS_Ast_Exec_ನಕ್ಷೆ;
    ಶೂನ್ಯ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    V3_ನಕ್ಷೆ* depGraphp() { ಹಿಂತಿರುಗಿ m_depGraphp; }
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ* depGraphp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_depGraphp; }
    AstCFunc* runfuncp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_runfuncp; }
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
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$fclose"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$fflush"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಮುಕ್ತಾಯ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಮುಕ್ತಾಯ(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ಮುಕ್ತಾಯ(fl) {}
    ASTGEN_MEMBERS_Ast_ಮುಕ್ತಾಯ;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }  // Rarely executes
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ fileline() == samep->fileline(); }
};
class Ast_ಮುಕ್ತಾಯ_Fork final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // $finish in fork
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಮುಕ್ತಾಯ_Fork(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ಮುಕ್ತಾಯ_Fork(fl) {}
    ASTGEN_MEMBERS_Ast_ಮುಕ್ತಾಯ_Fork;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }  // Rarely executes
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ fileline() == samep->fileline(); }
};
class AstFireEvent final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ಸ್ಥಿರಾಂಕ bool m_delayed;  // Delayed (->>) vs non-delayed (->)
ಸಾರ್ವಜನಿಕ:
    AstFireEvent(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isDelayed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_delayed; }
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
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ true; }
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
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Jump_ಖಂಡ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
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
    Ast_Jump_ಖಂಡ(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_Jump_ಖಂಡ(fl) {
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Jump_ಖಂಡ;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override;

ಖಾಸಗಿ:
    bool getPurityRecurse() ಸ್ಥಿರಾಂಕ;
};
class AstJumpGo final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
ಸಾರ್ವಜನಿಕ:
    AstJumpGo(File_ಸಾಲು* fl, Ast_Jump_ಖಂಡ* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isBrancher() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ true;  // SPECIAL: We don't process code after breaks
    }
    Ast_Jump_ಖಂಡ* blockp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_blockp; }
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
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* thatp) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    // ACCESSORS
    VOptionBool unroll() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unroll; }
    ಶೂನ್ಯ unroll(ಸ್ಥಿರಾಂಕ VOptionBool flag) { m_unroll = flag; }
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
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isBrancher() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    // ACCESSORS
    AstLoop* loopp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_loopp; }
};
class AstMonitorOff final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    ಸ್ಥಿರಾಂಕ bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

ಸಾರ್ವಜನಿಕ:
    AstMonitorOff(File_ಸಾಲು* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Though deleted before opt
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Though deleted before opt
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // Though deleted before opt
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // Though deleted before opt
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_off; }
};
class AstPExprClause final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    ಸ್ಥಿರಾಂಕ bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ಸ್ಥಿರಾಂಕ bool m_vacuous;  // Pass is vacuous

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_ಸಾಲು* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool pass() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pass; }
    bool vacuous() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vacuous; }
};
class Ast_Print_ಸಮಯ_Scale final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    explicit Ast_Print_ಸಮಯ_Scale(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Print_ಸಮಯ_Scale(fl) {}
    ASTGEN_MEMBERS_Ast_Print_ಸಮಯ_Scale;
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$printtimescale"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
};
class AstRSBreak final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randsequence break
ಸಾರ್ವಜನಿಕ:
    explicit AstRSBreak(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "break"; }
    bool isBrancher() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
ಸಾರ್ವಜನಿಕ:
    AstRSProd(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        ಕೂಡಿಸು_Portsp(portsp);
        ಕೂಡಿಸು_Rulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
};
class Ast_RSProd_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
ಸಾರ್ವಜನಿಕ:
    Ast_RSProd_ಅಂಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProd_ಅಂಶ(fl)
        , m_name{name} {
        ಕೂಡಿಸು_Argsp(argsp);
    }
    ASTGEN_MEMBERS_Ast_RSProd_ಅಂಶ;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    AstRSProd* prodp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_prodp; }
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
        ಕೂಡಿಸು_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool randJoin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_randJoin; }
    ಶೂನ್ಯ randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randsequence return
ಸಾರ್ವಜನಿಕ:
    explicit AstRSReturn(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "return"; }
    bool isBrancher() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstRSRule(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp, AstRSProdList* prodlistsp, Ast_ಗಂಟು* ತೂಕ_Stmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        ಕೂಡಿಸು_Prodlistsp(prodlistsp);
        ಕೂಡಿಸು_ತೂಕ_Stmtsp(ತೂಕ_Stmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstRandCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
ಸಾರ್ವಜನಿಕ:
    AstRandCase(File_ಸಾಲು* fl, Ast_Case_ಅಂಶ* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        ಕೂಡಿಸು_Itemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstRandSequence final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_ಆರಂಭ;  // Name of start production, or "" to use first production
ಸಾರ್ವಜನಿಕ:
    AstRandSequence(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಆರಂಭ, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_ಆರಂಭ{ಆರಂಭ} {
        ಕೂಡಿಸು_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Block name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string ಆರಂಭ() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_ಆರಂಭ; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    AstRSProd* prodp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_prodp; }
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
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Not relevant - converted to FOR
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstReturn final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    explicit AstReturn(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "return"; }
    bool isBrancher() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstSFormat(File_ಸಾಲು* fl, bool optionalFormat, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp,
               ಅಕ್ಷರ missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ಸ್ಥಿರಾಂಕ newp
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
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!fmtp());
        ಹಿಂತಿರುಗಿ nullptr;
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$sformat"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ true; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ false; }
    ವಾಸ್ತವಿಕ bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ರಾಶಿ_Trace_T final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // $stacktrace used as task
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ರಾಶಿ_Trace_T(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ರಾಶಿ_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_ರಾಶಿ_Trace_T;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$stacktrace"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ exprp()->is_ಶುದ್ಧ(); }
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
    ಸ್ಥಿರಾಂಕ bool m_isFatal;  // $fatal not $stop
ಸಾರ್ವಜನಿಕ:
    AstStop(File_ಸಾಲು* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }  // Rarely executes
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ fileline() == samep->fileline(); }
    string emitVerilog() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isFatal; }
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
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$system"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಸಮಯ_Format final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    Ast_ಸಮಯ_Format(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* unitsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* precisionp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* suffixp,
                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* widthp)
        : ASTGEN_SUPER_ಸಮಯ_Format(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_Ast_ಸಮಯ_Format;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$timeformat"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
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
    uint32_t m_ಸಂಕೇತ{std::numeric_limits<uint32_t>::max()};  // Trace identifier code
    uint32_t m_fidx{0};  // Trace function index
    ಸ್ಥಿರಾಂಕ string m_showname;  // Name of variable
    ಸ್ಥಿರಾಂಕ VNumRange m_ಬಿಟ್_Range;  // Property of var the trace details
    ಸ್ಥಿರಾಂಕ VNumRange m_arrayRange;  // Property of var the trace details
    ಸ್ಥಿರಾಂಕ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ಸ್ಥಿರಾಂಕ VDirection m_declDirection;  // Declared direction input/output etc
    ಸ್ಥಿರಾಂಕ bool m_ಒಳ_Dtype_Func;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    ಪೂರ್ಣಾಂಕ m_ಸಂಕೇತ_Inc{0};  // Code increment for type
ಸಾರ್ವಜನಿಕ:
    AstTraceDecl(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, ಸ್ಥಿರಾಂಕ VNumRange& ಬಿಟ್_Range, ಸ್ಥಿರಾಂಕ VNumRange& arrayRange,
                 AstCCall* ಸ್ಥಿರಾಂಕ dtypeCallp, ಸ್ಥಿರಾಂಕ bool ಒಳ_Dtype_Func)
        : ASTGEN_SUPER_TraceDecl(fl)
        , m_showname{showname}
        , m_ಬಿಟ್_Range{ಬಿಟ್_Range}
        , m_arrayRange{arrayRange}
        , m_varType{varp->varType()}
        , m_declDirection{varp->declDirection()}
        , m_ಒಳ_Dtype_Func{ಒಳ_Dtype_Func} {
        dtypeFrom(valuep);
        this->valuep(valuep);
        this->dtypeCallp(dtypeCallp);
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_showname; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    string showname() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t ಸಂಕೇತ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಕೇತ; }
    ಶೂನ್ಯ ಸಂಕೇತ_2(uint32_t ಸಂಕೇತ_2) { m_ಸಂಕೇತ = ಸಂಕೇತ_2; }
    bool ಸಂಕೇತ_Assigned() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಕೇತ != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fidx; }
    ಶೂನ್ಯ fidx(uint32_t fidx) { m_fidx = fidx; }
    ಶೂನ್ಯ ಸಂಕೇತ_Inc(uint32_t ಸಂಕೇತ_Inc) { m_ಸಂಕೇತ_Inc = ಸಂಕೇತ_Inc; }
    uint32_t ಸಂಕೇತ_Inc() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_ಸಂಕೇತ_Inc) { ಹಿಂತಿರುಗಿ m_ಸಂಕೇತ_Inc; }
        ಹಿಂತಿರುಗಿ (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->ಅಗಲ_ಪದಗಳು()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ಸ್ಥಿರಾಂಕ VNumRange& ಬಿಟ್_Range() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಿಟ್_Range; }
    ಸ್ಥಿರಾಂಕ VNumRange& arrayRange() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_arrayRange; }
    VVarType varType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_varType; }
    VDirection declDirection() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_declDirection; }
    AstCCall* dtypeCallp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dtypeCallp; }
    ಶೂನ್ಯ dtypeCallp(AstCCall* ಸ್ಥಿರಾಂಕ callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dtypeDeclp; }
    ಶೂನ್ಯ dtypeDeclp(AstTraceDecl* ಸ್ಥಿರಾಂಕ declp) { m_dtypeDeclp = declp; }
    bool ಒಳ_Dtype_Func() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಳ_Dtype_Func; }
};
class AstTraceInc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ಸ್ಥಿರಾಂಕ uint32_t m_ತಳ_ಸಂಕೇತ;  // Trace code base value in function containing this AstTraceInc
    ಸ್ಥಿರಾಂಕ VTraceType m_traceType;  // Is this a const/full/incremental dump

ಸಾರ್ವಜನಿಕ:
    AstTraceInc(File_ಸಾಲು* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t ತಳ_ಸಂಕೇತ = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_ತಳ_ಸಂಕೇತ{ತಳ_ಸಂಕೇತ}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_ವೃಕ್ಷ(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 10 + 2 * INSTR_ಎಣಿಕೆ_LD; }
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    AstTraceDecl* declp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_declp; }
    VTraceType traceType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceType; }
    uint32_t ತಳ_ಸಂಕೇತ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಳ_ಸಂಕೇತ; }
};
class Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
ಸಾರ್ವಜನಿಕ:
    explicit Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    ಸ್ಥಿರಾಂಕ string m_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Prefix to add to signal names
    ಸ್ಥಿರಾಂಕ VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type m_ಪೂರ್ವಪ್ರತ್ಯಯ_Type;  // Type of prefix being pushed
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಎಡ;  // Array left index, or struct/union member count
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಬಲ;  // Array right index
    ಸ್ಥಿರಾಂಕ bool m_quoted_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Quote prefix name
ಸಾರ್ವಜನಿಕ:
    Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ, VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type ಪೂರ್ವಪ್ರತ್ಯಯ_Type,
                       ಪೂರ್ಣಾಂಕ ಎಡ_2 = 0, ಪೂರ್ಣಾಂಕ ಬಲ_2 = 0, bool quoted_ಪೂರ್ವಪ್ರತ್ಯಯ = true)
        : ASTGEN_SUPER_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ(fl)
        , m_ಪೂರ್ವಪ್ರತ್ಯಯ{ಪೂರ್ವಪ್ರತ್ಯಯ}
        , m_ಪೂರ್ವಪ್ರತ್ಯಯ_Type{ಪೂರ್ವಪ್ರತ್ಯಯ_Type}
        , m_ಎಡ{ಎಡ_2}
        , m_ಬಲ{ಬಲ_2}
        , m_quoted_ಪೂರ್ವಪ್ರತ್ಯಯ{quoted_ಪೂರ್ವಪ್ರತ್ಯಯ} {}
    ASTGEN_MEMBERS_Ast_Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪೂರ್ವಪ್ರತ್ಯಯ; }
    VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type ಪೂರ್ವಪ್ರತ್ಯಯ_Type() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪೂರ್ವಪ್ರತ್ಯಯ_Type; }
    ಪೂರ್ಣಾಂಕ ಎಡ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಡ; }
    ಪೂರ್ಣಾಂಕ ಬಲ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಲ; }
    bool quoted_ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_quoted_ಪೂರ್ವಪ್ರತ್ಯಯ; }
};
class AstWait final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstWait(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstWaitFork final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stmt {
    // A "wait fork" statement
ಸಾರ್ವಜನಿಕ:
    explicit AstWaitFork(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "wait fork"; }
};

// === AstNodeAssign ===
class Ast_ನಿಯೋಜಿಸು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು {
ಸಾರ್ವಜನಿಕ:
    Ast_ನಿಯೋಜಿಸು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
              Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_ನಿಯೋಜಿಸು(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_ನಿಯೋಜಿಸು;
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಹಿಂತಿರುಗಿ new Ast_ನಿಯೋಜಿಸು{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_ನಿಯೋಜಿಸು_Compound final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು {
    // Compound assignments (+=, -=, *=, ...)
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ operation : uint8_t {
        ಕೂಡಿಸು_2,
        And,
        Div,
        ModDiv,
        Mul,
        Or,
        ಸರಿಸು_L,
        ಸರಿಸು_R,
        ಸರಿಸು_RS,
        Sub,
        Xor,
    };

ಖಾಸಗಿ:
    operation m_operation;

ಸಾರ್ವಜನಿಕ:
    Ast_ನಿಯೋಜಿಸು_Compound(Ast_ನಿಯೋಜಿಸು_Compound::operation operation, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp,
                      Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_ನಿಯೋಜಿಸು_Compound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_ನಿಯೋಜಿಸು_Compound;
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಹಿಂತಿರುಗಿ new Ast_ನಿಯೋಜಿಸು_Compound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { ಹಿಂತಿರುಗಿ m_operation; }
};
class Ast_ನಿಯೋಜಿಸು_Cont final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
ಸಾರ್ವಜನಿಕ:
    Ast_ನಿಯೋಜಿಸು_Cont(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
                  Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_ನಿಯೋಜಿಸು_Cont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_ನಿಯೋಜಿಸು_Cont;
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಹಿಂತಿರುಗಿ new Ast_ನಿಯೋಜಿಸು_Cont{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_ನಿಯೋಜಿಸು_Dly final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು {
ಸಾರ್ವಜನಿಕ:
    Ast_ನಿಯೋಜಿಸು_Dly(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
                 Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_ನಿಯೋಜಿಸು_Dly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_ನಿಯೋಜಿಸು_Dly;
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಹಿಂತಿರುಗಿ new Ast_ನಿಯೋಜಿಸು_Dly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "<="; }
};
class Ast_ನಿಯೋಜಿಸು_Force final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು {
    // Procedural 'force' statement
ಸಾರ್ವಜನಿಕ:
    Ast_ನಿಯೋಜಿಸು_Force(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ನಿಯೋಜಿಸು_Force(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_ನಿಯೋಜಿಸು_Force;
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        ಹಿಂತಿರುಗಿ new Ast_ನಿಯೋಜಿಸು_Force{fileline(), lhsp, rhsp};
    }
};
class Ast_ನಿಯೋಜಿಸು_W final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
ಸಾರ್ವಜನಿಕ:
    Ast_ನಿಯೋಜಿಸು_W(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
               Ast_ಗಂಟು* timingControlp = nullptr)
        : ASTGEN_SUPER_ನಿಯೋಜಿಸು_W(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_ನಿಯೋಜಿಸು_W;
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* cloneType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) override {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = timingControlp() ? timingControlp()->clone_ವೃಕ್ಷ(false) : nullptr;
        ಹಿಂತಿರುಗಿ new Ast_ನಿಯೋಜಿಸು_W{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ timingControlp() || lhsp()->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* refp) {
            ಹಿಂತಿರುಗಿ refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ಸ್ಥಿರಾಂಕ;
};

// === AstNodeBlock ===
class AstBegin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಖಂಡ {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ಸ್ಥಿರಾಂಕ bool m_implied : 1;  // Not inserted by user
ಸಾರ್ವಜನಿಕ:
    AstBegin(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ ಗಣ_Need_Process() { m_needProcess = true; }
    bool needProcess() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needProcess; }
    bool implied() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_implied; }
};
class AstFork final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಖಂಡ {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    ಸ್ಥಿರಾಂಕ VJoinType m_joinType;  // Join keyword type
    bool m_ತಕ್ಷಣ_ಆರಂಭ = false;  // Fork starts before its parent blocks or exits

ಸಾರ್ವಜನಿಕ:
    AstFork(File_ಸಾಲು* fl, VJoinType joinType, ಸ್ಥಿರಾಂಕ string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstFork* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, Fork);
        ಹಿಂತಿರುಗಿ joinType() == asamep->joinType() && ತಕ್ಷಣ_ಆರಂಭ() == asamep->ತಕ್ಷಣ_ಆರಂಭ();
    }
    bool isTimingControl() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ !joinType().joinNone(); }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    VJoinType joinType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_joinType; }
    bool ತಕ್ಷಣ_ಆರಂಭ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಕ್ಷಣ_ಆರಂಭ; }
    ಶೂನ್ಯ ತಕ್ಷಣ_ಆರಂಭ(bool flag) { m_ತಕ್ಷಣ_ಆರಂಭ = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_ಸಾಲು* fl, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* passsp, Ast_ಗಂಟು* failsp, VAssertType type,
              VAssertDirectiveType directive, ಸ್ಥಿರಾಂಕ string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        ಕೂಡಿಸು_Failsp(failsp);
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_ಸಾಲು* fl, Ast_ಗಂಟು* propp, Ast_ಗಂಟು* passsp, Ast_ಗಂಟು* failsp,
                       ಸ್ಥಿರಾಂಕ string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::ಆಂತರಿಕ,
                                       VAssertDirectiveType::INTRINSIC, name) {
        ಕೂಡಿಸು_Failsp(failsp);
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "assert"; }
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
             ಸ್ಥಿರಾಂಕ string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "cover"; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isCoverSeq() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isCoverSeq; }
    ಶೂನ್ಯ isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isSeqEvent; }
    ಶೂನ್ಯ isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Or_Assert {
ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_ಸಾಲು* fl, Ast_ಗಂಟು* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::ಆಂತರಿಕ,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
ಸಾರ್ವಜನಿಕ:
    AstConstraintForeach(File_ಸಾಲು* fl, Ast_Foreach_ಶೀರ್ಷಿಕೆ* headerp, Ast_ಗಂಟು* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_soft; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class AstForeach final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Foreach {
ಸಾರ್ವಜನಿಕ:
    AstForeach(File_ಸಾಲು* fl, Ast_Foreach_ಶೀರ್ಷಿಕೆ* headerp, Ast_ಗಂಟು* stmtsp)
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
    bool m_ಆದ್ಯತೆ_Pragma = false;  // priority case
ಸಾರ್ವಜನಿಕ:
    AstIf(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* thensp = nullptr, Ast_ಗಂಟು* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_uniquePragma; }
    ಶೂನ್ಯ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unique0Pragma; }
    ಶೂನ್ಯ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ಆದ್ಯತೆ_Pragma() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಆದ್ಯತೆ_Pragma; }
    ಶೂನ್ಯ ಆದ್ಯತೆ_Pragma(bool flag) { m_ಆದ್ಯತೆ_Pragma = flag; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Read_Write_Mem {
ಸಾರ್ವಜನಿಕ:
    AstReadMem(File_ಸಾಲು* fl, bool hex, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* memp,
               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ (isHex() ? "$readmemh" : "$readmemb"); }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cFuncPrefixp() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "VL_READMEM_"; }
};
class AstWriteMem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Read_Write_Mem {
ಸಾರ್ವಜನಿಕ:
    AstWriteMem(File_ಸಾಲು* fl, bool hex, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* memp,
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ (isHex() ? "$writememh" : "$writememb"); }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cFuncPrefixp() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "VL_WRITEMEM_"; }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
