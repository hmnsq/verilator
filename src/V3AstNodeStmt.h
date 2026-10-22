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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ASTNODESTMT_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ASTNODESTMT_H_

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3AST_H_
#ਗਲਤੀ "Use V3Ast.h as the include"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ਜੇ_ਅੰਤ

// === Abstract base node types (AstNode*) =====================================

class Ast_ਗੰਢ_Stmt VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // Procedural statement
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Stmt(VNType t, File_ਸਤਰ* fl)
        : Ast_ਗੰਢ{t, fl} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Stmt;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
};
class Ast_ਗੰਢ_ਸੌਂਪੋ VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_ਸੌਂਪੋ(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp,
                  Ast_ਗੰਢ* timingControlp = nullptr)
        : Ast_ਗੰਢ_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_ਸੌਂਪੋ;
    // Clone single node, just get same type back.
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਆਭਾਸੀ Ast_ਗੰਢ_ਸੌਂਪੋ* cloneType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) = 0;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਆਭਾਸੀ bool cleanRhs() ਅਚਲ { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "="; }
    bool isTimingControl() ਅਚਲ override { ਵਾਪਸ timingControlp(); }
};
class Ast_ਗੰਢ_ਖੰਡ VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_ਖੰਡ(VNType t, File_ਸਤਰ* fl, ਅਚਲ string& name)
        : Ast_ਗੰਢ_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_ਖੰਡ;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Block name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    bool unnamed() ਅਚਲ { ਵਾਪਸ m_unnamed; }
};
class Ast_ਗੰਢ_Cover_Or_Assert VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    ਅਚਲ VAssertType m_userType;  // Assertion/cover type for user enable/disable
    ਅਚਲ VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_ਤੁਰੰਤ = false;  // Immediate assert (may differ from userType being immediate)

ਜਨਤਕ:
    Ast_ਗੰਢ_Cover_Or_Assert(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ* propp, Ast_ਗੰਢ* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ਅਚਲ string& name = "")
        : Ast_ਗੰਢ_Stmt{t, fl}
        , m_name{name}
        , m_userType{userType}
        , m_directive{directive} {
        this->propp(propp);
        ਜੋੜੋ_Passsp(passsp);
        m_ਤੁਰੰਤ = m_userType.ਸ਼ਾਮਲ_ਹੈ_Any(VAssertType::SIMPLE_ਤੁਰੰਤ
                                             | VAssertType::OBSERVED_DEFERRED_ਤੁਰੰਤ
                                             | VAssertType::FINAL_DEFERRED_ਤੁਰੰਤ)
                      || m_userType == VAssertType::ਅੰਦਰੂਨੀ;
    }
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Cover_Or_Assert;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ samep->name() == name(); }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string type() { ਵਾਪਸ ""; }
    VAssertType userType() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_userType; }
    VAssertDirectiveType directive() ਅਚਲ { ਵਾਪਸ m_directive; }
    bool ਤੁਰੰਤ() ਅਚਲ { ਵਾਪਸ m_ਤੁਰੰਤ; }
    ਖਾਲੀ ਤੁਰੰਤ(bool flag) { m_ਤੁਰੰਤ = flag; }
    bool senFromAlways() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_senFromAlways; }
    ਖਾਲੀ senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_ਗੰਢ_Foreach VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
ਜਨਤਕ:
    Ast_ਗੰਢ_Foreach(VNType t, File_ਸਤਰ* fl, Ast_Foreach_ਸਿਰਲੇਖ* headerp, Ast_ਗੰਢ* bodyp)
        : Ast_ਗੰਢ_Stmt(t, fl) {
        this->headerp(headerp);
        ਜੋੜੋ_Bodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Foreach;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਗੰਢ_If VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_If(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ* thensp, Ast_ਗੰਢ* elsesp)
        : Ast_ਗੰਢ_Stmt{t, fl} {
        this->condp(condp);
        ਜੋੜੋ_Thensp(thensp);
        ਜੋੜੋ_Elsesp(elsesp);
        isBoundsCheck(false);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_If;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isGateDedupable() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ਅਚਲ { ਵਾਪਸ m_branchPred; }
    ਖਾਲੀ isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ਅਚਲ { ਵਾਪਸ m_isBoundsCheck; }
};
class Ast_ਗੰਢ_Read_Write_Mem VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ਅਚਲ bool m_isHex;  // readmemh, not readmemb
ਜਨਤਕ:
    Ast_ਗੰਢ_Read_Write_Mem(VNType t, File_ਸਤਰ* fl, bool hex, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filenamep,
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* memp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* msbp)
        : Ast_ਗੰਢ_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Read_Write_Mem;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ isHex() == VN_DBG_AS(samep, ਗੰਢ_Read_Write_Mem)->isHex();
    }
    bool isHex() ਅਚਲ { ਵਾਪਸ m_isHex; }
    ਆਭਾਸੀ ਅਚਲ ਅੱਖਰ* cFuncPrefixp() ਅਚਲ = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Case_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
ਜਨਤਕ:
    Ast_Case_ਵਸਤੂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condsp, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_Case_ਵਸਤੂ(fl) {
        ਜੋੜੋ_Condsp(condsp);
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Case_ਵਸਤੂ;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() + INSTR_ਗਿਣਤੀ_BRANCH; }
    bool isDefault() ਅਚਲ { ਵਾਪਸ condsp() == nullptr; }
};

class Ast_Foreach_ਸਿਰਲੇਖ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
ਜਨਤਕ:
    Ast_Foreach_ਸਿਰਲੇਖ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ* elementsp)
        : ASTGEN_SUPER_Foreach_ਸਿਰਲੇਖ(fl) {
        this->fromp(fromp);
        ਜੋੜੋ_Elementsp(elementsp);
    }
    ASTGEN_MEMBERS_Ast_Foreach_ਸਿਰਲੇਖ;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

