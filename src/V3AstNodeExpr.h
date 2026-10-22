// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode sub-types representing expressions
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
// This files contains all 'AstNode' sub-types that represent expressions,
// i.e.: those constructs that represent, or evaluate to [a possible void]
// value. The root of the hierarchy is 'AstNodeExpr'.
//
// Think of expressions in a very general sense as constructs that "name
// things". The "thing" can be considered the value, but can be highly
// structured. For example, an AstConst can name the value '1', which is
// hopefully familiar. On the opposite end of the spectrum of "things" named by
// expressions, consider AstClassOrPackageRef, that can name a collection of
// pairs (specifically the collection of ('member name', 'member thing')
// pairs). Nevertheless, that collection itself can be considered a value. The
// valid composition of expressions then defines the calculus of values in the
// language.
//
//*************************************************************************

#ifndef VERILATOR_V3ASTNODEEXPR_H_
#define VERILATOR_V3ASTNODEEXPR_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // An expression tree node
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    // Every expression must have a data type after V3Width
    bool hasDType() ಧ್ರುವ override final VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಆಭಾಸ string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    ಆಭಾಸ string emitC() = 0;
    ಆಭಾಸ string emitSMT() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ""; };
    ಆಭಾಸ string emitSimpleOperator() { ಪ್ರತ್ಯಾಗಮ ""; }  // "" means not ok to use
    ಆಭಾಸ bool emit_Check_Max_ಪದಗಳು() { ಪ್ರತ್ಯಾಗಮ false; }  // Check VL_MULS_MAX_WORDS
    ಆಭಾಸ bool cleanOut() ಧ್ರುವ = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    ಆಭಾಸ bool isMultiCycleSva() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }
    ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getVAccessTargetRecurse() ಧ್ರುವ;
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getVAccessTargetRecurse() {
        ಪ್ರತ್ಯಾಗಮ const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() ಧ್ರುವ;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_ವೃಕ್ಷ() ಧ್ರುವ;
    // Return base var (or const) nodep dereferences
    Ast_ಗಂಟು* baseFromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    ಅನ್ತಃಸ್ಥ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    ಆಭಾಸ ಧ್ರುವ ವರ್ಣ* widthMismatch() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string patternString(uint32_t depth = 0) ಧ್ರುವ;
    bool isFourstateOrNoDtype() ಧ್ರುವ {
        ಯದಿ (!dtypep()) ಪ್ರತ್ಯಾಗಮ true;
        ಪ್ರತ್ಯಾಗಮ dtypep()->skipRefp()->isFourstate();
    }
    ಸ್ಥಿರ bool isFourstateOrNoDtype(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p) {
        ಪ್ರತ್ಯಾಗಮ expr1p->isFourstateOrNoDtype();
    }
    ಸ್ಥಿರ bool isFourstateOrNoDtype(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p,
                                     ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr2p) {
        ಪ್ರತ್ಯಾಗಮ expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    ಸ್ಥಿರ bool isFourstateOrNoDtype(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p,
                                     ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr2p,
                                     ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr3p) {
        ಪ್ರತ್ಯಾಗಮ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    ಸ್ಥಿರ bool isFourstateOrNoDtype(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p,
                                     ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr2p,
                                     ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr3p,
                                     ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr4p) {
        ಪ್ರತ್ಯಾಗಮ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_ಗಂಟು_Biop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Biop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    ಆಭಾಸ ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) = 0;
    ಆಭಾಸ bool cleanLhs() ಧ್ರುವ = 0;  // True if LHS must have extra upper bits zero
    ಆಭಾಸ bool cleanRhs() ಧ್ರುವ = 0;  // True if RHS must have extra upper bits zero
    ಆಭಾಸ bool sizeMattersLhs() ಧ್ರುವ = 0;  // True if output result depends on lhs size
    ಆಭಾಸ bool sizeMattersRhs() ಧ್ರುವ = 0;  // True if output result depends on rhs size
    ಆಭಾಸ bool doubleFlavor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    ಆಭಾಸ bool signedFlavor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }
    ಆಭಾಸ bool stringFlavor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }  // N flavor of nodes with both flavors?
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;

ಗುಪ್ತ:
    bool getPurityRecurse() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ lhsp()->isPure() && rhsp()->isPure(); }
};
class Ast_ಗಂಟು_Bi_Com VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Binary expr with commutative properties
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Bi_Com(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhs, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhs)
        : Ast_ಗಂಟು_Biop{t, fl, lhs, rhs} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Bi_Com;
};
class Ast_ಗಂಟು_Bi_Com_Asv VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // Binary expr with commutative & associative properties
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Bi_Com_Asv(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhs, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhs)
        : Ast_ಗಂಟು_Bi_Com{t, fl, lhs, rhs} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Bi_Com_Asv;
};
class Ast_ಗಂಟು_Dist_Biop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Dist_Biop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : Ast_ಗಂಟು_Biop{t, fl, lhsp, rhsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Dist_Biop;
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL_TRIG; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
};
class Ast_ಗಂಟು_Sel VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Sel(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : Ast_ಗಂಟು_Biop{t, fl, fromp, bitp} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Sel;
    ಪೂರ್ಣಾಙ್ಕ bitConst() ಧ್ರುವ;
};
class Ast_ಗಂಟು_Stream VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Stream(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : Ast_ಗಂಟು_Biop{t, fl, lhsp, rhsp} {
        ಯದಿ (lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(lhsp->width(), lhsp->widthMin(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Stream;
};
class Ast_ಗಂಟು_System_Biop_D VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_System_Biop_D(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : Ast_ಗಂಟು_Biop{t, fl, lhsp, rhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_System_Biop_D;
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL_TRIG; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_ಗಂಟು_CCall VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_CCall(VNType t, File_ಸಾಲು* fl, AstCFunc* funcp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl}
        , m_funcp{funcp} {
        addArgsp(argsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_CCall;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_CALL; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ Ast_ಗಂಟು_CCall* ಧ್ರುವ asamep = VN_DBG_AS(samep, ಗಂಟು_CCall);
        ಪ್ರತ್ಯಾಗಮ (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override;
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ !isPure(); }
    AstCFunc* funcp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcp; }
    ಶೂನ್ಯ funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_argTypes; }
    ಶೂನ್ಯ argTypes(ಧ್ರುವ string& str) { m_argTypes = str; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ final override { ಪ್ರತ್ಯಾಗಮ true; }
    bool superReference() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_superReference; }
    ಶೂನ್ಯ superReference(bool flag) { m_superReference = flag; }
};
class Ast_ಗಂಟು_Cons VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Construct a value of an aggregate type
    friend class Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ;  // For m_knownConstantTree
    mutable bool m_known_Constant_ವೃಕ್ಷ = false;  // Known to be 'this->isConstantTree() == true'

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Cons(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Cons;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ final override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ final override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
};
class Ast_ಗಂಟು_FTask_Ref VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_ಗಂಟು_FTask_Ref;  // Gen pointers before other members for performance
ಗುಪ್ತ:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_FTask_Ref(VNType t, File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstArg* argsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl}
        , m_name{name} {
        addArgsp(argsp);
    }

ಸಾರ್ವಜನಿಕ:
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    bool isGateOptimizable() ಧ್ರುವ override;
    string dotted() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dotted; }  // * = Scope name or ""
    string inlinedDots() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inlinedDots; }
    ಶೂನ್ಯ inlinedDots(ಧ್ರುವ string& flag) { m_inlinedDots = flag; }
    Ast_ಗಂಟು_FTask* taskp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_taskp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ taskp(Ast_ಗಂಟು_FTask* taskp) { m_taskp = taskp; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    ಶೂನ್ಯ dotted(ಧ್ರುವ string& name) { m_dotted = name; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
    bool containsGenBlock() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_containsGenBlock; }
    ಶೂನ್ಯ containsGenBlock(ಧ್ರುವ bool flag) { m_containsGenBlock = flag; }
    bool pli() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pli; }
    ಶೂನ್ಯ pli(bool flag) { m_pli = flag; }
    bool superReference() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_superReference; }
    ಶೂನ್ಯ superReference(bool flag) { m_superReference = flag; }
    bool isPure() override;
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_taskp && m_taskp->isUnlikely(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ asamep = VN_DBG_AS(samep, ಗಂಟು_FTask_Ref);
        ಪ್ರತ್ಯಾಗಮ taskp() == asamep->taskp()  //
               && classOrPackagep() == asamep->classOrPackagep()  //
               && name() == asamep->name()  //
               && dotted() == asamep->dotted()  //
               && inlinedDots() == asamep->inlinedDots()  //
               && pli() == asamep->pli()  //
               && containsGenBlock() == asamep->containsGenBlock()
               && superReference() == asamep->superReference();
    }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ final override { V3ERROR_NA_RETURN(true); }

ಗುಪ್ತ:
    bool getPurityRecurse() ಧ್ರುವ;
};
class Ast_ಗಂಟು_Pre_Sel VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Pre_Sel(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Pre_Sel;
    // METHODS
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ final override { V3ERROR_NA_RETURN(true); }
    bool isPure() override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;

ಗುಪ್ತ:
    bool getPurityRecurse() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ fromp()->isPure() && rhsp()->isPure() && (!thsp() || thsp()->isPure());
    }
};
class Ast_ಗಂಟು_Quadop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Quadop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp,
                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fhsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    ಆಭಾಸ ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs,
                               ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& fhs)
        = 0;
    ಆಭಾಸ bool cleanLhs() ಧ್ರುವ = 0;  // True if LHS must have extra upper bits zero
    ಆಭಾಸ bool cleanRhs() ಧ್ರುವ = 0;  // True if RHS must have extra upper bits zero
    ಆಭಾಸ bool cleanThs() ಧ್ರುವ = 0;  // True if THS must have extra upper bits zero
    ಆಭಾಸ bool cleanFhs() ಧ್ರುವ = 0;  // True if THS must have extra upper bits zero
    ಆಭಾಸ bool sizeMattersLhs() ಧ್ರುವ = 0;  // True if output result depends on lhs size
    ಆಭಾಸ bool sizeMattersRhs() ಧ್ರುವ = 0;  // True if output result depends on rhs size
    ಆಭಾಸ bool sizeMattersThs() ಧ್ರುವ = 0;  // True if output result depends on ths size
    ಆಭಾಸ bool sizeMattersFhs() ಧ್ರುವ = 0;  // True if output result depends on ths size
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;

