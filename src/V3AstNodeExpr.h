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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ASTNODEEXPR_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ASTNODEEXPR_H_

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3AST_H_
#ದೋಷ "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ವ್ಯಾಖ್ಯಾನಿಸು VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ಆದರೆ_ಅಂತ್ಯ

// === Abstract base node types (AstNode*) =====================================

class Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // An expression tree node
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    // Every expression must have a data type after V3Width
    bool hasDType() ಸ್ಥಿರಾಂಕ override final VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ವಾಸ್ತವಿಕ string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    ವಾಸ್ತವಿಕ string emitC() = 0;
    ವಾಸ್ತವಿಕ string emitSMT() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; };
    ವಾಸ್ತವಿಕ string emitSimpleOperator() { ಹಿಂತಿರುಗಿ ""; }  // "" means not ok to use
    ವಾಸ್ತವಿಕ bool emit_Check_Max_ಪದಗಳು() { ಹಿಂತಿರುಗಿ false; }  // Check VL_MULS_MAX_WORDS
    ವಾಸ್ತವಿಕ bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    ವಾಸ್ತವಿಕ bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* get_VAccess_ಗುರಿ_Recurse() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* get_VAccess_ಗುರಿ_Recurse() {
        ಹಿಂತಿರುಗಿ const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(this)->get_VAccess_ಗುರಿ_Recurse());
    }
    VAccess getVAccessRecurse() ಸ್ಥಿರಾಂಕ;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ;
    // Return base var (or const) nodep dereferences
    Ast_ಗಂಟು* ತಳ_Fromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    ಸಾಲೊಳಗೆ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    ವಾಸ್ತವಿಕ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಅಗಲ_Mismatch() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string ನಮೂನೆ_String(uint32_t ಆಳ = 0) ಸ್ಥಿರಾಂಕ;
    bool isFourstateOrNoDtype() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!dtypep()) ಹಿಂತಿರುಗಿ true;
        ಹಿಂತಿರುಗಿ dtypep()->skipRefp()->isFourstate();
    }
    ಸ್ಥಿರ bool isFourstateOrNoDtype(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p) {
        ಹಿಂತಿರುಗಿ expr1p->isFourstateOrNoDtype();
    }
    ಸ್ಥಿರ bool isFourstateOrNoDtype(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p,
                                     ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr2p) {
        ಹಿಂತಿರುಗಿ expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    ಸ್ಥಿರ bool isFourstateOrNoDtype(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p,
                                     ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr2p,
                                     ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr3p) {
        ಹಿಂತಿರುಗಿ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    ಸ್ಥಿರ bool isFourstateOrNoDtype(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr1p,
                                     ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr2p,
                                     ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr3p,
                                     ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ expr4p) {
        ಹಿಂತಿರುಗಿ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_ಗಂಟು_Biop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Biop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) = 0;
    ವಾಸ್ತವಿಕ bool cleanLhs() ಸ್ಥಿರಾಂಕ = 0;  // True if LHS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool cleanRhs() ಸ್ಥಿರಾಂಕ = 0;  // True if RHS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on lhs size
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on rhs size
    ವಾಸ್ತವಿಕ bool doubleFlavor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    ವಾಸ್ತವಿಕ bool signedFlavor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    ವಾಸ್ತವಿಕ bool stringFlavor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }  // N flavor of nodes with both flavors?
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;

ಖಾಸಗಿ:
    bool getPurityRecurse() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ lhsp()->is_ಶುದ್ಧ() && rhsp()->is_ಶುದ್ಧ(); }
};
class Ast_ಗಂಟು_Bi_Com VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Binary expr with commutative properties
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Bi_Com(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhs, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhs)
        : Ast_ಗಂಟು_Biop{t, fl, lhs, rhs} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Bi_Com;
};
class Ast_ಗಂಟು_Bi_Com_Asv VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // Binary expr with commutative & associative properties
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Bi_Com_Asv(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhs, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhs)
        : Ast_ಗಂಟು_Bi_Com{t, fl, lhs, rhs} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Bi_Com_Asv;
};
class Ast_ಗಂಟು_Dist_Biop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Dist_Biop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : Ast_ಗಂಟು_Biop{t, fl, lhsp, rhsp} {
        dtype_ಗಣ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Dist_Biop;
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL_TRIG; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
};
class Ast_ಗಂಟು_Sel VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Sel(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : Ast_ಗಂಟು_Biop{t, fl, fromp, bitp} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Sel;
    ಪೂರ್ಣಾಂಕ ಬಿಟ್_Const() ಸ್ಥಿರಾಂಕ;
};
class Ast_ಗಂಟು_ಹರಿವು VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_ಹರಿವು(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : Ast_ಗಂಟು_Biop{t, fl, lhsp, rhsp} {
        ಆದರೆ (lhsp->dtypep()) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(lhsp->ಅಗಲ(), lhsp->ಅಗಲ_Min(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_ಹರಿವು;
};
class Ast_ಗಂಟು_System_Biop_D VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_System_Biop_D(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : Ast_ಗಂಟು_Biop{t, fl, lhsp, rhsp} {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_System_Biop_D;
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL_TRIG; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಗಂಟು_CCall VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_CCall(VNType t, File_ಸಾಲು* fl, AstCFunc* funcp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl}
        , m_funcp{funcp} {
        ಕೂಡಿಸು_Argsp(argsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_CCall;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_CALL; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_CCall* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ಗಂಟು_CCall);
        ಹಿಂತಿರುಗಿ (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override;
    bool isOutputter() override { ಹಿಂತಿರುಗಿ !is_ಶುದ್ಧ(); }
    AstCFunc* funcp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_funcp; }
    ಶೂನ್ಯ funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_argTypes; }
    ಶೂನ್ಯ argTypes(ಸ್ಥಿರಾಂಕ string& str) { m_argTypes = str; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ final override { ಹಿಂತಿರುಗಿ true; }
    bool superReference() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_superReference; }
    ಶೂನ್ಯ superReference(bool flag) { m_superReference = flag; }
};
class Ast_ಗಂಟು_Cons VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Construct a value of an aggregate type
    ಮಿತ್ರ class Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ;  // For m_knownConstantTree
    mutable bool m_ತಿಳಿದ_Constant_ವೃಕ್ಷ = false;  // Known to be 'this->isConstantTree() == true'

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Cons(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Cons;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ final override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ final override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
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
ಖಾಸಗಿ:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_FTask_Ref(VNType t, File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstArg* argsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl}
        , m_name{name} {
        ಕೂಡಿಸು_Argsp(argsp);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override;
    string dotted() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dotted; }  // * = Scope name or ""
    string inlinedDots() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_inlinedDots; }
    ಶೂನ್ಯ inlinedDots(ಸ್ಥಿರಾಂಕ string& flag) { m_inlinedDots = flag; }
    Ast_ಗಂಟು_FTask* taskp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_taskp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ taskp(Ast_ಗಂಟು_FTask* taskp) { m_taskp = taskp; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    ಶೂನ್ಯ dotted(ಸ್ಥಿರಾಂಕ string& name) { m_dotted = name; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
    bool ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ; }
    ಶೂನ್ಯ ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ(ಸ್ಥಿರಾಂಕ bool flag) { m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ = flag; }
    bool pli() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pli; }
    ಶೂನ್ಯ pli(bool flag) { m_pli = flag; }
    bool superReference() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_superReference; }
    ಶೂನ್ಯ superReference(bool flag) { m_superReference = flag; }
    bool is_ಶುದ್ಧ() override;
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_taskp && m_taskp->isUnlikely(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ಗಂಟು_FTask_Ref);
        ಹಿಂತಿರುಗಿ taskp() == asamep->taskp()  //
               && classOrPackagep() == asamep->classOrPackagep()  //
               && name() == asamep->name()  //
               && dotted() == asamep->dotted()  //
               && inlinedDots() == asamep->inlinedDots()  //
               && pli() == asamep->pli()  //
               && ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ() == asamep->ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ()
               && superReference() == asamep->superReference();
    }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ final override { V3ERROR_NA_RETURN(true); }

ಖಾಸಗಿ:
    bool getPurityRecurse() ಸ್ಥಿರಾಂಕ;
};
class Ast_ಗಂಟು_Pre_Sel VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Pre_Sel(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Pre_Sel;
    // METHODS
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ final override { V3ERROR_NA_RETURN(true); }
    bool is_ಶುದ್ಧ() override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;

ಖಾಸಗಿ:
    bool getPurityRecurse() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ fromp()->is_ಶುದ್ಧ() && rhsp()->is_ಶುದ್ಧ() && (!thsp() || thsp()->is_ಶುದ್ಧ());
    }
};
class Ast_ಗಂಟು_Quadop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ಸಂರಕ್ಷಿತ:
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
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs,
                               ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& fhs)
        = 0;
    ವಾಸ್ತವಿಕ bool cleanLhs() ಸ್ಥಿರಾಂಕ = 0;  // True if LHS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool cleanRhs() ಸ್ಥಿರಾಂಕ = 0;  // True if RHS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool cleanThs() ಸ್ಥಿರಾಂಕ = 0;  // True if THS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool cleanFhs() ಸ್ಥಿರಾಂಕ = 0;  // True if THS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on lhs size
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on rhs size
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Ths() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on ths size
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Fhs() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on ths size
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;

ಖಾಸಗಿ:
    bool getPurityRecurse() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ lhsp()->is_ಶುದ್ಧ() && rhsp()->is_ಶುದ್ಧ() && thsp()->is_ಶುದ್ಧ() && fhsp()->is_ಶುದ್ಧ();
    }
};
class Ast_ಗಂಟು_Termop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Terminal operator -- an operator with no "inputs"
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Termop(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Termop;
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಗಂಟು_Triop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Triop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Triop;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    // Set out to evaluation of a AstConst'ed
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs,
                               ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths)
        = 0;
    ವಾಸ್ತವಿಕ bool cleanLhs() ಸ್ಥಿರಾಂಕ = 0;  // True if LHS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool cleanRhs() ಸ್ಥಿರಾಂಕ = 0;  // True if RHS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool cleanThs() ಸ್ಥಿರಾಂಕ = 0;  // True if THS must have extra upper bits zero
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on lhs size
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on rhs size
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Ths() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on ths size
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;