ਜਨਤਕ:
    AstAssertCtl(File_ਸਤਰ* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_ਗੰਢ_ਵਿਅੰਜਕ* levelp = nullptr, Ast_ਗੰਢ_ਵਿਅੰਜਕ* itemsp = nullptr);
    AstAssertCtl(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* controlTypep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* assertTypesp = nullptr,
                 Ast_ਗੰਢ_ਵਿਅੰਜਕ* directiveTypep = nullptr, Ast_ਗੰਢ_ਵਿਅੰਜਕ* levelp = nullptr,
                 Ast_ਗੰਢ_ਵਿਅੰਜਕ* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ m_ctlType.ascii(); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    VAssertCtlType ctlType() ਅਚਲ { ਵਾਪਸ m_ctlType; }
    ਖਾਲੀ ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ਅਚਲ { ਵਾਪਸ m_assertTypes; }
    ਖਾਲੀ assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ਅਚਲ { ਵਾਪਸ m_directiveTypes; }
    ਖਾਲੀ directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
};
class AstBreak final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
ਜਨਤਕ:
    explicit AstBreak(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "break"; }
    bool isBrancher() ਅਚਲ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
ਜਨਤਕ:
    AstCAwait(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Sen_ਰੁੱਖ* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isTimingControl() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_Sen_ਰੁੱਖ* sentreep() ਅਚਲ { ਵਾਪਸ m_sentreep; }
    ਖਾਲੀ clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
ਜਨਤਕ:
    AstCLocalScope(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
ਜਨਤਕ:
    AstCReturn(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstCStmt final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ਅਚਲ VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    ਸਥਿਰ AstCStmt* profExecSection(File_ਸਤਰ* flp, ਅਚਲ std::string& section, bool ਧੱਕੋ) {
        // Compute the label
        std::string label;
        ਜੇ (v3Global.opt.hier_ਬੱਚਾ()) {
            label += v3Global.opt.ਸਿਖਰ_Module() + ":";
        } ਨਹੀਂਤਾਂ ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ਧੱਕੋ ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        ਜੇ (ਧੱਕੋ) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ਧੱਕੋ ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        ਵਾਪਸ new AstCStmt{flp, ਧੱਕੋ ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

ਜਨਤਕ:
    explicit AstCStmt(File_ਸਤਰ* fl, ਅਚਲ std::string& ਪਾਠ = "",
                      ਅਚਲ VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        ਜੇ (!ਪਾਠ.ਖਾਲੀ_ਥਾਵਾਂ()) ਜੋੜੋ(ਪਾਠ);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    VCStmtType stmtType() ਅਚਲ { ਵਾਪਸ m_stmtType; }
    // Add some text, or a node to this statement
    ਖਾਲੀ ਜੋੜੋ(ਅਚਲ std::string& ਪਾਠ) { ਜੋੜੋ_Nodesp(new Ast_ਪਾਠ{fileline(), ਪਾਠ}); }
    ਖਾਲੀ ਜੋੜੋ(Ast_ਗੰਢ* nodep) { ਜੋੜੋ_Nodesp(nodep); }
    // Static factory methods for specific cases
    ਸਥਿਰ AstCStmt* prof_Exec_Section_ਧੱਕੋ(File_ਸਤਰ* flp, ਅਚਲ std::string& section) {
        ਵਾਪਸ profExecSection(flp, section, true);
    }
    ਸਥਿਰ AstCStmt* prof_Exec_Section_ਕੱਢੋ(File_ਸਤਰ* flp, ਅਚਲ std::string& section) {
        ਵਾਪਸ profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ਅਚਲ bool m_fromDollarC;  // Is from source '$c', emit decoration
ਜਨਤਕ:
    explicit AstCStmtUser(File_ਸਤਰ* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    bool fromDollarC() ਅਚਲ { ਵਾਪਸ m_fromDollarC; }
    // Add some text, or a node to this statement
    ਖਾਲੀ ਜੋੜੋ(ਅਚਲ std::string& ਪਾਠ) { ਜੋੜੋ_Nodesp(new Ast_ਪਾਠ{fileline(), ਪਾਠ}); }
    ਖਾਲੀ ਜੋੜੋ(Ast_ਗੰਢ* nodep) { ਜੋੜੋ_Nodesp(nodep); }
};
class AstCase final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Case statement
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstCaseItem]
    // @astgen op3 := notParallelp : List[AstNode] // assertion code for non-full case's
    VCaseType m_caseType;  // 0=case, 1=casex, 2=casez
    bool m_ਭਰਿਆ_Pragma = false;  // Synthesis full_case
    bool m_parallelPragma = false;  // Synthesis parallel_case
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ਤਰਜੀਹ_Pragma = false;  // priority case
ਜਨਤਕ:
    AstCase(File_ਸਤਰ* fl, VCaseType caseType, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Case_ਵਸਤੂ* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ਅਚਲ { ਵਾਪਸ m_caseType == VCaseType::CT_CASEX; }
    bool casez() ਅਚਲ { ਵਾਪਸ m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ਅਚਲ { ਵਾਪਸ m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ਅਚਲ { ਵਾਪਸ m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ਅਚਲ { ਵਾਪਸ m_caseType == VCaseType::CT_CASE; }
    ਖਾਲੀ case_Inside_ਸੈੱਟ() { m_caseType = VCaseType::CT_CASEINSIDE; }
    ਖਾਲੀ case_Matches_ਸੈੱਟ() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool ਭਰਿਆ_Pragma() ਅਚਲ { ਵਾਪਸ m_ਭਰਿਆ_Pragma; }
    ਖਾਲੀ ਭਰਿਆ_Pragma(bool flag) { m_ਭਰਿਆ_Pragma = flag; }
    bool parallelPragma() ਅਚਲ { ਵਾਪਸ m_parallelPragma; }
    ਖਾਲੀ parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ਅਚਲ { ਵਾਪਸ m_uniquePragma; }
    ਖਾਲੀ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ਅਚਲ { ਵਾਪਸ m_unique0Pragma; }
    ਖਾਲੀ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ਤਰਜੀਹ_Pragma() ਅਚਲ { ਵਾਪਸ m_ਤਰਜੀਹ_Pragma; }
    ਖਾਲੀ ਤਰਜੀਹ_Pragma(bool flag) { m_ਤਰਜੀਹ_Pragma = flag; }
    string pragmaString() ਅਚਲ;
};
class AstComment final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Some comment to put into the output stream
    ਅਚਲ string m_name;  // Text of comment
    ਅਚਲ bool m_showAt;  // Show "at <fileline>"
ਜਨਤਕ:
    AstComment(File_ਸਤਰ* fl, ਅਚਲ string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Text
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ true; }  // Ignore name in comments
    ਆਭਾਸੀ bool showAt() ਅਚਲ { ਵਾਪਸ m_showAt; }
};
class AstConstraintBefore final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
ਜਨਤਕ:
    AstConstraintBefore(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhssp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        ਜੋੜੋ_Lhssp(lhssp);
        ਜੋੜੋ_Rhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Constraint_ਵਿਅੰਜਕ final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
ਜਨਤਕ:
    Ast_Constraint_ਵਿਅੰਜਕ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_Constraint_ਵਿਅੰਜਕ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_ਵਿਅੰਜਕ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isDisableSoft() ਅਚਲ { ਵਾਪਸ m_isDisableSoft; }
    ਖਾਲੀ isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ਅਚਲ { ਵਾਪਸ m_isSoft; }
    ਖਾਲੀ isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
ਜਨਤਕ:
    AstConstraintUnique(File_ਸਤਰ* fl, Ast_ਗੰਢ* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        ਜੋੜੋ_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstContinue final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
ਜਨਤਕ:
    explicit AstContinue(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "continue"; }
    bool isBrancher() ਅਚਲ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

ਜਨਤਕ:
    AstCoverInc(File_ਸਤਰ* fl, Ast_ਗੰਢ_Cover_Decl* declp, ਅਚਲ bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 1 + 2 * INSTR_ਗਿਣਤੀ_LD; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstCoverInc* ਅਚਲ otherp = VN_DBG_AS(samep, CoverInc);
        ਵਾਪਸ declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    Ast_ਗੰਢ_Cover_Decl* declp() ਅਚਲ { ਵਾਪਸ m_declp; }  // Where defined
    ਖਾਲੀ declp(Ast_ਗੰਢ_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ਅਚਲ { ਵਾਪਸ m_isDuplicate; }
};
class AstDeassign final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
ਜਨਤਕ:
    AstDeassign(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ਅਚਲ bool m_is_ਚੱਕਰ_Delay;  // True if it is a cycle delay
ਜਨਤਕ:
    AstDelay(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, bool is_ਚੱਕਰ_Delay)
        : ASTGEN_SUPER_Delay(fl)
        , m_is_ਚੱਕਰ_Delay{is_ਚੱਕਰ_Delay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isTimingControl() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    bool is_ਚੱਕਰ_Delay() ਅਚਲ { ਵਾਪਸ m_is_ਚੱਕਰ_Delay; }
    bool isRangeDelay() ਅਚਲ { ਵਾਪਸ m_is_ਚੱਕਰ_Delay && rhsp(); }
    bool isUnbounded() ਅਚਲ { ਵਾਪਸ isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    ਖਾਲੀ fallDelay(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fallDelayp) { rhsp(fallDelayp); }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* fallDelay() ਅਚਲ { ਵਾਪਸ m_is_ਚੱਕਰ_Delay ? nullptr : rhsp(); }
};
class AstDisable final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
ਜਨਤਕ:
    AstDisable(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਨਿਸ਼ਾਨਾ_Refp)
        : ASTGEN_SUPER_Disable(fl) {
        this->ਨਿਸ਼ਾਨਾ_Refp(ਨਿਸ਼ਾਨਾ_Refp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ targetp(Ast_ਗੰਢ* nodep) { m_targetp = nodep; }
    Ast_ਗੰਢ* targetp() ਅਚਲ { ਵਾਪਸ m_targetp; }
    bool isBrancher() ਅਚਲ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // A "disable fork" statement
ਜਨਤਕ:
    explicit AstDisableFork(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "disable fork"; }
};
class AstDisplay final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

ਜਨਤਕ:
    AstDisplay(File_ਸਤਰ* fl, VDisplayType dispType, ਅਚਲ string& ਪਾਠ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep,
               Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp, ਅੱਖਰ missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, ਪਾਠ, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_ਸਤਰ* fl, VDisplayType dispType, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp,
               ਅੱਖਰ missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ਅਚਲ newp
            = new AstSFormatF{fl, AstSFormatF::ਵਿਅੰਜਕ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!fmtp());
        ਵਾਪਸ nullptr;
    }
    string verilogKwd() ਅਚਲ override {
        ਵਾਪਸ (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    VDisplayType displayType() ਅਚਲ { ਵਾਪਸ m_displayType; }
    ਖਾਲੀ displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool ਜੋੜੋ_Newline() ਅਚਲ { ਵਾਪਸ displayType().ਜੋੜੋ_Newline(); }
};
class AstDumpCtl final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ਅਚਲ VDumpCtlType m_ctlType;  // Type of operation
ਜਨਤਕ:
    AstDumpCtl(File_ਸਤਰ* fl, VDumpCtlType ctlType, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ ctlType().ascii(); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    ਆਭਾਸੀ bool clean_ਬਾਹਰਲਾ() ਅਚਲ { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    VDumpCtlType ctlType() ਅਚਲ { ਵਾਪਸ m_ctlType; }
};
class AstEventControl final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
ਜਨਤਕ:
    AstEventControl(File_ਸਤਰ* fl, Ast_Sen_ਰੁੱਖ* sentreep, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "@(%l) %r"; }
    bool isTimingControl() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
};
class Ast_Exec_ਗ੍ਰਾਫ final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
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
    V3_ਗ੍ਰਾਫ* ਅਚਲ m_depGraphp;  // contains ExecMTask vertices
    ਅਚਲ string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

ਜਨਤਕ:
    Ast_Exec_ਗ੍ਰਾਫ(File_ਸਤਰ* fl, ਅਚਲ string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~Ast_Exec_ਗ੍ਰਾਫ() override;
    ASTGEN_MEMBERS_Ast_Exec_ਗ੍ਰਾਫ;
    ਖਾਲੀ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    V3_ਗ੍ਰਾਫ* depGraphp() { ਵਾਪਸ m_depGraphp; }
    ਅਚਲ V3_ਗ੍ਰਾਫ* depGraphp() ਅਚਲ { ਵਾਪਸ m_depGraphp; }
    AstCFunc* runfuncp() ਅਚਲ { ਵਾਪਸ m_runfuncp; }
    ਖਾਲੀ runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
ਜਨਤਕ:
    AstFClose(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$fclose"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFFlush final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
ਜਨਤਕ:
    AstFFlush(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$fflush"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਅੰਤ final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
ਜਨਤਕ:
    explicit Ast_ਅੰਤ(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਅੰਤ(fl) {}
    ASTGEN_MEMBERS_Ast_ਅੰਤ;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }  // Rarely executes
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ fileline() == samep->fileline(); }
};
class Ast_ਅੰਤ_Fork final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // $finish in fork
ਜਨਤਕ:
    explicit Ast_ਅੰਤ_Fork(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਅੰਤ_Fork(fl) {}
    ASTGEN_MEMBERS_Ast_ਅੰਤ_Fork;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }  // Rarely executes
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ fileline() == samep->fileline(); }
};
class AstFireEvent final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ਅਚਲ bool m_delayed;  // Delayed (->>) vs non-delayed (->)
ਜਨਤਕ:
    AstFireEvent(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isDelayed() ਅਚਲ { ਵਾਪਸ m_delayed; }
};
class AstInitialAutomaticStmt final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
ਜਨਤਕ:
    AstInitialAutomaticStmt(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ true; }
};
class AstInitialStaticStmt final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
ਜਨਤਕ:
    AstInitialStaticStmt(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ true; }
};
class Ast_Jump_ਖੰਡ final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
ਜਨਤਕ:
    // After construction must call ->labelp to associate with appropriate label
    Ast_Jump_ਖੰਡ(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_Jump_ਖੰਡ(fl) {
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Jump_ਖੰਡ;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override;

ਨਿਜੀ:
    bool getPurityRecurse() ਅਚਲ;
};
class AstJumpGo final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
ਜਨਤਕ:
    AstJumpGo(File_ਸਤਰ* fl, Ast_Jump_ਖੰਡ* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isBrancher() ਅਚਲ override {
        ਵਾਪਸ true;  // SPECIAL: We don't process code after breaks
    }
    Ast_Jump_ਖੰਡ* blockp() ਅਚਲ { ਵਾਪਸ m_blockp; }
};
class AstLoop final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
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
ਜਨਤਕ:
    explicit AstLoop(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* thatp) ਅਚਲ override {
        ਵਾਪਸ m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    // ACCESSORS
    VOptionBool unroll() ਅਚਲ { ਵਾਪਸ m_unroll; }
    ਖਾਲੀ unroll(ਅਚਲ VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
ਜਨਤਕ:
    AstLoopTest(File_ਸਤਰ* fl, AstLoop* loopp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isBrancher() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    // ACCESSORS
    AstLoop* loopp() ਅਚਲ { ਵਾਪਸ m_loopp; }
};
class AstMonitorOff final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    ਅਚਲ bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

ਜਨਤਕ:
    AstMonitorOff(File_ਸਤਰ* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }  // Though deleted before opt
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }  // Though deleted before opt
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // Though deleted before opt
    bool isOutputter() override { ਵਾਪਸ true; }  // Though deleted before opt
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ਅਚਲ { ਵਾਪਸ m_off; }
};
class AstPExprClause final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    ਅਚਲ bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ਅਚਲ bool m_vacuous;  // Pass is vacuous

ਜਨਤਕ:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_ਸਤਰ* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool pass() ਅਚਲ { ਵਾਪਸ m_pass; }
    bool vacuous() ਅਚਲ { ਵਾਪਸ m_vacuous; }
};
class Ast_Print_ਸਮਾਂ_Scale final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
ਜਨਤਕ:
    explicit Ast_Print_ਸਮਾਂ_Scale(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_Print_ਸਮਾਂ_Scale(fl) {}
    ASTGEN_MEMBERS_Ast_Print_ਸਮਾਂ_Scale;
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$printtimescale"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
};
class AstRSBreak final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // randsequence break
ਜਨਤਕ:
    explicit AstRSBreak(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "break"; }
    bool isBrancher() ਅਚਲ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
ਜਨਤਕ:
    AstRSProd(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        ਜੋੜੋ_Portsp(portsp);
        ਜੋੜੋ_Rulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
};
class Ast_RSProd_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
ਜਨਤਕ:
    Ast_RSProd_ਵਸਤੂ(File_ਸਤਰ* fl, ਅਚਲ string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProd_ਵਸਤੂ(fl)
        , m_name{name} {
        ਜੋੜੋ_Argsp(argsp);
    }
    ASTGEN_MEMBERS_Ast_RSProd_ਵਸਤੂ;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    AstRSProd* prodp() ਅਚਲ { ਵਾਪਸ m_prodp; }
    ਖਾਲੀ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
ਜਨਤਕ:
    AstRSProdList(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* weightp, Ast_ਗੰਢ* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        ਜੋੜੋ_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    bool randJoin() ਅਚਲ { ਵਾਪਸ m_randJoin; }
    ਖਾਲੀ randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // randsequence return
ਜਨਤਕ:
    explicit AstRSReturn(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "return"; }
    bool isBrancher() ਅਚਲ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
ਜਨਤਕ:
    AstRSRule(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* weightp, AstRSProdList* prodlistsp, Ast_ਗੰਢ* ਭਾਰ_Stmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        ਜੋੜੋ_Prodlistsp(prodlistsp);
        ਜੋੜੋ_ਭਾਰ_Stmtsp(ਭਾਰ_Stmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
};
class AstRandCase final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
ਜਨਤਕ:
    AstRandCase(File_ਸਤਰ* fl, Ast_Case_ਵਸਤੂ* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        ਜੋੜੋ_Itemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
};
class AstRandSequence final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_ਸ਼ੁਰੂ;  // Name of start production, or "" to use first production
ਜਨਤਕ:
    AstRandSequence(File_ਸਤਰ* fl, ਅਚਲ string& ਸ਼ੁਰੂ, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_ਸ਼ੁਰੂ{ਸ਼ੁਰੂ} {
        ਜੋੜੋ_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Block name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string ਸ਼ੁਰੂ() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_ਸ਼ੁਰੂ; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    AstRSProd* prodp() ਅਚਲ { ਵਾਪਸ m_prodp; }
    ਖਾਲੀ prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
ਜਨਤਕ:
    AstRelease(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
ਜਨਤਕ:
    AstRepeat(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }  // Not relevant - converted to FOR
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstReturn final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
ਜਨਤਕ:
    explicit AstReturn(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "return"; }
    bool isBrancher() ਅਚਲ override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
ਜਨਤਕ:
    AstSFormat(File_ਸਤਰ* fl, bool optionalFormat, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp,
               ਅੱਖਰ missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ਅਚਲ newp
            = new AstSFormatF{fl, AstSFormatF::ਵਿਅੰਜਕ_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_ਸਤਰ* fl, AstSFormatF* fmtp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!fmtp());
        ਵਾਪਸ nullptr;
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$sformat"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ true; }
    bool isOutputter() override { ਵਾਪਸ false; }
    ਆਭਾਸੀ bool clean_ਬਾਹਰਲਾ() ਅਚਲ { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSetuphold final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
ਜਨਤਕ:
    AstSetuphold(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* refevp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* dataevp,
                 Ast_ਗੰਢ_ਵਿਅੰਜਕ* delrefp = nullptr, Ast_ਗੰਢ_ਵਿਅੰਜਕ* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਢੇਰ_Trace_T final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // $stacktrace used as task
ਜਨਤਕ:
    explicit Ast_ਢੇਰ_Trace_T(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਢੇਰ_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_ਢੇਰ_Trace_T;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$stacktrace"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Stmt_ਵਿਅੰਜਕ final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
ਜਨਤਕ:
    Ast_Stmt_ਵਿਅੰਜਕ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_Stmt_ਵਿਅੰਜਕ(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_ਵਿਅੰਜਕ;
    // cppcheck-suppress uselessOverride
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ exprp()->is_ਸ਼ੁੱਧ(); }
};
class AstStmtPragma final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
ਜਨਤਕ:
    AstStmtPragma(File_ਸਤਰ* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    ਅਚਲ bool m_isFatal;  // $fatal not $stop
ਜਨਤਕ:
    AstStop(File_ਸਤਰ* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }  // Rarely executes
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ fileline() == samep->fileline(); }
    string emitVerilog() ਅਚਲ { ਵਾਪਸ m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ਅਚਲ { ਵਾਪਸ m_isFatal; }
};
class AstSystemT final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
ਜਨਤਕ:
    AstSystemT(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$system"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਸਮਾਂ_Format final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
ਜਨਤਕ:
    Ast_ਸਮਾਂ_Format(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* unitsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* precisionp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* suffixp,
                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* widthp)
        : ASTGEN_SUPER_ਸਮਾਂ_Format(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_Ast_ਸਮਾਂ_Format;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$timeformat"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
};
class AstTraceDecl final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Trace point declaration
    // Separate from AstTraceInc; as a declaration can't be deleted
    // Parents:  {statement list}
    // Expression being traced - Moved to AstTraceInc by V3Trace
    // @astgen op1 := valuep : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_dtypeCallp: Optional[AstCCall] // Type init function call
    // @astgen ptr := m_dtypeDeclp: Optional[AstTraceDecl] // CCall TraceDecl which replaces this
    uint32_t m_ਕੋਡ{std::numeric_limits<uint32_t>::max()};  // Trace identifier code
    uint32_t m_fidx{0};  // Trace function index
    ਅਚਲ string m_showname;  // Name of variable
    ਅਚਲ VNumRange m_ਦੋਆਧਾਰੀ_ਅੰਕ_Range;  // Property of var the trace details
    ਅਚਲ VNumRange m_arrayRange;  // Property of var the trace details
    ਅਚਲ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ਅਚਲ VDirection m_declDirection;  // Declared direction input/output etc
    ਅਚਲ bool m_ਵਿੱਚ_Dtype_Func;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    ਪੂਰਨ m_ਕੋਡ_Inc{0};  // Code increment for type
ਜਨਤਕ:
    AstTraceDecl(File_ਸਤਰ* fl, ਅਚਲ string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, ਅਚਲ VNumRange& ਦੋਆਧਾਰੀ_ਅੰਕ_Range, ਅਚਲ VNumRange& arrayRange,
                 AstCCall* ਅਚਲ dtypeCallp, ਅਚਲ bool ਵਿੱਚ_Dtype_Func)
        : ASTGEN_SUPER_TraceDecl(fl)
        , m_showname{showname}
        , m_ਦੋਆਧਾਰੀ_ਅੰਕ_Range{ਦੋਆਧਾਰੀ_ਅੰਕ_Range}
        , m_arrayRange{arrayRange}
        , m_varType{varp->varType()}
        , m_declDirection{varp->declDirection()}
        , m_ਵਿੱਚ_Dtype_Func{ਵਿੱਚ_Dtype_Func} {
        dtypeFrom(valuep);
        this->valuep(valuep);
        this->dtypeCallp(dtypeCallp);
    }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_showname; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ true; }
    string showname() ਅਚਲ { ਵਾਪਸ m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t ਕੋਡ() ਅਚਲ { ਵਾਪਸ m_ਕੋਡ; }
    ਖਾਲੀ ਕੋਡ(uint32_t ਕੋਡ) { m_ਕੋਡ = ਕੋਡ; }
    bool ਕੋਡ_Assigned() ਅਚਲ { ਵਾਪਸ m_ਕੋਡ != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ਅਚਲ { ਵਾਪਸ m_fidx; }
    ਖਾਲੀ fidx(uint32_t fidx) { m_fidx = fidx; }
    ਖਾਲੀ ਕੋਡ_Inc(uint32_t ਕੋਡ_Inc) { m_ਕੋਡ_Inc = ਕੋਡ_Inc; }
    uint32_t ਕੋਡ_Inc() ਅਚਲ {
        ਜੇ (m_ਕੋਡ_Inc) { ਵਾਪਸ m_ਕੋਡ_Inc; }
        ਵਾਪਸ (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->ਚੌੜਾਈ_ਸ਼ਬਦ()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ਅਚਲ VNumRange& ਦੋਆਧਾਰੀ_ਅੰਕ_Range() ਅਚਲ { ਵਾਪਸ m_ਦੋਆਧਾਰੀ_ਅੰਕ_Range; }
    ਅਚਲ VNumRange& arrayRange() ਅਚਲ { ਵਾਪਸ m_arrayRange; }
    VVarType varType() ਅਚਲ { ਵਾਪਸ m_varType; }
    VDirection declDirection() ਅਚਲ { ਵਾਪਸ m_declDirection; }
    AstCCall* dtypeCallp() ਅਚਲ { ਵਾਪਸ m_dtypeCallp; }
    ਖਾਲੀ dtypeCallp(AstCCall* ਅਚਲ callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ਅਚਲ { ਵਾਪਸ m_dtypeDeclp; }
    ਖਾਲੀ dtypeDeclp(AstTraceDecl* ਅਚਲ declp) { m_dtypeDeclp = declp; }
    bool ਵਿੱਚ_Dtype_Func() ਅਚਲ { ਵਾਪਸ m_ਵਿੱਚ_Dtype_Func; }
};
class AstTraceInc final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ਅਚਲ uint32_t m_ਅਧਾਰ_ਕੋਡ;  // Trace code base value in function containing this AstTraceInc
    ਅਚਲ VTraceType m_traceType;  // Is this a const/full/incremental dump

ਜਨਤਕ:
    AstTraceInc(File_ਸਤਰ* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t ਅਧਾਰ_ਕੋਡ = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_ਅਧਾਰ_ਕੋਡ{ਅਧਾਰ_ਕੋਡ}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_ਰੁੱਖ(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 10 + 2 * INSTR_ਗਿਣਤੀ_LD; }
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    AstTraceDecl* declp() ਅਚਲ { ਵਾਪਸ m_declp; }
    VTraceType traceType() ਅਚਲ { ਵਾਪਸ m_traceType; }
    uint32_t ਅਧਾਰ_ਕੋਡ() ਅਚਲ { ਵਾਪਸ m_ਅਧਾਰ_ਕੋਡ; }
};
class Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
ਜਨਤਕ:
    explicit Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_Trace_ਕੱਢੋ_ਅਗੇਤਰ(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_ਕੱਢੋ_ਅਗੇਤਰ;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    ਅਚਲ string m_ਅਗੇਤਰ;  // Prefix to add to signal names
    ਅਚਲ VTrace_ਅਗੇਤਰ_Type m_ਅਗੇਤਰ_Type;  // Type of prefix being pushed
    ਅਚਲ ਪੂਰਨ m_ਖੱਬਾ;  // Array left index, or struct/union member count
    ਅਚਲ ਪੂਰਨ m_ਸੱਜਾ;  // Array right index
    ਅਚਲ bool m_quoted_ਅਗੇਤਰ;  // Quote prefix name
ਜਨਤਕ:
    Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ(File_ਸਤਰ* fl, ਅਚਲ string& ਅਗੇਤਰ, VTrace_ਅਗੇਤਰ_Type ਅਗੇਤਰ_Type,
                       ਪੂਰਨ ਖੱਬਾ = 0, ਪੂਰਨ ਸੱਜਾ = 0, bool quoted_ਅਗੇਤਰ = true)
        : ASTGEN_SUPER_Trace_ਧੱਕੋ_ਅਗੇਤਰ(fl)
        , m_ਅਗੇਤਰ{ਅਗੇਤਰ}
        , m_ਅਗੇਤਰ_Type{ਅਗੇਤਰ_Type}
        , m_ਖੱਬਾ{ਖੱਬਾ}
        , m_ਸੱਜਾ{ਸੱਜਾ}
        , m_quoted_ਅਗੇਤਰ{quoted_ਅਗੇਤਰ} {}
    ASTGEN_MEMBERS_Ast_Trace_ਧੱਕੋ_ਅਗੇਤਰ;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ false; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string ਅਗੇਤਰ() ਅਚਲ { ਵਾਪਸ m_ਅਗੇਤਰ; }
    VTrace_ਅਗੇਤਰ_Type ਅਗੇਤਰ_Type() ਅਚਲ { ਵਾਪਸ m_ਅਗੇਤਰ_Type; }
    ਪੂਰਨ ਖੱਬਾ() ਅਚਲ { ਵਾਪਸ m_ਖੱਬਾ; }
    ਪੂਰਨ ਸੱਜਾ() ਅਚਲ { ਵਾਪਸ m_ਸੱਜਾ; }
    bool quoted_ਅਗੇਤਰ() ਅਚਲ { ਵਾਪਸ m_quoted_ਅਗੇਤਰ; }
};
class AstWait final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
ਜਨਤਕ:
    AstWait(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstWaitFork final : ਜਨਤਕ Ast_ਗੰਢ_Stmt {
    // A "wait fork" statement
ਜਨਤਕ:
    explicit AstWaitFork(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ਅਚਲ override { ਵਾਪਸ true; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "wait fork"; }
};

// === AstNodeAssign ===
class Ast_ਸੌਂਪੋ final : ਜਨਤਕ Ast_ਗੰਢ_ਸੌਂਪੋ {
ਜਨਤਕ:
    Ast_ਸੌਂਪੋ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp,
              Ast_ਗੰਢ* timingControlp = nullptr)
        : ASTGEN_SUPER_ਸੌਂਪੋ(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_ਸੌਂਪੋ;
    Ast_ਗੰਢ_ਸੌਂਪੋ* cloneType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) override {
        Ast_ਗੰਢ* ਅਚਲ controlp = timingControlp() ? timingControlp()->clone_ਰੁੱਖ(false) : nullptr;
        ਵਾਪਸ new Ast_ਸੌਂਪੋ{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_ਸੌਂਪੋ_Compound final : ਜਨਤਕ Ast_ਗੰਢ_ਸੌਂਪੋ {
    // Compound assignments (+=, -=, *=, ...)
ਜਨਤਕ:
    ਗਿਣਤੀ operation : uint8_t {
        ਜੋੜੋ_2,
        And,
        Div,
        ModDiv,
        Mul,
        Or,
        ਖਿਸਕਾਓ_L,
        ਖਿਸਕਾਓ_R,
        ਖਿਸਕਾਓ_RS,
        Sub,
        Xor,
    };

ਨਿਜੀ:
    operation m_operation;

ਜਨਤਕ:
    Ast_ਸੌਂਪੋ_Compound(Ast_ਸੌਂਪੋ_Compound::operation operation, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp,
                      Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ* timingControlp = nullptr)
        : ASTGEN_SUPER_ਸੌਂਪੋ_Compound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_ਸੌਂਪੋ_Compound;
    Ast_ਗੰਢ_ਸੌਂਪੋ* cloneType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) override {
        Ast_ਗੰਢ* ਅਚਲ controlp = timingControlp() ? timingControlp()->clone_ਰੁੱਖ(false) : nullptr;
        ਵਾਪਸ new Ast_ਸੌਂਪੋ_Compound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { ਵਾਪਸ m_operation; }
};
class Ast_ਸੌਂਪੋ_Cont final : ਜਨਤਕ Ast_ਗੰਢ_ਸੌਂਪੋ {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
ਜਨਤਕ:
    Ast_ਸੌਂਪੋ_Cont(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp,
                  Ast_ਗੰਢ* timingControlp = nullptr)
        : ASTGEN_SUPER_ਸੌਂਪੋ_Cont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_ਸੌਂਪੋ_Cont;
    Ast_ਗੰਢ_ਸੌਂਪੋ* cloneType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) override {
        Ast_ਗੰਢ* ਅਚਲ controlp = timingControlp() ? timingControlp()->clone_ਰੁੱਖ(false) : nullptr;
        ਵਾਪਸ new Ast_ਸੌਂਪੋ_Cont{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_ਸੌਂਪੋ_Dly final : ਜਨਤਕ Ast_ਗੰਢ_ਸੌਂਪੋ {
ਜਨਤਕ:
    Ast_ਸੌਂਪੋ_Dly(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp,
                 Ast_ਗੰਢ* timingControlp = nullptr)
        : ASTGEN_SUPER_ਸੌਂਪੋ_Dly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_ਸੌਂਪੋ_Dly;
    Ast_ਗੰਢ_ਸੌਂਪੋ* cloneType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) override {
        Ast_ਗੰਢ* ਅਚਲ controlp = timingControlp() ? timingControlp()->clone_ਰੁੱਖ(false) : nullptr;
        ਵਾਪਸ new Ast_ਸੌਂਪੋ_Dly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "<="; }
};
class Ast_ਸੌਂਪੋ_Force final : ਜਨਤਕ Ast_ਗੰਢ_ਸੌਂਪੋ {
    // Procedural 'force' statement
ਜਨਤਕ:
    Ast_ਸੌਂਪੋ_Force(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ਸੌਂਪੋ_Force(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_ਸੌਂਪੋ_Force;
    Ast_ਗੰਢ_ਸੌਂਪੋ* cloneType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) override {
        ਵਾਪਸ new Ast_ਸੌਂਪੋ_Force{fileline(), lhsp, rhsp};
    }
};
class Ast_ਸੌਂਪੋ_W final : ਜਨਤਕ Ast_ਗੰਢ_ਸੌਂਪੋ {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
ਜਨਤਕ:
    Ast_ਸੌਂਪੋ_W(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp,
               Ast_ਗੰਢ* timingControlp = nullptr)
        : ASTGEN_SUPER_ਸੌਂਪੋ_W(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_ਸੌਂਪੋ_W;
    Ast_ਗੰਢ_ਸੌਂਪੋ* cloneType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) override {
        Ast_ਗੰਢ* ਅਚਲ controlp = timingControlp() ? timingControlp()->clone_ਰੁੱਖ(false) : nullptr;
        ਵਾਪਸ new Ast_ਸੌਂਪੋ_W{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ਅਚਲ override {
        ਵਾਪਸ timingControlp() || lhsp()->exists([](ਅਚਲ Ast_ਗੰਢ_Var_Ref* refp) {
            ਵਾਪਸ refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ਅਚਲ;
};

// === AstNodeBlock ===
class AstBegin final : ਜਨਤਕ Ast_ਗੰਢ_ਖੰਡ {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ਅਚਲ bool m_implied : 1;  // Not inserted by user
ਜਨਤਕ:
    AstBegin(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        ਜੋੜੋ_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ ਸੈੱਟ_Need_Process() { m_needProcess = true; }
    bool needProcess() ਅਚਲ { ਵਾਪਸ m_needProcess; }
    bool implied() ਅਚਲ { ਵਾਪਸ m_implied; }
};
class AstFork final : ਜਨਤਕ Ast_ਗੰਢ_ਖੰਡ {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    ਅਚਲ VJoinType m_joinType;  // Join keyword type
    bool m_ਤੁਰੰਤ_ਸ਼ੁਰੂ = false;  // Fork starts before its parent blocks or exits

ਜਨਤਕ:
    AstFork(File_ਸਤਰ* fl, VJoinType joinType, ਅਚਲ string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstFork* ਅਚਲ asamep = VN_DBG_AS(samep, Fork);
        ਵਾਪਸ joinType() == asamep->joinType() && ਤੁਰੰਤ_ਸ਼ੁਰੂ() == asamep->ਤੁਰੰਤ_ਸ਼ੁਰੂ();
    }
    bool isTimingControl() ਅਚਲ override { ਵਾਪਸ !joinType().joinNone(); }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    VJoinType joinType() ਅਚਲ { ਵਾਪਸ m_joinType; }
    bool ਤੁਰੰਤ_ਸ਼ੁਰੂ() ਅਚਲ { ਵਾਪਸ m_ਤੁਰੰਤ_ਸ਼ੁਰੂ; }
    ਖਾਲੀ ਤੁਰੰਤ_ਸ਼ੁਰੂ(bool flag) { m_ਤੁਰੰਤ_ਸ਼ੁਰੂ = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : ਜਨਤਕ Ast_ਗੰਢ_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

ਜਨਤਕ:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_ਸਤਰ* fl, Ast_ਗੰਢ* propp, Ast_ਗੰਢ* passsp, Ast_ਗੰਢ* failsp, VAssertType type,
              VAssertDirectiveType directive, ਅਚਲ string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        ਜੋੜੋ_Failsp(failsp);
    }
    string verilogKwd() ਅਚਲ override {
        ਵਾਪਸ directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : ਜਨਤਕ Ast_ਗੰਢ_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
ਜਨਤਕ:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_ਸਤਰ* fl, Ast_ਗੰਢ* propp, Ast_ਗੰਢ* passsp, Ast_ਗੰਢ* failsp,
                       ਅਚਲ string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::ਅੰਦਰੂਨੀ,
                                       VAssertDirectiveType::INTRINSIC, name) {
        ਜੋੜੋ_Failsp(failsp);
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "assert"; }
};
class AstCover final : ਜਨਤਕ Ast_ਗੰਢ_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
ਜਨਤਕ:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_ਸਤਰ* fl, Ast_ਗੰਢ* propp, Ast_ਗੰਢ* stmtsp, VAssertType type,
             ਅਚਲ string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "cover"; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isCoverSeq() ਅਚਲ { ਵਾਪਸ m_isCoverSeq; }
    ਖਾਲੀ isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ਅਚਲ { ਵਾਪਸ m_isSeqEvent; }
    ਖਾਲੀ isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : ਜਨਤਕ Ast_ਗੰਢ_Cover_Or_Assert {
ਜਨਤਕ:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_ਸਤਰ* fl, Ast_ਗੰਢ* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::ਅੰਦਰੂਨੀ,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : ਜਨਤਕ Ast_ਗੰਢ_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
ਜਨਤਕ:
    AstConstraintForeach(File_ਸਤਰ* fl, Ast_Foreach_ਸਿਰਲੇਖ* headerp, Ast_ਗੰਢ* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ਅਚਲ { ਵਾਪਸ m_soft; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class AstForeach final : ਜਨਤਕ Ast_ਗੰਢ_Foreach {
ਜਨਤਕ:
    AstForeach(File_ਸਤਰ* fl, Ast_Foreach_ਸਿਰਲੇਖ* headerp, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : ਜਨਤਕ Ast_ਗੰਢ_If {
ਜਨਤਕ:
    AstConstraintIf(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ* thensp, Ast_ਗੰਢ* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : ਜਨਤਕ Ast_ਗੰਢ_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ਤਰਜੀਹ_Pragma = false;  // priority case
ਜਨਤਕ:
    AstIf(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ* thensp = nullptr, Ast_ਗੰਢ* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ਅਚਲ { ਵਾਪਸ m_uniquePragma; }
    ਖਾਲੀ uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ਅਚਲ { ਵਾਪਸ m_unique0Pragma; }
    ਖਾਲੀ unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ਤਰਜੀਹ_Pragma() ਅਚਲ { ਵਾਪਸ m_ਤਰਜੀਹ_Pragma; }
    ਖਾਲੀ ਤਰਜੀਹ_Pragma(bool flag) { m_ਤਰਜੀਹ_Pragma = flag; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : ਜਨਤਕ Ast_ਗੰਢ_Read_Write_Mem {
ਜਨਤਕ:
    AstReadMem(File_ਸਤਰ* fl, bool hex, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filenamep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* memp,
               Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ (isHex() ? "$readmemh" : "$readmemb"); }
    ਅਚਲ ਅੱਖਰ* cFuncPrefixp() ਅਚਲ override { ਵਾਪਸ "VL_READMEM_"; }
};
class AstWriteMem final : ਜਨਤਕ Ast_ਗੰਢ_Read_Write_Mem {
ਜਨਤਕ:
    AstWriteMem(File_ਸਤਰ* fl, bool hex, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filenamep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* memp,
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ (isHex() ? "$writememh" : "$writememb"); }
    ਅਚਲ ਅੱਖਰ* cFuncPrefixp() ਅਚਲ override { ਵਾਪਸ "VL_WRITEMEM_"; }
};

#ਜੇ_ਅੰਤ  // Guard