ಗುಪ್ತ:
    bool getPurityRecurse() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure() && fhsp()->isPure();
    }
};
class Ast_ಗಂಟು_Termop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Terminal operator -- an operator with no "inputs"
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Termop(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Termop;
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_ಗಂಟು_Triop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Triop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Triop;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    // Set out to evaluation of a AstConst'ed
    ಆಭಾಸ ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs,
                               ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths)
        = 0;
    ಆಭಾಸ bool cleanLhs() ಧ್ರುವ = 0;  // True if LHS must have extra upper bits zero
    ಆಭಾಸ bool cleanRhs() ಧ್ರುವ = 0;  // True if RHS must have extra upper bits zero
    ಆಭಾಸ bool cleanThs() ಧ್ರುವ = 0;  // True if THS must have extra upper bits zero
    ಆಭಾಸ bool sizeMattersLhs() ಧ್ರುವ = 0;  // True if output result depends on lhs size
    ಆಭಾಸ bool sizeMattersRhs() ಧ್ರುವ = 0;  // True if output result depends on rhs size
    ಆಭಾಸ bool sizeMattersThs() ಧ್ರುವ = 0;  // True if output result depends on ths size
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;

ಗುಪ್ತ:
    bool getPurityRecurse() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure();
    }
};
class Ast_ಗಂಟು_Dist_Triop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Dist_Triop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
                     Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : Ast_ಗಂಟು_Triop{t, fl, lhsp, rhsp, thsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Dist_Triop;
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL_TRIG; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs,
                       ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths) override {
        V3ERROR_NA;
    }
};
class Ast_ಗಂಟು_Uniop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Uniop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Uniop;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    // Set out to evaluation of a AstConst'ed lhs
    ಆಭಾಸ ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) = 0;
    ಆಭಾಸ bool cleanLhs() ಧ್ರುವ = 0;
    ಆಭಾಸ bool sizeMattersLhs() ಧ್ರುವ = 0;  // True if output result depends on lhs size
    ಆಭಾಸ bool doubleFlavor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    ಆಭಾಸ bool signedFlavor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }
    ಆಭಾಸ bool stringFlavor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }  // N flavor of nodes with both flavors?
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
};
class Ast_ಗಂಟು_System_Uniop_D VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_System_Uniop_D(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : Ast_ಗಂಟು_Uniop{t, fl, lhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_System_Uniop_D;
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL_TRIG; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_ಗಂಟು_Var_Ref VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Var_Ref;  // Gen pointers before other members for performance
ಗುಪ್ತ:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Var_Ref(VNType t, File_ಸಾಲು* fl, AstVar* varp, ಧ್ರುವ VAccess& access)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    VAccess access() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_access; }
    ಶೂನ್ಯ access(ಧ್ರುವ VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_varp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_ಗಂಟು*>(varp));
    }
    AstVarScope* varScopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_varScopep; }
    ಶೂನ್ಯ varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    ಧ್ರುವ VSelfPointerText& selfPointer() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_selfPointer; }
    ಶೂನ್ಯ selfPointer(ಧ್ರುವ VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ selfPointer().protect(useSelfForThis, protect());
    }
    Ast_ಗಂಟು_Module* classOrPackagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
ಸಾರ್ವಜನಿಕ:
    AstArg(File_ಸಾಲು* fl, ಧ್ರುವ std::string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ std::string& name) override { m_name = name; }
    bool emptyConnectNoNext() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !exprp() && name() == "" && !nextp(); }
};
class AstWith final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Similar to AstArg, but this is essentially a lambda passed to a call.
    // Not an AstNodeExpr as there is no concept of function values in Verilator
    // The dtypep() contains the with lambda's return dtype, not a function type.
    // Parents: NodeFTaskRef, CMethodHard
    // Children: LambdaArgRef that declares the item variable
    // Children: LambdaArgRef that declares the item.index variable
    // Children: expression (equation establishing the with)
    // @astgen op1 := indexArgRefp : AstLambdaArgRef
    // @astgen op2 := valueArgRefp : AstLambdaArgRef
    // @astgen op3 := exprp : List[AstNode]  // Pins, expression and constraints
    // TODO: Separate expression and constraints
ಗುಪ್ತ:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::set<std::string> m_restrictedNames;