ಖಾಸಗಿ:
    bool getPurityRecurse() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ lhsp()->is_ಶುದ್ಧ() && rhsp()->is_ಶುದ್ಧ() && thsp()->is_ಶುದ್ಧ();
    }
};
class Ast_ಗಂಟು_Dist_Triop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Dist_Triop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp,
                     Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : Ast_ಗಂಟು_Triop{t, fl, lhsp, rhsp, thsp} {
        dtype_ಗಣ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Dist_Triop;
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanThs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Ths() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL_TRIG; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs,
                       ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths) override {
        V3ERROR_NA;
    }
};
class Ast_ಗಂಟು_Uniop VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Uniop(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Uniop;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    // Set out to evaluation of a AstConst'ed lhs
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) = 0;
    ವಾಸ್ತವಿಕ bool cleanLhs() ಸ್ಥಿರಾಂಕ = 0;
    ವಾಸ್ತವಿಕ bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ = 0;  // True if output result depends on lhs size
    ವಾಸ್ತವಿಕ bool doubleFlavor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    ವಾಸ್ತವಿಕ bool signedFlavor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    ವಾಸ್ತವಿಕ bool stringFlavor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }  // N flavor of nodes with both flavors?
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
};
class Ast_ಗಂಟು_System_Uniop_D VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_System_Uniop_D(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : Ast_ಗಂಟು_Uniop{t, fl, lhsp} {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_Ast_ಗಂಟು_System_Uniop_D;
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL_TRIG; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಗಂಟು_Var_Ref VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Var_Ref;  // Gen pointers before other members for performance
ಖಾಸಗಿ:
    VAccess m_access;  // Left hand side assignment
    VSelf_ಸೂಚಕ_ಪಠ್ಯ m_self_ಸೂಚಕ
        = VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::ಖಾಲಿಗಳು_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Var_Ref(VNType t, File_ಸಾಲು* fl, AstVar* varp, ಸ್ಥಿರಾಂಕ VAccess& access)
        : Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    VAccess access() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_access; }
    ಶೂನ್ಯ access(ಸ್ಥಿರಾಂಕ VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_varp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_ಗಂಟು*>(varp));
    }
    AstVarScope* varScopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_varScopep; }
    ಶೂನ್ಯ varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    ಸ್ಥಿರಾಂಕ VSelf_ಸೂಚಕ_ಪಠ್ಯ& self_ಸೂಚಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_self_ಸೂಚಕ; }
    ಶೂನ್ಯ self_ಸೂಚಕ(ಸ್ಥಿರಾಂಕ VSelf_ಸೂಚಕ_ಪಠ್ಯ& self_ಸೂಚಕ) { m_self_ಸೂಚಕ = self_ಸೂಚಕ; }
    string self_ಸೂಚಕ_Protect(bool useSelfForThis) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ self_ಸೂಚಕ().protect(useSelfForThis, protect());
    }
    Ast_ಗಂಟು_Module* classOrPackagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
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
    AstArg(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ std::string& name) override { m_name = name; }
    bool ಖಾಲಿಗಳು_Connect_No_ಮುಂದಿನ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !exprp() && name() == "" && !nextp(); }
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
ಖಾಸಗಿ:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::ಗಣ<std::string> m_restricted_ಹೆಸರುಗಳು;

ಸಾರ್ವಜನಿಕ:
    AstWith(File_ಸಾಲು* fl, AstLambdaArgRef* ಸೂಚ್ಯಂಕ_Arg_Refp, AstLambdaArgRef* ಮೌಲ್ಯ_Arg_Refp,
            Ast_ಗಂಟು* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->ಸೂಚ್ಯಂಕ_Arg_Refp(ಸೂಚ್ಯಂಕ_Arg_Refp);
        this->ಮೌಲ್ಯ_Arg_Refp(ಮೌಲ್ಯ_Arg_Refp);
        ಕೂಡಿಸು_Exprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!ಸೂಚ್ಯಂಕ_Arg_Refp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!ಮೌಲ್ಯ_Arg_Refp());  // varp needed to know lambda's arg dtype
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ restricted(bool flag) { m_restricted = flag; }
    bool restricted() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_restricted; }
    bool validated() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_validated; }
    ಶೂನ್ಯ validated(bool flag) { m_validated = flag; }
    ಶೂನ್ಯ ಕೂಡಿಸು_Restricted_Name(ಸ್ಥಿರಾಂಕ std::string& name) { m_restricted_ಹೆಸರುಗಳು.ಸೇರಿಸು(name); }
    ಸ್ಥಿರಾಂಕ std::ಗಣ<std::string>& restricted_ಹೆಸರುಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_restricted_ಹೆಸರುಗಳು; }
    // True if 'name' binds into the randomize() target class.
    bool name_Resolves_ಗಮ್ಯ_ಗುರಿ(ಸ್ಥಿರಾಂಕ std::string& name) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ !m_restricted || m_restricted_ಹೆಸರುಗಳು.ಎಣಿಕೆ_2(name);
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
    VAbort_ಬಗೆ m_ಬಗೆ{VAbort_ಬಗೆ::ACCEPT_ON};

ಸಾರ್ವಜನಿಕ:
    AstAbortOn(File_ಸಾಲು* fl, VAbort_ಬಗೆ ಬಗೆ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_ಬಗೆ{ಬಗೆ} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    VAbort_ಬಗೆ ಬಗೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಗೆ; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Addr_ದ_CFunc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

ಸಾರ್ವಜನಿಕ:
    Ast_Addr_ದ_CFunc(File_ಸಾಲು* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_Addr_ದ_CFunc(fl)
        , m_funcp{funcp} {
        dtypep(ಕಂಡುಹಿಡಿ_CHandle_DType());
    }

    ASTGEN_MEMBERS_Ast_Addr_ದ_CFunc;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    AstCFunc* funcp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_funcp; }
};
class Ast_Attr_ದ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
ಸಾರ್ವಜನಿಕ:
    Ast_Attr_ದ(File_ಸಾಲು* fl, VAttrType attrtype, Ast_ಗಂಟು* fromp = nullptr,
              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* dimp = nullptr)
        : ASTGEN_SUPER_Attr_ದ(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_Ast_Attr_ದ;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    VAttrType attrType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attrType; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ಸ್ಥಿರಾಂಕ bool m_ಶುದ್ಧ;  // Pure optimizable

    ಶೂನ್ಯ init(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, ಪೂರ್ಣಾಂಕ setwidth) {
        ಆದರೆ (!ಪಠ್ಯ.ಖಾಲಿಗಳು()) ಕೂಡಿಸು(ಪಠ್ಯ);
        ಆದರೆ (setwidth) {
            dtype_ಗಣ_ಬಿಟ್_Sized(setwidth, VSigning::UNSIGNED);
        } ಇಲ್ಲದಿದ್ದರೆ {
            dtype_ಗಣ_Void();  // Caller to override if necessary
        }
    }

ಸಾರ್ವಜನಿಕ:
    class ಶುದ್ಧ {};
    explicit AstCExpr(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ = "", ಪೂರ್ಣಾಂಕ setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_ಶುದ್ಧ{false} {
        init(ಪಠ್ಯ, setwidth);
    }
    AstCExpr(File_ಸಾಲು* fl, ಶುದ್ಧ, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ = "", ಪೂರ್ಣಾಂಕ setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_ಶುದ್ಧ{true} {
        init(ಪಠ್ಯ, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ಶುದ್ಧ; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ಶುದ್ಧ; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ m_ಶುದ್ಧ; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    // Add some text, or a node to this expression
    ಶೂನ್ಯ ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) { ಕೂಡಿಸು_Nodesp(new Ast_ಪಠ್ಯ{fileline(), ಪಠ್ಯ}); }
    ಶೂನ್ಯ ಕೂಡಿಸು(Ast_ಗಂಟು* nodep) { ಕೂಡಿಸು_Nodesp(nodep); }
};
class AstCExprUser final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ಸ್ಥಿರಾಂಕ bool m_ಶುದ್ಧ;  // Whether the function is pure
ಸಾರ್ವಜನಿಕ:
    class ಶುದ್ಧ {};
    explicit AstCExprUser(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_ಶುದ್ಧ{false} {}
    AstCExprUser(File_ಸಾಲು* fl, ಶುದ್ಧ)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_ಶುದ್ಧ{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ಶುದ್ಧ; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ಶುದ್ಧ; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ m_ಶುದ್ಧ; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    // Add some text, or a node to this expression
    ಶೂನ್ಯ ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) { ಕೂಡಿಸು_Nodesp(new Ast_ಪಠ್ಯ{fileline(), ಪಠ್ಯ}); }
    ಶೂನ್ಯ ಕೂಡಿಸು(Ast_ಗಂಟು* nodep) { ಕೂಡಿಸು_Nodesp(nodep); }
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
        ಕೂಡಿಸು_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ function().ascii(); }
    bool is_ಶುದ್ಧ() override {
        ಆದರೆ (!m_purity.isCached()) m_purity.ಗಣ(getPurity());
        ಹಿಂತಿರುಗಿ m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    VCFunction function() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_function; }

ಖಾಸಗಿ:
    bool getPurity();
};
class AstCMethodHard final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_ವಿಧಾನ;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
ಸಾರ್ವಜನಿಕ:
    AstCMethodHard(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, VCMethod ವಿಧಾನ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_ವಿಧಾನ{ವಿಧಾನ} {
        this->fromp(fromp);
        ಕೂಡಿಸು_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ ವಿಧಾನ().ascii(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, CMethodHard);
        ಹಿಂತಿರುಗಿ (m_ವಿಧಾನ == asamep->m_ವಿಧಾನ);
    }
    bool is_ಶುದ್ಧ() override {
        ಆದರೆ (!m_purity.isCached()) m_purity.ಗಣ(getPurity());
        ಹಿಂತಿರುಗಿ m_purity.get();
    }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ವಿಧಾನ.isClean(); }
    bool usePtr() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_usePtr; }
    ಶೂನ್ಯ usePtr(bool flag) { m_usePtr = flag; }
    VCMethod ವಿಧಾನ() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_ವಿಧಾನ; }
    ಶೂನ್ಯ ವಿಧಾನ(VCMethod ಮೌಲ್ಯ) { m_ವಿಧಾನ = ಮೌಲ್ಯ; }

ಖಾಸಗಿ:
    bool getPurity();
};
class AstCReset final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Reset variable at startup
    ಸ್ಥಿರಾಂಕ bool m_constructing;  // Previously cleared by constructor
ಸಾರ್ವಜನಿಕ:
    AstCReset(File_ಸಾಲು* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!VN_IS(backp(), ಗಂಟು_ನಿಯೋಜಿಸು));  // V3Emit* assumption
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_constructing; }
};
class AstCast final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
ಸಾರ್ವಜನಿಕ:
    AstCast(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        ಮಗು_DTypep(dtp);
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ dtypep() ? dtypep() : ಮಗು_DTypep(); }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cast_ಗಾತ್ರ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
ಸಾರ್ವಜನಿಕ:
    Ast_Cast_ಗಾತ್ರ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_Cast_ಗಾತ್ರ(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_Ast_Cast_ಗಾತ್ರ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
ಸಾರ್ವಜನಿಕ:
    AstCellArrayRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        ಕೂಡಿಸು_Selp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
ಸಾರ್ವಜನಿಕ:
    AstCellRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* cellp, Ast_ಗಂಟು* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
ಸಾರ್ವಜನಿಕ:
    AstClassOrPackageRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        ಕೂಡಿಸು_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_ಗಂಟು_Module* classOrPackageSkipp(ಸ್ಥಿರಾಂಕ bool doRefs = true) ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು* classOrPackageNodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classOrPackageNodep; }
    ಶೂನ್ಯ classOrPackageNodep(Ast_ಗಂಟು* nodep) { m_classOrPackageNodep = nodep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_ಗಂಟು*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A constant
    V3_ಸಂಖ್ಯೆ m_ಅಂಕೆ;  // Constant value
    ಶೂನ್ಯ init_With_ಸಂಖ್ಯೆ() {
        ಆದರೆ (m_ಅಂಕೆ.isDouble()) {
            dtype_ಗಣ_Double();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಅಂಕೆ.isString()) {
            dtype_ಗಣ_String();
        } ಇಲ್ಲದಿದ್ದರೆ {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(m_ಅಂಕೆ.ಅಗಲ(), (m_ಅಂಕೆ.sized() ? 0 : m_ಅಂಕೆ.ಅಗಲ_ಗಮ್ಯ_Fit()),
                                      VSigning::fromBool(m_ಅಂಕೆ.isSigned()), m_ಅಂಕೆ.isAnyXZ());
        }
        m_ಅಂಕೆ.nodep(this);
    }

ಸಾರ್ವಜನಿಕ:
    ~AstConst() override;
    AstConst(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{ಅಂಕೆ} {
        init_With_ಸಂಖ್ಯೆ();
    }
    class Widthed_ಮೌಲ್ಯ {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, Widthed_ಮೌಲ್ಯ, ಪೂರ್ಣಾಂಕ ಅಗಲ, uint32_t ಮೌಲ್ಯ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, ಅಗಲ, ಮೌಲ್ಯ) {  // Need () constructor
        init_With_ಸಂಖ್ಯೆ();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_ಸಾಲು* fl, DTyped, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, nodedtypep} {
        init_With_ಸಂಖ್ಯೆ();
    }
    class String_ಗಮ್ಯ_Parse {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, String_ಗಮ್ಯ_Parse, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::Verilog_ಸಂಖ್ಯೆ_Literal{}, sourcep} {
        init_With_ಸಂಖ್ಯೆ();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, VerilogStringLiteral, ಸ್ಥಿರಾಂಕ string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::VerilogStringLiteral{}, str} {
        init_With_ಸಂಖ್ಯೆ();
    }
    AstConst(File_ಸಾಲು* fl, uint32_t ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 32, ಅಂಕೆ) {  // Need () constructor
        dtype_ಗಣ_ಬಿಟ್_Unsized(m_ಅಂಕೆ.ಅಗಲ(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, Unsized32, uint32_t ಅಂಕೆ)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 32, ಅಂಕೆ) {  // Need () constructor
        m_ಅಂಕೆ.ಅಗಲ(32, false);
        dtype_ಗಣ_ಬಿಟ್_Unsized(32, m_ಅಂಕೆ.ಅಗಲ_ಗಮ್ಯ_Fit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, Signed32, int32_t ಅಂಕೆ)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 32, ಅಂಕೆ) {  // Need () constructor
        m_ಅಂಕೆ.ಅಗಲ(32, true);
        dtype_ಗಣ_ಬಿಟ್_Unsized(32, m_ಅಂಕೆ.ಅಗಲ_ಗಮ್ಯ_Fit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, Unsized64, uint64_t ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 64, 0) {  // Need () constructor
        m_ಅಂಕೆ.ಗಣ_Quad(ಅಂಕೆ);
        dtype_ಗಣ_ಬಿಟ್_Sized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, SizedEData, uint64_t ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_ಅಂಕೆ.ಗಣ_Quad(ಅಂಕೆ);
        dtype_ಗಣ_ಬಿಟ್_Sized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, RealDouble, ದ್ವಿಗುಣ ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 64) {  // Need () constructor
        m_ಅಂಕೆ.ಗಣ_Double(ಅಂಕೆ);
        dtype_ಗಣ_Double();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_ಸಾಲು* fl, String, ಸ್ಥಿರಾಂಕ string& ಅಂಕೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::String{}, ಅಂಕೆ} {
        dtype_ಗಣ_String();
    }
    class ಬಿಟ್_False {};
    AstConst(File_ಸಾಲು* fl, ಬಿಟ್_False)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 1, 0) {  // Need () constructor
        dtype_ಗಣ_ಬಿಟ್();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class ಬಿಟ್_False_Erroring {};
    AstConst(File_ಸಾಲು* fl, ಬಿಟ್_False_Erroring)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 1, 0) {  // Need () constructor
        dtype_ಗಣ_ಬಿಟ್();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ newfl = new File_ಸಾಲು{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class ಬಿಟ್_True {};
    AstConst(File_ಸಾಲು* fl, ಬಿಟ್_True, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ(this, 1, on) {  // Need () constructor
        dtype_ಗಣ_ಬಿಟ್();
    }
    class All0 {};
    AstConst(File_ಸಾಲು* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::Verilog_ಸಂಖ್ಯೆ_Literal{}, "'0"} {
        init_With_ಸಂಖ್ಯೆ();
        fl->warnOff(V3_Error_ಸಂಕೇತ::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_ಸಾಲು* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::Verilog_ಸಂಖ್ಯೆ_Literal{}, "'1"} {
        init_With_ಸಂಖ್ಯೆ();
        fl->warnOff(V3_Error_ಸಂಕೇತ::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_ಸಾಲು* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::Null{}} {
        dtypep(ಕಂಡುಹಿಡಿ_Null_DType());
        init_With_ಸಂಖ್ಯೆ();
    }
    class One_ಹೆಜ್ಜೆ {};
    AstConst(File_ಸಾಲು* fl, One_ಹೆಜ್ಜೆ)
        : ASTGEN_SUPER_Const(fl)
        , m_ಅಂಕೆ{this, V3_ಸಂಖ್ಯೆ::One_ಹೆಜ್ಜೆ{}} {
        dtype_ಗಣ_ಬಿಟ್_Sized(64, VSigning::UNSIGNED);
        init_With_ಸಂಖ್ಯೆ();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ ಅಂಕೆ().ascii(); }  // * = Value
    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಅಂಕೆ; }  // * = Value
    V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() { ಹಿಂತಿರುಗಿ m_ಅಂಕೆ; }  // * = Value
    string origParamName() ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ origParamName(ಸ್ಥಿರಾಂಕ string& name);
    uint32_t ಗಮ್ಯ_UInt() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಕೆ().ಗಮ್ಯ_UInt(); }
    int32_t ಗಮ್ಯ_SInt() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ ಅಂಕೆ().ಗಮ್ಯ_SInt(); }
    uint64_t ಗಮ್ಯ_UQuad() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಕೆ().ಗಮ್ಯ_UQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, Const);
        ಹಿಂತಿರುಗಿ ಅಂಕೆ().isCaseEq(sp->ಅಂಕೆ());
    }
    ಶೂನ್ಯ cloneRelink() override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(m_ಅಂಕೆ.nodep() && m_ಅಂಕೆ.nodep() != this);
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool isEqAllOnes() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಕೆ().isEqAllOnes(ಅಗಲ()); }
    bool isEqAllOnesV() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಕೆ().isEqAllOnes(ಅಗಲ_Min_V()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_ಮೌಲ್ಯ_Type(ಸ್ಥಿರಾಂಕ AstConst* samep) ಸ್ಥಿರಾಂಕ;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    ಸ್ಥಿರ AstConst* parseParamLiteral(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& literal);
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
        dtypep(ಕಂಡುಹಿಡಿ_Constraint_Ref_DType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE;
    AstConstraint* constrp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_constrp; }
    ಶೂನ್ಯ constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
    // TODO: this setter is unused
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ final override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cvt_Array_ಗಮ್ಯ_Array final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ bool m_ಹಿಮ್ಮುಖ;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಖಂಡ_ಗಾತ್ರ;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_dst_Element_ಬಿಟ್ಟುಗಳು;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_src_Element_ಬಿಟ್ಟುಗಳು;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_ಗಮ್ಯ_Array(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp, bool ಹಿಮ್ಮುಖ,
                       ಪೂರ್ಣಾಂಕ ಖಂಡ_ಗಾತ್ರ, ಪೂರ್ಣಾಂಕ dst_Element_ಬಿಟ್ಟುಗಳು, ಪೂರ್ಣಾಂಕ src_Element_ಬಿಟ್ಟುಗಳು)
        : ASTGEN_SUPER_Cvt_Array_ಗಮ್ಯ_Array(fl)
        , m_ಹಿಮ್ಮುಖ{ಹಿಮ್ಮುಖ}
        , m_ಖಂಡ_ಗಾತ್ರ{ಖಂಡ_ಗಾತ್ರ}
        , m_dst_Element_ಬಿಟ್ಟುಗಳು{dst_Element_ಬಿಟ್ಟುಗಳು}
        , m_src_Element_ಬಿಟ್ಟುಗಳು{src_Element_ಬಿಟ್ಟುಗಳು} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_ಗಮ್ಯ_Array;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಹಿಮ್ಮುಖ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಹಿಮ್ಮುಖ; }
    ಪೂರ್ಣಾಂಕ ಖಂಡ_ಗಾತ್ರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಖಂಡ_ಗಾತ್ರ; }
    ಪೂರ್ಣಾಂಕ dst_Element_ಬಿಟ್ಟುಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dst_Element_ಬಿಟ್ಟುಗಳು; }
    ಪೂರ್ಣಾಂಕ src_Element_ಬಿಟ್ಟುಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_src_Element_ಬಿಟ್ಟುಗಳು; }
};
class Ast_Cvt_Array_ಗಮ್ಯ_Packed final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_ಗಮ್ಯ_Packed(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Cvt_Array_ಗಮ್ಯ_Packed(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_ಗಮ್ಯ_Packed;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Cvt_Packed_ಗಮ್ಯ_Array final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Packed_ಗಮ್ಯ_Array(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Cvt_Packed_ಗಮ್ಯ_Array(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Packed_ಗಮ್ಯ_Array;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Cvt_Unpack_ಗಮ್ಯ_ಸರತಿ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Unpack_ಗಮ್ಯ_ಸರತಿ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_ಗಮ್ಯ_ಸರತಿ(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_ಗಮ್ಯ_ಸರತಿ;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_ಗಮ್ಯ_Unpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstDist final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
ಸಾರ್ವಜನಿಕ:
    AstDist(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Dist_ಅಂಶ* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        ಕೂಡಿಸು_Itemsp(itemsp);
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // NA
};
class Ast_Dist_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_is_ಸಂಪೂರ್ಣ = false;  // True for weight ':/', false for ':='
ಸಾರ್ವಜನಿಕ:
    Ast_Dist_ಅಂಶ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp)
        : ASTGEN_SUPER_Dist_ಅಂಶ(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_Ast_Dist_ಅಂಶ;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l "s + (m_is_ಸಂಪೂರ್ಣ ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // NA
    ಶೂನ್ಯ is_ಸಂಪೂರ್ಣ(bool flag) { m_is_ಸಂಪೂರ್ಣ = flag; }
    bool is_ಸಂಪೂರ್ಣ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_ಸಂಪೂರ್ಣ; }
};
class AstDot final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    ಸ್ಥಿರಾಂಕ bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
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
        ಆದರೆ (!packageOrClassp) ಹಿಂತಿರುಗಿ rhsp;
        ಹಿಂತಿರುಗಿ new AstDot{fl, true, packageOrClassp, rhsp};
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool colon() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class Ast_ಖಾಲಿಗಳು_ಸರತಿ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಖಾಲಿಗಳು_ಸರತಿ(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ಖಾಲಿಗಳು_ಸರತಿ(fl) {}
    ASTGEN_MEMBERS_Ast_ಖಾಲಿಗಳು_ಸರತಿ;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "{}"; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Enum_ಅಂಶ_Ref final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ = false;  // Contains gen block reference
ಸಾರ್ವಜನಿಕ:
    Ast_Enum_ಅಂಶ_Ref(File_ಸಾಲು* fl, Ast_Enum_ಅಂಶ* itemp, Ast_ಗಂಟು_Module* classOrPackagep)
        : ASTGEN_SUPER_Enum_ಅಂಶ_Ref(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_Ast_Enum_ಅಂಶ_Ref;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ itemp() ? itemp()->name() : m_name; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ_Ref* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, Enum_ಅಂಶ_Ref);
        ಹಿಂತಿರುಗಿ itemp() == sp->itemp() && dotted() == sp->dotted()
               && ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ() == sp->ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ();
    }
    Ast_Enum_ಅಂಶ* itemp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_itemp; }
    ಶೂನ್ಯ itemp(Ast_Enum_ಅಂಶ* nodep) { m_itemp = nodep; }
    string dotted() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dotted; }
    ಶೂನ್ಯ dotted(ಸ್ಥಿರಾಂಕ string& dotted) { m_dotted = dotted; }
    bool ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ; }
    ಶೂನ್ಯ ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ(ಸ್ಥಿರಾಂಕ bool flag) { m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
ಖಾಸಗಿ:
    bool m_has_ಫಲಿತಾಂಶ = true;  // Returns result via resultp()

ಸಾರ್ವಜನಿಕ:
    Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp)
        : ASTGEN_SUPER_ಅಭಿವ್ಯಕ್ತಿ_Stmt(fl) {
        ಕೂಡಿಸು_Stmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override {
        ಆದರೆ (Ast_ಗಂಟು::afterCommentp(stmtsp())) ಹಿಂತಿರುಗಿ false;
        ಹಿಂತಿರುಗಿ resultp()->is_ಶುದ್ಧ();
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool has_ಫಲಿತಾಂಶ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_has_ಫಲಿತಾಂಶ; }
    ಶೂನ್ಯ has_ಫಲಿತಾಂಶ(bool flag) { m_has_ಫಲಿತಾಂಶ = flag; }
};
class AstFError final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstFError(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 64; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstFOpen final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstFOpen(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* modep)
        : ASTGEN_SUPER_FOpen(fl) {
        this->filenamep(filenamep);
        this->modep(modep);
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpen;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$fopen"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstFOpenMcd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op2 := filenamep : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstFOpenMcd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$fopen"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
        dtype_ಗಣ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: makes output
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstFRewind final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := filep : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstFRewind(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstFScanF final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_ಪಠ್ಯ;

ಸಾರ್ವಜನಿಕ:
    AstFScanF(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_ಪಠ್ಯ{ಪಠ್ಯ} {
        ಕೂಡಿಸು_Exprsp(exprsp);
        this->filep(filep);
        dtype_ಗಣ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: makes output
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಪಠ್ಯ() == VN_DBG_AS(samep, FScanF)->ಪಠ್ಯ();
    }
    string ಪಠ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }  // * = Text to display
    ಶೂನ್ಯ ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) { m_ಪಠ್ಯ = ಪಠ್ಯ; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstFSeek final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstFSeek(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಾನಾಂತರ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->ಸ್ಥಾನಾಂತರ(ಸ್ಥಾನಾಂತರ);
        this->operation(operation);
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: makes output
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstFTell final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
ಸಾರ್ವಜನಿಕ:
    AstFTell(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Get_Initial_Random_ಬೀಜ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $get_initial_random_seed()
ಸಾರ್ವಜನಿಕ:
    explicit Ast_Get_Initial_Random_ಬೀಜ(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Get_Initial_Random_ಬೀಜ(fl) {
        dtype_ಗಣ_Int();
    }
    ASTGEN_MEMBERS_Ast_Get_Initial_Random_ಬೀಜ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstImplication final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    ಸ್ಥಿರಾಂಕ bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

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
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isOverlapped; }
    bool isFollowedBy() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isFollowedBy; }
};
class AstInside final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    AstInside(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        ಕೂಡಿಸು_Itemsp(itemsp);
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // NA
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newAndFromInside(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp);
};
class AstLambdaArgRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_ಸೂಚ್ಯಂಕ;  // Index, not value

ಸಾರ್ವಜನಿಕ:
    AstLambdaArgRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, bool ಸೂಚ್ಯಂಕ)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_ಸೂಚ್ಯಂಕ{ಸೂಚ್ಯಂಕ} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    bool ಸೂಚ್ಯಂಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸೂಚ್ಯಂಕ; }
    bool is_ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_ಹೊಂದಿಕೆ_Masked final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
ಸಾರ್ವಜನಿಕ:
    ಸಾಲೊಳಗೆ Ast_ಹೊಂದಿಕೆ_Masked(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_Ast_ಹೊಂದಿಕೆ_Masked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಸ್ಥಿರ uint32_t fold(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, AstVar* ಹೊಂದಿಕೆ_Varp);
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstMemberSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
ಸಾರ್ವಜನಿಕ:
    AstMemberSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, VFlag_ಮಗು_DType, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    VAccess access() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_access; }
    ಶೂನ್ಯ access(ಸ್ಥಿರಾಂಕ VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    AstVar* varp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_varp; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
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
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
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
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

ಸಾರ್ವಜನಿಕ:
    AstParseRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nullptr,
                Ast_ಗಂಟು_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstParseRef* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ParseRef);
        ಹಿಂತಿರುಗಿ m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
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
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_propertyTiming; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
        ಕೂಡಿಸು_Lhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 2; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    bool isConcat() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isConcat; }
    ಶೂನ್ಯ isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isDefault; }
    ಶೂನ್ಯ isDefault(bool flag) { m_isDefault = flag; }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಕೀಲಿ_Exprp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varrefp() ? varrefp() : VN_CAST(keyp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ); }
};
class Ast_ನಮೂನೆ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
ಸಾರ್ವಜನಿಕ:
    Ast_ನಮೂನೆ(File_ಸಾಲು* fl, Ast_ಗಂಟು* itemsp)
        : ASTGEN_SUPER_ನಮೂನೆ(fl) {
        ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_ನಮೂನೆ;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ dtypep() ? dtypep() : ಮಗು_DTypep(); }
};
class Ast_ನಮೂನೆ_ನಕ್ಷತ್ರ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Pattern wildcard: ".*"
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ನಮೂನೆ_ನಕ್ಷತ್ರ(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ನಮೂನೆ_ನಕ್ಷತ್ರ(fl) {}
    ASTGEN_MEMBERS_Ast_ನಮೂನೆ_ನಕ್ಷತ್ರ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ನಮೂನೆ_Var final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
ಸಾರ್ವಜನಿಕ:
    Ast_ನಮೂನೆ_Var(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_ನಮೂನೆ_Var(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ನಮೂನೆ_Var;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_name == VN_DBG_AS(samep, ನಮೂನೆ_Var)->m_name;
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
};
class AstPropAlways final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    ಸ್ಥಿರಾಂಕ bool m_isStrong = false;  // s_always
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
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(0); }
    bool isStrong() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isStrong; }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstRand final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    ಸ್ಥಿರಾಂಕ bool m_urandom;  // $urandom vs $random
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
        dtype_ಗಣ_UInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override {
        ಹಿಂತಿರುಗಿ seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        ಆದರೆ (seedp()) {
            ಆದರೆ (urandom()) ಹಿಂತಿರುಗಿ "VL_URANDOM_SEEDED_%nq%lq(%li)";
            ಹಿಂತಿರುಗಿ "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        ಆದರೆ (isWide()) ಹಿಂತಿರುಗಿ "VL_RANDOM_%nq(%nw, %P)";
        ಹಿಂತಿರುಗಿ "VL_RANDOM_%nq()";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ !seedp(); }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool combinable(ಸ್ಥಿರಾಂಕ AstRand* samep) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_urandom; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$rngrandom()"; }
    string emitC() override {
        ಹಿಂತಿರುಗಿ isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSClocked final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstSClocked(File_ಸಾಲು* fl, Ast_Sen_ಅಂಶ* sensesp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
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
    ಸ್ಥಿರಾಂಕ bool m_unbounded = false;  // True when the upper bound is $
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
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(0); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {  // LCOV_EXCL_LINE
        ಹಿಂತಿರುಗಿ m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unbounded; }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
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
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$sformatarg"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && ಮೊದಲ_Abovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    VFormatAttr formatAttr() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_formatAttr; }
    ಶೂನ್ಯ formatAttr(ಸ್ಥಿರಾಂಕ VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    ಸ್ಥಿರ VFormatAttr formatAttrDefauled(ಸ್ಥಿರಾಂಕ AstSFormatArg* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep);
};
class AstSFormatF final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_ಪಠ್ಯ;
    bool m_hidden;  // Under display, etc
    bool m_ಅಭಿವ್ಯಕ್ತಿ_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    class ಅಭಿವ್ಯಕ್ತಿ_Format {};
    AstSFormatF(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, bool hidden, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp,
                ಅಕ್ಷರ missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_ಪಠ್ಯ{ಪಠ್ಯ}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtype_ಗಣ_String();
        ಕೂಡಿಸು_Exprsp(exprsp);
    }
    AstSFormatF(File_ಸಾಲು* fl, ಅಭಿವ್ಯಕ್ತಿ_Format, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp, ಅಕ್ಷರ missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_ಪಠ್ಯ{""}
        , m_hidden{hidden}
        , m_ಅಭಿವ್ಯಕ್ತಿ_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtype_ಗಣ_String();
        ಕೂಡಿಸು_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_ಪಠ್ಯ = name; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಪಠ್ಯ() == VN_DBG_AS(samep, SFormatF)->ಪಠ್ಯ();
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$sformatf"; }
    string ಪಠ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }  // * = Text to display
    ಶೂನ್ಯ ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) { m_ಪಠ್ಯ = ಪಠ್ಯ; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(ಪಠ್ಯ() != "" && ಅಭಿವ್ಯಕ್ತಿ_Format_2());  // Expr format means no literal format
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool formatScopeTracking() ಸ್ಥಿರಾಂಕ {  // Track scopeNamep();  Ok if false positive
        ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_Format_2() || name().ಕಂಡುಹಿಡಿ("%m") != string::npos || name().ಕಂಡುಹಿಡಿ("%M") != string::npos)
            ಹಿಂತಿರುಗಿ true;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ fmtp = VN_CAST(exprp, Const)) {
                ಆದರೆ (fmtp->ಅಂಕೆ().isFromString()) {
                    ಸ್ಥಿರಾಂಕ string str = fmtp->ಅಂಕೆ().ಗಮ್ಯ_String();
                    ಆದರೆ (str.ಕಂಡುಹಿಡಿ("%m") != string::npos || str.ಕಂಡುಹಿಡಿ("%M") != string::npos)
                        ಹಿಂತಿರುಗಿ true;
                }
            }
        }
        ಹಿಂತಿರುಗಿ false;
    }
    bool hidden() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hidden; }
    ಶೂನ್ಯ hidden(bool flag) { m_hidden = flag; }
    bool ಅಭಿವ್ಯಕ್ತಿ_Format_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಭಿವ್ಯಕ್ತಿ_Format; }
    ಶೂನ್ಯ ಅಭಿವ್ಯಕ್ತಿ_Format_2(bool flag) { m_ಅಭಿವ್ಯಕ್ತಿ_Format = flag; }
    bool optionalFormat() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_optionalFormat; }
    ಶೂನ್ಯ optionalFormat(bool flag) { m_optionalFormat = flag; }
    ಅಕ್ಷರ missingArgChar() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_missingArgChar; }
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSScanF final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_ಪಠ್ಯ;
    VTimescale m_timeunit;  // Parent module time unit