ಸಾರ್ವಜನಿಕ:
    AstWith(File_ಸಾಲು* fl, AstLambdaArgRef* indexArgRefp, AstLambdaArgRef* ಮೌಲ್ಯ_Arg_Refp,
            Ast_ಗಂಟು* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->indexArgRefp(indexArgRefp);
        this->ಮೌಲ್ಯ_Arg_Refp(ಮೌಲ್ಯ_Arg_Refp);
        addExprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!indexArgRefp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!ಮೌಲ್ಯ_Arg_Refp());  // varp needed to know lambda's arg dtype
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ restricted(bool flag) { m_restricted = flag; }
    bool restricted() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_restricted; }
    bool validated() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_validated; }
    ಶೂನ್ಯ validated(bool flag) { m_validated = flag; }
    ಶೂನ್ಯ addRestrictedName(ಧ್ರುವ std::string& name) { m_restrictedNames.ಸೇರಿಸು(name); }
    ಧ್ರುವ std::set<std::string>& restrictedNames() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_restrictedNames; }
    // True if 'name' binds into the randomize() target class.
    bool nameResolvesToTarget(ಧ್ರುವ std::string& name) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ !m_restricted || m_restrictedNames.ಎಣಿಕೆ(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbortKind m_kind{VAbortKind::ACCEPT_ON};

ಸಾರ್ವಜನಿಕ:
    AstAbortOn(File_ಸಾಲು* fl, VAbortKind kind, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_kind{kind} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    VAbortKind kind() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_kind; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstAddrOfCFunc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

ಸಾರ್ವಜನಿಕ:
    AstAddrOfCFunc(File_ಸಾಲು* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_AstAddrOfCFunc;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    AstCFunc* funcp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcp; }
};
class AstAttrOf final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
ಸಾರ್ವಜನಿಕ:
    AstAttrOf(File_ಸಾಲು* fl, VAttrType attrtype, Ast_ಗಂಟು* fromp = nullptr,
              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* dimp = nullptr)
        : ASTGEN_SUPER_AttrOf(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_AstAttrOf;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    VAttrType attrType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrType; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ಧ್ರುವ bool m_pure;  // Pure optimizable

    ಶೂನ್ಯ init(ಧ್ರುವ string& text, ಪೂರ್ಣಾಙ್ಕ setwidth) {
        ಯದಿ (!text.empty()) add(text);
        ಯದಿ (setwidth) {
            dtypeSetBitSized(setwidth, VSigning::UNSIGNED);
        } ಅನ್ಯಥಾ {
            dtypeSetVoid();  // Caller to override if necessary
        }
    }

ಸಾರ್ವಜನಿಕ:
    class Pure {};
    explicit AstCExpr(File_ಸಾಲು* fl, ಧ್ರುವ string& text = "", ಪೂರ್ಣಾಙ್ಕ setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{false} {
        init(text, setwidth);
    }
    AstCExpr(File_ಸಾಲು* fl, Pure, ಧ್ರುವ string& text = "", ಪೂರ್ಣಾಙ್ಕ setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_pure; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_pure; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ m_pure; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    // Add some text, or a node to this expression
    ಶೂನ್ಯ add(ಧ್ರುವ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ಶೂನ್ಯ add(Ast_ಗಂಟು* nodep) { addNodesp(nodep); }
};
class AstCExprUser final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ಧ್ರುವ bool m_pure;  // Whether the function is pure
ಸಾರ್ವಜನಿಕ:
    class Pure {};
    explicit AstCExprUser(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{false} {}
    AstCExprUser(File_ಸಾಲು* fl, Pure)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_pure; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_pure; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ m_pure; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    // Add some text, or a node to this expression
    ಶೂನ್ಯ add(ಧ್ರುವ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ಶೂನ್ಯ add(Ast_ಗಂಟು* nodep) { addNodesp(nodep); }
};
class AstCFuncHard final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
ಸಾರ್ವಜನಿಕ:
    AstCFuncHard(File_ಸಾಲು* fl, VCFunction function, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ function().ascii(); }
    bool isPure() override {
        ಯದಿ (!m_purity.isCached()) m_purity.set(getPurity());
        ಪ್ರತ್ಯಾಗಮ m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    VCFunction function() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_function; }

ಗುಪ್ತ:
    bool getPurity();
};
class AstCMethodHard final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_method;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
ಸಾರ್ವಜನಿಕ:
    AstCMethodHard(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, VCMethod method, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_method{method} {
        this->fromp(fromp);
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ method().ascii(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstCMethodHard* ಧ್ರುವ asamep = VN_DBG_AS(samep, CMethodHard);
        ಪ್ರತ್ಯಾಗಮ (m_method == asamep->m_method);
    }
    bool isPure() override {
        ಯದಿ (!m_purity.isCached()) m_purity.set(getPurity());
        ಪ್ರತ್ಯಾಗಮ m_purity.get();
    }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_method.isClean(); }
    bool usePtr() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_usePtr; }
    ಶೂನ್ಯ usePtr(bool flag) { m_usePtr = flag; }
    VCMethod method() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_method; }
    ಶೂನ್ಯ method(VCMethod ಮೌಲ್ಯ) { m_method = ಮೌಲ್ಯ; }

ಗುಪ್ತ:
    bool getPurity();
};
class AstCReset final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Reset variable at startup
    ಧ್ರುವ bool m_constructing;  // Previously cleared by constructor
ಸಾರ್ವಜನಿಕ:
    AstCReset(File_ಸಾಲು* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!VN_IS(backp(), ಗಂಟು_Assign));  // V3Emit* assumption
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constructing; }
};
class AstCast final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
ಸಾರ್ವಜನಿಕ:
    AstCast(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, VFlagChildDType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
ಸಾರ್ವಜನಿಕ:
    AstCastParse(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstCastSize final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
ಸಾರ್ವಜನಿಕ:
    AstCastSize(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_CastSize(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCastSize;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
ಸಾರ್ವಜನಿಕ:
    AstCellArrayRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        addSelp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
ಸಾರ್ವಜನಿಕ:
    AstCellRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* cellp, Ast_ಗಂಟು* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
ಸಾರ್ವಜನಿಕ:
    AstClassOrPackageRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_ಗಂಟು_Module* classOrPackageSkipp(ಧ್ರುವ bool doRefs = true) ಧ್ರುವ;
    Ast_ಗಂಟು* classOrPackageNodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classOrPackageNodep; }
    ಶೂನ್ಯ classOrPackageNodep(Ast_ಗಂಟು* nodep) { m_classOrPackageNodep = nodep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_ಗಂಟು*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A constant
    V3_ಸಂಖ್ಯೆ m_ಅಂಕೆ;  // Constant value
    ಶೂನ್ಯ init_With_ಸಂಖ್ಯೆ() {
        ಯದಿ (m_ಅಂಕೆ.isDouble()) {
            dtypeSetDouble();
        } ಅನ್ಯಥಾ ಯದಿ (m_ಅಂಕೆ.isString()) {
            dtypeSetString();
        } ಅನ್ಯಥಾ {
            dtypeSetBitOrLogicUnsized(m_ಅಂಕೆ.width(), (m_ಅಂಕೆ.sized() ? 0 : m_ಅಂಕೆ.widthToFit()),
                                      VSigning::fromBool(m_ಅಂಕೆ.isSigned()), m_ಅಂಕೆ.isAnyXZ());
        }
        m_ಅಂಕೆ.nodep(this);
    }

ಸಾರ್ವಜನಿಕ:
    ~AstConst() override;
    AstConst(File_ಸಾಲು* fl, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{ಅಂಕೆ} {
        init_With_ಸಂಖ್ಯೆ();
    }
    class Widthed_ಮೌಲ್ಯ {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, Widthed_ಮೌಲ್ಯ, ಪೂರ್ಣಾಙ್ಕ width, uint32_t ಮೌಲ್ಯ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, width, ಮೌಲ್ಯ) {  // Need () constructor
        init_With_ಸಂಖ್ಯೆ();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_ಸಾಲು* fl, DTyped, ಧ್ರುವ Ast_ಗಂಟು_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, nodedtypep} {
        init_With_ಸಂಖ್ಯೆ();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, StringToParse, ಧ್ರುವ ವರ್ಣ* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::Verilog_ಸಂಖ್ಯೆ_Literal{}, sourcep} {
        init_With_ಸಂಖ್ಯೆ();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, VerilogStringLiteral, ಧ್ರುವ string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::VerilogStringLiteral{}, str} {
        init_With_ಸಂಖ್ಯೆ();
    }
    AstConst(File_ಸಾಲು* fl, uint32_t ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 32, ಅಂಕೆ) {  // Need () constructor
        dtypeSetBitUnsized(m_ಅಂಕೆ.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, Unsized32, uint32_t ಅಂಕೆ)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 32, ಅಂಕೆ) {  // Need () constructor
        m_ಅಂಕೆ.width(32, false);
        dtypeSetBitUnsized(32, m_ಅಂಕೆ.widthToFit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, Signed32, int32_t ಅಂಕೆ)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 32, ಅಂಕೆ) {  // Need () constructor
        m_ಅಂಕೆ.width(32, true);
        dtypeSetBitUnsized(32, m_ಅಂಕೆ.widthToFit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, Unsized64, uint64_t ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 64, 0) {  // Need () constructor
        m_ಅಂಕೆ.setQuad(ಅಂಕೆ);
        dtypeSetBitSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, SizedEData, uint64_t ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_ಅಂಕೆ.setQuad(ಅಂಕೆ);
        dtypeSetBitSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, RealDouble, ದ್ವಿಗುಣ ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 64) {  // Need () constructor
        m_ಅಂಕೆ.setDouble(ಅಂಕೆ);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, String, ಧ್ರುವ string& ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::String{}, ಅಂಕೆ} {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(File_ಸಾಲು* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_ಸಾಲು* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
        File_ಸಾಲು* ಧ್ರುವ newfl = new File_ಸಾಲು{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_ಸಾಲು* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 1, on) {  // Need () constructor
        dtypeSetBit();
    }
    class All0 {};
    AstConst(File_ಸಾಲು* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::Verilog_ಸಂಖ್ಯೆ_Literal{}, "'0"} {
        init_With_ಸಂಖ್ಯೆ();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_ಸಾಲು* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::Verilog_ಸಂಖ್ಯೆ_Literal{}, "'1"} {
        init_With_ಸಂಖ್ಯೆ();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_ಸಾಲು* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::Null{}} {
        dtypep(findNullDType());
        init_With_ಸಂಖ್ಯೆ();
    }
    class OneStep {};
    AstConst(File_ಸಾಲು* fl, OneStep)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::OneStep{}} {
        dtypeSetBitSized(64, VSigning::UNSIGNED);
        init_With_ಸಂಖ್ಯೆ();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ().ascii(); }  // * = Value
    ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ; }  // * = Value
    V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() { ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ; }  // * = Value
    string origParamName() ಧ್ರುವ;
    ಶೂನ್ಯ origParamName(ಧ್ರುವ string& name);
    uint32_t toUInt() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ().toUInt(); }
    int32_t toSInt() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ().toSInt(); }
    uint64_t toUQuad() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstConst* ಧ್ರುವ sp = VN_DBG_AS(samep, Const);
        ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ().isCaseEq(sp->ಅಂಕೆ());
    }
    ಶೂನ್ಯ cloneRelink() override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(m_ಅಂಕೆ.nodep() && m_ಅಂಕೆ.nodep() != this);
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool isEqAllOnes() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ().isEqAllOnes(width()); }
    bool isEqAllOnesV() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಅಂಕೆ().isEqAllOnes(widthMinV()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_ಮೌಲ್ಯ_Type(ಧ್ರುವ AstConst* samep) ಧ್ರುವ;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    ಸ್ಥಿರ AstConst* parseParamLiteral(File_ಸಾಲು* fl, ಧ್ರುವ string& literal);
};
class AstConstraintRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

ಸಾರ್ವಜನಿಕ:
    AstConstraintRef(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() ಧ್ರುವ override VL_MT_STABLE;
    AstConstraint* constrp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_constrp; }
    ಶೂನ್ಯ constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    // TODO: this setter is unused
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ final override { V3ERROR_NA_RETURN(true); }
};
class AstCvtArrayToArray final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
ಗುಪ್ತ:
    ಧ್ರುವ bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_blockSize;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_dstElementBits;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_srcElementBits;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToArray(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp, bool reverse,
                       ಪೂರ್ಣಾಙ್ಕ blockSize, ಪೂರ್ಣಾಙ್ಕ dstElementBits, ಪೂರ್ಣಾಙ್ಕ srcElementBits)
        : ASTGEN_SUPER_CvtArrayToArray(fl)
        , m_reverse{reverse}
        , m_blockSize{blockSize}
        , m_dstElementBits{dstElementBits}
        , m_srcElementBits{srcElementBits} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToArray;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool reverse() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_reverse; }
    ಪೂರ್ಣಾಙ್ಕ blockSize() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_blockSize; }
    ಪೂರ್ಣಾಙ್ಕ dstElementBits() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dstElementBits; }
    ಪೂರ್ಣಾಙ್ಕ srcElementBits() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_srcElementBits; }
};
class AstCvtArrayToPacked final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToPacked(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_CvtArrayToPacked(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToPacked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCvtPackedToArray final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtPackedToArray(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_CvtPackedToArray(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtPackedToArray;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCvtUnpackToQueue final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtUnpackToQueue(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToQueue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToQueue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCvtUnpackToUnpack final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstCvtUnpackToUnpack(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToUnpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToUnpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstDist final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
ಸಾರ್ವಜನಿಕ:
    AstDist(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // NA
};
class AstDistItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_isWhole = false;  // True for weight ':/', false for ':='
ಸಾರ್ವಜನಿಕ:
    AstDistItem(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l "s + (m_isWhole ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // NA
    ಶೂನ್ಯ isWhole(bool flag) { m_isWhole = flag; }
    bool isWhole() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isWhole; }
};
class AstDot final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    ಧ್ರುವ bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
ಸಾರ್ವಜನಿಕ:
    AstDot(File_ಸಾಲು* fl, bool colon, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newIfPkg(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* packageOrClassp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        ಯದಿ (!packageOrClassp) ಪ್ರತ್ಯಾಗಮ rhsp;
        ಪ್ರತ್ಯಾಗಮ new AstDot{fl, true, packageOrClassp, rhsp};
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool colon() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstEmptyQueue final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
ಸಾರ್ವಜನಿಕ:
    explicit AstEmptyQueue(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_AstEmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "{}"; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstEnumItemRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_containsGenBlock = false;  // Contains gen block reference
ಸಾರ್ವಜನಿಕ:
    AstEnumItemRef(File_ಸಾಲು* fl, AstEnumItem* itemp, Ast_ಗಂಟು_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ itemp() ? itemp()->name() : m_name; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstEnumItemRef* ಧ್ರುವ sp = VN_DBG_AS(samep, EnumItemRef);
        ಪ್ರತ್ಯಾಗಮ itemp() == sp->itemp() && dotted() == sp->dotted()
               && containsGenBlock() == sp->containsGenBlock();
    }
    AstEnumItem* itemp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_itemp; }
    ಶೂನ್ಯ itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dotted; }
    ಶೂನ್ಯ dotted(ಧ್ರುವ string& dotted) { m_dotted = dotted; }
    bool containsGenBlock() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_containsGenBlock; }
    ಶೂನ್ಯ containsGenBlock(ಧ್ರುವ bool flag) { m_containsGenBlock = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
ಗುಪ್ತ:
    bool m_hasResult = true;  // Returns result via resultp()

ಸಾರ್ವಜನಿಕ:
    Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp)
        : ASTGEN_SUPER_ಅಭಿವ್ಯಕ್ತಿ_Stmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override {
        ಯದಿ (Ast_ಗಂಟು::afterCommentp(stmtsp())) ಪ್ರತ್ಯಾಗಮ false;
        ಪ್ರತ್ಯಾಗಮ resultp()->isPure();
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool hasResult() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasResult; }
    ಶೂನ್ಯ hasResult(bool flag) { m_hasResult = flag; }
};
class AstFError final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstFError(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 64; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFOpen final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstFOpen(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* modep)
        : ASTGEN_SUPER_FOpen(fl) {
        this->filenamep(filenamep);
        this->modep(modep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpen;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$fopen"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFOpenMcd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op2 := filenamep : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstFOpenMcd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$fopen"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFRead final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
ಸಾರ್ವಜನಿಕ:
    AstFRead(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* memp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* startp,
             Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: makes output
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFRewind final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := filep : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstFRewind(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFScanF final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

ಸಾರ್ವಜನಿಕ:
    AstFScanF(File_ಸಾಲು* fl, ಧ್ರುವ string& text, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_text; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: makes output
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_text; }  // * = Text to display
    ಶೂನ್ಯ text(ಧ್ರುವ string& text) { m_text = text; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFSeek final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstFSeek(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* offset, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: makes output
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFTell final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
ಸಾರ್ವಜನಿಕ:
    AstFTell(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFalling final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstFalling(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFell final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ಸಾರ್ವಜನಿಕ:
    AstFell(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Sen_ವೃಕ್ಷ* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFuture final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ಸಾರ್ವಜನಿಕ:
    AstFuture(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Sen_ವೃಕ್ಷ* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstGatePin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
ಸಾರ್ವಜನಿಕ:
    AstGatePin(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstGetInitialRandomSeed final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $get_initial_random_seed()
ಸಾರ್ವಜನಿಕ:
    explicit AstGetInitialRandomSeed(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_GetInitialRandomSeed(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstGetInitialRandomSeed;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstImplication final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

ಗುಪ್ತ:
    ಧ್ರುವ bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    ಧ್ರುವ bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

ಸಾರ್ವಜನಿಕ:
    AstImplication(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isOverlapped; }
    bool isFollowedBy() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isFollowedBy; }
};
class AstInside final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstInside(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // NA
};
class AstInsideRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstInsideRange(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newAndFromInside(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp);
};
class AstLambdaArgRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_index;  // Index, not value

ಸಾರ್ವಜನಿಕ:
    AstLambdaArgRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, bool index)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_index{index} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    bool index() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_index; }
    bool is_ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstMatchMasked final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
ಸಾರ್ವಜನಿಕ:
    ಅನ್ತಃಸ್ಥ AstMatchMasked(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_AstMatchMasked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಸ್ಥಿರ uint32_t fold(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, AstVar* matchVarp);
};
class AstMatches final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
ಸಾರ್ವಜನಿಕ:
    AstMatches(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstMemberSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
ಸಾರ್ವಜನಿಕ:
    AstMemberSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, VFlagChildDType, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    VAccess access() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_access; }
    ಶೂನ್ಯ access(ಧ್ರುವ VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    AstVar* varp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_varp; }
    ಶೂನ್ಯ varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstNewCopy(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
};
class AstNewDynamic final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstNewDynamic(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
};
class AstPExpr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
ಸಾರ್ವಜನಿಕ:
    explicit AstPExpr(File_ಸಾಲು* fl, AstBegin* bodyp, Ast_ಗಂಟು_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_ಸಾಲು* fl, AstBegin* bodyp, Ast_ಗಂಟು_Stmt* finalp, Ast_ಗಂಟು_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
};
class AstParseHolder final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
ಸಾರ್ವಜನಿಕ:
    explicit AstParseHolder(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

ಸಾರ್ವಜನಿಕ:
    AstParseRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nullptr,
                Ast_ಗಂಟು_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstParseRef* ಧ್ರುವ asamep = VN_DBG_AS(samep, ParseRef);
        ಪ್ರತ್ಯಾಗಮ m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

ಸಾರ್ವಜನಿಕ:
    AstPast(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ticksp = nullptr,
            Ast_Sen_ವೃಕ್ಷ* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_propertyTiming; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstPatMember final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog '{a} or '{a{b}}
    // Parents: AstPattern
    // Children: expression, AstPattern, replication count, decoded nodep if TEXT
    // Expression to assign or another AstPattern (list if replicated)
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := keyp : Optional[AstNode]
    // @astgen op3 := repp : Optional[AstNodeExpr]  // replication count, or nullptr for count 1
    // @astgen op4 := varrefp : Optional[AstNodeExpr]  // Decoded variable if TEXT
    bool m_isDefault = false;  // Has default
    bool m_isConcat = false;  // From concatenate

ಸಾರ್ವಜನಿಕ:
    AstPatMember(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhssp, Ast_ಗಂಟು* keyp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 2; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    bool isConcat() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isConcat; }
    ಶೂನ್ಯ isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isDefault; }
    ಶೂನ್ಯ isDefault(bool flag) { m_isDefault = flag; }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಕೀಲಿ_Exprp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varrefp() ? varrefp() : VN_CAST(keyp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ); }
};
class AstPattern final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
ಸಾರ್ವಜನಿಕ:
    AstPattern(File_ಸಾಲು* fl, Ast_ಗಂಟು* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstPattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ dtypep() ? dtypep() : childDTypep(); }
};
class AstPatternStar final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Pattern wildcard: ".*"
ಸಾರ್ವಜನಿಕ:
    explicit AstPatternStar(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_PatternStar(fl) {}
    ASTGEN_MEMBERS_AstPatternStar;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstPatternVar final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
ಸಾರ್ವಜನಿಕ:
    AstPatternVar(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_PatternVar(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPatternVar;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_name == VN_DBG_AS(samep, PatternVar)->m_name;
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
};
class AstPropAlways final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    ಧ್ರುವ bool m_isStrong = false;  // s_always
ಸಾರ್ವಜನಿಕ:
    AstPropAlways(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* loBoundp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { V3ERROR_NA_RETURN(0); }
    bool isStrong() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isStrong; }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstRand final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    ಧ್ರುವ bool m_urandom;  // $urandom vs $random
ಸಾರ್ವಜನಿಕ:
    class Reset {};
    AstRand(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_ಸಾಲು* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtypeSetUInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override {
        ಪ್ರತ್ಯಾಗಮ seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        ಯದಿ (seedp()) {
            ಯದಿ (urandom()) ಪ್ರತ್ಯಾಗಮ "VL_URANDOM_SEEDED_%nq%lq(%li)";
            ಪ್ರತ್ಯಾಗಮ "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        ಯದಿ (isWide()) ಪ್ರತ್ಯಾಗಮ "VL_RANDOM_%nq(%nw, %P)";
        ಪ್ರತ್ಯಾಗಮ "VL_RANDOM_%nq()";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ !seedp(); }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool combinable(ಧ್ರುವ AstRand* samep) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_urandom; }
};
class AstRandRNG final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
ಸಾರ್ವಜನಿಕ:
    AstRandRNG(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$rngrandom()"; }
    string emitC() override {
        ಪ್ರತ್ಯಾಗಮ isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstRising final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstRising(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstRose final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ಸಾರ್ವಜನಿಕ:
    AstRose(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Sen_ವೃಕ್ಷ* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSClocked final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstSClocked(File_ಸಾಲು* fl, AstSenItem* sensesp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Consecutive repetition [*N], [*N:M], [*N:$], [+], [*] (IEEE 1800-2023 16.9.2)
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (N); always a non-negative constant after V3Width
    // op3 := maxCountp -- max repetition count (M); nullptr when exact or unbounded
    //
    // Encoding:
    //   [*N]:   countp=N, maxCountp=nullptr, unbounded=false
    //   [*N:M]: countp=N, maxCountp=M,       unbounded=false
    //   [*N:$]: countp=N, maxCountp=nullptr,  unbounded=true
    //   [+]:    countp=1, maxCountp=nullptr,  unbounded=true  (= [*1:$])
    //   [*]:    countp=0, maxCountp=nullptr,  unbounded=true  (= [*0:$])
    //
    // Lowering:
    //   Exact [*N] standalone: V3AssertPre saturating counter
    //   All other forms and all SExpr-contained forms: V3AssertNfa
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
    ಧ್ರುವ bool m_unbounded = false;  // True when the upper bound is $
ಸಾರ್ವಜನಿಕ:
    // Exact [*N]
    AstSConsRep(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { V3ERROR_NA_RETURN(0); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {  // LCOV_EXCL_LINE
        ಪ್ರತ್ಯಾಗಮ m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unbounded; }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSEventually final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    explicit AstSEventually(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    explicit AstSExpr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* preExprp, Ast_ಗಂಟು_Stmt* delayp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_ಸಾಲು* fl, Ast_ಗಂಟು_Stmt* delayp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSFormatArg final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

ಸಾರ್ವಜನಿಕ:
    AstSFormatArg(File_ಸಾಲು* fl, VFormatAttr formatAttr, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$sformatarg"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    VFormatAttr formatAttr() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_formatAttr; }
    ಶೂನ್ಯ formatAttr(ಧ್ರುವ VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    ಸ್ಥಿರ VFormatAttr formatAttrDefauled(ಧ್ರುವ AstSFormatArg* nodep, ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep);
};
class AstSFormatF final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_ಅಭಿವ್ಯಕ್ತಿ_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    ಧ್ರುವ ವರ್ಣ m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    class ಅಭಿವ್ಯಕ್ತಿ_Format {};
    AstSFormatF(File_ಸಾಲು* fl, ಧ್ರುವ string& text, bool hidden, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp,
                ವರ್ಣ missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    AstSFormatF(File_ಸಾಲು* fl, ಅಭಿವ್ಯಕ್ತಿ_Format, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp, ವರ್ಣ missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_ಅಭಿವ್ಯಕ್ತಿ_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_text; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_text = name; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$sformatf"; }
    string text() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_text; }  // * = Text to display
    ಶೂನ್ಯ text(ಧ್ರುವ string& text) { m_text = text; }
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(text() != "" && ಅಭಿವ್ಯಕ್ತಿ_Format_2());  // Expr format means no literal format
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool formatScopeTracking() ಧ್ರುವ {  // Track scopeNamep();  Ok if false positive
        ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            ಪ್ರತ್ಯಾಗಮ true;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ fmtp = VN_CAST(exprp, Const)) {
                ಯದಿ (fmtp->ಅಂಕೆ().isFromString()) {
                    ಧ್ರುವ string str = fmtp->ಅಂಕೆ().toString();
                    ಯದಿ (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        ಪ್ರತ್ಯಾಗಮ true;
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ false;
    }
    bool hidden() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hidden; }
    ಶೂನ್ಯ hidden(bool flag) { m_hidden = flag; }
    bool ಅಭಿವ್ಯಕ್ತಿ_Format_2() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಅಭಿವ್ಯಕ್ತಿ_Format; }
    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Format_2(bool flag) { m_ಅಭಿವ್ಯಕ್ತಿ_Format = flag; }
    bool optionalFormat() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_optionalFormat; }
    ಶೂನ್ಯ optionalFormat(bool flag) { m_optionalFormat = flag; }
    ವರ್ಣ missingArgChar() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_missingArgChar; }
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSGotoRep final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    // Exact [->N]
    AstSGotoRep(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSNonConsRep final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    // Exact [=N]
    AstSNonConsRep(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSScanF final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

ಸಾರ್ವಜನಿಕ:
    AstSScanF(File_ಸಾಲು* fl, ಧ್ರುವ string& text, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->fromp(fromp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_text; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: makes output
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstSScanF* ಧ್ರುವ asamep = VN_DBG_AS(samep, SScanF);
        ಪ್ರತ್ಯಾಗಮ text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_text; }  // * = Text to display
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSampled final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_internal : 1;  // Internally created, not from a source $sampled
ಸಾರ್ವಜನಿಕ:
    AstSampled(File_ಸಾಲು* fl, Ast_ಗಂಟು* exprp, Ast_ಗಂಟು_DType* dtypep, bool internal = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_internal{internal} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_internal == VN_DBG_AS(samep, Sampled)->m_internal;
    }
    bool internal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_internal; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstScopeName final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    ಧ್ರುವ bool m_forFormat;  // Is for a format %m
    ಸ್ಥಿರ std::string scopeNameFormatter(ಧ್ರುವ std::string& text);
    ಸ್ಥಿರ std::string scopePrettyNameFormatter(ಧ್ರುವ std::string& text);

ಸಾರ್ವಜನಿಕ:
    class ForFormat {};
    AstScopeName(File_ಸಾಲು* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstScopeName* ಧ್ರುವ sp = VN_DBG_AS(samep, ScopeName);
        ಪ್ರತ್ಯಾಗಮ m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    // ACCESSORS
    ಧ್ರುವ std::string& scopeAttr() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopeAttr; }
    ಶೂನ್ಯ scopeAttr(ಧ್ರುವ std::string& val) { m_scopeAttr = val; }
    ಧ್ರುವ std::string& scopeEntr() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopeEntr; }
    ಶೂನ್ಯ scopeEntr(ಧ್ರುವ std::string& val) { m_scopeEntr = val; }
    bool dpiExport() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiExport; }
    ಶೂನ್ಯ dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_selfRelative; }
    ಶೂನ್ಯ selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ಸಾರ್ವಜನಿಕ:
    AstStable(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Sen_ವೃಕ್ಷ* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_ರಾಶಿ_Trace_F final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // $stacktrace used as function
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ರಾಶಿ_Trace_F(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ರಾಶಿ_Trace_F(fl) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_Ast_ರಾಶಿ_Trace_F;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$stacktrace"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ verilogKwd(); }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSteady final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstSteady(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstStructSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
ಸಾರ್ವಜನಿಕ:
    AstStructSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override {
        // Not a union
        ಪ್ರತ್ಯಾಗಮ VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstStructSel* ಧ್ರುವ sp = VN_DBG_AS(samep, StructSel);
        ಪ್ರತ್ಯಾಗಮ m_name == sp->m_name;
    }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
};
class AstSysIgnore final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
ಸಾರ್ವಜನಿಕ:
    AstSysIgnore(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$ignored"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Though deleted before opt
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Though deleted before opt
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // Though deleted before opt
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // Though deleted before opt
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstSystemF(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$system"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ verilogKwd(); }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
ಸಾರ್ವಜನಿಕ:
    Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Tagged_ಅಭಿವ್ಯಕ್ತಿ(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_name == VN_DBG_AS(samep, Tagged_ಅಭಿವ್ಯಕ್ತಿ)->m_name;
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
};
class AstTaggedPattern final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
ಸಾರ್ವಜನಿಕ:
    AstTaggedPattern(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* patternp)
        : ASTGEN_SUPER_TaggedPattern(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstTaggedPattern;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_name == VN_DBG_AS(samep, TaggedPattern)->m_name;
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
};
class AstTestPlusArgs final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstTestPlusArgs(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* searchp)
        : ASTGEN_SUPER_TestPlusArgs(fl) {
        this->searchp(searchp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstTestPlusArgs;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$test$plusargs"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ verilogKwd(); }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    // but isPure() true
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstThisRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
ಸಾರ್ವಜನಿಕ:
    AstThisRef(File_ಸಾಲು* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_ಸಾಲು* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "this"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "this"; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
};
class AstTimePrecision final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $timeprecision
ಸಾರ್ವಜನಿಕ:
    explicit AstTimePrecision(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstTimeUnit final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
ಸಾರ್ವಜನಿಕ:
    explicit AstTimeUnit(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    ಧ್ರುವ std::vector<uint32_t> m_indices;  // Indices of the triggers tested
ಸಾರ್ವಜನಿಕ:
    AstTriggered(File_ಸಾಲು* fl, AstVarScope* trigVscp, ಧ್ರುವ std::vector<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    ಧ್ರುವ std::vector<uint32_t>& indices() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_indices; }
};
class AstUnbounded final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
ಸಾರ್ವಜನಿಕ:
    explicit AstUnbounded(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstUnlinkedRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
ಸಾರ್ವಜನಿಕ:
    AstUnlinkedRef(File_ಸಾಲು* fl, Ast_ಗಂಟು* refp, ಧ್ರುವ string& name, Ast_ಗಂಟು* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    ಧ್ರುವ bool m_strong;  // 's_' preffix
    ಧ್ರುವ bool m_overlapping;  // '_with` suffix
ಸಾರ್ವಜನಿಕ:
    AstUntil(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isStrong() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_strong; }
    bool isOverlapping() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_overlapping; }
};
class Ast_ಮೌಲ್ಯ_Plus_Args final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
ಸಾರ್ವಜನಿಕ:
    Ast_ಮೌಲ್ಯ_Plus_Args(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* searchp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outp)
        : ASTGEN_SUPER_ಮೌಲ್ಯ_Plus_Args(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_ಮೌಲ್ಯ_Plus_Args;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$value$plusargs"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ !outp(); }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstWithParse final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
ಗುಪ್ತ:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

ಸಾರ್ವಜನಿಕ:
    AstWithParse(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* funcrefp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp,
                 Ast_ಗಂಟು* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        addExprsp(exprsp);
        addConstraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool restricted() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_restricted; }
    ಶೂನ್ಯ restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
ಸಾರ್ವಜನಿಕ:
    AstBufIf1(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstCastDynamic final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
ಸಾರ್ವಜನಿಕ:
    AstCastDynamic(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtypeSetBitUnsized(32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$cast(%r, %l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 20; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstCompareNN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog str.compare() and str.icompare()
    ಧ್ರುವ bool m_ignoreCase;  // True for str.icompare()
ಸಾರ್ವಜನಿಕ:
    AstCompareNN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        ಪ್ರತ್ಯಾಗಮ m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        ಪ್ರತ್ಯಾಗಮ m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool ignoreCase() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ignoreCase; }
};
class AstConcat final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstConcat(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        ಯದಿ (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetBitOrLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f{%l, %k%r}"; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(concat %l %r)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 2; }
};
class AstConcatN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // String concatenate
ಸಾರ್ವಜನಿಕ:
    AstConcatN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f{%l, %k%r}"; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstDiv final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstDiv(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f/ %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvudiv %l %r)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_DIV; }
};
class AstDivD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstDivD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstDivD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "/"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL_DIV; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstDivS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstDivS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f/ %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvsdiv %l %r)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_DIV; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstEqWild final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstEqWild(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    ಸ್ಥಿರ Ast_ಗಂಟು_Biop* newTyped(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp);
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f==? %r)"; }
    string emitC() override {
        ಯದಿ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        ಯದಿ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ "==";
        }
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstFGetS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
ಸಾರ್ವಜನಿಕ:
    AstFGetS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$fgets(%l,%r)"; }
    string emitC() override {
        ಪ್ರತ್ಯಾಗಮ strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 64; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstFUngetC(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        ಪ್ರತ್ಯಾಗಮ "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 64; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ಗಂಟು* filep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ lhsp(); }
    Ast_ಗಂಟು* charp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ rhsp(); }
};
class AstGetcN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog string.getc()
ಸಾರ್ವಜನಿಕ:
    AstGetcN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ "getc"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l.getc(%r))"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstGetcRefN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
ಸಾರ್ವಜನಿಕ:
    AstGetcRefN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstGt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstGt(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f> %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ">"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstGtD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstGtD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ">"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstGtN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstGtN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ">"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstGtS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstGtS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f> %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstGte final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstGte(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f>= %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ">="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstGteD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstGteD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ">="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstGteN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstGteN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ">="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstGteS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstGteS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f>= %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstLogAnd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLogAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f&& %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvand %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "&&"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstLogIf final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLogIf(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f-> %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "->"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstLogOr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLogOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f|| %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvor %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "||"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstLt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLt(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f< %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "<"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstLtD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstLtD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "<"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstLtN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstLtN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "<"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstLtS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLtS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f< %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstLte final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLte(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f<= %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "<="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstLteD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstLteD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "<="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstLteN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstLteN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "<="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstLteS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLteS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f<= %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstModDiv final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstModDiv(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f%% %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvurem %l %r)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_DIV; }
};
class AstModDivS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstModDivS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f%% %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvsmod %l %r)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_DIV; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstNeqWild final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNeqWild(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f!=? %r)"; }
    string emitC() override {
        ಯದಿ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        ಯದಿ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ "!=";
        }
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstPow final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstPow(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f** %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_MUL * 10; }
};
class AstPowD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstPowD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstPowD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f** %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "pow(%li,%ri)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL_DIV * 5; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstPowSS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstPowSS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f** %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_MUL * 10; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstPowSU final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstPowSU(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f** %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_MUL * 10; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstPowUS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstPowUS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f** %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_MUL * 10; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstReplicate final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
ಸಾರ್ವಜನಿಕ:
    AstReplicate(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        ಯದಿ (lhsp) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(rhsp, Const)) {
                ಯದಿ (constp->ಅಂಕೆ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ಅಂಕೆ().isNegative())) {
                    dtypeSetBitOrLogicSized(lhsp->width(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } ಅನ್ಯಥಾ {
                    dtypeSetBitOrLogicSized(lhsp->width() * constp->toSInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, uint32_t rep_ಎಣಿಕೆ)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_ಎಣಿಕೆ}} {}
    ASTGEN_MEMBERS_AstReplicate;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f{%r{%k%l}}"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ "((_ repeat " + cvtToStr(width() / lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 2; }
};
class AstReplicateN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // String replicate
ಸಾರ್ವಜನಿಕ:
    AstReplicateN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f{%r{%k%l}}"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 2; }
    bool stringFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSAnd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    ಧ್ರುವ bool m_propertyControl;  // Parser-generated property if/case branch conjunction
ಸಾರ್ವಜನಿಕ:
    AstSAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {  // LCOV_EXCL_LINE
        ಪ್ರತ್ಯಾಗಮ m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_propertyControl; }
};
class AstSIntersect final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
ಸಾರ್ವಜನಿಕ:
    AstSIntersect(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSOr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
ಸಾರ್ವಜನಿಕ:
    AstSOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSOr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSThroughout final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
ಸಾರ್ವಜನಿಕ:
    AstSThroughout(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSWithin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
ಸಾರ್ವಜನಿಕ:
    AstSWithin(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    ಪೂರ್ಣಾಙ್ಕ m_declElWidth;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    ಪೂರ್ಣಾಙ್ಕ m_widthConst;

ಸಾರ್ವಜನಿಕ:
    AstSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp, ಪೂರ್ಣಾಙ್ಕ bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicSized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ bitwidth, ಪೂರ್ಣಾಙ್ಕ bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicUnsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& from, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& bit) override {
        out.opSel(from, bit.toUInt() + widthConst() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        ಪ್ರತ್ಯಾಗಮ widthConst() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "((_ extract %t %r) %l)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* otherp) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ widthConst() == VN_DBG_AS(otherp, Sel)->widthConst();
    }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    ಪೂರ್ಣಾಙ್ಕ widthConst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_widthConst; }
    ಶೂನ್ಯ widthConst(ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) { m_widthConst = ಮೌಲ್ಯ; }
    ಪೂರ್ಣಾಙ್ಕ lsbConst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_AS(lsbp(), Const)->toSInt(); }
    ಪೂರ್ಣಾಙ್ಕ msbConst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ lsbConst() + widthConst() - 1; }
    VNumRange& declRange() VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_declRange; }
    ಧ್ರುವ VNumRange& declRange() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_declRange; }
    ಶೂನ್ಯ declRange(ಧ್ರುವ VNumRange& flag) { m_declRange = flag; }
    ಪೂರ್ಣಾಙ್ಕ declElWidth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_declElWidth; }
    ಶೂನ್ಯ declElWidth(ಪೂರ್ಣಾಙ್ಕ flag) { m_declElWidth = flag; }
};
class AstShiftL final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstShiftL(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಙ್ಕ setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        ಯದಿ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftL;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f<< %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        ಪ್ರತ್ಯಾಗಮ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstShiftLOvr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
ಸಾರ್ವಜನಿಕ:
    AstShiftLOvr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಙ್ಕ setwidth = 0)
        : ASTGEN_SUPER_ShiftLOvr(fl, lhsp, rhsp) {
        ಯದಿ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftLOvr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f<< %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstShiftR final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstShiftR(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಙ್ಕ setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        ಯದಿ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftR;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f>> %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        ಪ್ರತ್ಯಾಗಮ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstShiftROvr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
ಸಾರ್ವಜನಿಕ:
    AstShiftROvr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಙ್ಕ setwidth = 0)
        : ASTGEN_SUPER_ShiftROvr(fl, lhsp, rhsp) {
        ಯದಿ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftROvr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f>> %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstShiftRS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstShiftRS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಙ್ಕ setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        ಯದಿ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f>>> %r)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvashr %l %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstShiftRSOvr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
ಸಾರ್ವಜನಿಕ:
    AstShiftRSOvr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಙ್ಕ setwidth = 0)
        : ASTGEN_SUPER_ShiftRSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        ಯದಿ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRSOvr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f>>> %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSub final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstSub(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f- %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvsub %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "-"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstSubD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstSubD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstSubD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "-"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstURandomRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // $urandom_range
ಸಾರ್ವಜನಿಕ:
    AstURandomRange(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$urandom_range(%l, %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstEq(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    ಸ್ಥಿರ Ast_ಗಂಟು_Biop* newTyped(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp);
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f== %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "=="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstEqCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstEqCase(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f=== %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "=="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstEqD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
ಸಾರ್ವಜನಿಕ:
    AstEqD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "=="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstEqN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
ಸಾರ್ವಜನಿಕ:
    AstEqN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "=="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstEqT final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // Equal (==) for data types
ಸಾರ್ವಜನಿಕ:
    AstEqT(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqT;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "=="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
};
class AstLogEq final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLogEq(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f<-> %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "<->"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstNeq final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNeq(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    ಸ್ಥಿರ Ast_ಗಂಟು_Biop* newTyped(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp);
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f!= %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "!="; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (not (= %l %r)))"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstNeqCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNeqCase(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f!== %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "!="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstNeqD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
ಸಾರ್ವಜನಿಕ:
    AstNeqD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "!="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstNeqN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
ಸಾರ್ವಜನಿಕ:
    AstNeqN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "!="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstNeqT final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // Not-equal (!=) for data types
ಸಾರ್ವಜನಿಕ:
    AstNeqT(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "!="; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_STR; }
};

// === AstNodeBiComAsv ===
class AstAdd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstAdd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAdd;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f+ %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvadd %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "+"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstAddD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
ಸಾರ್ವಜನಿಕ:
    AstAddD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstAddD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "+"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstAnd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f& %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvand %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "&"; }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಧ್ರುವ ವರ್ಣ* widthMismatch() ಧ್ರುವ override VL_MT_STABLE;
};
class AstMul final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstMul(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f* %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "*"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_MUL; }
};
class AstMulD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
ಸಾರ್ವಜನಿಕ:
    AstMulD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstMulD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "*"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstMulS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstMulS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f* %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * INSTR_ಎಣಿಕೆ_INT_MUL; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstOr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f| %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvor %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "|"; }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಧ್ರುವ ವರ್ಣ* widthMismatch() ಧ್ರುವ override VL_MT_STABLE;
};
class AstXor final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstXor(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f^ %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvxor %l %r)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "^"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Lclean && Rclean
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಧ್ರುವ ವರ್ಣ* widthMismatch() ಧ್ರುವ override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class AstDistChiSquare final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Biop {
ಸಾರ್ವಜನಿಕ:
    AstDistChiSquare(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DistChiSquare(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistChiSquare;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class AstDistExponential final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Biop {
ಸಾರ್ವಜನಿಕ:
    AstDistExponential(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DistExponential(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistExponential;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$dist_exponential(%l, %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class AstDistPoisson final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Biop {
ಸಾರ್ವಜನಿಕ:
    AstDistPoisson(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DistPoisson(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistPoisson;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$dist_poisson(%l, %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Biop {
ಸಾರ್ವಜನಿಕ:
    AstDistT(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$dist_t(%l, %r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Sel {
    // @astgen makeDfgVertex
    ಶೂನ್ಯ init(ಧ್ರುವ Ast_ಗಂಟು* fromp) {
        ಯದಿ (fromp && VN_IS(fromp->dtypep()->skipRefp(), ಗಂಟು_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), ಗಂಟು_Array_DType)->subDTypep());
        }
    }

ಸಾರ್ವಜನಿಕ:
    AstArraySel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಪೂರ್ಣಾಙ್ಕ bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(select %l %r)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
};
class AstAssocSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Sel {
    ಶೂನ್ಯ init(ಧ್ರುವ Ast_ಗಂಟು* fromp) {
        ಯದಿ (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

ಸಾರ್ವಜನಿಕ:
    AstAssocSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l%f[%r])"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "%li%k[%ri]"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(select %l %r)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isGateOptimizable() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ !isLValue(); }  // AssocSel creates on miss
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
};
class AstWildcardSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Sel {
    ಶೂನ್ಯ init(ಧ್ರುವ Ast_ಗಂಟು* fromp) {
        ಯದಿ (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

ಸಾರ್ವಜನಿಕ:
    AstWildcardSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l%f[%r])"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "%li%k[%ri]"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
};
class AstWordSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Sel {
    // Select a single word from a multi-word wide value
ಸಾರ್ವಜನಿಕ:
    AstWordSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};

// === AstNodeStream ===
class AstStreamL final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstStreamL(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamL;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f{ << %r %k{%l} }"; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 2; }
};
class AstStreamR final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstStreamR(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamR;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f{ >> %r %k{%l} }"; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Biop_D {
ಸಾರ್ವಜನಿಕ:
    AstAtan2D(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$atan2(%l,%r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "atan2(%li,%ri)"; }
};
class AstHypotD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Biop_D {
ಸಾರ್ವಜನಿಕ:
    AstHypotD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$hypot(%l,%r)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
ಸಾರ್ವಜನಿಕ:
    AstCCall(File_ಸಾಲು* fl, AstCFunc* funcp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    ಧ್ರುವ VSelfPointerText& selfPointer() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_selfPointer; }
    ಶೂನ್ಯ selfPointer(ಧ್ರುವ VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ selfPointer().protect(useSelfForThis, protect());
    }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCMethodCall final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstCMethodCall(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, AstCFunc* funcp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_CCall {
    // C++ new() call
ಸಾರ್ವಜನಿಕ:
    AstCNew(File_ಸಾಲು* fl, AstCFunc* funcp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

ಸಾರ್ವಜನಿಕ:
    AstConsArray(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* newDTypep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಶೂನ್ಯ cloneRelink() override;
    ಅನ್ತಃಸ್ಥ AstConsArrayItem* itemsp() ಧ್ರುವ VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep(uint64_t index) ಧ್ರುವ;
    // Add the value of the element at the index, which must not be given yet
    ಶೂನ್ಯ valuep(uint64_t index, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp);
    // Add the elements of the array 'arrayp', starting at the index
    ಶೂನ್ಯ splice_ಮೌಲ್ಯಗಳು(uint64_t index, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arrayp);
};
class AstConsAssoc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
ಸಾರ್ವಜನಿಕ:
    explicit AstConsAssoc(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};
class AstConsDynArray final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
ಸಾರ್ವಜನಿಕ:
    explicit AstConsDynArray(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
};
class AstConsQueue final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
ಸಾರ್ವಜನಿಕ:
    explicit AstConsQueue(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};
class AstConsStruct final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
ಸಾರ್ವಜನಿಕ:
    AstConsStruct(File_ಸಾಲು* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    // The value of the member called 'name'
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* memberValuep(ಧ್ರುವ string& name) ಧ್ರುವ;
};
class AstConsWildcard final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
ಸಾರ್ವಜನಿಕ:
    explicit AstConsWildcard(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask_Ref {
    // A reference to a function
ಸಾರ್ವಜನಿಕ:
    ಅನ್ತಃಸ್ಥ AstFuncRef(File_ಸಾಲು* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class AstMethodCall final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
ಸಾರ್ವಜನಿಕ:
    AstMethodCall(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, VFlagChildDType, ಧ್ರುವ string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMethodCall(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಧ್ರುವ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_AstMethodCall;
};
class AstNew final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
ಸಾರ್ವಜನಿಕ:
    AstNew(File_ಸಾಲು* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool isImplicit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isImplicit; }
    ಶೂನ್ಯ isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isScoped; }
    ಶೂನ್ಯ isScoped(bool flag) { m_isScoped = flag; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstTaskRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask_Ref {
    // A reference to a task
ಸಾರ್ವಜನಿಕ:
    ಅನ್ತಃಸ್ಥ AstTaskRef(File_ಸಾಲು* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtypeSetVoid();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
ಗುಪ್ತ:
    VAccess m_access;  // Left hand side assignment
ಸಾರ್ವಜನಿಕ:
    AstSelBit(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    VAccess access() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_access; }
    ಶೂನ್ಯ access(ಧ್ರುವ VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
ಸಾರ್ವಜನಿಕ:
    AstSelExtract(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* msbp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
ಸಾರ್ವಜನಿಕ:
    AstSelMinus(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
ಸಾರ್ವಜನಿಕ:
    AstSelPlus(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_ಎಣಿಕೆ_Bits final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Quadop {
    // Number of bits set in vector
ಸಾರ್ವಜನಿಕ:
    Ast_ಎಣಿಕೆ_Bits(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl1p)
        : ASTGEN_SUPER_ಎಣಿಕೆ_Bits(fl, exprp, ctrl1p, ctrl1p->clone_ವೃಕ್ಷ_Pure(false),
                                 ctrl1p->clone_ವೃಕ್ಷ_Pure(false)) {
        dtypeSetInt();
    }
    Ast_ಎಣಿಕೆ_Bits(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl1p, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl2p)
        : ASTGEN_SUPER_ಎಣಿಕೆ_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_ವೃಕ್ಷ_Pure(false)) {
        dtypeSetInt();
    }
    Ast_ಎಣಿಕೆ_Bits(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl1p, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl2p,
                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl3p)
        : ASTGEN_SUPER_ಎಣಿಕೆ_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_ಎಣಿಕೆ_Bits;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಭಿವ್ಯಕ್ತಿ, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl1,
                       ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl2, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl3) override {
        out.op_ಎಣಿಕೆ_Bits(ಅಭಿವ್ಯಕ್ತಿ, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanFhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersFhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Termop {
ಸಾರ್ವಜನಿಕ:
    explicit AstInferredDisable(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstTime final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Termop {
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    AstTime(File_ಸಾಲು* fl, ಧ್ರುವ VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_TIME; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Termop {
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    AstTimeD(File_ಸಾಲು* fl, ಧ್ರುವ VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_TIME; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstCond(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thenp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* elsep);
    ASTGEN_MEMBERS_AstCond;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs,
                       ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths) override {
        out.opAssign(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l %f? %r %k: %t)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(ite (__Vbool %l) %r %t)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // clean if e1 & e2 clean
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Propagates up
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstPutcN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
    // Verilog string.putc()
ಸಾರ್ವಜನಿಕ:
    AstPutcN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstPutcN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs,
                       ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ "putc"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l.putc(%r,%t))"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstSliceSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
ಸಾರ್ವಜನಿಕ:
    AstSliceSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಧ್ರುವ VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_AstSliceSel;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& from, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lo,
                       ಧ್ರುವ V3_ಸಂಖ್ಯೆ& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_declRange; }
    ಧ್ರುವ VNumRange& declRange() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_declRange; }
};
class AstSubstrN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
    // Verilog string.substr()
ಸಾರ್ವಜನಿಕ:
    AstSubstrN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs,
                       ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ "substr"; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l.substr(%r,%t))"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ ""; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersRhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersThs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Triop {
ಸಾರ್ವಜನಿಕ:
    AstDistErlang(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class AstDistNormal final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Triop {
ಸಾರ್ವಜನಿಕ:
    AstDistNormal(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : ASTGEN_SUPER_DistNormal(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistNormal;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class AstDistUniform final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Triop {
ಸಾರ್ವಜನಿಕ:
    AstDistUniform(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : ASTGEN_SUPER_DistUniform(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistUniform;
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ FmtType : ಪೂರ್ಣಾಙ್ಕ { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

ಗುಪ್ತ:
    // dist-ast-dump-suppress  // Part of name()
    ಧ್ರುವ FmtType m_fmtType;  // Operation type
ಸಾರ್ವಜನಿಕ:
    AstAtoN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtypeSetDouble() : dtypeSetInteger();
    }
    ASTGEN_MEMBERS_AstAtoN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opAtoN(lhs, m_fmtType); }
    string name() ಧ್ರುವ override VL_MT_STABLE {
        ವಿಕಲ್ಪ (m_fmtType) {
        ಪಕ್ಷ ATOI: ಪ್ರತ್ಯಾಗಮ "atoi";
        ಪಕ್ಷ ATOHEX: ಪ್ರತ್ಯಾಗಮ "atohex";
        ಪಕ್ಷ ATOOCT: ಪ್ರತ್ಯಾಗಮ "atooct";
        ಪಕ್ಷ ATOBIN: ಪ್ರತ್ಯಾಗಮ "atobin";
        ಪಕ್ಷ ATOREAL: ಪ್ರತ್ಯಾಗಮ "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l." + name() + "()"; }
    string emitC() override {
        ವಿಕಲ್ಪ (m_fmtType) {
        ಪಕ್ಷ ATOI: ಪ್ರತ್ಯಾಗಮ "VL_ATOI_N(%li, 10)";
        ಪಕ್ಷ ATOHEX: ಪ್ರತ್ಯಾಗಮ "VL_ATOI_N(%li, 16)";
        ಪಕ್ಷ ATOOCT: ಪ್ರತ್ಯಾಗಮ "VL_ATOI_N(%li, 8)";
        ಪಕ್ಷ ATOBIN: ಪ್ರತ್ಯಾಗಮ "VL_ATOI_N(%li, 2)";
        ಪಕ್ಷ ATOREAL: ಪ್ರತ್ಯಾಗಮ "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    FmtType fmtType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fmtType; }
};
class AstBitsToRealD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstBitsToRealD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstBitsToRealD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$bitstoreal(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_CVT_D_Q(%li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Eliminated before matters
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Eliminated before matters
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCCast final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Cast to C-based data type
    ಪೂರ್ಣಾಙ್ಕ m_size;

ಸಾರ್ವಜನಿಕ:
    AstCCast(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಪೂರ್ಣಾಙ್ಕ setwidth, ಪೂರ್ಣಾಙ್ಕ minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_size = setwidth;
        ಯದಿ (setwidth) {
            ಯದಿ (minwidth == -1) minwidth = setwidth;
            dtypeSetBitUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_size = width();
    }
    ASTGEN_MEMBERS_AstCCast;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$_CAST(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Special cased in V3Cast
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ size() == VN_DBG_AS(samep, CCast)->size();
    }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    //
    ಪೂರ್ಣಾಙ್ಕ size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_size; }
};
class AstCLog2 final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstCLog2(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$clog2(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 16; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCastWrap final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
ಸಾರ್ವಜನಿಕ:
    AstCastWrap(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
};
class Ast_ಎಣಿಕೆ_Ones final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    Ast_ಎಣಿಕೆ_Ones(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ಎಣಿಕೆ_Ones(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_ಎಣಿಕೆ_Ones;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.op_ಎಣಿಕೆ_Ones(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$countones(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 16; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCvtPackString final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
ಸಾರ್ವಜನಿಕ:
    AstCvtPackString(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstExtend final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstExtend(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetBitOrLogicUnsized(width, widthMin ? widthMin : width, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ "((_ zero_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ false;  // Because the EXTEND operator self-casts
    }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
};
class AstExtendS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstExtendS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಪೂರ್ಣಾಙ್ಕ width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ "((_ sign_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ false;  // Because the EXTEND operator self-casts
    }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool signedFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstFEof final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstFEof(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$feof(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 16; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು* filep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ lhsp(); }
};
class AstFGetC final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstFGetC(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 64; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ಗಂಟು* filep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ lhsp(); }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstISToRD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // $itor where lhs is signed
ಸಾರ್ವಜನಿಕ:
    AstISToRD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstISToRD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$itor($signed(%l))"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstIToRD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // $itor where lhs is unsigned
ಸಾರ್ವಜನಿಕ:
    AstIToRD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstIToRD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$itor(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstIsUnbounded final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // True if is unbounded ($)
ಸಾರ್ವಜನಿಕ:
    AstIsUnbounded(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstIsUnknown final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // True if any unknown bits
ಸಾರ್ವಜನಿಕ:
    AstIsUnknown(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstLenN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Length of a string
ಸಾರ್ವಜನಿಕ:
    AstLenN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstLenN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LEN_IN(%li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstLogNot final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಗುಪ್ತ:
    ಧ್ರುವ bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
ಸಾರ್ವಜನಿಕ:
    AstLogNot(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f(! %l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "!"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool fromProperty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fromProperty; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class AstMostSetBitP1 final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
ಸಾರ್ವಜನಿಕ:
    AstMostSetBitP1(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_MostSetBitP1(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstMostSetBitP1;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opMostSetBitP1(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$mostsetbitp1(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 16; }
};
class AstNToI final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // String to any-size integral
ಸಾರ್ವಜನಿಕ:
    AstNToI(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opNToI(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "'(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstNegate final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNegate(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f(- %l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvneg %l)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "-"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstNegateD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstNegateD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstNegateD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "-"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstNot final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNot(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f(~ %l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "(bvnot %l)"; }
    string emitSimpleOperator() override { ಪ್ರತ್ಯಾಗಮ "~"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಧ್ರುವ ವರ್ಣ* widthMismatch() ಧ್ರುವ override VL_MT_STABLE;
};
class AstNullCheck final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Return LHS after checking that LHS is non-null
ಸಾರ್ವಜನಿಕ:
    AstNullCheck(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }  // Rarely executes
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ fileline() == samep->fileline(); }
};
class AstOneHot final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstOneHot(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$onehot(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 4; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstOneHot0 final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstOneHot0(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$onehot0(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs() * 3; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstPostDec final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
ಸಾರ್ವಜನಿಕ:
    AstPostDec(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
ಸಾರ್ವಜನಿಕ:
    AstPostInc(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
ಸಾರ್ವಜನಿಕ:
    AstPreDec(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
ಸಾರ್ವಜನಿಕ:
    AstPreInc(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ಧ್ರುವ override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
ಸಾರ್ವಜನಿಕ:
    AstRToIRoundS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        ಪ್ರತ್ಯಾಗಮ isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstRToIS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // $rtoi(lhs)
ಸಾರ್ವಜನಿಕ:
    AstRToIS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$rtoi(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_RTOI_I_D(%li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Eliminated before matters
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Eliminated before matters
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstRealToBits final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstRealToBits(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstRealToBits;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$realtobits(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_CVT_Q_D(%li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Eliminated before matters
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Eliminated before matters
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstRedAnd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstRedAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f(& %l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstRedOr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstRedOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f(| %l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstRedXor final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstRedXor(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f(^ %l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ w = lhsp()->width();
        ಪ್ರತ್ಯಾಗಮ (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1 + V3_ಸಂಖ್ಯೆ::log2b(width()); }
};
class AstResizeLValue final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
ಸಾರ್ವಜನಿಕ:
    AstResizeLValue(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
};
class AstSigned final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // $signed(lhs)
ಸಾರ್ವಜನಿಕ:
    AstSigned(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Eliminated before matters
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }  // Eliminated before matters
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstTimeImport final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    AstTimeImport(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }
};
class AstToLowerN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // string.tolower()
ಸಾರ್ವಜನಿಕ:
    AstToLowerN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToLowerN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l.tolower()"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstToStringN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstToStringN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ToStringN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToStringN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        ಪ್ರತ್ಯಾಗಮ isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstToUpperN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // string.toupper()
ಸಾರ್ವಜನಿಕ:
    AstToUpperN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToUpperN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%l.toupper()"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstUnsigned final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // $unsigned(lhs)
ಸಾರ್ವಜನಿಕ:
    AstUnsigned(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool cleanLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }  // Eliminated before matters
    bool sizeMattersLhs() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }  // Eliminated before matters
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool isSystemFunc() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAcosD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$acos(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "acos(%li)"; }
};
class AstAcoshD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAcoshD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$acosh(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "acosh(%li)"; }
};
class AstAsinD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAsinD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$asin(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "asin(%li)"; }
};
class AstAsinhD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAsinhD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$asinh(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "asinh(%li)"; }
};
class AstAtanD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAtanD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$atan(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "atan(%li)"; }
};
class AstAtanhD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAtanhD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$atanh(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "atanh(%li)"; }
};
class AstCeilD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstCeilD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::ceil(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$ceil(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "ceil(%li)"; }
};
class AstCosD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstCosD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$cos(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "cos(%li)"; }
};
class AstCoshD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstCoshD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$cosh(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "cosh(%li)"; }
};
class AstExpD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstExpD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$exp(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "exp(%li)"; }
};
class AstFloorD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstFloorD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::floor(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$floor(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "floor(%li)"; }
};
class AstLog10D final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstLog10D(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$log10(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "log10(%li)"; }
};
class AstLogD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstLogD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$ln(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "log(%li)"; }
};
class AstSinD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstSinD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$sin(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "sin(%li)"; }
};
class AstSinhD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstSinhD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$sinh(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "sinh(%li)"; }
};
class AstSqrtD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstSqrtD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::sqrt(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$sqrt(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "sqrt(%li)"; }
};
class AstTanD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstTanD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$tan(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "tan(%li)"; }
};
class AstTanhD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstTanhD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_Operate(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { ಪ್ರತ್ಯಾಗಮ "%f$tanh(%l)"; }
    string emitC() override { ಪ್ರತ್ಯಾಗಮ "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
ಸಾರ್ವಜನಿಕ:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    ಅನ್ತಃಸ್ಥ AstVarRef(File_ಸಾಲು* fl, AstVar* varp, ಧ್ರುವ VAccess& access);
    ಅನ್ತಃಸ್ಥ AstVarRef(File_ಸಾಲು* fl, Ast_ಗಂಟು_Module* classOrPackagep, AstVar* varp,
                     ಧ್ರುವ VAccess& access);
    // This form only allowed post-link (see above)
    ಅನ್ತಃಸ್ಥ AstVarRef(File_ಸಾಲು* fl, AstVarScope* varscp, ಧ್ರುವ VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಅನ್ತಃಸ್ಥ string name() ಧ್ರುವ override;  // * = Var name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    ಅನ್ತಃಸ್ಥ bool same_ಗಂಟು(ಧ್ರುವ AstVarRef* samep) ಧ್ರುವ;
    ಅನ್ತಃಸ್ಥ bool sameNoLvalue(ಧ್ರುವ AstVarRef* samep) ಧ್ರುವ;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstVarXRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
ಸಾರ್ವಜನಿಕ:
    AstVarXRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& dotted, ಧ್ರುವ VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    ಅನ್ತಃಸ್ಥ AstVarXRef(File_ಸಾಲು* fl, AstVar* varp, ಧ್ರುವ string& dotted, ಧ್ರುವ VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    ಶೂನ್ಯ name(ಧ್ರುವ std::string& name) override { m_name = name; }  // * = Var name
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string dotted() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dotted; }
    ಶೂನ್ಯ dotted(ಧ್ರುವ string& dotted) { m_dotted = dotted; }
    string inlinedDots() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inlinedDots; }
    ಶೂನ್ಯ inlinedDots(ಧ್ರುವ string& flag) { m_inlinedDots = flag; }
    bool containsGenBlock() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_containsGenBlock; }
    ಶೂನ್ಯ containsGenBlock(ಧ್ರುವ bool flag) { m_containsGenBlock = flag; }
    bool readOnlyModport() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_readOnlyModport; }
    ಶೂನ್ಯ readOnlyModport(ಧ್ರುವ bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ widthInstrs(); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        ಪ್ರತ್ಯಾಗಮ (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#endif  // Guard