ಸಾರ್ವಜನಿಕ:
    AstSScanF(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_ಪಠ್ಯ{ಪಠ್ಯ} {
        ಕೂಡಿಸು_Exprsp(exprsp);
        this->fromp(fromp);
        dtype_ಗಣ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: makes output
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstSScanF* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, SScanF);
        ಹಿಂತಿರುಗಿ ಪಠ್ಯ() == asamep->ಪಠ್ಯ() && timeunit() == asamep->timeunit();
    }
    string ಪಠ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }  // * = Text to display
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSampled final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_ಆಂತರಿಕ : 1;  // Internally created, not from a source $sampled
ಸಾರ್ವಜನಿಕ:
    AstSampled(File_ಸಾಲು* fl, Ast_ಗಂಟು* exprp, Ast_ಗಂಟು_DType* dtypep, bool ಆಂತರಿಕ_2 = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_ಆಂತರಿಕ{ಆಂತರಿಕ_2} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_ಆಂತರಿಕ == VN_DBG_AS(samep, Sampled)->m_ಆಂತರಿಕ;
    }
    bool ಆಂತರಿಕ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಆಂತರಿಕ; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstScopeName final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    ಸ್ಥಿರಾಂಕ bool m_forFormat;  // Is for a format %m
    ಸ್ಥಿರ std::string scopeNameFormatter(ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ);
    ಸ್ಥಿರ std::string scopePrettyNameFormatter(ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ);

ಸಾರ್ವಜನಿಕ:
    class ForFormat {};
    AstScopeName(File_ಸಾಲು* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtype_ಗಣ_UInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstScopeName* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, ScopeName);
        ಹಿಂತಿರುಗಿ m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    // ACCESSORS
    ಸ್ಥಿರಾಂಕ std::string& scopeAttr() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopeAttr; }
    ಶೂನ್ಯ scopeAttr(ಸ್ಥಿರಾಂಕ std::string& val) { m_scopeAttr = val; }
    ಸ್ಥಿರಾಂಕ std::string& scopeEntr() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopeEntr; }
    ಶೂನ್ಯ scopeEntr(ಸ್ಥಿರಾಂಕ std::string& val) { m_scopeEntr = val; }
    bool dpiExport() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiExport; }
    ಶೂನ್ಯ dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_selfRelative; }
    ಶೂನ್ಯ selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ scopePrettyNameFormatter(m_scopeEntr); }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ರಾಶಿ_Trace_F final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // $stacktrace used as function
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ರಾಶಿ_Trace_F(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ರಾಶಿ_Trace_F(fl) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_Ast_ರಾಶಿ_Trace_F;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$stacktrace"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ verilogKwd(); }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
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
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstStructSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
ಸಾರ್ವಜನಿಕ:
    AstStructSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override {
        // Not a union
        ಹಿಂತಿರುಗಿ VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, StructSel);
        ಹಿಂತಿರುಗಿ m_name == sp->m_name;
    }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
};
class AstSysIgnore final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
ಸಾರ್ವಜನಿಕ:
    AstSysIgnore(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        ಕೂಡಿಸು_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$ignored"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Though deleted before opt
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Though deleted before opt
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // Though deleted before opt
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // Though deleted before opt
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstSystemF(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtype_ಗಣ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$system"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ verilogKwd(); }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
ಸಾರ್ವಜನಿಕ:
    Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_Tagged_ಅಭಿವ್ಯಕ್ತಿ(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_name == VN_DBG_AS(samep, Tagged_ಅಭಿವ್ಯಕ್ತಿ)->m_name;
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
};
class Ast_Tagged_ನಮೂನೆ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
ಸಾರ್ವಜನಿಕ:
    Ast_Tagged_ನಮೂನೆ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* patternp)
        : ASTGEN_SUPER_Tagged_ನಮೂನೆ(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_ನಮೂನೆ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_name == VN_DBG_AS(samep, Tagged_ನಮೂನೆ)->m_name;
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
};
class Ast_Test_Plus_ವಾದಗಳು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
ಸಾರ್ವಜನಿಕ:
    Ast_Test_Plus_ವಾದಗಳು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* searchp)
        : ASTGEN_SUPER_Test_Plus_ವಾದಗಳು(fl) {
        this->searchp(searchp);
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_Test_Plus_ವಾದಗಳು;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$test$plusargs"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ verilogKwd(); }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    // but isPure() true
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstThisRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
ಸಾರ್ವಜನಿಕ:
    AstThisRef(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        ಮಗು_DTypep(dtypep);
    }
    AstThisRef(File_ಸಾಲು* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { ಹಿಂತಿರುಗಿ "this"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "this"; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
};
class Ast_ಸಮಯ_Precision final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Verilog $timeprecision
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಸಮಯ_Precision(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ಸಮಯ_Precision(fl) {
        dtype_ಗಣ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ಸಮಯ_Precision;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಸಮಯ_Unit final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಸಮಯ_Unit(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ಸಮಯ_Unit(fl) {
        dtype_ಗಣ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ಸಮಯ_Unit;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<uint32_t> m_indices;  // Indices of the triggers tested
ಸಾರ್ವಜನಿಕ:
    AstTriggered(File_ಸಾಲು* fl, AstVarScope* trigVscp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<uint32_t>& indices() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_indices; }
};
class AstUnbounded final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
ಸಾರ್ವಜನಿಕ:
    explicit AstUnbounded(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtype_ಗಣ_Int();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstUnlinkedRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
ಸಾರ್ವಜನಿಕ:
    AstUnlinkedRef(File_ಸಾಲು* fl, Ast_ಗಂಟು* refp, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    ಸ್ಥಿರಾಂಕ bool m_strong;  // 's_' preffix
    ಸ್ಥಿರಾಂಕ bool m_overlapping;  // '_with` suffix
ಸಾರ್ವಜನಿಕ:
    AstUntil(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isStrong() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_strong; }
    bool isOverlapping() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_overlapping; }
};
class Ast_ಮೌಲ್ಯ_Plus_ವಾದಗಳು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
ಸಾರ್ವಜನಿಕ:
    Ast_ಮೌಲ್ಯ_Plus_ವಾದಗಳು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* searchp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outp)
        : ASTGEN_SUPER_ಮೌಲ್ಯ_Plus_ವಾದಗಳು(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_ಮೌಲ್ಯ_Plus_ವಾದಗಳು;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$value$plusargs"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ !outp(); }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstWithParse final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
ಖಾಸಗಿ:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

ಸಾರ್ವಜನಿಕ:
    AstWithParse(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* funcrefp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp,
                 Ast_ಗಂಟು* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        ಕೂಡಿಸು_Exprsp(exprsp);
        ಕೂಡಿಸು_Constraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool restricted() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_restricted; }
    ಶೂನ್ಯ restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
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
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
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
        dtype_ಗಣ_ಬಿಟ್_Unsized(32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$cast(%r, %l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DYNAMIC_CAST(%r, %l)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 20; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
};
class AstCompareNN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog str.compare() and str.icompare()
    ಸ್ಥಿರಾಂಕ bool m_ignoreCase;  // True for str.icompare()
ಸಾರ್ವಜನಿಕ:
    AstCompareNN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtype_ಗಣ_Int();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        ಹಿಂತಿರುಗಿ m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        ಹಿಂತಿರುಗಿ m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ignoreCase() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ignoreCase; }
};
class AstConcat final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstConcat(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        ಆದರೆ (lhsp->dtypep() && rhsp->dtypep()) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Sized(lhsp->dtypep()->ಅಗಲ() + rhsp->dtypep()->ಅಗಲ(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f{%l, %k%r}"; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opConcat(lhs, rhs);
    }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(concat %l %r)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 2; }
};
class AstConcatN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // String concatenate
ಸಾರ್ವಜನಿಕ:
    AstConcatN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f{%l, %k%r}"; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opConcatN(lhs, rhs);
    }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_CONCATN_NNN(%li, %ri)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstDiv final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstDiv(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opDiv(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f/ %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvudiv %l %r)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_DIV; }
};
class AstDivD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstDivD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_AstDivD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opDivD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "/"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL_DIV; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstDivS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstDivS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opDivS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f/ %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvsdiv %l %r)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_DIV; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstEqWild final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstEqWild(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    ಸ್ಥಿರ Ast_ಗಂಟು_Biop* newTyped(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp);
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f==? %r)"; }
    string emitC() override {
        ಆದರೆ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        ಆದರೆ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ "==";
        }
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstFGetS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
ಸಾರ್ವಜನಿಕ:
    AstFGetS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtype_ಗಣ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$fgets(%l,%r)"; }
    string emitC() override {
        ಹಿಂತಿರುಗಿ strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 64; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstFUngetC(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        ಹಿಂತಿರುಗಿ "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 64; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ಗಂಟು* filep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ lhsp(); }
    Ast_ಗಂಟು* charp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ rhsp(); }
};
class AstGetcN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog string.getc()
ಸಾರ್ವಜನಿಕ:
    AstGetcN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGetcN(lhs, rhs);
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ "getc"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l.getc(%r))"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstGetcRefN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
ಸಾರ್ವಜನಿಕ:
    AstGetcRefN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstGt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstGt(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGt(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f> %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ">"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstGtD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstGtD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstGtD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGtD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ">"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstGtN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstGtN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstGtN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGtN(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ">"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstGtS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstGtS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGtS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f> %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstGte final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstGte(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGte(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f>= %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ">="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstGteD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstGteD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstGteD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGteD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ">="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstGteN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstGteN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstGteN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGteN(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ">="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstGteS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstGteS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opGteS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f>= %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstLogAnd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLogAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f&& %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvand %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "&&"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstLogIf final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLogIf(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f-> %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "->"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstLogOr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLogOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f|| %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvor %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "||"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstLt final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLt(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLt(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f< %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "<"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstLtD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstLtD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstLtD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLtD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "<"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstLtN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstLtN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstLtN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLtN(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "<"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstLtS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLtS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLtS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f< %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstLte final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLte(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLte(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f<= %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "<="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstLteD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstLteD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstLteD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLteD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "<="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstLteN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstLteN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstLteN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLteN(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "<="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstLteS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLteS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLteS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f<= %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstModDiv final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstModDiv(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f%% %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvurem %l %r)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_DIV; }
};
class AstModDivS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstModDivS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f%% %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvsmod %l %r)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_DIV; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstNeqWild final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNeqWild(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f!=? %r)"; }
    string emitC() override {
        ಆದರೆ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        ಆದರೆ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ "!=";
        }
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstPow final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstPow(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opPow(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f** %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_MUL * 10; }
};
class AstPowD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstPowD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_AstPowD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opPowD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f** %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "pow(%li,%ri)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL_DIV * 5; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstPowSS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstPowSS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f** %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_MUL * 10; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstPowSU final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstPowSU(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f** %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_MUL * 10; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstPowUS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstPowUS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f** %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_MUL * 10; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstReplicate final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
ಸಾರ್ವಜನಿಕ:
    AstReplicate(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        ಆದರೆ (lhsp) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(rhsp, Const)) {
                ಆದರೆ (constp->ಅಂಕೆ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ಅಂಕೆ().isNegative())) {
                    dtype_ಗಣ_ಬಿಟ್_Or_Logic_Sized(lhsp->ಅಗಲ(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } ಇಲ್ಲದಿದ್ದರೆ {
                    dtype_ಗಣ_ಬಿಟ್_Or_Logic_Sized(lhsp->ಅಗಲ() * constp->ಗಮ್ಯ_SInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, uint32_t rep_ಎಣಿಕೆ)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_ಎಣಿಕೆ}} {}
    ASTGEN_MEMBERS_AstReplicate;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opRepl(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f{%r{%k%l}}"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ "((_ repeat " + cvt_ಗಮ್ಯ_Str(ಅಗಲ() / lhsp()->ಅಗಲ()) + ") %l)";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 2; }
};
class AstReplicateN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // String replicate
ಸಾರ್ವಜನಿಕ:
    AstReplicateN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opReplN(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f{%r{%k%l}}"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 2; }
    bool stringFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSAnd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    ಸ್ಥಿರಾಂಕ bool m_propertyControl;  // Parser-generated property if/case branch conjunction
ಸಾರ್ವಜನಿಕ:
    AstSAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstSAnd;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {  // LCOV_EXCL_LINE
        ಹಿಂತಿರುಗಿ m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_propertyControl; }
};
class AstSIntersect final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
ಸಾರ್ವಜನಿಕ:
    AstSIntersect(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
    // LCOV_EXCL_STOP
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSOr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
ಸಾರ್ವಜನಿಕ:
    AstSOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstSOr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSThroughout final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
ಸಾರ್ವಜನಿಕ:
    AstSThroughout(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    // LCOV_EXCL_STOP
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSWithin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
ಸಾರ್ವಜನಿಕ:
    AstSWithin(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    // LCOV_EXCL_STOP
    bool is_Multi_ಚಕ್ರ_Sva() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    ಪೂರ್ಣಾಂಕ m_decl_El_ಅಗಲ;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    ಪೂರ್ಣಾಂಕ m_ಅಗಲ_Const;

ಸಾರ್ವಜನಿಕ:
    AstSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp, ಪೂರ್ಣಾಂಕ bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_decl_El_ಅಗಲ{1}
        , m_ಅಗಲ_Const{bitwidth} {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic_Sized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ bitwidth, ಪೂರ್ಣಾಂಕ bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_decl_El_ಅಗಲ{1}
        , m_ಅಗಲ_Const{bitwidth} {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& from, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಬಿಟ್_2) override {
        ಹೊರಹೋಗು.opSel(from, ಬಿಟ್_2.ಗಮ್ಯ_UInt() + ಅಗಲ_Const() - 1, ಬಿಟ್_2.ಗಮ್ಯ_UInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        ಹಿಂತಿರುಗಿ ಅಗಲ_Const() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "((_ extract %t %r) %l)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* otherp) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಅಗಲ_Const() == VN_DBG_AS(otherp, Sel)->ಅಗಲ_Const();
    }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Const() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಗಲ_Const; }
    ಶೂನ್ಯ ಅಗಲ_Const(ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) { m_ಅಗಲ_Const = ಮೌಲ್ಯ; }
    ಪೂರ್ಣಾಂಕ lsbConst() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_AS(lsbp(), Const)->ಗಮ್ಯ_SInt(); }
    ಪೂರ್ಣಾಂಕ msbConst() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ lsbConst() + ಅಗಲ_Const() - 1; }
    VNumRange& declRange() VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_declRange; }
    ಸ್ಥಿರಾಂಕ VNumRange& declRange() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_declRange; }
    ಶೂನ್ಯ declRange(ಸ್ಥಿರಾಂಕ VNumRange& flag) { m_declRange = flag; }
    ಪೂರ್ಣಾಂಕ decl_El_ಅಗಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_decl_El_ಅಗಲ; }
    ಶೂನ್ಯ decl_El_ಅಗಲ(ಪೂರ್ಣಾಂಕ flag) { m_decl_El_ಅಗಲ = flag; }
};
class Ast_ಸರಿಸು_L final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    Ast_ಸರಿಸು_L(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಂಕ setwidth = 0)
        : ASTGEN_SUPER_ಸರಿಸು_L(fl, lhsp, rhsp) {
        ಆದರೆ (setwidth) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = lhsp->dtypep()) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(dtypep->ಅಗಲ(), dtypep->ಅಗಲ_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ಸರಿಸು_L;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಸರಿಸು_L(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f<< %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        ಹಿಂತಿರುಗಿ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_ಸರಿಸು_LOvr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
ಸಾರ್ವಜನಿಕ:
    Ast_ಸರಿಸು_LOvr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಂಕ setwidth = 0)
        : ASTGEN_SUPER_ಸರಿಸು_LOvr(fl, lhsp, rhsp) {
        ಆದರೆ (setwidth) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(dtypep->ಅಗಲ(), dtypep->ಅಗಲ_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ಸರಿಸು_LOvr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಸರಿಸು_L(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f<< %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_ಸರಿಸು_R final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    Ast_ಸರಿಸು_R(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಂಕ setwidth = 0)
        : ASTGEN_SUPER_ಸರಿಸು_R(fl, lhsp, rhsp) {
        ಆದರೆ (setwidth) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = lhsp->dtypep()) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(dtypep->ಅಗಲ(), dtypep->ಅಗಲ_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ಸರಿಸು_R;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಸರಿಸು_R(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f>> %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        ಹಿಂತಿರುಗಿ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    // LHS size might be > output size, so don't want to force size
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_ಸರಿಸು_ROvr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
ಸಾರ್ವಜನಿಕ:
    Ast_ಸರಿಸು_ROvr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಂಕ setwidth = 0)
        : ASTGEN_SUPER_ಸರಿಸು_ROvr(fl, lhsp, rhsp) {
        ಆದರೆ (setwidth) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(dtypep->ಅಗಲ(), dtypep->ಅಗಲ_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ಸರಿಸು_ROvr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಸರಿಸು_R(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f>> %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    // LHS size might be > output size, so don't want to force size
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_ಸರಿಸು_RS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    Ast_ಸರಿಸು_RS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಂಕ setwidth = 0)
        : ASTGEN_SUPER_ಸರಿಸು_RS(fl, lhsp, rhsp) {
        ಆದರೆ (setwidth) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = lhsp->dtypep()) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(dtypep->ಅಗಲ(), dtypep->ಅಗಲ_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ಸರಿಸು_RS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಸರಿಸು_RS(lhs, rhs, lhsp()->ಅಗಲ_Min_V());
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f>>> %r)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvashr %l %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಸರಿಸು_RSOvr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
ಸಾರ್ವಜನಿಕ:
    Ast_ಸರಿಸು_RSOvr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, ಪೂರ್ಣಾಂಕ setwidth = 0)
        : ASTGEN_SUPER_ಸರಿಸು_RSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        ಆದರೆ (setwidth) {
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(dtypep->ಅಗಲ(), dtypep->ಅಗಲ_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ಸರಿಸು_RSOvr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಸರಿಸು_RS(lhs, rhs, lhsp()->ಅಗಲ_Min_V());
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f>>> %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSub final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstSub(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opSub(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f- %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvsub %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "-"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstSubD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
ಸಾರ್ವಜನಿಕ:
    AstSubD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_AstSubD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opSubD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "-"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstURandomRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Biop {
    // $urandom_range
ಸಾರ್ವಜನಿಕ:
    AstURandomRange(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtype_ಗಣ_UInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$urandom_range(%l, %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstEq(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    ಸ್ಥಿರ Ast_ಗಂಟು_Biop* newTyped(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp);
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opEq(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f== %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "=="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstEqCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstEqCase(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstEqCase;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f=== %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "=="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstEqD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
ಸಾರ್ವಜನಿಕ:
    AstEqD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstEqD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opEqD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "=="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstEqN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
ಸಾರ್ವಜನಿಕ:
    AstEqN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstEqN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opEqN(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "=="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstEqT final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // Equal (==) for data types
ಸಾರ್ವಜನಿಕ:
    AstEqT(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstEqT;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "=="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
};
class AstLogEq final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstLogEq(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f<-> %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "<->"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() + INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstNeq final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNeq(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    ಸ್ಥಿರ Ast_ಗಂಟು_Biop* newTyped(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp);
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opNeq(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f!= %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "!="; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (not (= %l %r)))"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstNeqCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNeqCase(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f!== %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "!="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstNeqD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
ಸಾರ್ವಜನಿಕ:
    AstNeqD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstNeqD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "!="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstNeqN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
ಸಾರ್ವಜನಿಕ:
    AstNeqN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstNeqN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "!="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
    bool stringFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstNeqT final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com {
    // Not-equal (!=) for data types
ಸಾರ್ವಜನಿಕ:
    AstNeqT(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstNeqT;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "!="; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_STR; }
};

// === AstNodeBiComAsv ===
class Ast_ಕೂಡಿಸು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    Ast_ಕೂಡಿಸು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ಕೂಡಿಸು(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_ಕೂಡಿಸು;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಕೂಡಿಸು(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f+ %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvadd %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "+"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಕೂಡಿಸು_D final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
ಸಾರ್ವಜನಿಕ:
    Ast_ಕೂಡಿಸು_D(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ಕೂಡಿಸು_D(fl, lhsp, rhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_Ast_ಕೂಡಿಸು_D;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಕೂಡಿಸು_D(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "+"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstAnd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opAnd(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f& %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvand %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "&"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಅಗಲ_Mismatch() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE;
};
class AstMul final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstMul(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opMul(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f* %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "*"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_MUL; }
};
class AstMulD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
ಸಾರ್ವಜನಿಕ:
    AstMulD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_AstMulD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opMulD(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "*"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstMulS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstMulS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opMulS(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f* %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * INSTR_ಎಣಿಕೆ_INT_MUL; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstOr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opOr(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f| %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvor %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "|"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಅಗಲ_Mismatch() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE;
};
class AstXor final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Bi_Com_Asv {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstXor(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.opXor(lhs, rhs);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f^ %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvxor %l %r)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "^"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Lclean && Rclean
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಅಗಲ_Mismatch() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class Ast_Dist_Chi_ವರ್ಗಫಲ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Biop {
ಸಾರ್ವಜನಿಕ:
    Ast_Dist_Chi_ವರ್ಗಫಲ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Dist_Chi_ವರ್ಗಫಲ(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_Chi_ವರ್ಗಫಲ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class Ast_Dist_ಘಾತೀಯ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Biop {
ಸಾರ್ವಜನಿಕ:
    Ast_Dist_ಘಾತೀಯ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Dist_ಘಾತೀಯ(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ಘಾತೀಯ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$dist_exponential(%l, %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class Ast_Dist_ಪಾಯಿಸನ್ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Biop {
ಸಾರ್ವಜನಿಕ:
    Ast_Dist_ಪಾಯಿಸನ್(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Dist_ಪಾಯಿಸನ್(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ಪಾಯಿಸನ್;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$dist_poisson(%l, %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Biop {
ಸಾರ್ವಜನಿಕ:
    AstDistT(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$dist_t(%l, %r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Sel {
    // @astgen makeDfgVertex
    ಶೂನ್ಯ init(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromp) {
        ಆದರೆ (fromp && VN_IS(fromp->dtypep()->skipRefp(), ಗಂಟು_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), ಗಂಟು_Array_DType)->subDTypep());
        }
    }

ಸಾರ್ವಜನಿಕ:
    AstArraySel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಪೂರ್ಣಾಂಕ ಬಿಟ್_2)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, ಬಿಟ್_2)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(select %l %r)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
};
class AstAssocSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Sel {
    ಶೂನ್ಯ init(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromp) {
        ಆದರೆ (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
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
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l%f[%r])"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "%li%k[%ri]"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(select %l %r)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ !isLValue(); }  // AssocSel creates on miss
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
};
class AstWildcardSel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Sel {
    ಶೂನ್ಯ init(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromp) {
        ಆದರೆ (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
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
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l%f[%r])"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "%li%k[%ri]"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
};
class Ast_ಶಬ್ದ_Sel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Sel {
    // Select a single word from a multi-word wide value
ಸಾರ್ವಜನಿಕ:
    Ast_ಶಬ್ದ_Sel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : ASTGEN_SUPER_ಶಬ್ದ_Sel(fl, fromp, bitp) {
        dtype_ಗಣ_UInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_Ast_ಶಬ್ದ_Sel;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};

// === AstNodeStream ===
class Ast_ಹರಿವು_L final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಹರಿವು {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    Ast_ಹರಿವು_L(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ಹರಿವು_L(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_ಹರಿವು_L;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f{ << %r %k{%l} }"; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ಹರಿವು_L(lhs, rhs);
    }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 2; }
};
class Ast_ಹರಿವು_R final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_ಹರಿವು {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    Ast_ಹರಿವು_R(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_ಹರಿವು_R(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_ಹರಿವು_R;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f{ >> %r %k{%l} }"; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(lhs);
    }
    string emitC() override { ಹಿಂತಿರುಗಿ isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Biop_D {
ಸಾರ್ವಜನಿಕ:
    AstAtan2D(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::atan2(lhs.ಗಮ್ಯ_Double(), rhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$atan2(%l,%r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "atan2(%li,%ri)"; }
};
class AstHypotD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Biop_D {
ಸಾರ್ವಜನಿಕ:
    AstHypotD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::hypot(lhs.ಗಮ್ಯ_Double(), rhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$hypot(%l,%r)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_CCall {
    // C++ function call
    VSelf_ಸೂಚಕ_ಪಠ್ಯ m_self_ಸೂಚಕ
        = VSelf_ಸೂಚಕ_ಪಠ್ಯ{VSelf_ಸೂಚಕ_ಪಠ್ಯ::ಖಾಲಿಗಳು_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
ಸಾರ್ವಜನಿಕ:
    AstCCall(File_ಸಾಲು* fl, AstCFunc* funcp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    ಸ್ಥಿರಾಂಕ VSelf_ಸೂಚಕ_ಪಠ್ಯ& self_ಸೂಚಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_self_ಸೂಚಕ; }
    ಶೂನ್ಯ self_ಸೂಚಕ(ಸ್ಥಿರಾಂಕ VSelf_ಸೂಚಕ_ಪಠ್ಯ& self_ಸೂಚಕ) { m_self_ಸೂಚಕ = self_ಸೂಚಕ; }
    string self_ಸೂಚಕ_Protect(bool useSelfForThis) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ self_ಸೂಚಕ().protect(useSelfForThis, protect());
    }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
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
    std::map<uint64_t, Ast_Cons_Array_ಅಂಶ*> m_itemps;  // The item at each index given

ಸಾರ್ವಜನಿಕ:
    AstConsArray(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* newDTypep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ cloneRelink() override;
    ಸಾಲೊಳಗೆ Ast_Cons_Array_ಅಂಶ* itemsp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep(uint64_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ;
    // Add the value of the element at the index, which must not be given yet
    ಶೂನ್ಯ valuep(uint64_t ಸೂಚ್ಯಂಕ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp);
    // Add the elements of the array 'arrayp', starting at the index
    ಶೂನ್ಯ splice_ಮೌಲ್ಯಗಳು(uint64_t ಸೂಚ್ಯಂಕ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arrayp);
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
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        ಹಿಂತಿರುಗಿ nullptr;
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
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
};
class Ast_Cons_ಸರತಿ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
ಸಾರ್ವಜನಿಕ:
    explicit Ast_Cons_ಸರತಿ(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Cons_ಸರತಿ(fl) {}
    ASTGEN_MEMBERS_Ast_Cons_ಸರತಿ;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), ಸರತಿ_DType));
        ಹಿಂತಿರುಗಿ nullptr;
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
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    // The value of the member called 'name'
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* memberValuep(ಸ್ಥಿರಾಂಕ string& name) ಸ್ಥಿರಾಂಕ;
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
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        ಹಿಂತಿರುಗಿ nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask_Ref {
    // A reference to a function
ಸಾರ್ವಜನಿಕ:
    ಸಾಲೊಳಗೆ AstFuncRef(File_ಸಾಲು* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class Ast_ವಿಧಾನ_Call final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
ಸಾರ್ವಜನಿಕ:
    Ast_ವಿಧಾನ_Call(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, VFlag_ಮಗು_DType, ಸ್ಥಿರಾಂಕ string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_ವಿಧಾನ_Call(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_ವಿಧಾನ_Call(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಸ್ಥಿರಾಂಕ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_ವಿಧಾನ_Call(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_Ast_ವಿಧಾನ_Call;
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
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool isImplicit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isImplicit; }
    ಶೂನ್ಯ isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isScoped; }
    ಶೂನ್ಯ isScoped(bool flag) { m_isScoped = flag; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
};
class AstTaskRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask_Ref {
    // A reference to a task
ಸಾರ್ವಜನಿಕ:
    ಸಾಲೊಳಗೆ AstTaskRef(File_ಸಾಲು* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtype_ಗಣ_Void();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class Ast_Sel_ಬಿಟ್ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
ಖಾಸಗಿ:
    VAccess m_access;  // Left hand side assignment
ಸಾರ್ವಜನಿಕ:
    Ast_Sel_ಬಿಟ್(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bitp)
        : ASTGEN_SUPER_Sel_ಬಿಟ್(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_Ast_Sel_ಬಿಟ್;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    VAccess access() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_access; }
    ಶೂನ್ಯ access(ಸ್ಥಿರಾಂಕ VAccess& flag) { m_access = flag; }
};
class Ast_Sel_ಹೊರತೆಗೆ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
ಸಾರ್ವಜನಿಕ:
    Ast_Sel_ಹೊರತೆಗೆ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* msbp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp)
        : ASTGEN_SUPER_Sel_ಹೊರತೆಗೆ(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_Ast_Sel_ಹೊರತೆಗೆ;
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
class Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Quadop {
    // Number of bits set in vector
ಸಾರ್ವಜನಿಕ:
    Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl1p)
        : ASTGEN_SUPER_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(fl, exprp, ctrl1p, ctrl1p->clone_ವೃಕ್ಷ_ಶುದ್ಧ(false),
                                 ctrl1p->clone_ವೃಕ್ಷ_ಶುದ್ಧ(false)) {
        dtype_ಗಣ_Int();
    }
    Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl1p, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl2p)
        : ASTGEN_SUPER_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_ವೃಕ್ಷ_ಶುದ್ಧ(false)) {
        dtype_ಗಣ_Int();
    }
    Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl1p, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl2p,
                 Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrl3p)
        : ASTGEN_SUPER_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtype_ಗಣ_Int();
    }
    ASTGEN_MEMBERS_Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಭಿವ್ಯಕ್ತಿ, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl1,
                       ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl2, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl3) override {
        ಹೊರಹೋಗು.op_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ಅಭಿವ್ಯಕ್ತಿ, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanThs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanFhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Ths() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Fhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Termop {
ಸಾರ್ವಜನಿಕ:
    explicit AstInferredDisable(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಸಮಯ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Termop {
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    Ast_ಸಮಯ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ VTimescale& timeunit)
        : ASTGEN_SUPER_ಸಮಯ(fl)
        , m_timeunit{timeunit} {
        dtype_ಗಣ_UInt64();
    }
    ASTGEN_MEMBERS_Ast_ಸಮಯ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_ಸಮಯ; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }
};
class Ast_ಸಮಯ_D final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Termop {
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    Ast_ಸಮಯ_D(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ VTimescale& timeunit)
        : ASTGEN_SUPER_ಸಮಯ_D(fl)
        , m_timeunit{timeunit} {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_Ast_ಸಮಯ_D;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_ಸಮಯ; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }
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
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs,
                       ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths) override {
        ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l %f? %r %k: %t)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(ite (__Vbool %l) %r %t)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // clean if e1 & e2 clean
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanThs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Propagates up
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Ths() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_BRANCH; }
};
class AstPutcN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
    // Verilog string.putc()
ಸಾರ್ವಜನಿಕ:
    AstPutcN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_AstPutcN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs,
                       ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths) override {
        ಹೊರಹೋಗು.opPutcN(lhs, rhs, ths);
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ "putc"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l.putc(%r,%t))"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanThs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Ths() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_ತುಂಡು_Sel final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
ಸಾರ್ವಜನಿಕ:
    Ast_ತುಂಡು_Sel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಸ್ಥಿರಾಂಕ VNumRange& declRange)
        : ASTGEN_SUPER_ತುಂಡು_Sel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_Ast_ತುಂಡು_Sel;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& from, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lo,
                       ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಗಲ) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanThs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Ths() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_declRange; }
    ಸ್ಥಿರಾಂಕ VNumRange& declRange() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_declRange; }
};
class AstSubstrN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Triop {
    // Verilog string.substr()
ಸಾರ್ವಜನಿಕ:
    AstSubstrN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs,
                       ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths) override {
        ಹೊರಹೋಗು.opSubstrN(lhs, rhs, ths);
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ "substr"; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l.substr(%r,%t))"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ ""; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanRhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanThs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Rhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Ths() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Triop {
ಸಾರ್ವಜನಿಕ:
    AstDistErlang(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class Ast_Dist_ಸಾಮಾನ್ಯ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Triop {
ಸಾರ್ವಜನಿಕ:
    Ast_Dist_ಸಾಮಾನ್ಯ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : ASTGEN_SUPER_Dist_ಸಾಮಾನ್ಯ(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ಸಾಮಾನ್ಯ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class Ast_Dist_ಏಕರೂಪ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Dist_Triop {
ಸಾರ್ವಜನಿಕ:
    Ast_Dist_ಏಕರೂಪ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp)
        : ASTGEN_SUPER_Dist_ಏಕರೂಪ(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ಏಕರೂಪ;
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ FmtType : ಪೂರ್ಣಾಂಕ { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

ಖಾಸಗಿ:
    // dist-ast-dump-suppress  // Part of name()
    ಸ್ಥಿರಾಂಕ FmtType m_fmtType;  // Operation type
ಸಾರ್ವಜನಿಕ:
    AstAtoN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtype_ಗಣ_Double() : dtype_ಗಣ_Integer();
    }
    ASTGEN_MEMBERS_AstAtoN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opAtoN(lhs, m_fmtType); }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಆಯ್ಕೆ (m_fmtType) {
        ಸ್ಥಿತಿ ATOI: ಹಿಂತಿರುಗಿ "atoi";
        ಸ್ಥಿತಿ ATOHEX: ಹಿಂತಿರುಗಿ "atohex";
        ಸ್ಥಿತಿ ATOOCT: ಹಿಂತಿರುಗಿ "atooct";
        ಸ್ಥಿತಿ ATOBIN: ಹಿಂತಿರುಗಿ "atobin";
        ಸ್ಥಿತಿ ATOREAL: ಹಿಂತಿರುಗಿ "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l." + name() + "()"; }
    string emitC() override {
        ಆಯ್ಕೆ (m_fmtType) {
        ಸ್ಥಿತಿ ATOI: ಹಿಂತಿರುಗಿ "VL_ATOI_N(%li, 10)";
        ಸ್ಥಿತಿ ATOHEX: ಹಿಂತಿರುಗಿ "VL_ATOI_N(%li, 16)";
        ಸ್ಥಿತಿ ATOOCT: ಹಿಂತಿರುಗಿ "VL_ATOI_N(%li, 8)";
        ಸ್ಥಿತಿ ATOBIN: ಹಿಂತಿರುಗಿ "VL_ATOI_N(%li, 2)";
        ಸ್ಥಿತಿ ATOREAL: ಹಿಂತಿರುಗಿ "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    FmtType fmtType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fmtType; }
};
class Ast_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D(fl, lhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_Ast_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$bitstoreal(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_CVT_D_Q(%li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Eliminated before matters
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Eliminated before matters
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstCCast final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Cast to C-based data type
    ಪೂರ್ಣಾಂಕ m_ಗಾತ್ರ;

ಸಾರ್ವಜನಿಕ:
    AstCCast(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಪೂರ್ಣಾಂಕ setwidth, ಪೂರ್ಣಾಂಕ minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_ಗಾತ್ರ = setwidth;
        ಆದರೆ (setwidth) {
            ಆದರೆ (minwidth == -1) minwidth = setwidth;
            dtype_ಗಣ_ಬಿಟ್_Unsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_ಗಾತ್ರ = ಅಗಲ();
    }
    ASTGEN_MEMBERS_AstCCast;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$_CAST(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Special cased in V3Cast
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಗಾತ್ರ_2() == VN_DBG_AS(samep, CCast)->ಗಾತ್ರ_2();
    }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    //
    ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಗಾತ್ರ; }
};
class AstCLog2 final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstCLog2(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtype_ಗಣ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opCLog2(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$clog2(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 16; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstCastWrap final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
ಸಾರ್ವಜನಿಕ:
    AstCastWrap(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
};
class Ast_ಎಣಿಕೆ_Ones final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    Ast_ಎಣಿಕೆ_Ones(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ಎಣಿಕೆ_Ones(fl, lhsp) {
        dtype_ಗಣ_Int();
    }
    ASTGEN_MEMBERS_Ast_ಎಣಿಕೆ_Ones;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_ಎಣಿಕೆ_Ones(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$countones(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 16; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstCvtPackString final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
ಸಾರ್ವಜನಿಕ:
    AstCvtPackString(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstExtend final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstExtend(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(ಅಗಲ, ಅಗಲ_Min ? ಅಗಲ_Min : ಅಗಲ, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ "((_ zero_extend " + cvt_ಗಮ್ಯ_Str(ಅಗಲ() - lhsp()->ಅಗಲ()) + ") %l)";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ false;  // Because the EXTEND operator self-casts
    }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
};
class AstExtendS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstExtendS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಪೂರ್ಣಾಂಕ ಅಗಲ)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic_Sized(ಅಗಲ, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.opExtendS(lhs, lhsp()->ಅಗಲ_Min_V());
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ "((_ sign_extend " + cvt_ಗಮ್ಯ_Str(ಅಗಲ() - lhsp()->ಅಗಲ()) + ") %l)";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ false;  // Because the EXTEND operator self-casts
    }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool signedFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstFEof final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstFEof(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$feof(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 16; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು* filep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ lhsp(); }
};
class AstFGetC final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstFGetC(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtype_ಗಣ_Logic2_State_Unsized(
            32, v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::MATCHES_ಅಗಲ ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { ಹಿಂತಿರುಗಿ "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 64; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ಗಂಟು* filep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ lhsp(); }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstISToRD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // $itor where lhs is signed
ಸಾರ್ವಜನಿಕ:
    AstISToRD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_AstISToRD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opISToRD(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$itor($signed(%l))"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_ಪದಗಳು() override { ಹಿಂತಿರುಗಿ true; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstIToRD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // $itor where lhs is unsigned
ಸಾರ್ವಜನಿಕ:
    AstIToRD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_AstIToRD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opIToRD(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$itor(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_ITOR_D_%lq(%lw, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstIsUnbounded final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // True if is unbounded ($)
ಸಾರ್ವಜನಿಕ:
    AstIsUnbounded(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ&) override {
        // Any constant isn't unbounded
        ಹೊರಹೋಗು.ಗಣ_Zero();
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstIsUnknown final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // True if any unknown bits
ಸಾರ್ವಜನಿಕ:
    AstIsUnknown(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opIsUnknown(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstLenN final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Length of a string
ಸಾರ್ವಜನಿಕ:
    AstLenN(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtype_ಗಣ_Int();
    }
    ASTGEN_MEMBERS_AstLenN;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opLenN(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LEN_IN(%li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstLogNot final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
ಸಾರ್ವಜನಿಕ:
    AstLogNot(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opLogNot(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f(! %l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "!"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool fromProperty() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fromProperty; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class Ast_Most_ಗಣ_ಬಿಟ್_P1 final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
ಸಾರ್ವಜನಿಕ:
    Ast_Most_ಗಣ_ಬಿಟ್_P1(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Most_ಗಣ_ಬಿಟ್_P1(fl, lhsp) {
        dtype_ಗಣ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_Most_ಗಣ_ಬಿಟ್_P1;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_Most_ಗಣ_ಬಿಟ್_P1(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$mostsetbitp1(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 16; }
};
class AstNToI final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // String to any-size integral
ಸಾರ್ವಜನಿಕ:
    AstNToI(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opNToI(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "'(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstNegate final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNegate(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opNegate(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f(- %l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvneg %l)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "-"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstNegateD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    AstNegateD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtype_ಗಣ_Double();
    }
    ASTGEN_MEMBERS_AstNegateD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opNegateD(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "-"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool doubleFlavor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstNot final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstNot(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opNot(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f(~ %l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "(bvnot %l)"; }
    string emitSimpleOperator() override { ಹಿಂತಿರುಗಿ "~"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಅಗಲ_Mismatch() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE;
};
class AstNullCheck final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Return LHS after checking that LHS is non-null
ಸಾರ್ವಜನಿಕ:
    AstNullCheck(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }  // Rarely executes
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ fileline() == samep->fileline(); }
};
class AstOneHot final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstOneHot(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstOneHot;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opOneHot(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$onehot(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 4; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstOneHot0 final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstOneHot0(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opOneHot0(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$onehot0(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs() * 3; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstPostDec final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
ಸಾರ್ವಜನಿಕ:
    AstPostDec(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
ಸಾರ್ವಜನಿಕ:
    AstPostInc(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
ಸಾರ್ವಜನಿಕ:
    AstPreDec(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
ಸಾರ್ವಜನಿಕ:
    AstPreInc(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
ಸಾರ್ವಜನಿಕ:
    AstRToIRoundS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtype_ಗಣ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opRToIRoundS(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        ಹಿಂತಿರುಗಿ isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstRToIS final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // $rtoi(lhs)
ಸಾರ್ವಜನಿಕ:
    AstRToIS(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtype_ಗಣ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opRToIS(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$rtoi(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_RTOI_I_D(%li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Eliminated before matters
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Eliminated before matters
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    Ast_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು(fl, lhsp) {
        dtype_ಗಣ_UInt64();
    }
    ASTGEN_MEMBERS_Ast_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$realtobits(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_CVT_Q_D(%li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Eliminated before matters
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Eliminated before matters
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_DBL; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstRedAnd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstRedAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opRedAnd(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f(& %l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstRedOr final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstRedOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opRedOr(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f(| %l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstRedXor final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // @astgen makeDfgVertex
ಸಾರ್ವಜನಿಕ:
    AstRedXor(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtype_ಗಣ_ಬಿಟ್_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.opRedXor(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f(^ %l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ w = lhsp()->ಅಗಲ();
        ಹಿಂತಿರುಗಿ (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1 + V3_ಸಂಖ್ಯೆ::log2b(ಅಗಲ()); }
};
class AstResizeLValue final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
ಸಾರ್ವಜನಿಕ:
    AstResizeLValue(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
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
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(lhs);
        ಹೊರಹೋಗು.isSigned(false);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Eliminated before matters
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }  // Eliminated before matters
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಸಮಯ_Import final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
ಸಾರ್ವಜನಿಕ:
    Ast_ಸಮಯ_Import(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ಸಮಯ_Import(fl, lhsp) {}
    ASTGEN_MEMBERS_Ast_ಸಮಯ_Import;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }
};
class Ast_ಗಮ್ಯ_ಅಧೋ_N final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // string.tolower()
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಮ್ಯ_ಅಧೋ_N(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ಗಮ್ಯ_ಅಧೋ_N(fl, lhsp) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_Ast_ಗಮ್ಯ_ಅಧೋ_N;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_ಗಮ್ಯ_ಅಧೋ_N(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l.tolower()"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_TOLOWER_NN(%li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_ಗಮ್ಯ_String_N final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಮ್ಯ_String_N(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ಗಮ್ಯ_String_N(fl, lhsp) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_Ast_ಗಮ್ಯ_String_N;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        ಹಿಂತಿರುಗಿ isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class Ast_ಗಮ್ಯ_ಊರ್ಧ್ವ_N final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Uniop {
    // string.toupper()
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಮ್ಯ_ಊರ್ಧ್ವ_N(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ಗಮ್ಯ_ಊರ್ಧ್ವ_N(fl, lhsp) {
        dtype_ಗಣ_String();
    }
    ASTGEN_MEMBERS_Ast_ಗಮ್ಯ_ಊರ್ಧ್ವ_N;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override { ಹೊರಹೋಗು.op_ಗಮ್ಯ_ಊರ್ಧ್ವ_N(lhs); }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%l.toupper()"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "VL_TOUPPER_NN(%li)"; }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
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
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(lhs);
        ಹೊರಹೋಗು.isSigned(false);
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool cleanLhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }  // Eliminated before matters
    bool ಗಾತ್ರ_Matters_Lhs() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }  // Eliminated before matters
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool isSystemFunc() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAcosD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::acos(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$acos(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "acos(%li)"; }
};
class AstAcoshD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAcoshD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::acosh(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$acosh(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "acosh(%li)"; }
};
class AstAsinD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAsinD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::asin(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$asin(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "asin(%li)"; }
};
class AstAsinhD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAsinhD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::asinh(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$asinh(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "asinh(%li)"; }
};
class AstAtanD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAtanD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::atan(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$atan(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "atan(%li)"; }
};
class AstAtanhD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstAtanhD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::atanh(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$atanh(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "atanh(%li)"; }
};
class AstCeilD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstCeilD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::ceil(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$ceil(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "ceil(%li)"; }
};
class AstCosD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstCosD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::ಕೋಜ್ಯಾ(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$cos(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "cos(%li)"; }
};
class AstCoshD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstCoshD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::cosh(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$cosh(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "cosh(%li)"; }
};
class AstExpD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstExpD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::ಘಾತಾಂಕ(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$exp(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "exp(%li)"; }
};
class AstFloorD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstFloorD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::floor(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$floor(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "floor(%li)"; }
};
class AstLog10D final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstLog10D(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::log10(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$log10(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "log10(%li)"; }
};
class AstLogD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstLogD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::ಲಘುಗಣಕ(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$ln(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "log(%li)"; }
};
class AstSinD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstSinD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::ಜ್ಯಾ(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$sin(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "sin(%li)"; }
};
class AstSinhD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstSinhD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::sinh(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$sinh(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "sinh(%li)"; }
};
class AstSqrtD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstSqrtD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::ವರ್ಗಮೂಲ(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$sqrt(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "sqrt(%li)"; }
};
class AstTanD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstTanD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::tan(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$tan(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "tan(%li)"; }
};
class AstTanhD final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_System_Uniop_D {
ಸಾರ್ವಜನಿಕ:
    AstTanhD(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) override {
        ಹೊರಹೋಗು.ಗಣ_Double(std::tanh(lhs.ಗಮ್ಯ_Double()));
    }
    string emitVerilog() override { ಹಿಂತಿರುಗಿ "%f$tanh(%l)"; }
    string emitC() override { ಹಿಂತಿರುಗಿ "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
ಸಾರ್ವಜನಿಕ:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    ಸಾಲೊಳಗೆ AstVarRef(File_ಸಾಲು* fl, AstVar* varp, ಸ್ಥಿರಾಂಕ VAccess& access);
    ಸಾಲೊಳಗೆ AstVarRef(File_ಸಾಲು* fl, Ast_ಗಂಟು_Module* classOrPackagep, AstVar* varp,
                     ಸ್ಥಿರಾಂಕ VAccess& access);
    // This form only allowed post-link (see above)
    ಸಾಲೊಳಗೆ AstVarRef(File_ಸಾಲು* fl, AstVarScope* varscp, ಸ್ಥಿರಾಂಕ VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
    ಸಾಲೊಳಗೆ string name() ಸ್ಥಿರಾಂಕ override;  // * = Var name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    ಸಾಲೊಳಗೆ bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ AstVarRef* samep) ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool sameNoLvalue(ಸ್ಥಿರಾಂಕ AstVarRef* samep) ಸ್ಥಿರಾಂಕ;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstVarXRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
ಸಾರ್ವಜನಿಕ:
    AstVarXRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& dotted, ಸ್ಥಿರಾಂಕ VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    ಸಾಲೊಳಗೆ AstVarXRef(File_ಸಾಲು* fl, AstVar* varp, ಸ್ಥಿರಾಂಕ string& dotted, ಸ್ಥಿರಾಂಕ VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ std::string& name) override { m_name = name; }  // * = Var name
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string dotted() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dotted; }
    ಶೂನ್ಯ dotted(ಸ್ಥಿರಾಂಕ string& dotted) { m_dotted = dotted; }
    string inlinedDots() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_inlinedDots; }
    ಶೂನ್ಯ inlinedDots(ಸ್ಥಿರಾಂಕ string& flag) { m_inlinedDots = flag; }
    bool ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ; }
    ಶೂನ್ಯ ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ(ಸ್ಥಿರಾಂಕ bool flag) { m_ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ = flag; }
    bool readOnlyModport() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_readOnlyModport; }
    ಶೂನ್ಯ readOnlyModport(ಸ್ಥಿರಾಂಕ bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ಹೊರಹೋಗು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಅಗಲ_Instrs(); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        ಹಿಂತಿರುಗಿ (self_ಸೂಚಕ() == asamep->self_ಸೂಚಕ() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
