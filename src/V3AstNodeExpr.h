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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ASTNODEEXPR_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ASTNODEEXPR_H_

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3AST_H_
#ਗਲਤੀ "Use V3Ast.h as the include"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ਜੇ_ਅੰਤ

// === Abstract base node types (AstNode*) =====================================

class Ast_ਗੰਢ_ਵਿਅੰਜਕ VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // An expression tree node
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_ਵਿਅੰਜਕ(VNType t, File_ਸਤਰ* fl)
        : Ast_ਗੰਢ{t, fl} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_ਵਿਅੰਜਕ;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    // Every expression must have a data type after V3Width
    bool hasDType() ਅਚਲ override final VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਆਭਾਸੀ string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    ਆਭਾਸੀ string emitC() = 0;
    ਆਭਾਸੀ string emitSMT() ਅਚਲ { ਵਾਪਸ ""; };
    ਆਭਾਸੀ string emitSimpleOperator() { ਵਾਪਸ ""; }  // "" means not ok to use
    ਆਭਾਸੀ bool emit_Check_Max_ਸ਼ਬਦ() { ਵਾਪਸ false; }  // Check VL_MULS_MAX_WORDS
    ਆਭਾਸੀ bool clean_ਬਾਹਰਲਾ() ਅਚਲ = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    ਆਭਾਸੀ bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ { ਵਾਪਸ false; }
    ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* get_VAccess_ਨਿਸ਼ਾਨਾ_Recurse() ਅਚਲ;
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* get_VAccess_ਨਿਸ਼ਾਨਾ_Recurse() {
        ਵਾਪਸ const_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(this)->get_VAccess_ਨਿਸ਼ਾਨਾ_Recurse());
    }
    VAccess getVAccessRecurse() ਅਚਲ;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() ਅਚਲ { ਵਾਪਸ getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_ਰੁੱਖ() ਅਚਲ;
    // Return base var (or const) nodep dereferences
    Ast_ਗੰਢ* ਅਧਾਰ_Fromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    ਸਤਰਅੰਦਰ Ast_Stmt_ਵਿਅੰਜਕ* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    ਆਭਾਸੀ ਅਚਲ ਅੱਖਰ* ਚੌੜਾਈ_Mismatch() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string ਨਮੂਨਾ_String(uint32_t ਡੂੰਘਾਈ = 0) ਅਚਲ;
    bool isFourstateOrNoDtype() ਅਚਲ {
        ਜੇ (!dtypep()) ਵਾਪਸ true;
        ਵਾਪਸ dtypep()->skipRefp()->isFourstate();
    }
    ਸਥਿਰ bool isFourstateOrNoDtype(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p) {
        ਵਾਪਸ expr1p->isFourstateOrNoDtype();
    }
    ਸਥਿਰ bool isFourstateOrNoDtype(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p,
                                     ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr2p) {
        ਵਾਪਸ expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    ਸਥਿਰ bool isFourstateOrNoDtype(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p,
                                     ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr2p,
                                     ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr3p) {
        ਵਾਪਸ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    ਸਥਿਰ bool isFourstateOrNoDtype(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p,
                                     ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr2p,
                                     ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr3p,
                                     ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr4p) {
        ਵਾਪਸ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_ਗੰਢ_Biop VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Biop(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    ਆਭਾਸੀ ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) = 0;
    ਆਭਾਸੀ bool cleanLhs() ਅਚਲ = 0;  // True if LHS must have extra upper bits zero
    ਆਭਾਸੀ bool cleanRhs() ਅਚਲ = 0;  // True if RHS must have extra upper bits zero
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Lhs() ਅਚਲ = 0;  // True if output result depends on lhs size
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Rhs() ਅਚਲ = 0;  // True if output result depends on rhs size
    ਆਭਾਸੀ bool doubleFlavor() ਅਚਲ { ਵਾਪਸ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    ਆਭਾਸੀ bool signedFlavor() ਅਚਲ { ਵਾਪਸ false; }
    ਆਭਾਸੀ bool stringFlavor() ਅਚਲ { ਵਾਪਸ false; }  // N flavor of nodes with both flavors?
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;

ਨਿਜੀ:
    bool getPurityRecurse() ਅਚਲ { ਵਾਪਸ lhsp()->is_ਸ਼ੁੱਧ() && rhsp()->is_ਸ਼ੁੱਧ(); }
};
class Ast_ਗੰਢ_Bi_Com VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Binary expr with commutative properties
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Bi_Com(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhs, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhs)
        : Ast_ਗੰਢ_Biop{t, fl, lhs, rhs} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Bi_Com;
};
class Ast_ਗੰਢ_Bi_Com_Asv VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
    // Binary expr with commutative & associative properties
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Bi_Com_Asv(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhs, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhs)
        : Ast_ਗੰਢ_Bi_Com{t, fl, lhs, rhs} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Bi_Com_Asv;
};
class Ast_ਗੰਢ_Dist_Biop VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    Ast_ਗੰਢ_Dist_Biop(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : Ast_ਗੰਢ_Biop{t, fl, lhsp, rhsp} {
        dtype_ਸੈੱਟ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Dist_Biop;
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL_TRIG; }
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
};
class Ast_ਗੰਢ_Sel VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Sel(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp)
        : Ast_ਗੰਢ_Biop{t, fl, fromp, bitp} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Sel;
    ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_Const() ਅਚਲ;
};
class Ast_ਗੰਢ_ਧਾਰਾ VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_ਧਾਰਾ(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : Ast_ਗੰਢ_Biop{t, fl, lhsp, rhsp} {
        ਜੇ (lhsp->dtypep()) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(lhsp->ਚੌੜਾਈ(), lhsp->ਚੌੜਾਈ_Min(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_ਧਾਰਾ;
};
class Ast_ਗੰਢ_System_Biop_D VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    Ast_ਗੰਢ_System_Biop_D(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : Ast_ਗੰਢ_Biop{t, fl, lhsp, rhsp} {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_Ast_ਗੰਢ_System_Biop_D;
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL_TRIG; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਗੰਢ_CCall VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_CCall(VNType t, File_ਸਤਰ* fl, AstCFunc* funcp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp = nullptr)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl}
        , m_funcp{funcp} {
        ਜੋੜੋ_Argsp(argsp);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_CCall;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_CALL; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ Ast_ਗੰਢ_CCall* ਅਚਲ asamep = VN_DBG_AS(samep, ਗੰਢ_CCall);
        ਵਾਪਸ (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override;
    bool isOutputter() override { ਵਾਪਸ !is_ਸ਼ੁੱਧ(); }
    AstCFunc* funcp() ਅਚਲ { ਵਾਪਸ m_funcp; }
    ਖਾਲੀ funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() ਅਚਲ { ਵਾਪਸ m_argTypes; }
    ਖਾਲੀ argTypes(ਅਚਲ string& str) { m_argTypes = str; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ final override { ਵਾਪਸ true; }
    bool superReference() ਅਚਲ { ਵਾਪਸ m_superReference; }
    ਖਾਲੀ superReference(bool flag) { m_superReference = flag; }
};
class Ast_ਗੰਢ_Cons VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Construct a value of an aggregate type
    ਮਿੱਤਰ class Ast_ਗੰਢ_ਵਿਅੰਜਕ;  // For m_knownConstantTree
    mutable bool m_ਗਿਆਤ_Constant_ਰੁੱਖ = false;  // Known to be 'this->isConstantTree() == true'

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Cons(VNType t, File_ਸਤਰ* fl)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Cons;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ final override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ final override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
};
class Ast_ਗੰਢ_FTask_Ref VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_ਗੰਢ_FTask_Ref;  // Gen pointers before other members for performance
ਨਿਜੀ:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_FTask_Ref(VNType t, File_ਸਤਰ* fl, ਅਚਲ string& name, AstArg* argsp)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl}
        , m_name{name} {
        ਜੋੜੋ_Argsp(argsp);
    }

ਜਨਤਕ:
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    bool isGateOptimizable() ਅਚਲ override;
    string dotted() ਅਚਲ { ਵਾਪਸ m_dotted; }  // * = Scope name or ""
    string inlinedDots() ਅਚਲ { ਵਾਪਸ m_inlinedDots; }
    ਖਾਲੀ inlinedDots(ਅਚਲ string& flag) { m_inlinedDots = flag; }
    Ast_ਗੰਢ_FTask* taskp() ਅਚਲ { ਵਾਪਸ m_taskp; }  // [After Link] Pointer to variable
    ਖਾਲੀ taskp(Ast_ਗੰਢ_FTask* taskp) { m_taskp = taskp; }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    ਖਾਲੀ dotted(ਅਚਲ string& name) { m_dotted = name; }
    Ast_ਗੰਢ_Module* classOrPackagep() ਅਚਲ { ਵਾਪਸ m_classOrPackagep; }
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* nodep) { m_classOrPackagep = nodep; }
    bool ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ() ਅਚਲ { ਵਾਪਸ m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ; }
    ਖਾਲੀ ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ(ਅਚਲ bool flag) { m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ = flag; }
    bool pli() ਅਚਲ { ਵਾਪਸ m_pli; }
    ਖਾਲੀ pli(bool flag) { m_pli = flag; }
    bool superReference() ਅਚਲ { ਵਾਪਸ m_superReference; }
    ਖਾਲੀ superReference(bool flag) { m_superReference = flag; }
    bool is_ਸ਼ੁੱਧ() override;
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ m_taskp && m_taskp->isUnlikely(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ Ast_ਗੰਢ_FTask_Ref* ਅਚਲ asamep = VN_DBG_AS(samep, ਗੰਢ_FTask_Ref);
        ਵਾਪਸ taskp() == asamep->taskp()  //
               && classOrPackagep() == asamep->classOrPackagep()  //
               && name() == asamep->name()  //
               && dotted() == asamep->dotted()  //
               && inlinedDots() == asamep->inlinedDots()  //
               && pli() == asamep->pli()  //
               && ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ() == asamep->ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ()
               && superReference() == asamep->superReference();
    }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ final override { V3ERROR_NA_RETURN(true); }

ਨਿਜੀ:
    bool getPurityRecurse() ਅਚਲ;
};
class Ast_ਗੰਢ_Pre_Sel VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Pre_Sel(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsp)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Pre_Sel;
    // METHODS
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ final override { V3ERROR_NA_RETURN(true); }
    bool is_ਸ਼ੁੱਧ() override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;

ਨਿਜੀ:
    bool getPurityRecurse() ਅਚਲ {
        ਵਾਪਸ fromp()->is_ਸ਼ੁੱਧ() && rhsp()->is_ਸ਼ੁੱਧ() && (!thsp() || thsp()->is_ਸ਼ੁੱਧ());
    }
};
class Ast_ਗੰਢ_Quadop VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Quadop(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsp,
                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* fhsp)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    ਆਭਾਸੀ ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs,
                               ਅਚਲ V3_ਸੰਖਿਆ& ths, ਅਚਲ V3_ਸੰਖਿਆ& fhs)
        = 0;
    ਆਭਾਸੀ bool cleanLhs() ਅਚਲ = 0;  // True if LHS must have extra upper bits zero
    ਆਭਾਸੀ bool cleanRhs() ਅਚਲ = 0;  // True if RHS must have extra upper bits zero
    ਆਭਾਸੀ bool cleanThs() ਅਚਲ = 0;  // True if THS must have extra upper bits zero
    ਆਭਾਸੀ bool cleanFhs() ਅਚਲ = 0;  // True if THS must have extra upper bits zero
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Lhs() ਅਚਲ = 0;  // True if output result depends on lhs size
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Rhs() ਅਚਲ = 0;  // True if output result depends on rhs size
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Ths() ਅਚਲ = 0;  // True if output result depends on ths size
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Fhs() ਅਚਲ = 0;  // True if output result depends on ths size
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;

ਨਿਜੀ:
    bool getPurityRecurse() ਅਚਲ {
        ਵਾਪਸ lhsp()->is_ਸ਼ੁੱਧ() && rhsp()->is_ਸ਼ੁੱਧ() && thsp()->is_ਸ਼ੁੱਧ() && fhsp()->is_ਸ਼ੁੱਧ();
    }
};
class Ast_ਗੰਢ_Termop VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Terminal operator -- an operator with no "inputs"
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Termop(VNType t, File_ਸਤਰ* fl)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Termop;
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਗੰਢ_Triop VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Triop(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsp)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Triop;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    // Set out to evaluation of a AstConst'ed
    ਆਭਾਸੀ ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs,
                               ਅਚਲ V3_ਸੰਖਿਆ& ths)
        = 0;
    ਆਭਾਸੀ bool cleanLhs() ਅਚਲ = 0;  // True if LHS must have extra upper bits zero
    ਆਭਾਸੀ bool cleanRhs() ਅਚਲ = 0;  // True if RHS must have extra upper bits zero
    ਆਭਾਸੀ bool cleanThs() ਅਚਲ = 0;  // True if THS must have extra upper bits zero
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Lhs() ਅਚਲ = 0;  // True if output result depends on lhs size
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Rhs() ਅਚਲ = 0;  // True if output result depends on rhs size
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Ths() ਅਚਲ = 0;  // True if output result depends on ths size
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;

ਨਿਜੀ:
    bool getPurityRecurse() ਅਚਲ {
        ਵਾਪਸ lhsp()->is_ਸ਼ੁੱਧ() && rhsp()->is_ਸ਼ੁੱਧ() && thsp()->is_ਸ਼ੁੱਧ();
    }
};
class Ast_ਗੰਢ_Dist_Triop VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Triop {
ਜਨਤਕ:
    Ast_ਗੰਢ_Dist_Triop(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp,
                     Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsp)
        : Ast_ਗੰਢ_Triop{t, fl, lhsp, rhsp, thsp} {
        dtype_ਸੈੱਟ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Dist_Triop;
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanThs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Ths() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL_TRIG; }
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs,
                       ਅਚਲ V3_ਸੰਖਿਆ& ths) override {
        V3ERROR_NA;
    }
};
class Ast_ਗੰਢ_Uniop VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Uniop(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Uniop;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    // Set out to evaluation of a AstConst'ed lhs
    ਆਭਾਸੀ ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) = 0;
    ਆਭਾਸੀ bool cleanLhs() ਅਚਲ = 0;
    ਆਭਾਸੀ bool ਮਾਪ_Matters_Lhs() ਅਚਲ = 0;  // True if output result depends on lhs size
    ਆਭਾਸੀ bool doubleFlavor() ਅਚਲ { ਵਾਪਸ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    ਆਭਾਸੀ bool signedFlavor() ਅਚਲ { ਵਾਪਸ false; }
    ਆਭਾਸੀ bool stringFlavor() ਅਚਲ { ਵਾਪਸ false; }  // N flavor of nodes with both flavors?
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
};
class Ast_ਗੰਢ_System_Uniop_D VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
ਜਨਤਕ:
    Ast_ਗੰਢ_System_Uniop_D(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : Ast_ਗੰਢ_Uniop{t, fl, lhsp} {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_Ast_ਗੰਢ_System_Uniop_D;
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL_TRIG; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਗੰਢ_Var_Ref VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Var_Ref;  // Gen pointers before other members for performance
ਨਿਜੀ:
    VAccess m_access;  // Left hand side assignment
    VSelf_ਸੂਚਕ_ਪਾਠ m_self_ਸੂਚਕ
        = VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::ਖਾਲੀ_ਥਾਵਾਂ_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Var_Ref(VNType t, File_ਸਤਰ* fl, AstVar* varp, ਅਚਲ VAccess& access)
        : Ast_ਗੰਢ_ਵਿਅੰਜਕ{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

ਜਨਤਕ:
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    VAccess access() ਅਚਲ { ਵਾਪਸ m_access; }
    ਖਾਲੀ access(ਅਚਲ VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_varp; }  // [After Link] Pointer to variable
    ਖਾਲੀ varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_ਗੰਢ*>(varp));
    }
    AstVarScope* varScopep() ਅਚਲ { ਵਾਪਸ m_varScopep; }
    ਖਾਲੀ varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    ਅਚਲ VSelf_ਸੂਚਕ_ਪਾਠ& self_ਸੂਚਕ() ਅਚਲ { ਵਾਪਸ m_self_ਸੂਚਕ; }
    ਖਾਲੀ self_ਸੂਚਕ(ਅਚਲ VSelf_ਸੂਚਕ_ਪਾਠ& self_ਸੂਚਕ) { m_self_ਸੂਚਕ = self_ਸੂਚਕ; }
    string self_ਸੂਚਕ_Protect(bool useSelfForThis) ਅਚਲ {
        ਵਾਪਸ self_ਸੂਚਕ().protect(useSelfForThis, protect());
    }
    Ast_ਗੰਢ_Module* classOrPackagep() ਅਚਲ { ਵਾਪਸ m_classOrPackagep; }
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : ਜਨਤਕ Ast_ਗੰਢ {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
ਜਨਤਕ:
    AstArg(File_ਸਤਰ* fl, ਅਚਲ std::string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ std::string& name) override { m_name = name; }
    bool ਖਾਲੀ_ਥਾਵਾਂ_Connect_No_ਅਗਲਾ() ਅਚਲ { ਵਾਪਸ !exprp() && name() == "" && !nextp(); }
};
class AstWith final : ਜਨਤਕ Ast_ਗੰਢ {
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
ਨਿਜੀ:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::ਸੈੱਟ<std::string> m_restricted_ਨਾਂ;

ਜਨਤਕ:
    AstWith(File_ਸਤਰ* fl, AstLambdaArgRef* ਸੂਚਕਾਂਕ_Arg_Refp, AstLambdaArgRef* ਮੁੱਲ_Arg_Refp,
            Ast_ਗੰਢ* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->ਸੂਚਕਾਂਕ_Arg_Refp(ਸੂਚਕਾਂਕ_Arg_Refp);
        this->ਮੁੱਲ_Arg_Refp(ਮੁੱਲ_Arg_Refp);
        ਜੋੜੋ_Exprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!ਸੂਚਕਾਂਕ_Arg_Refp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!ਮੁੱਲ_Arg_Refp());  // varp needed to know lambda's arg dtype
        ਵਾਪਸ nullptr;
    }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ restricted(bool flag) { m_restricted = flag; }
    bool restricted() ਅਚਲ { ਵਾਪਸ m_restricted; }
    bool validated() ਅਚਲ { ਵਾਪਸ m_validated; }
    ਖਾਲੀ validated(bool flag) { m_validated = flag; }
    ਖਾਲੀ ਜੋੜੋ_Restricted_Name(ਅਚਲ std::string& name) { m_restricted_ਨਾਂ.ਪਾਓ(name); }
    ਅਚਲ std::ਸੈੱਟ<std::string>& restricted_ਨਾਂ() ਅਚਲ { ਵਾਪਸ m_restricted_ਨਾਂ; }
    // True if 'name' binds into the randomize() target class.
    bool name_Resolves_ਮੰਜ਼ਿਲ_ਨਿਸ਼ਾਨਾ(ਅਚਲ std::string& name) ਅਚਲ {
        ਵਾਪਸ !m_restricted || m_restricted_ਨਾਂ.ਗਿਣਤੀ_2(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbort_ਕਿਸਮ m_ਕਿਸਮ{VAbort_ਕਿਸਮ::ACCEPT_ON};

ਜਨਤਕ:
    AstAbortOn(File_ਸਤਰ* fl, VAbort_ਕਿਸਮ ਕਿਸਮ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_ਕਿਸਮ{ਕਿਸਮ} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    VAbort_ਕਿਸਮ ਕਿਸਮ() ਅਚਲ { ਵਾਪਸ m_ਕਿਸਮ; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Addr_ਦਾ_CFunc final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

ਜਨਤਕ:
    Ast_Addr_ਦਾ_CFunc(File_ਸਤਰ* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_Addr_ਦਾ_CFunc(fl)
        , m_funcp{funcp} {
        dtypep(ਲੱਭੋ_CHandle_DType());
    }

    ASTGEN_MEMBERS_Ast_Addr_ਦਾ_CFunc;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    AstCFunc* funcp() ਅਚਲ { ਵਾਪਸ m_funcp; }
};
class Ast_Attr_ਦਾ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
ਜਨਤਕ:
    Ast_Attr_ਦਾ(File_ਸਤਰ* fl, VAttrType attrtype, Ast_ਗੰਢ* fromp = nullptr,
              Ast_ਗੰਢ_ਵਿਅੰਜਕ* dimp = nullptr)
        : ASTGEN_SUPER_Attr_ਦਾ(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_Ast_Attr_ਦਾ;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    VAttrType attrType() ਅਚਲ { ਵਾਪਸ m_attrType; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ਅਚਲ bool m_ਸ਼ੁੱਧ;  // Pure optimizable

    ਖਾਲੀ init(ਅਚਲ string& ਪਾਠ, ਪੂਰਨ setwidth) {
        ਜੇ (!ਪਾਠ.ਖਾਲੀ_ਥਾਵਾਂ()) ਜੋੜੋ(ਪਾਠ);
        ਜੇ (setwidth) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(setwidth, VSigning::UNSIGNED);
        } ਨਹੀਂਤਾਂ {
            dtype_ਸੈੱਟ_Void();  // Caller to override if necessary
        }
    }

ਜਨਤਕ:
    class ਸ਼ੁੱਧ {};
    explicit AstCExpr(File_ਸਤਰ* fl, ਅਚਲ string& ਪਾਠ = "", ਪੂਰਨ setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_ਸ਼ੁੱਧ{false} {
        init(ਪਾਠ, setwidth);
    }
    AstCExpr(File_ਸਤਰ* fl, ਸ਼ੁੱਧ, ਅਚਲ string& ਪਾਠ = "", ਪੂਰਨ setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_ਸ਼ੁੱਧ{true} {
        init(ਪਾਠ, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ m_ਸ਼ੁੱਧ; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ m_ਸ਼ੁੱਧ; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ m_ਸ਼ੁੱਧ; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    // Add some text, or a node to this expression
    ਖਾਲੀ ਜੋੜੋ(ਅਚਲ std::string& ਪਾਠ) { ਜੋੜੋ_Nodesp(new Ast_ਪਾਠ{fileline(), ਪਾਠ}); }
    ਖਾਲੀ ਜੋੜੋ(Ast_ਗੰਢ* nodep) { ਜੋੜੋ_Nodesp(nodep); }
};
class AstCExprUser final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ਅਚਲ bool m_ਸ਼ੁੱਧ;  // Whether the function is pure
ਜਨਤਕ:
    class ਸ਼ੁੱਧ {};
    explicit AstCExprUser(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_ਸ਼ੁੱਧ{false} {}
    AstCExprUser(File_ਸਤਰ* fl, ਸ਼ੁੱਧ)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_ਸ਼ੁੱਧ{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ m_ਸ਼ੁੱਧ; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ m_ਸ਼ੁੱਧ; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ m_ਸ਼ੁੱਧ; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    // Add some text, or a node to this expression
    ਖਾਲੀ ਜੋੜੋ(ਅਚਲ std::string& ਪਾਠ) { ਜੋੜੋ_Nodesp(new Ast_ਪਾਠ{fileline(), ਪਾਠ}); }
    ਖਾਲੀ ਜੋੜੋ(Ast_ਗੰਢ* nodep) { ਜੋੜੋ_Nodesp(nodep); }
};
class AstCFuncHard final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
ਜਨਤਕ:
    AstCFuncHard(File_ਸਤਰ* fl, VCFunction function, Ast_ਗੰਢ_ਵਿਅੰਜਕ* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        ਜੋੜੋ_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ function().ascii(); }
    bool is_ਸ਼ੁੱਧ() override {
        ਜੇ (!m_purity.isCached()) m_purity.ਸੈੱਟ(getPurity());
        ਵਾਪਸ m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    VCFunction function() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_function; }

ਨਿਜੀ:
    bool getPurity();
};
class AstCMethodHard final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_ਵਿਧੀ;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
ਜਨਤਕ:
    AstCMethodHard(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, VCMethod ਵਿਧੀ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_ਵਿਧੀ{ਵਿਧੀ} {
        this->fromp(fromp);
        ਜੋੜੋ_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ ਵਿਧੀ().ascii(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstCMethodHard* ਅਚਲ asamep = VN_DBG_AS(samep, CMethodHard);
        ਵਾਪਸ (m_ਵਿਧੀ == asamep->m_ਵਿਧੀ);
    }
    bool is_ਸ਼ੁੱਧ() override {
        ਜੇ (!m_purity.isCached()) m_purity.ਸੈੱਟ(getPurity());
        ਵਾਪਸ m_purity.get();
    }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ m_ਵਿਧੀ.isClean(); }
    bool usePtr() ਅਚਲ { ਵਾਪਸ m_usePtr; }
    ਖਾਲੀ usePtr(bool flag) { m_usePtr = flag; }
    VCMethod ਵਿਧੀ() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_ਵਿਧੀ; }
    ਖਾਲੀ ਵਿਧੀ(VCMethod ਮੁੱਲ) { m_ਵਿਧੀ = ਮੁੱਲ; }

ਨਿਜੀ:
    bool getPurity();
};
class AstCReset final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Reset variable at startup
    ਅਚਲ bool m_constructing;  // Previously cleared by constructor
ਜਨਤਕ:
    AstCReset(File_ਸਤਰ* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!VN_IS(backp(), ਗੰਢ_ਸੌਂਪੋ));  // V3Emit* assumption
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() ਅਚਲ { ਵਾਪਸ m_constructing; }
};
class AstCast final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
ਜਨਤਕ:
    AstCast(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        ਬੱਚਾ_DTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { ਵਾਪਸ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ dtypep() ? dtypep() : ਬੱਚਾ_DTypep(); }
};
class AstCastParse final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
ਜਨਤਕ:
    AstCastParse(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { ਵਾਪਸ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cast_ਮਾਪ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
ਜਨਤਕ:
    Ast_Cast_ਮਾਪ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_Cast_ਮਾਪ(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_Ast_Cast_ਮਾਪ;
    string emitVerilog() override { ਵਾਪਸ "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
ਜਨਤਕ:
    AstCellArrayRef(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        ਜੋੜੋ_Selp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
ਜਨਤਕ:
    AstCellRef(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* cellp, Ast_ਗੰਢ* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
ਜਨਤਕ:
    AstClassOrPackageRef(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        ਜੋੜੋ_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_ਗੰਢ_Module* classOrPackageSkipp(ਅਚਲ bool doRefs = true) ਅਚਲ;
    Ast_ਗੰਢ* classOrPackageNodep() ਅਚਲ { ਵਾਪਸ m_classOrPackageNodep; }
    ਖਾਲੀ classOrPackageNodep(Ast_ਗੰਢ* nodep) { m_classOrPackageNodep = nodep; }
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_ਗੰਢ*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A constant
    V3_ਸੰਖਿਆ m_ਅੰਕ;  // Constant value
    ਖਾਲੀ init_With_ਸੰਖਿਆ() {
        ਜੇ (m_ਅੰਕ.isDouble()) {
            dtype_ਸੈੱਟ_Double();
        } ਨਹੀਂਤਾਂ ਜੇ (m_ਅੰਕ.isString()) {
            dtype_ਸੈੱਟ_String();
        } ਨਹੀਂਤਾਂ {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(m_ਅੰਕ.ਚੌੜਾਈ(), (m_ਅੰਕ.sized() ? 0 : m_ਅੰਕ.ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit()),
                                      VSigning::fromBool(m_ਅੰਕ.isSigned()), m_ਅੰਕ.isAnyXZ());
        }
        m_ਅੰਕ.nodep(this);
    }

ਜਨਤਕ:
    ~AstConst() override;
    AstConst(File_ਸਤਰ* fl, ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{ਅੰਕ} {
        init_With_ਸੰਖਿਆ();
    }
    class Widthed_ਮੁੱਲ {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, Widthed_ਮੁੱਲ, ਪੂਰਨ ਚੌੜਾਈ, uint32_t ਮੁੱਲ)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, ਚੌੜਾਈ, ਮੁੱਲ) {  // Need () constructor
        init_With_ਸੰਖਿਆ();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_ਸਤਰ* fl, DTyped, ਅਚਲ Ast_ਗੰਢ_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{this, nodedtypep} {
        init_With_ਸੰਖਿਆ();
    }
    class String_ਮੰਜ਼ਿਲ_Parse {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, String_ਮੰਜ਼ਿਲ_Parse, ਅਚਲ ਅੱਖਰ* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{this, V3_ਸੰਖਿਆ::Verilog_ਸੰਖਿਆ_Literal{}, sourcep} {
        init_With_ਸੰਖਿਆ();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, VerilogStringLiteral, ਅਚਲ string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{this, V3_ਸੰਖਿਆ::VerilogStringLiteral{}, str} {
        init_With_ਸੰਖਿਆ();
    }
    AstConst(File_ਸਤਰ* fl, uint32_t ਅੰਕ)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, 32, ਅੰਕ) {  // Need () constructor
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(m_ਅੰਕ.ਚੌੜਾਈ(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, Unsized32, uint32_t ਅੰਕ)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, 32, ਅੰਕ) {  // Need () constructor
        m_ਅੰਕ.ਚੌੜਾਈ(32, false);
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, m_ਅੰਕ.ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, Signed32, int32_t ਅੰਕ)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, 32, ਅੰਕ) {  // Need () constructor
        m_ਅੰਕ.ਚੌੜਾਈ(32, true);
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, m_ਅੰਕ.ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, Unsized64, uint64_t ਅੰਕ)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, 64, 0) {  // Need () constructor
        m_ਅੰਕ.ਸੈੱਟ_Quad(ਅੰਕ);
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, SizedEData, uint64_t ਅੰਕ)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_ਅੰਕ.ਸੈੱਟ_Quad(ਅੰਕ);
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, RealDouble, ਦੁੱਗਣਾ ਅੰਕ)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, 64) {  // Need () constructor
        m_ਅੰਕ.ਸੈੱਟ_Double(ਅੰਕ);
        dtype_ਸੈੱਟ_Double();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_ਸਤਰ* fl, String, ਅਚਲ string& ਅੰਕ)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{this, V3_ਸੰਖਿਆ::String{}, ਅੰਕ} {
        dtype_ਸੈੱਟ_String();
    }
    class ਦੋਆਧਾਰੀ_ਅੰਕ_False {};
    AstConst(File_ਸਤਰ* fl, ਦੋਆਧਾਰੀ_ਅੰਕ_False)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, 1, 0) {  // Need () constructor
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring {};
    AstConst(File_ਸਤਰ* fl, ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, 1, 0) {  // Need () constructor
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        File_ਸਤਰ* ਅਚਲ newfl = new File_ਸਤਰ{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class ਦੋਆਧਾਰੀ_ਅੰਕ_True {};
    AstConst(File_ਸਤਰ* fl, ਦੋਆਧਾਰੀ_ਅੰਕ_True, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ(this, 1, on) {  // Need () constructor
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    class All0 {};
    AstConst(File_ਸਤਰ* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{this, V3_ਸੰਖਿਆ::Verilog_ਸੰਖਿਆ_Literal{}, "'0"} {
        init_With_ਸੰਖਿਆ();
        fl->warnOff(V3_Error_ਕੋਡ::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_ਸਤਰ* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{this, V3_ਸੰਖਿਆ::Verilog_ਸੰਖਿਆ_Literal{}, "'1"} {
        init_With_ਸੰਖਿਆ();
        fl->warnOff(V3_Error_ਕੋਡ::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_ਸਤਰ* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{this, V3_ਸੰਖਿਆ::Null{}} {
        dtypep(ਲੱਭੋ_Null_DType());
        init_With_ਸੰਖਿਆ();
    }
    class One_ਕਦਮ {};
    AstConst(File_ਸਤਰ* fl, One_ਕਦਮ)
        : ASTGEN_SUPER_Const(fl)
        , m_ਅੰਕ{this, V3_ਸੰਖਿਆ::One_ਕਦਮ{}} {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(64, VSigning::UNSIGNED);
        init_With_ਸੰਖਿਆ();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ ਅੰਕ().ascii(); }  // * = Value
    ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅੰਕ; }  // * = Value
    V3_ਸੰਖਿਆ& ਅੰਕ() { ਵਾਪਸ m_ਅੰਕ; }  // * = Value
    string origParamName() ਅਚਲ;
    ਖਾਲੀ origParamName(ਅਚਲ string& name);
    uint32_t ਮੰਜ਼ਿਲ_UInt() ਅਚਲ { ਵਾਪਸ ਅੰਕ().ਮੰਜ਼ਿਲ_UInt(); }
    int32_t ਮੰਜ਼ਿਲ_SInt() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ ਅੰਕ().ਮੰਜ਼ਿਲ_SInt(); }
    uint64_t ਮੰਜ਼ਿਲ_UQuad() ਅਚਲ { ਵਾਪਸ ਅੰਕ().ਮੰਜ਼ਿਲ_UQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstConst* ਅਚਲ sp = VN_DBG_AS(samep, Const);
        ਵਾਪਸ ਅੰਕ().isCaseEq(sp->ਅੰਕ());
    }
    ਖਾਲੀ cloneRelink() override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(m_ਅੰਕ.nodep() && m_ਅੰਕ.nodep() != this);
        ਵਾਪਸ nullptr;
    }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool isEqAllOnes() ਅਚਲ { ਵਾਪਸ ਅੰਕ().isEqAllOnes(ਚੌੜਾਈ()); }
    bool isEqAllOnesV() ਅਚਲ { ਵਾਪਸ ਅੰਕ().isEqAllOnes(ਚੌੜਾਈ_Min_V()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_ਮੁੱਲ_Type(ਅਚਲ AstConst* samep) ਅਚਲ;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    ਸਥਿਰ AstConst* parseParamLiteral(File_ਸਤਰ* fl, ਅਚਲ string& literal);
};
class AstConstraintRef final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

ਜਨਤਕ:
    AstConstraintRef(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(ਲੱਭੋ_Constraint_Ref_DType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() ਅਚਲ override VL_MT_STABLE;
    AstConstraint* constrp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_constrp; }
    ਖਾਲੀ constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_ਗੰਢ_Module* classOrPackagep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_classOrPackagep; }
    // TODO: this setter is unused
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ final override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
ਨਿਜੀ:
    ਅਚਲ bool m_ਉਲਟ;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    ਅਚਲ ਪੂਰਨ m_ਖੰਡ_ਮਾਪ;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    ਅਚਲ ਪੂਰਨ m_dst_Element_ਬਿੱਟ;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    ਅਚਲ ਪੂਰਨ m_src_Element_ਬਿੱਟ;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

ਜਨਤਕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_DType* dtp, bool ਉਲਟ,
                       ਪੂਰਨ ਖੰਡ_ਮਾਪ, ਪੂਰਨ dst_Element_ਬਿੱਟ, ਪੂਰਨ src_Element_ਬਿੱਟ)
        : ASTGEN_SUPER_Cvt_Array_ਮੰਜ਼ਿਲ_Array(fl)
        , m_ਉਲਟ{ਉਲਟ}
        , m_ਖੰਡ_ਮਾਪ{ਖੰਡ_ਮਾਪ}
        , m_dst_Element_ਬਿੱਟ{dst_Element_ਬਿੱਟ}
        , m_src_Element_ਬਿੱਟ{src_Element_ਬਿੱਟ} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਉਲਟ() ਅਚਲ { ਵਾਪਸ m_ਉਲਟ; }
    ਪੂਰਨ ਖੰਡ_ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_ਖੰਡ_ਮਾਪ; }
    ਪੂਰਨ dst_Element_ਬਿੱਟ() ਅਚਲ { ਵਾਪਸ m_dst_Element_ਬਿੱਟ; }
    ਪੂਰਨ src_Element_ਬਿੱਟ() ਅਚਲ { ਵਾਪਸ m_src_Element_ਬਿੱਟ; }
};
class Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
ਜਨਤਕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Array_ਮੰਜ਼ਿਲ_Packed(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
ਜਨਤਕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Packed_ਮੰਜ਼ਿਲ_Array(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_ਕਤਾਰ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
ਜਨਤਕ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_ਕਤਾਰ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_ਮੰਜ਼ਿਲ_ਕਤਾਰ(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_ਕਤਾਰ;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { ਵਾਪਸ "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
ਜਨਤਕ:
    Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstDist final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
ਜਨਤਕ:
    AstDist(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Dist_ਵਸਤੂ* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        ਜੋੜੋ_Itemsp(itemsp);
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { ਵਾਪਸ "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }  // NA
};
class Ast_Dist_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_is_ਸਾਰਾ = false;  // True for weight ':/', false for ':='
ਜਨਤਕ:
    Ast_Dist_ਵਸਤੂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rangep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* weightp)
        : ASTGEN_SUPER_Dist_ਵਸਤੂ(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_Ast_Dist_ਵਸਤੂ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { ਵਾਪਸ "%l "s + (m_is_ਸਾਰਾ ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }  // NA
    ਖਾਲੀ is_ਸਾਰਾ(bool flag) { m_is_ਸਾਰਾ = flag; }
    bool is_ਸਾਰਾ() ਅਚਲ { ਵਾਪਸ m_is_ਸਾਰਾ; }
};
class AstDot final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    ਅਚਲ bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
ਜਨਤਕ:
    AstDot(File_ਸਤਰ* fl, bool colon, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* newIfPkg(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* packageOrClassp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        ਜੇ (!packageOrClassp) ਵਾਪਸ rhsp;
        ਵਾਪਸ new AstDot{fl, true, packageOrClassp, rhsp};
    }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool colon() ਅਚਲ { ਵਾਪਸ m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
ਜਨਤਕ:
    explicit Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ(fl) {}
    ASTGEN_MEMBERS_Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { ਵਾਪਸ "{}"; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Enum_ਵਸਤੂ_Ref final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ = false;  // Contains gen block reference
ਜਨਤਕ:
    Ast_Enum_ਵਸਤੂ_Ref(File_ਸਤਰ* fl, Ast_Enum_ਵਸਤੂ* itemp, Ast_ਗੰਢ_Module* classOrPackagep)
        : ASTGEN_SUPER_Enum_ਵਸਤੂ_Ref(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_Ast_Enum_ਵਸਤੂ_Ref;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ itemp() ? itemp()->name() : m_name; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ Ast_Enum_ਵਸਤੂ_Ref* ਅਚਲ sp = VN_DBG_AS(samep, Enum_ਵਸਤੂ_Ref);
        ਵਾਪਸ itemp() == sp->itemp() && dotted() == sp->dotted()
               && ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ() == sp->ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ();
    }
    Ast_Enum_ਵਸਤੂ* itemp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_itemp; }
    ਖਾਲੀ itemp(Ast_Enum_ਵਸਤੂ* nodep) { m_itemp = nodep; }
    string dotted() ਅਚਲ { ਵਾਪਸ m_dotted; }
    ਖਾਲੀ dotted(ਅਚਲ string& dotted) { m_dotted = dotted; }
    bool ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ() ਅਚਲ { ਵਾਪਸ m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ; }
    ਖਾਲੀ ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ(ਅਚਲ bool flag) { m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_ਗੰਢ_Module* classOrPackagep() ਅਚਲ { ਵਾਪਸ m_classOrPackagep; }
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_ਵਿਅੰਜਕ_Stmt final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
ਨਿਜੀ:
    bool m_has_ਨਤੀਜਾ = true;  // Returns result via resultp()

ਜਨਤਕ:
    Ast_ਵਿਅੰਜਕ_Stmt(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp)
        : ASTGEN_SUPER_ਵਿਅੰਜਕ_Stmt(fl) {
        ਜੋੜੋ_Stmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_ਵਿਅੰਜਕ_Stmt;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override {
        ਜੇ (Ast_ਗੰਢ::afterCommentp(stmtsp())) ਵਾਪਸ false;
        ਵਾਪਸ resultp()->is_ਸ਼ੁੱਧ();
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    bool has_ਨਤੀਜਾ() ਅਚਲ { ਵਾਪਸ m_has_ਨਤੀਜਾ; }
    ਖਾਲੀ has_ਨਤੀਜਾ(bool flag) { m_has_ਨਤੀਜਾ = flag; }
};
class AstFError final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
ਜਨਤਕ:
    AstFError(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { ਵਾਪਸ "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 64; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFOpen final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
ਜਨਤਕ:
    AstFOpen(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filenamep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* modep)
        : ASTGEN_SUPER_FOpen(fl) {
        this->filenamep(filenamep);
        this->modep(modep);
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpen;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$fopen"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFOpenMcd final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op2 := filenamep : AstNodeExpr
ਜਨਤਕ:
    AstFOpenMcd(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$fopen"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFRead final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
ਜਨਤਕ:
    AstFRead(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* memp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* startp,
             Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtype_ਸੈੱਟ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: makes output
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFRewind final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := filep : Optional[AstNodeExpr]
ਜਨਤਕ:
    AstFRewind(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFScanF final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_ਪਾਠ;

ਜਨਤਕ:
    AstFScanF(File_ਸਤਰ* fl, ਅਚਲ string& ਪਾਠ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_ਪਾਠ{ਪਾਠ} {
        ਜੋੜੋ_Exprsp(exprsp);
        this->filep(filep);
        dtype_ਸੈੱਟ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_ਪਾਠ; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: makes output
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ ਪਾਠ() == VN_DBG_AS(samep, FScanF)->ਪਾਠ();
    }
    string ਪਾਠ() ਅਚਲ { ਵਾਪਸ m_ਪਾਠ; }  // * = Text to display
    ਖਾਲੀ ਪਾਠ(ਅਚਲ string& ਪਾਠ) { m_ਪਾਠ = ਪਾਠ; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFSeek final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
ਜਨਤਕ:
    AstFSeek(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿੱਥ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->ਵਿੱਥ(ਵਿੱਥ);
        this->operation(operation);
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: makes output
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFTell final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
ਜਨਤਕ:
    AstFTell(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFalling final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
ਜਨਤਕ:
    AstFalling(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { ਵਾਪਸ "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFell final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ਜਨਤਕ:
    AstFell(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Sen_ਰੁੱਖ* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { ਵਾਪਸ "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFuture final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ਜਨਤਕ:
    AstFuture(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Sen_ਰੁੱਖ* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { ਵਾਪਸ "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstGatePin final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
ਜਨਤਕ:
    AstGatePin(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { ਵਾਪਸ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Get_Initial_Random_ਬੀਜ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $get_initial_random_seed()
ਜਨਤਕ:
    explicit Ast_Get_Initial_Random_ਬੀਜ(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_Get_Initial_Random_ਬੀਜ(fl) {
        dtype_ਸੈੱਟ_Int();
    }
    ASTGEN_MEMBERS_Ast_Get_Initial_Random_ਬੀਜ;
    string emitVerilog() override { ਵਾਪਸ "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ true; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstImplication final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

ਨਿਜੀ:
    ਅਚਲ bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    ਅਚਲ bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

ਜਨਤਕ:
    AstImplication(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() ਅਚਲ { ਵਾਪਸ m_isOverlapped; }
    bool isFollowedBy() ਅਚਲ { ਵਾਪਸ m_isFollowedBy; }
};
class AstInside final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
ਜਨਤਕ:
    AstInside(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        ਜੋੜੋ_Itemsp(itemsp);
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { ਵਾਪਸ "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }  // NA
};
class AstInsideRange final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
ਜਨਤਕ:
    AstInsideRange(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { ਵਾਪਸ "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newAndFromInside(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp);
};
class AstLambdaArgRef final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_ਸੂਚਕਾਂਕ;  // Index, not value

ਜਨਤਕ:
    AstLambdaArgRef(File_ਸਤਰ* fl, ਅਚਲ string& name, bool ਸੂਚਕਾਂਕ)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_ਸੂਚਕਾਂਕ{ਸੂਚਕਾਂਕ} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    string emitVerilog() override { ਵਾਪਸ name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    bool ਸੂਚਕਾਂਕ() ਅਚਲ { ਵਾਪਸ m_ਸੂਚਕਾਂਕ; }
    bool is_ਵਿਅੰਜਕ_Coverage_Eligible() ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_ਮੇਲ_Masked final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
ਜਨਤਕ:
    ਸਤਰਅੰਦਰ Ast_ਮੇਲ_Masked(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_Ast_ਮੇਲ_Masked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { ਵਾਪਸ "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    ਸਥਿਰ uint32_t fold(ਅਚਲ V3_ਸੰਖਿਆ& lhs, AstVar* ਮੇਲ_Varp);
};
class AstMatches final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
ਜਨਤਕ:
    AstMatches(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { ਵਾਪਸ "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstMemberSel final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
ਜਨਤਕ:
    AstMemberSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, VFlag_ਬੱਚਾ_DType, ਅਚਲ string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    VAccess access() ਅਚਲ { ਵਾਪਸ m_access; }
    ਖਾਲੀ access(ਅਚਲ VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    AstVar* varp() ਅਚਲ { ਵਾਪਸ m_varp; }
    ਖਾਲੀ varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
ਜਨਤਕ:
    AstNewCopy(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { ਵਾਪਸ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
};
class AstNewDynamic final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
ਜਨਤਕ:
    AstNewDynamic(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* sizep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { ਵਾਪਸ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
};
class AstPExpr final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
ਜਨਤਕ:
    explicit AstPExpr(File_ਸਤਰ* fl, AstBegin* bodyp, Ast_ਗੰਢ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_ਸਤਰ* fl, AstBegin* bodyp, Ast_ਗੰਢ_Stmt* finalp, Ast_ਗੰਢ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
};
class AstParseHolder final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
ਜਨਤਕ:
    explicit AstParseHolder(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

ਜਨਤਕ:
    AstParseRef(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nullptr,
                Ast_ਗੰਢ_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstParseRef* ਅਚਲ asamep = VN_DBG_AS(samep, ParseRef);
        ਵਾਪਸ m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

ਜਨਤਕ:
    AstPast(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ticksp = nullptr,
            Ast_Sen_ਰੁੱਖ* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() ਅਚਲ { ਵਾਪਸ m_propertyTiming; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstPatMember final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
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

ਜਨਤਕ:
    AstPatMember(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhssp, Ast_ਗੰਢ* keyp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        ਜੋੜੋ_Lhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { ਵਾਪਸ lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 2; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    bool isConcat() ਅਚਲ { ਵਾਪਸ m_isConcat; }
    ਖਾਲੀ isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() ਅਚਲ { ਵਾਪਸ m_isDefault; }
    ਖਾਲੀ isDefault(bool flag) { m_isDefault = flag; }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਕੁੰਜੀ_Exprp() ਅਚਲ { ਵਾਪਸ varrefp() ? varrefp() : VN_CAST(keyp(), ਗੰਢ_ਵਿਅੰਜਕ); }
};
class Ast_ਨਮੂਨਾ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
ਜਨਤਕ:
    Ast_ਨਮੂਨਾ(File_ਸਤਰ* fl, Ast_ਗੰਢ* itemsp)
        : ASTGEN_SUPER_ਨਮੂਨਾ(fl) {
        ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_ਨਮੂਨਾ;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ dtypep() ? dtypep() : ਬੱਚਾ_DTypep(); }
};
class Ast_ਨਮੂਨਾ_ਤਾਰਾ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Pattern wildcard: ".*"
ਜਨਤਕ:
    explicit Ast_ਨਮੂਨਾ_ਤਾਰਾ(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਨਮੂਨਾ_ਤਾਰਾ(fl) {}
    ASTGEN_MEMBERS_Ast_ਨਮੂਨਾ_ਤਾਰਾ;
    string emitVerilog() override { ਵਾਪਸ ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਨਮੂਨਾ_Var final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
ਜਨਤਕ:
    Ast_ਨਮੂਨਾ_Var(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_ਨਮੂਨਾ_Var(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ਨਮੂਨਾ_Var;
    string emitVerilog() override { ਵਾਪਸ ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_name == VN_DBG_AS(samep, ਨਮੂਨਾ_Var)->m_name;
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
};
class AstPropAlways final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    ਅਚਲ bool m_isStrong = false;  // s_always
ਜਨਤਕ:
    AstPropAlways(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* propp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* loBoundp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { V3ERROR_NA_RETURN(0); }
    bool isStrong() ਅਚਲ { ਵਾਪਸ m_isStrong; }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstRand final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    ਅਚਲ bool m_urandom;  // $urandom vs $random
ਜਨਤਕ:
    class Reset {};
    AstRand(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_ਸਤਰ* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtype_ਸੈੱਟ_UInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override {
        ਵਾਪਸ seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        ਜੇ (seedp()) {
            ਜੇ (urandom()) ਵਾਪਸ "VL_URANDOM_SEEDED_%nq%lq(%li)";
            ਵਾਪਸ "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        ਜੇ (isWide()) ਵਾਪਸ "VL_RANDOM_%nq(%nw, %P)";
        ਵਾਪਸ "VL_RANDOM_%nq()";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ !seedp(); }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool combinable(ਅਚਲ AstRand* samep) ਅਚਲ {
        ਵਾਪਸ !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() ਅਚਲ { ਵਾਪਸ m_urandom; }
};
class AstRandRNG final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
ਜਨਤਕ:
    AstRandRNG(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { ਵਾਪਸ "%f$rngrandom()"; }
    string emitC() override {
        ਵਾਪਸ isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstRising final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
ਜਨਤਕ:
    AstRising(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { ਵਾਪਸ "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstRose final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ਜਨਤਕ:
    AstRose(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Sen_ਰੁੱਖ* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { ਵਾਪਸ "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSClocked final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
ਜਨਤਕ:
    AstSClocked(File_ਸਤਰ* fl, Ast_Sen_ਵਸਤੂ* sensesp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
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
    ਅਚਲ bool m_unbounded = false;  // True when the upper bound is $
ਜਨਤਕ:
    // Exact [*N]
    AstSConsRep(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { V3ERROR_NA_RETURN(0); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {  // LCOV_EXCL_LINE
        ਵਾਪਸ m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() ਅਚਲ { ਵਾਪਸ m_unbounded; }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSEventually final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
ਜਨਤਕ:
    explicit AstSEventually(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
ਜਨਤਕ:
    explicit AstSExpr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* preExprp, Ast_ਗੰਢ_Stmt* delayp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_ਸਤਰ* fl, Ast_ਗੰਢ_Stmt* delayp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSFormatArg final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

ਜਨਤਕ:
    AstSFormatArg(File_ਸਤਰ* fl, VFormatAttr formatAttr, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$sformatarg"; }
    string emitVerilog() override { ਵਾਪਸ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && ਪਹਿਲਾ_Abovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        ਵਾਪਸ nullptr;
    }
    VFormatAttr formatAttr() ਅਚਲ { ਵਾਪਸ m_formatAttr; }
    ਖਾਲੀ formatAttr(ਅਚਲ VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    ਸਥਿਰ VFormatAttr formatAttrDefauled(ਅਚਲ AstSFormatArg* nodep, ਅਚਲ Ast_ਗੰਢ_DType* dtypep);
};
class AstSFormatF final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_ਪਾਠ;
    bool m_hidden;  // Under display, etc
    bool m_ਵਿਅੰਜਕ_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    ਅਚਲ ਅੱਖਰ m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
ਜਨਤਕ:
    class ਵਿਅੰਜਕ_Format {};
    AstSFormatF(File_ਸਤਰ* fl, ਅਚਲ string& ਪਾਠ, bool hidden, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp,
                ਅੱਖਰ missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_ਪਾਠ{ਪਾਠ}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtype_ਸੈੱਟ_String();
        ਜੋੜੋ_Exprsp(exprsp);
    }
    AstSFormatF(File_ਸਤਰ* fl, ਵਿਅੰਜਕ_Format, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp, ਅੱਖਰ missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_ਪਾਠ{""}
        , m_hidden{hidden}
        , m_ਵਿਅੰਜਕ_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtype_ਸੈੱਟ_String();
        ਜੋੜੋ_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_ਪਾਠ; }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_ਪਾਠ = name; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ ਪਾਠ() == VN_DBG_AS(samep, SFormatF)->ਪਾਠ();
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$sformatf"; }
    string ਪਾਠ() ਅਚਲ { ਵਾਪਸ m_ਪਾਠ; }  // * = Text to display
    ਖਾਲੀ ਪਾਠ(ਅਚਲ string& ਪਾਠ) { m_ਪਾਠ = ਪਾਠ; }
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(ਪਾਠ() != "" && ਵਿਅੰਜਕ_Format_2());  // Expr format means no literal format
        ਵਾਪਸ nullptr;
    }
    bool formatScopeTracking() ਅਚਲ {  // Track scopeNamep();  Ok if false positive
        ਜੇ (ਵਿਅੰਜਕ_Format_2() || name().ਲੱਭੋ("%m") != string::npos || name().ਲੱਭੋ("%M") != string::npos)
            ਵਾਪਸ true;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ fmtp = VN_CAST(exprp, Const)) {
                ਜੇ (fmtp->ਅੰਕ().isFromString()) {
                    ਅਚਲ string str = fmtp->ਅੰਕ().ਮੰਜ਼ਿਲ_String();
                    ਜੇ (str.ਲੱਭੋ("%m") != string::npos || str.ਲੱਭੋ("%M") != string::npos)
                        ਵਾਪਸ true;
                }
            }
        }
        ਵਾਪਸ false;
    }
    bool hidden() ਅਚਲ { ਵਾਪਸ m_hidden; }
    ਖਾਲੀ hidden(bool flag) { m_hidden = flag; }
    bool ਵਿਅੰਜਕ_Format_2() ਅਚਲ { ਵਾਪਸ m_ਵਿਅੰਜਕ_Format; }
    ਖਾਲੀ ਵਿਅੰਜਕ_Format_2(bool flag) { m_ਵਿਅੰਜਕ_Format = flag; }
    bool optionalFormat() ਅਚਲ { ਵਾਪਸ m_optionalFormat; }
    ਖਾਲੀ optionalFormat(bool flag) { m_optionalFormat = flag; }
    ਅੱਖਰ missingArgChar() ਅਚਲ { ਵਾਪਸ m_missingArgChar; }
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSGotoRep final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
ਜਨਤਕ:
    // Exact [->N]
    AstSGotoRep(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSNonConsRep final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
ਜਨਤਕ:
    // Exact [=N]
    AstSNonConsRep(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSScanF final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_ਪਾਠ;
    VTimescale m_timeunit;  // Parent module time unit

ਜਨਤਕ:
    AstSScanF(File_ਸਤਰ* fl, ਅਚਲ string& ਪਾਠ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_ਪਾਠ{ਪਾਠ} {
        ਜੋੜੋ_Exprsp(exprsp);
        this->fromp(fromp);
        dtype_ਸੈੱਟ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_ਪਾਠ; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: makes output
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstSScanF* ਅਚਲ asamep = VN_DBG_AS(samep, SScanF);
        ਵਾਪਸ ਪਾਠ() == asamep->ਪਾਠ() && timeunit() == asamep->timeunit();
    }
    string ਪਾਠ() ਅਚਲ { ਵਾਪਸ m_ਪਾਠ; }  // * = Text to display
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSampled final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_ਅੰਦਰੂਨੀ : 1;  // Internally created, not from a source $sampled
ਜਨਤਕ:
    AstSampled(File_ਸਤਰ* fl, Ast_ਗੰਢ* exprp, Ast_ਗੰਢ_DType* dtypep, bool ਅੰਦਰੂਨੀ_2 = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_ਅੰਦਰੂਨੀ{ਅੰਦਰੂਨੀ_2} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { ਵਾਪਸ "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_ਅੰਦਰੂਨੀ == VN_DBG_AS(samep, Sampled)->m_ਅੰਦਰੂਨੀ;
    }
    bool ਅੰਦਰੂਨੀ_2() ਅਚਲ { ਵਾਪਸ m_ਅੰਦਰੂਨੀ; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstScopeName final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    ਅਚਲ bool m_forFormat;  // Is for a format %m
    ਸਥਿਰ std::string scopeNameFormatter(ਅਚਲ std::string& ਪਾਠ);
    ਸਥਿਰ std::string scopePrettyNameFormatter(ਅਚਲ std::string& ਪਾਠ);

ਜਨਤਕ:
    class ForFormat {};
    AstScopeName(File_ਸਤਰ* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtype_ਸੈੱਟ_UInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstScopeName* ਅਚਲ sp = VN_DBG_AS(samep, ScopeName);
        ਵਾਪਸ m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    // ACCESSORS
    ਅਚਲ std::string& scopeAttr() ਅਚਲ { ਵਾਪਸ m_scopeAttr; }
    ਖਾਲੀ scopeAttr(ਅਚਲ std::string& val) { m_scopeAttr = val; }
    ਅਚਲ std::string& scopeEntr() ਅਚਲ { ਵਾਪਸ m_scopeEntr; }
    ਖਾਲੀ scopeEntr(ਅਚਲ std::string& val) { m_scopeEntr = val; }
    bool dpiExport() ਅਚਲ { ਵਾਪਸ m_dpiExport; }
    ਖਾਲੀ dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() ਅਚਲ { ਵਾਪਸ m_selfRelative; }
    ਖਾਲੀ selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() ਅਚਲ { ਵਾਪਸ m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() ਅਚਲ { ਵਾਪਸ scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() ਅਚਲ { ਵਾਪਸ scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() ਅਚਲ { ਵਾਪਸ scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() ਅਚਲ { ਵਾਪਸ scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ਜਨਤਕ:
    AstStable(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Sen_ਰੁੱਖ* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { ਵਾਪਸ "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਢੇਰ_Trace_F final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // $stacktrace used as function
ਜਨਤਕ:
    explicit Ast_ਢੇਰ_Trace_F(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਢੇਰ_Trace_F(fl) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_Ast_ਢੇਰ_Trace_F;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$stacktrace"; }
    string emitVerilog() override { ਵਾਪਸ verilogKwd(); }
    string emitC() override { ਵਾਪਸ "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSteady final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
ਜਨਤਕ:
    AstSteady(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { ਵਾਪਸ "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstStructSel final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
ਜਨਤਕ:
    AstStructSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, ਅਚਲ string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override {
        // Not a union
        ਵਾਪਸ VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstStructSel* ਅਚਲ sp = VN_DBG_AS(samep, StructSel);
        ਵਾਪਸ m_name == sp->m_name;
    }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
};
class AstSysIgnore final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
ਜਨਤਕ:
    AstSysIgnore(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        ਜੋੜੋ_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$ignored"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }  // Though deleted before opt
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }  // Though deleted before opt
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // Though deleted before opt
    bool isOutputter() override { ਵਾਪਸ true; }  // Though deleted before opt
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
ਜਨਤਕ:
    AstSystemF(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtype_ਸੈੱਟ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$system"; }
    string emitVerilog() override { ਵਾਪਸ verilogKwd(); }
    string emitC() override { ਵਾਪਸ "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Tagged_ਵਿਅੰਜਕ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
ਜਨਤਕ:
    Ast_Tagged_ਵਿਅੰਜਕ(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_Tagged_ਵਿਅੰਜਕ(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_ਵਿਅੰਜਕ;
    string emitVerilog() override { ਵਾਪਸ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_name == VN_DBG_AS(samep, Tagged_ਵਿਅੰਜਕ)->m_name;
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
};
class Ast_Tagged_ਨਮੂਨਾ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
ਜਨਤਕ:
    Ast_Tagged_ਨਮੂਨਾ(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* patternp)
        : ASTGEN_SUPER_Tagged_ਨਮੂਨਾ(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_ਨਮੂਨਾ;
    string emitVerilog() override { ਵਾਪਸ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_name == VN_DBG_AS(samep, Tagged_ਨਮੂਨਾ)->m_name;
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
};
class Ast_Test_Plus_ਦਲੀਲਾਂ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
ਜਨਤਕ:
    Ast_Test_Plus_ਦਲੀਲਾਂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* searchp)
        : ASTGEN_SUPER_Test_Plus_ਦਲੀਲਾਂ(fl) {
        this->searchp(searchp);
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_Test_Plus_ਦਲੀਲਾਂ;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$test$plusargs"; }
    string emitVerilog() override { ਵਾਪਸ verilogKwd(); }
    string emitC() override { ਵਾਪਸ "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    // but isPure() true
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstThisRef final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
ਜਨਤਕ:
    AstThisRef(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        ਬੱਚਾ_DTypep(dtypep);
    }
    AstThisRef(File_ਸਤਰ* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { ਵਾਪਸ "this"; }
    string emitVerilog() override { ਵਾਪਸ "this"; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
};
class Ast_ਸਮਾਂ_Precision final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Verilog $timeprecision
ਜਨਤਕ:
    explicit Ast_ਸਮਾਂ_Precision(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਸਮਾਂ_Precision(fl) {
        dtype_ਸੈੱਟ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ਸਮਾਂ_Precision;
    string emitVerilog() override { ਵਾਪਸ "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਸਮਾਂ_Unit final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
ਜਨਤਕ:
    explicit Ast_ਸਮਾਂ_Unit(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਸਮਾਂ_Unit(fl) {
        dtype_ਸੈੱਟ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ਸਮਾਂ_Unit;
    string emitVerilog() override { ਵਾਪਸ "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    ਅਚਲ std::ਸਦਿਸ਼<uint32_t> m_indices;  // Indices of the triggers tested
ਜਨਤਕ:
    AstTriggered(File_ਸਤਰ* fl, AstVarScope* trigVscp, ਅਚਲ std::ਸਦਿਸ਼<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    ਅਚਲ std::ਸਦਿਸ਼<uint32_t>& indices() ਅਚਲ { ਵਾਪਸ m_indices; }
};
class AstUnbounded final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
ਜਨਤਕ:
    explicit AstUnbounded(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtype_ਸੈੱਟ_Int();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { ਵਾਪਸ "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstUnlinkedRef final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
ਜਨਤਕ:
    AstUnlinkedRef(File_ਸਤਰ* fl, Ast_ਗੰਢ* refp, ਅਚਲ string& name, Ast_ਗੰਢ* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    ਅਚਲ bool m_strong;  // 's_' preffix
    ਅਚਲ bool m_overlapping;  // '_with` suffix
ਜਨਤਕ:
    AstUntil(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() ਅਚਲ override {
        ਵਾਪਸ (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
    bool isStrong() ਅਚਲ { ਵਾਪਸ m_strong; }
    bool isOverlapping() ਅਚਲ { ਵਾਪਸ m_overlapping; }
};
class Ast_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
ਜਨਤਕ:
    Ast_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* searchp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* outp)
        : ASTGEN_SUPER_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$value$plusargs"; }
    string emitVerilog() override { ਵਾਪਸ "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ !outp(); }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstWithParse final : ਜਨਤਕ Ast_ਗੰਢ_ਵਿਅੰਜਕ {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
ਨਿਜੀ:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

ਜਨਤਕ:
    AstWithParse(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* funcrefp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp,
                 Ast_ਗੰਢ* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        ਜੋੜੋ_Exprsp(exprsp);
        ਜੋੜੋ_Constraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool restricted() ਅਚਲ { ਵਾਪਸ m_restricted; }
    ਖਾਲੀ restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
ਜਨਤਕ:
    AstBufIf1(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstCastDynamic final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
ਜਨਤਕ:
    AstCastDynamic(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { ਵਾਪਸ "%f$cast(%r, %l)"; }
    string emitC() override { ਵਾਪਸ "VL_DYNAMIC_CAST(%r, %l)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 20; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
};
class AstCompareNN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Verilog str.compare() and str.icompare()
    ਅਚਲ bool m_ignoreCase;  // True for str.icompare()
ਜਨਤਕ:
    AstCompareNN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtype_ਸੈੱਟ_Int();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        ਵਾਪਸ m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        ਵਾਪਸ m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ignoreCase() ਅਚਲ { ਵਾਪਸ m_ignoreCase; }
};
class AstConcat final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstConcat(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        ਜੇ (lhsp->dtypep() && rhsp->dtypep()) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Sized(lhsp->dtypep()->ਚੌੜਾਈ() + rhsp->dtypep()->ਚੌੜਾਈ(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { ਵਾਪਸ "%f{%l, %k%r}"; }
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opConcat(lhs, rhs);
    }
    string emitC() override { ਵਾਪਸ "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(concat %l %r)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 2; }
};
class AstConcatN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // String concatenate
ਜਨਤਕ:
    AstConcatN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { ਵਾਪਸ "%f{%l, %k%r}"; }
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opConcatN(lhs, rhs);
    }
    string emitC() override { ਵਾਪਸ "VL_CONCATN_NNN(%li, %ri)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
    bool stringFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstDiv final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstDiv(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opDiv(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f/ %r)"; }
    string emitC() override { ਵਾਪਸ "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvudiv %l %r)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_DIV; }
};
class AstDivD final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstDivD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_AstDivD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opDivD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "/"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL_DIV; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstDivS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstDivS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opDivS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f/ %r)"; }
    string emitC() override { ਵਾਪਸ "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvsdiv %l %r)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_DIV; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstEqWild final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstEqWild(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    ਸਥਿਰ Ast_ਗੰਢ_Biop* newTyped(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp);
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f==? %r)"; }
    string emitC() override {
        ਜੇ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        ਜੇ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ "==";
        }
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstFGetS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
ਜਨਤਕ:
    AstFGetS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%f$fgets(%l,%r)"; }
    string emitC() override {
        ਵਾਪਸ strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 64; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstFUngetC(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        ਵਾਪਸ "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 64; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ਗੰਢ* filep() ਅਚਲ { ਵਾਪਸ lhsp(); }
    Ast_ਗੰਢ* charp() ਅਚਲ { ਵਾਪਸ rhsp(); }
};
class AstGetcN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Verilog string.getc()
ਜਨਤਕ:
    AstGetcN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGetcN(lhs, rhs);
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ "getc"; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l.getc(%r))"; }
    string emitC() override { ਵਾਪਸ "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstGetcRefN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
ਜਨਤਕ:
    AstGetcRefN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstGt final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstGt(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGt(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f> %r)"; }
    string emitC() override { ਵਾਪਸ "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ ">"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstGtD final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstGtD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstGtD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGtD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ ">"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstGtN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstGtN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstGtN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGtN(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ ">"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
    bool stringFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstGtS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstGtS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGtS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f> %r)"; }
    string emitC() override { ਵਾਪਸ "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstGte final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstGte(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGte(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f>= %r)"; }
    string emitC() override { ਵਾਪਸ "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ ">="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstGteD final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstGteD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstGteD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGteD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ ">="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstGteN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstGteN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstGteN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGteN(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ ">="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
    bool stringFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstGteS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstGteS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opGteS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f>= %r)"; }
    string emitC() override { ਵਾਪਸ "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstLogAnd final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstLogAnd(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f&& %r)"; }
    string emitC() override { ਵਾਪਸ "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvand %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "&&"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() + INSTR_ਗਿਣਤੀ_BRANCH; }
};
class AstLogIf final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstLogIf(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f-> %r)"; }
    string emitC() override { ਵਾਪਸ "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { ਵਾਪਸ "->"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() + INSTR_ਗਿਣਤੀ_BRANCH; }
};
class AstLogOr final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstLogOr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f|| %r)"; }
    string emitC() override { ਵਾਪਸ "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvor %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "||"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() + INSTR_ਗਿਣਤੀ_BRANCH; }
};
class AstLt final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstLt(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLt(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f< %r)"; }
    string emitC() override { ਵਾਪਸ "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ "<"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstLtD final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstLtD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstLtD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLtD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "<"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstLtN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstLtN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstLtN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLtN(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "<"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
    bool stringFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstLtS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstLtS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLtS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f< %r)"; }
    string emitC() override { ਵਾਪਸ "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstLte final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstLte(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLte(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f<= %r)"; }
    string emitC() override { ਵਾਪਸ "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ "<="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstLteD final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstLteD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstLteD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLteD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "<="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstLteN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstLteN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstLteN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLteN(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "<="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
    bool stringFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstLteS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstLteS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLteS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f<= %r)"; }
    string emitC() override { ਵਾਪਸ "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstModDiv final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstModDiv(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f%% %r)"; }
    string emitC() override { ਵਾਪਸ "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvurem %l %r)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_DIV; }
};
class AstModDivS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstModDivS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f%% %r)"; }
    string emitC() override { ਵਾਪਸ "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvsmod %l %r)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_DIV; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstNeqWild final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstNeqWild(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f!=? %r)"; }
    string emitC() override {
        ਜੇ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        ਜੇ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ "!=";
        }
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstPow final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstPow(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opPow(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f** %r)"; }
    string emitC() override { ਵਾਪਸ "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_MUL * 10; }
};
class AstPowD final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstPowD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_AstPowD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opPowD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f** %r)"; }
    string emitC() override { ਵਾਪਸ "pow(%li,%ri)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL_DIV * 5; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstPowSS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstPowSS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f** %r)"; }
    string emitC() override { ਵਾਪਸ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_MUL * 10; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstPowSU final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstPowSU(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f** %r)"; }
    string emitC() override { ਵਾਪਸ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_MUL * 10; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstPowUS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstPowUS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f** %r)"; }
    string emitC() override { ਵਾਪਸ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_MUL * 10; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstReplicate final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
ਜਨਤਕ:
    AstReplicate(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        ਜੇ (lhsp) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(rhsp, Const)) {
                ਜੇ (constp->ਅੰਕ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ਅੰਕ().isNegative())) {
                    dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Sized(lhsp->ਚੌੜਾਈ(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } ਨਹੀਂਤਾਂ {
                    dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Sized(lhsp->ਚੌੜਾਈ() * constp->ਮੰਜ਼ਿਲ_SInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, uint32_t rep_ਗਿਣਤੀ)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_ਗਿਣਤੀ}} {}
    ASTGEN_MEMBERS_AstReplicate;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opRepl(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%f{%r{%k%l}}"; }
    string emitC() override { ਵਾਪਸ "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override {
        ਵਾਪਸ "((_ repeat " + cvt_ਮੰਜ਼ਿਲ_Str(ਚੌੜਾਈ() / lhsp()->ਚੌੜਾਈ()) + ") %l)";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 2; }
};
class AstReplicateN final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // String replicate
ਜਨਤਕ:
    AstReplicateN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opReplN(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%f{%r{%k%l}}"; }
    string emitC() override { ਵਾਪਸ "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 2; }
    bool stringFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSAnd final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    ਅਚਲ bool m_propertyControl;  // Parser-generated property if/case branch conjunction
ਜਨਤਕ:
    AstSAnd(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstSAnd;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() + INSTR_ਗਿਣਤੀ_BRANCH; }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {  // LCOV_EXCL_LINE
        ਵਾਪਸ m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() ਅਚਲ { ਵਾਪਸ m_propertyControl; }
};
class AstSIntersect final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
ਜਨਤਕ:
    AstSIntersect(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() + INSTR_ਗਿਣਤੀ_BRANCH; }
    // LCOV_EXCL_STOP
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSOr final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
ਜਨਤਕ:
    AstSOr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstSOr;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() + INSTR_ਗਿਣਤੀ_BRANCH; }
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSThroughout final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
ਜਨਤਕ:
    AstSThroughout(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    // LCOV_EXCL_STOP
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSWithin final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
ਜਨਤਕ:
    AstSWithin(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    // LCOV_EXCL_STOP
    bool is_Multi_ਚੱਕਰ_Sva() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSel final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    ਪੂਰਨ m_decl_El_ਚੌੜਾਈ;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    ਪੂਰਨ m_ਚੌੜਾਈ_Const;

ਜਨਤਕ:
    AstSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp, ਪੂਰਨ bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_decl_El_ਚੌੜਾਈ{1}
        , m_ਚੌੜਾਈ_Const{bitwidth} {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Sized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, ਪੂਰਨ lsb, ਪੂਰਨ bitwidth, ਪੂਰਨ bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_decl_El_ਚੌੜਾਈ{1}
        , m_ਚੌੜਾਈ_Const{bitwidth} {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& from, ਅਚਲ V3_ਸੰਖਿਆ& ਦੋਆਧਾਰੀ_ਅੰਕ_2) override {
        ਬਾਹਰਲਾ.opSel(from, ਦੋਆਧਾਰੀ_ਅੰਕ_2.ਮੰਜ਼ਿਲ_UInt() + ਚੌੜਾਈ_Const() - 1, ਦੋਆਧਾਰੀ_ਅੰਕ_2.ਮੰਜ਼ਿਲ_UInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        ਵਾਪਸ ਚੌੜਾਈ_Const() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "((_ extract %t %r) %l)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* otherp) ਅਚਲ override {
        ਵਾਪਸ ਚੌੜਾਈ_Const() == VN_DBG_AS(otherp, Sel)->ਚੌੜਾਈ_Const();
    }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override {
        ਵਾਪਸ ਚੌੜਾਈ_Instrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    ਪੂਰਨ ਚੌੜਾਈ_Const() ਅਚਲ { ਵਾਪਸ m_ਚੌੜਾਈ_Const; }
    ਖਾਲੀ ਚੌੜਾਈ_Const(ਪੂਰਨ ਮੁੱਲ) { m_ਚੌੜਾਈ_Const = ਮੁੱਲ; }
    ਪੂਰਨ lsbConst() ਅਚਲ { ਵਾਪਸ VN_AS(lsbp(), Const)->ਮੰਜ਼ਿਲ_SInt(); }
    ਪੂਰਨ msbConst() ਅਚਲ { ਵਾਪਸ lsbConst() + ਚੌੜਾਈ_Const() - 1; }
    VNumRange& declRange() VL_MT_STABLE { ਵਾਪਸ m_declRange; }
    ਅਚਲ VNumRange& declRange() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_declRange; }
    ਖਾਲੀ declRange(ਅਚਲ VNumRange& flag) { m_declRange = flag; }
    ਪੂਰਨ decl_El_ਚੌੜਾਈ() ਅਚਲ { ਵਾਪਸ m_decl_El_ਚੌੜਾਈ; }
    ਖਾਲੀ decl_El_ਚੌੜਾਈ(ਪੂਰਨ flag) { m_decl_El_ਚੌੜਾਈ = flag; }
};
class Ast_ਖਿਸਕਾਓ_L final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    Ast_ਖਿਸਕਾਓ_L(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਪੂਰਨ setwidth = 0)
        : ASTGEN_SUPER_ਖਿਸਕਾਓ_L(fl, lhsp, rhsp) {
        ਜੇ (setwidth) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = lhsp->dtypep()) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(dtypep->ਚੌੜਾਈ(), dtypep->ਚੌੜਾਈ_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ਖਿਸਕਾਓ_L;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_L(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f<< %r)"; }
    string emitC() override { ਵਾਪਸ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        ਵਾਪਸ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_ਖਿਸਕਾਓ_LOvr final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
ਜਨਤਕ:
    Ast_ਖਿਸਕਾਓ_LOvr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਪੂਰਨ setwidth = 0)
        : ASTGEN_SUPER_ਖਿਸਕਾਓ_LOvr(fl, lhsp, rhsp) {
        ਜੇ (setwidth) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(dtypep->ਚੌੜਾਈ(), dtypep->ਚੌੜਾਈ_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ਖਿਸਕਾਓ_LOvr;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_L(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f<< %r)"; }
    string emitC() override { ਵਾਪਸ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_ਖਿਸਕਾਓ_R final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    Ast_ਖਿਸਕਾਓ_R(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਪੂਰਨ setwidth = 0)
        : ASTGEN_SUPER_ਖਿਸਕਾਓ_R(fl, lhsp, rhsp) {
        ਜੇ (setwidth) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = lhsp->dtypep()) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(dtypep->ਚੌੜਾਈ(), dtypep->ਚੌੜਾਈ_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ਖਿਸਕਾਓ_R;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_R(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f>> %r)"; }
    string emitC() override { ਵਾਪਸ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        ਵਾਪਸ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    // LHS size might be > output size, so don't want to force size
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_ਖਿਸਕਾਓ_ROvr final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
ਜਨਤਕ:
    Ast_ਖਿਸਕਾਓ_ROvr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਪੂਰਨ setwidth = 0)
        : ASTGEN_SUPER_ਖਿਸਕਾਓ_ROvr(fl, lhsp, rhsp) {
        ਜੇ (setwidth) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(dtypep->ਚੌੜਾਈ(), dtypep->ਚੌੜਾਈ_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ਖਿਸਕਾਓ_ROvr;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_R(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f>> %r)"; }
    string emitC() override { ਵਾਪਸ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    // LHS size might be > output size, so don't want to force size
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_ਖਿਸਕਾਓ_RS final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
ਜਨਤਕ:
    Ast_ਖਿਸਕਾਓ_RS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਪੂਰਨ setwidth = 0)
        : ASTGEN_SUPER_ਖਿਸਕਾਓ_RS(fl, lhsp, rhsp) {
        ਜੇ (setwidth) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = lhsp->dtypep()) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(dtypep->ਚੌੜਾਈ(), dtypep->ਚੌੜਾਈ_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ਖਿਸਕਾਓ_RS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_RS(lhs, rhs, lhsp()->ਚੌੜਾਈ_Min_V());
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f>>> %r)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvashr %l %r)"; }
    string emitC() override { ਵਾਪਸ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਖਿਸਕਾਓ_RSOvr final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
ਜਨਤਕ:
    Ast_ਖਿਸਕਾਓ_RSOvr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, ਪੂਰਨ setwidth = 0)
        : ASTGEN_SUPER_ਖਿਸਕਾਓ_RSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        ਜੇ (setwidth) {
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(dtypep->ਚੌੜਾਈ(), dtypep->ਚੌੜਾਈ_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_ਖਿਸਕਾਓ_RSOvr;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_RS(lhs, rhs, lhsp()->ਚੌੜਾਈ_Min_V());
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f>>> %r)"; }
    string emitC() override { ਵਾਪਸ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSub final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstSub(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opSub(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f- %r)"; }
    string emitC() override { ਵਾਪਸ "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvsub %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "-"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstSubD final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
ਜਨਤਕ:
    AstSubD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_AstSubD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opSubD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "-"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstURandomRange final : ਜਨਤਕ Ast_ਗੰਢ_Biop {
    // $urandom_range
ਜਨਤਕ:
    AstURandomRange(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_UInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%f$urandom_range(%l, %r)"; }
    string emitC() override { ਵਾਪਸ "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstEq(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    ਸਥਿਰ Ast_ਗੰਢ_Biop* newTyped(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp);
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opEq(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f== %r)"; }
    string emitC() override { ਵਾਪਸ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { ਵਾਪਸ "=="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstEqCase final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstEqCase(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstEqCase;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f=== %r)"; }
    string emitC() override { ਵਾਪਸ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "=="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstEqD final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
ਜਨਤਕ:
    AstEqD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstEqD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opEqD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "=="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstEqN final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
ਜਨਤਕ:
    AstEqN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstEqN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opEqN(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "=="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
    bool stringFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstEqT final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
    // Equal (==) for data types
ਜਨਤਕ:
    AstEqT(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstEqT;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "=="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
};
class AstLogEq final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstLogEq(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f<-> %r)"; }
    string emitC() override { ਵਾਪਸ "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "<->"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() + INSTR_ਗਿਣਤੀ_BRANCH; }
};
class AstNeq final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstNeq(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    ਸਥਿਰ Ast_ਗੰਢ_Biop* newTyped(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp);
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opNeq(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f!= %r)"; }
    string emitC() override { ਵਾਪਸ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "!="; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (not (= %l %r)))"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstNeqCase final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstNeqCase(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f!== %r)"; }
    string emitC() override { ਵਾਪਸ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "!="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstNeqD final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
ਜਨਤਕ:
    AstNeqD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstNeqD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "!="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstNeqN final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
ਜਨਤਕ:
    AstNeqN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstNeqN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "!="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
    bool stringFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstNeqT final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com {
    // Not-equal (!=) for data types
ਜਨਤਕ:
    AstNeqT(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstNeqT;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "!="; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_STR; }
};

// === AstNodeBiComAsv ===
class Ast_ਜੋੜੋ final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com_Asv {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    Ast_ਜੋੜੋ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ਜੋੜੋ(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_ਜੋੜੋ;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਜੋੜੋ(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f+ %r)"; }
    string emitC() override { ਵਾਪਸ "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvadd %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "+"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਜੋੜੋ_D final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com_Asv {
ਜਨਤਕ:
    Ast_ਜੋੜੋ_D(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ਜੋੜੋ_D(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_Ast_ਜੋੜੋ_D;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਜੋੜੋ_D(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "+"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstAnd final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com_Asv {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstAnd(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opAnd(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f& %r)"; }
    string emitC() override { ਵਾਪਸ "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvand %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "&"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਅਚਲ ਅੱਖਰ* ਚੌੜਾਈ_Mismatch() ਅਚਲ override VL_MT_STABLE;
};
class AstMul final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com_Asv {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstMul(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opMul(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f* %r)"; }
    string emitC() override { ਵਾਪਸ "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "*"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_MUL; }
};
class AstMulD final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com_Asv {
ਜਨਤਕ:
    AstMulD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_AstMulD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opMulD(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "*"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstMulS final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com_Asv {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstMulS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opMulS(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f* %r)"; }
    string emitC() override { ਵਾਪਸ "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * INSTR_ਗਿਣਤੀ_INT_MUL; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstOr final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com_Asv {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstOr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opOr(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f| %r)"; }
    string emitC() override { ਵਾਪਸ "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvor %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "|"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਅਚਲ ਅੱਖਰ* ਚੌੜਾਈ_Mismatch() ਅਚਲ override VL_MT_STABLE;
};
class AstXor final : ਜਨਤਕ Ast_ਗੰਢ_Bi_Com_Asv {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstXor(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.opXor(lhs, rhs);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f^ %r)"; }
    string emitC() override { ਵਾਪਸ "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvxor %l %r)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "^"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }  // Lclean && Rclean
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਅਚਲ ਅੱਖਰ* ਚੌੜਾਈ_Mismatch() ਅਚਲ override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class Ast_Dist_Chi_ਵਰਗਫਲ final : ਜਨਤਕ Ast_ਗੰਢ_Dist_Biop {
ਜਨਤਕ:
    Ast_Dist_Chi_ਵਰਗਫਲ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Dist_Chi_ਵਰਗਫਲ(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_Chi_ਵਰਗਫਲ;
    string emitVerilog() override { ਵਾਪਸ "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { ਵਾਪਸ "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class Ast_Dist_ਘਾਤਾਂਕੀ final : ਜਨਤਕ Ast_ਗੰਢ_Dist_Biop {
ਜਨਤਕ:
    Ast_Dist_ਘਾਤਾਂਕੀ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Dist_ਘਾਤਾਂਕੀ(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ਘਾਤਾਂਕੀ;
    string emitVerilog() override { ਵਾਪਸ "%f$dist_exponential(%l, %r)"; }
    string emitC() override { ਵਾਪਸ "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class Ast_Dist_ਪੁਆਸੋਂ final : ਜਨਤਕ Ast_ਗੰਢ_Dist_Biop {
ਜਨਤਕ:
    Ast_Dist_ਪੁਆਸੋਂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Dist_ਪੁਆਸੋਂ(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ਪੁਆਸੋਂ;
    string emitVerilog() override { ਵਾਪਸ "%f$dist_poisson(%l, %r)"; }
    string emitC() override { ਵਾਪਸ "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : ਜਨਤਕ Ast_ਗੰਢ_Dist_Biop {
ਜਨਤਕ:
    AstDistT(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { ਵਾਪਸ "%f$dist_t(%l, %r)"; }
    string emitC() override { ਵਾਪਸ "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : ਜਨਤਕ Ast_ਗੰਢ_Sel {
    // @astgen makeDfgVertex
    ਖਾਲੀ init(ਅਚਲ Ast_ਗੰਢ* fromp) {
        ਜੇ (fromp && VN_IS(fromp->dtypep()->skipRefp(), ਗੰਢ_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), ਗੰਢ_Array_DType)->subDTypep());
        }
    }

ਜਨਤਕ:
    AstArraySel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(select %l %r)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
};
class AstAssocSel final : ਜਨਤਕ Ast_ਗੰਢ_Sel {
    ਖਾਲੀ init(ਅਚਲ Ast_ਗੰਢ* fromp) {
        ਜੇ (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

ਜਨਤਕ:
    AstAssocSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l%f[%r])"; }
    string emitC() override { ਵਾਪਸ "%li%k[%ri]"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(select %l %r)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isGateOptimizable() ਅਚਲ override {
        ਵਾਪਸ !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ !isLValue(); }  // AssocSel creates on miss
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
};
class AstWildcardSel final : ਜਨਤਕ Ast_ਗੰਢ_Sel {
    ਖਾਲੀ init(ਅਚਲ Ast_ਗੰਢ* fromp) {
        ਜੇ (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

ਜਨਤਕ:
    AstWildcardSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l%f[%r])"; }
    string emitC() override { ਵਾਪਸ "%li%k[%ri]"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
};
class Ast_ਸ਼ਬਦ_Sel final : ਜਨਤਕ Ast_ਗੰਢ_Sel {
    // Select a single word from a multi-word wide value
ਜਨਤਕ:
    Ast_ਸ਼ਬਦ_Sel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp)
        : ASTGEN_SUPER_ਸ਼ਬਦ_Sel(fl, fromp, bitp) {
        dtype_ਸੈੱਟ_UInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_Ast_ਸ਼ਬਦ_Sel;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&, ਅਚਲ V3_ਸੰਖਿਆ&) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};

// === AstNodeStream ===
class Ast_ਧਾਰਾ_L final : ਜਨਤਕ Ast_ਗੰਢ_ਧਾਰਾ {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
ਜਨਤਕ:
    Ast_ਧਾਰਾ_L(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ਧਾਰਾ_L(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_ਧਾਰਾ_L;
    string emitVerilog() override { ਵਾਪਸ "%f{ << %r %k{%l} }"; }
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਧਾਰਾ_L(lhs, rhs);
    }
    string emitC() override { ਵਾਪਸ "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 2; }
};
class Ast_ਧਾਰਾ_R final : ਜਨਤਕ Ast_ਗੰਢ_ਧਾਰਾ {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
ਜਨਤਕ:
    Ast_ਧਾਰਾ_R(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_ਧਾਰਾ_R(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_ਧਾਰਾ_R;
    string emitVerilog() override { ਵਾਪਸ "%f{ >> %r %k{%l} }"; }
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(lhs);
    }
    string emitC() override { ਵਾਪਸ isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : ਜਨਤਕ Ast_ਗੰਢ_System_Biop_D {
ਜਨਤਕ:
    AstAtan2D(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::atan2(lhs.ਮੰਜ਼ਿਲ_Double(), rhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$atan2(%l,%r)"; }
    string emitC() override { ਵਾਪਸ "atan2(%li,%ri)"; }
};
class AstHypotD final : ਜਨਤਕ Ast_ਗੰਢ_System_Biop_D {
ਜਨਤਕ:
    AstHypotD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::hypot(lhs.ਮੰਜ਼ਿਲ_Double(), rhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$hypot(%l,%r)"; }
    string emitC() override { ਵਾਪਸ "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : ਜਨਤਕ Ast_ਗੰਢ_CCall {
    // C++ function call
    VSelf_ਸੂਚਕ_ਪਾਠ m_self_ਸੂਚਕ
        = VSelf_ਸੂਚਕ_ਪਾਠ{VSelf_ਸੂਚਕ_ਪਾਠ::ਖਾਲੀ_ਥਾਵਾਂ_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
ਜਨਤਕ:
    AstCCall(File_ਸਤਰ* fl, AstCFunc* funcp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    ਅਚਲ VSelf_ਸੂਚਕ_ਪਾਠ& self_ਸੂਚਕ() ਅਚਲ { ਵਾਪਸ m_self_ਸੂਚਕ; }
    ਖਾਲੀ self_ਸੂਚਕ(ਅਚਲ VSelf_ਸੂਚਕ_ਪਾਠ& self_ਸੂਚਕ) { m_self_ਸੂਚਕ = self_ਸੂਚਕ; }
    string self_ਸੂਚਕ_Protect(bool useSelfForThis) ਅਚਲ {
        ਵਾਪਸ self_ਸੂਚਕ().protect(useSelfForThis, protect());
    }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
};
class AstCMethodCall final : ਜਨਤਕ Ast_ਗੰਢ_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
ਜਨਤਕ:
    AstCMethodCall(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, AstCFunc* funcp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : ਜਨਤਕ Ast_ਗੰਢ_CCall {
    // C++ new() call
ਜਨਤਕ:
    AstCNew(File_ਸਤਰ* fl, AstCFunc* funcp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : ਜਨਤਕ Ast_ਗੰਢ_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, Ast_Cons_Array_ਵਸਤੂ*> m_itemps;  // The item at each index given

ਜਨਤਕ:
    AstConsArray(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* newDTypep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਖਾਲੀ cloneRelink() override;
    ਸਤਰਅੰਦਰ Ast_Cons_Array_ਵਸਤੂ* itemsp() ਅਚਲ VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep(uint64_t ਸੂਚਕਾਂਕ) ਅਚਲ;
    // Add the value of the element at the index, which must not be given yet
    ਖਾਲੀ valuep(uint64_t ਸੂਚਕਾਂਕ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp);
    // Add the elements of the array 'arrayp', starting at the index
    ਖਾਲੀ splice_ਮੁੱਲਾਂ(uint64_t ਸੂਚਕਾਂਕ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* arrayp);
};
class AstConsAssoc final : ਜਨਤਕ Ast_ਗੰਢ_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
ਜਨਤਕ:
    explicit AstConsAssoc(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        ਵਾਪਸ nullptr;
    }
};
class AstConsDynArray final : ਜਨਤਕ Ast_ਗੰਢ_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
ਜਨਤਕ:
    explicit AstConsDynArray(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
};
class Ast_Cons_ਕਤਾਰ final : ਜਨਤਕ Ast_ਗੰਢ_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
ਜਨਤਕ:
    explicit Ast_Cons_ਕਤਾਰ(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_Cons_ਕਤਾਰ(fl) {}
    ASTGEN_MEMBERS_Ast_Cons_ਕਤਾਰ;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), ਕਤਾਰ_DType));
        ਵਾਪਸ nullptr;
    }
};
class AstConsStruct final : ਜਨਤਕ Ast_ਗੰਢ_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
ਜਨਤਕ:
    AstConsStruct(File_ਸਤਰ* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    // The value of the member called 'name'
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* memberValuep(ਅਚਲ string& name) ਅਚਲ;
};
class AstConsWildcard final : ਜਨਤਕ Ast_ਗੰਢ_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
ਜਨਤਕ:
    explicit AstConsWildcard(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        ਵਾਪਸ nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : ਜਨਤਕ Ast_ਗੰਢ_FTask_Ref {
    // A reference to a function
ਜਨਤਕ:
    ਸਤਰਅੰਦਰ AstFuncRef(File_ਸਤਰ* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_ਸਤਰ* fl, ਅਚਲ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class Ast_ਵਿਧੀ_Call final : ਜਨਤਕ Ast_ਗੰਢ_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
ਜਨਤਕ:
    Ast_ਵਿਧੀ_Call(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, VFlag_ਬੱਚਾ_DType, ਅਚਲ string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_ਵਿਧੀ_Call(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_ਵਿਧੀ_Call(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, ਅਚਲ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_ਵਿਧੀ_Call(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_Ast_ਵਿਧੀ_Call;
};
class AstNew final : ਜਨਤਕ Ast_ਗੰਢ_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
ਜਨਤਕ:
    AstNew(File_ਸਤਰ* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool isImplicit() ਅਚਲ { ਵਾਪਸ m_isImplicit; }
    ਖਾਲੀ isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() ਅਚਲ { ਵਾਪਸ m_isScoped; }
    ਖਾਲੀ isScoped(bool flag) { m_isScoped = flag; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
};
class AstTaskRef final : ਜਨਤਕ Ast_ਗੰਢ_FTask_Ref {
    // A reference to a task
ਜਨਤਕ:
    ਸਤਰਅੰਦਰ AstTaskRef(File_ਸਤਰ* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_ਸਤਰ* fl, ਅਚਲ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtype_ਸੈੱਟ_Void();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ final : ਜਨਤਕ Ast_ਗੰਢ_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
ਨਿਜੀ:
    VAccess m_access;  // Left hand side assignment
ਜਨਤਕ:
    Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp)
        : ASTGEN_SUPER_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    VAccess access() ਅਚਲ { ਵਾਪਸ m_access; }
    ਖਾਲੀ access(ਅਚਲ VAccess& flag) { m_access = flag; }
};
class Ast_Sel_ਕੱਢੋ final : ਜਨਤਕ Ast_ਗੰਢ_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
ਜਨਤਕ:
    Ast_Sel_ਕੱਢੋ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* msbp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp)
        : ASTGEN_SUPER_Sel_ਕੱਢੋ(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_Ast_Sel_ਕੱਢੋ;
};
class AstSelMinus final : ਜਨਤਕ Ast_ਗੰਢ_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
ਜਨਤਕ:
    AstSelMinus(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : ਜਨਤਕ Ast_ਗੰਢ_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
ਜਨਤਕ:
    AstSelPlus(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bitp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_ਗਿਣਤੀ_ਬਿੱਟ final : ਜਨਤਕ Ast_ਗੰਢ_Quadop {
    // Number of bits set in vector
ਜਨਤਕ:
    Ast_ਗਿਣਤੀ_ਬਿੱਟ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ctrl1p)
        : ASTGEN_SUPER_ਗਿਣਤੀ_ਬਿੱਟ(fl, exprp, ctrl1p, ctrl1p->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(false),
                                 ctrl1p->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(false)) {
        dtype_ਸੈੱਟ_Int();
    }
    Ast_ਗਿਣਤੀ_ਬਿੱਟ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ctrl1p, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ctrl2p)
        : ASTGEN_SUPER_ਗਿਣਤੀ_ਬਿੱਟ(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(false)) {
        dtype_ਸੈੱਟ_Int();
    }
    Ast_ਗਿਣਤੀ_ਬਿੱਟ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ctrl1p, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ctrl2p,
                 Ast_ਗੰਢ_ਵਿਅੰਜਕ* ctrl3p)
        : ASTGEN_SUPER_ਗਿਣਤੀ_ਬਿੱਟ(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtype_ਸੈੱਟ_Int();
    }
    ASTGEN_MEMBERS_Ast_ਗਿਣਤੀ_ਬਿੱਟ;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& ਵਿਅੰਜਕ, ਅਚਲ V3_ਸੰਖਿਆ& ctrl1,
                       ਅਚਲ V3_ਸੰਖਿਆ& ctrl2, ਅਚਲ V3_ਸੰਖਿਆ& ctrl3) override {
        ਬਾਹਰਲਾ.op_ਗਿਣਤੀ_ਬਿੱਟ(ਵਿਅੰਜਕ, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { ਵਾਪਸ "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanThs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanFhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Ths() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Fhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : ਜਨਤਕ Ast_ਗੰਢ_Termop {
ਜਨਤਕ:
    explicit AstInferredDisable(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { ਵਾਪਸ "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਸਮਾਂ final : ਜਨਤਕ Ast_ਗੰਢ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
ਜਨਤਕ:
    Ast_ਸਮਾਂ(File_ਸਤਰ* fl, ਅਚਲ VTimescale& timeunit)
        : ASTGEN_SUPER_ਸਮਾਂ(fl)
        , m_timeunit{timeunit} {
        dtype_ਸੈੱਟ_UInt64();
    }
    ASTGEN_MEMBERS_Ast_ਸਮਾਂ;
    string emitVerilog() override { ਵਾਪਸ "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_ਸਮਾਂ; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }
};
class Ast_ਸਮਾਂ_D final : ਜਨਤਕ Ast_ਗੰਢ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
ਜਨਤਕ:
    Ast_ਸਮਾਂ_D(File_ਸਤਰ* fl, ਅਚਲ VTimescale& timeunit)
        : ASTGEN_SUPER_ਸਮਾਂ_D(fl)
        , m_timeunit{timeunit} {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_Ast_ਸਮਾਂ_D;
    string emitVerilog() override { ਵਾਪਸ "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_ਸਮਾਂ; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : ਜਨਤਕ Ast_ਗੰਢ_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstCond(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* thenp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* elsep);
    ASTGEN_MEMBERS_AstCond;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs,
                       ਅਚਲ V3_ਸੰਖਿਆ& ths) override {
        ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l %f? %r %k: %t)"; }
    string emitC() override { ਵਾਪਸ "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(ite (__Vbool %l) %r %t)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }  // clean if e1 & e2 clean
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanThs() ਅਚਲ override { ਵਾਪਸ false; }  // Propagates up
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Ths() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_BRANCH; }
};
class AstPutcN final : ਜਨਤਕ Ast_ਗੰਢ_Triop {
    // Verilog string.putc()
ਜਨਤਕ:
    AstPutcN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_AstPutcN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs,
                       ਅਚਲ V3_ਸੰਖਿਆ& ths) override {
        ਬਾਹਰਲਾ.opPutcN(lhs, rhs, ths);
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ "putc"; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l.putc(%r,%t))"; }
    string emitC() override { ਵਾਪਸ "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanThs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Ths() ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_ਟੁਕੜਾ_Sel final : ਜਨਤਕ Ast_ਗੰਢ_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
ਜਨਤਕ:
    Ast_ਟੁਕੜਾ_Sel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, ਅਚਲ VNumRange& declRange)
        : ASTGEN_SUPER_ਟੁਕੜਾ_Sel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_Ast_ਟੁਕੜਾ_Sel;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& from, ਅਚਲ V3_ਸੰਖਿਆ& lo,
                       ਅਚਲ V3_ਸੰਖਿਆ& ਚੌੜਾਈ) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanThs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Ths() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { ਵਾਪਸ m_declRange; }
    ਅਚਲ VNumRange& declRange() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_declRange; }
};
class AstSubstrN final : ਜਨਤਕ Ast_ਗੰਢ_Triop {
    // Verilog string.substr()
ਜਨਤਕ:
    AstSubstrN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs,
                       ਅਚਲ V3_ਸੰਖਿਆ& ths) override {
        ਬਾਹਰਲਾ.opSubstrN(lhs, rhs, ths);
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ "substr"; }
    string emitVerilog() override { ਵਾਪਸ "%k(%l.substr(%r,%t))"; }
    string emitC() override { ਵਾਪਸ "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { ਵਾਪਸ ""; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanRhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanThs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Rhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Ths() ਅਚਲ override { ਵਾਪਸ false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : ਜਨਤਕ Ast_ਗੰਢ_Dist_Triop {
ਜਨਤਕ:
    AstDistErlang(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { ਵਾਪਸ "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { ਵਾਪਸ "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class Ast_Dist_ਸਾਧਾਰਨ final : ਜਨਤਕ Ast_ਗੰਢ_Dist_Triop {
ਜਨਤਕ:
    Ast_Dist_ਸਾਧਾਰਨ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsp)
        : ASTGEN_SUPER_Dist_ਸਾਧਾਰਨ(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ਸਾਧਾਰਨ;
    string emitVerilog() override { ਵਾਪਸ "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { ਵਾਪਸ "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class Ast_Dist_ਇਕਸਾਰ final : ਜਨਤਕ Ast_ਗੰਢ_Dist_Triop {
ਜਨਤਕ:
    Ast_Dist_ਇਕਸਾਰ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsp)
        : ASTGEN_SUPER_Dist_ਇਕਸਾਰ(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ਇਕਸਾਰ;
    string emitVerilog() override { ਵਾਪਸ "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { ਵਾਪਸ "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
ਜਨਤਕ:
    ਗਿਣਤੀ FmtType : ਪੂਰਨ { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

ਨਿਜੀ:
    // dist-ast-dump-suppress  // Part of name()
    ਅਚਲ FmtType m_fmtType;  // Operation type
ਜਨਤਕ:
    AstAtoN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtype_ਸੈੱਟ_Double() : dtype_ਸੈੱਟ_Integer();
    }
    ASTGEN_MEMBERS_AstAtoN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opAtoN(lhs, m_fmtType); }
    string name() ਅਚਲ override VL_MT_STABLE {
        ਚੋਣ (m_fmtType) {
        ਹਾਲਤ ATOI: ਵਾਪਸ "atoi";
        ਹਾਲਤ ATOHEX: ਵਾਪਸ "atohex";
        ਹਾਲਤ ATOOCT: ਵਾਪਸ "atooct";
        ਹਾਲਤ ATOBIN: ਵਾਪਸ "atobin";
        ਹਾਲਤ ATOREAL: ਵਾਪਸ "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { ਵਾਪਸ "%l." + name() + "()"; }
    string emitC() override {
        ਚੋਣ (m_fmtType) {
        ਹਾਲਤ ATOI: ਵਾਪਸ "VL_ATOI_N(%li, 10)";
        ਹਾਲਤ ATOHEX: ਵਾਪਸ "VL_ATOI_N(%li, 16)";
        ਹਾਲਤ ATOOCT: ਵਾਪਸ "VL_ATOI_N(%li, 8)";
        ਹਾਲਤ ATOBIN: ਵਾਪਸ "VL_ATOI_N(%li, 2)";
        ਹਾਲਤ ATOREAL: ਵਾਪਸ "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    FmtType fmtType() ਅਚਲ { ਵਾਪਸ m_fmtType; }
};
class Ast_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
ਜਨਤਕ:
    Ast_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D(fl, lhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_Ast_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$bitstoreal(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_CVT_D_Q(%li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }  // Eliminated before matters
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }  // Eliminated before matters
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstCCast final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Cast to C-based data type
    ਪੂਰਨ m_ਮਾਪ;

ਜਨਤਕ:
    AstCCast(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, ਪੂਰਨ setwidth, ਪੂਰਨ minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_ਮਾਪ = setwidth;
        ਜੇ (setwidth) {
            ਜੇ (minwidth == -1) minwidth = setwidth;
            dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_ਮਾਪ = ਚੌੜਾਈ();
    }
    ASTGEN_MEMBERS_AstCCast;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$_CAST(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }  // Special cased in V3Cast
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ ਮਾਪ() == VN_DBG_AS(samep, CCast)->ਮਾਪ();
    }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    //
    ਪੂਰਨ ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_ਮਾਪ; }
};
class AstCLog2 final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
ਜਨਤਕ:
    AstCLog2(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtype_ਸੈੱਟ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opCLog2(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$clog2(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 16; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstCastWrap final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
ਜਨਤਕ:
    AstCastWrap(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(lhs); }
    string emitVerilog() override { ਵਾਪਸ "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
};
class Ast_ਗਿਣਤੀ_Ones final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
ਜਨਤਕ:
    Ast_ਗਿਣਤੀ_Ones(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ਗਿਣਤੀ_Ones(fl, lhsp) {
        dtype_ਸੈੱਟ_Int();
    }
    ASTGEN_MEMBERS_Ast_ਗਿਣਤੀ_Ones;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_ਗਿਣਤੀ_Ones(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$countones(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 16; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstCvtPackString final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
ਜਨਤਕ:
    AstCvtPackString(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstExtend final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstExtend(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(ਚੌੜਾਈ, ਚੌੜਾਈ_Min ? ਚੌੜਾਈ_Min : ਚੌੜਾਈ, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%l"; }
    string emitC() override { ਵਾਪਸ "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ਅਚਲ override {
        ਵਾਪਸ "((_ zero_extend " + cvt_ਮੰਜ਼ਿਲ_Str(ਚੌੜਾਈ() - lhsp()->ਚੌੜਾਈ()) + ") %l)";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override {
        ਵਾਪਸ false;  // Because the EXTEND operator self-casts
    }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
};
class AstExtendS final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstExtendS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, ਪੂਰਨ ਚੌੜਾਈ)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Sized(ਚੌੜਾਈ, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.opExtendS(lhs, lhsp()->ਚੌੜਾਈ_Min_V());
    }
    string emitVerilog() override { ਵਾਪਸ "%l"; }
    string emitC() override { ਵਾਪਸ "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ਅਚਲ override {
        ਵਾਪਸ "((_ sign_extend " + cvt_ਮੰਜ਼ਿਲ_Str(ਚੌੜਾਈ() - lhsp()->ਚੌੜਾਈ()) + ") %l)";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override {
        ਵਾਪਸ false;  // Because the EXTEND operator self-casts
    }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool signedFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstFEof final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
ਜਨਤਕ:
    AstFEof(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%f$feof(%l)"; }
    string emitC() override { ਵਾਪਸ "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 16; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_ਗੰਢ* filep() ਅਚਲ { ਵਾਪਸ lhsp(); }
};
class AstFGetC final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
ਜਨਤਕ:
    AstFGetC(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtype_ਸੈੱਟ_Logic2_State_Unsized(
            32, v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::MATCHES_ਚੌੜਾਈ ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { ਵਾਪਸ "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 64; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ਗੰਢ* filep() ਅਚਲ { ਵਾਪਸ lhsp(); }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstISToRD final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // $itor where lhs is signed
ਜਨਤਕ:
    AstISToRD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_AstISToRD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opISToRD(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$itor($signed(%l))"; }
    string emitC() override { ਵਾਪਸ "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_ਸ਼ਬਦ() override { ਵਾਪਸ true; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstIToRD final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // $itor where lhs is unsigned
ਜਨਤਕ:
    AstIToRD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_AstIToRD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opIToRD(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$itor(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_ITOR_D_%lq(%lw, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstIsUnbounded final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // True if is unbounded ($)
ਜਨਤਕ:
    AstIsUnbounded(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ&) override {
        // Any constant isn't unbounded
        ਬਾਹਰਲਾ.ਸੈੱਟ_Zero();
    }
    string emitVerilog() override { ਵਾਪਸ "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstIsUnknown final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // True if any unknown bits
ਜਨਤਕ:
    AstIsUnknown(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opIsUnknown(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstLenN final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Length of a string
ਜਨਤਕ:
    AstLenN(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtype_ਸੈੱਟ_Int();
    }
    ASTGEN_MEMBERS_AstLenN;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opLenN(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_LEN_IN(%li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstLogNot final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // @astgen makeDfgVertex
ਨਿਜੀ:
    ਅਚਲ bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
ਜਨਤਕ:
    AstLogNot(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opLogNot(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f(! %l)"; }
    string emitC() override { ਵਾਪਸ "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { ਵਾਪਸ "!"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool fromProperty() ਅਚਲ { ਵਾਪਸ m_fromProperty; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class Ast_Most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1 final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
ਜਨਤਕ:
    Ast_Most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1(fl, lhsp) {
        dtype_ਸੈੱਟ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_Most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_Most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$mostsetbitp1(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 16; }
};
class AstNToI final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // String to any-size integral
ਜਨਤਕ:
    AstNToI(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opNToI(lhs); }
    string emitVerilog() override { ਵਾਪਸ "'(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstNegate final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstNegate(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opNegate(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f(- %l)"; }
    string emitC() override { ਵਾਪਸ "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvneg %l)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "-"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstNegateD final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
ਜਨਤਕ:
    AstNegateD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtype_ਸੈੱਟ_Double();
    }
    ASTGEN_MEMBERS_AstNegateD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opNegateD(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ਵਾਪਸ "-"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool doubleFlavor() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstNot final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstNot(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opNot(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f(~ %l)"; }
    string emitC() override { ਵਾਪਸ "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() ਅਚਲ override { ਵਾਪਸ "(bvnot %l)"; }
    string emitSimpleOperator() override { ਵਾਪਸ "~"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }
    ਅਚਲ ਅੱਖਰ* ਚੌੜਾਈ_Mismatch() ਅਚਲ override VL_MT_STABLE;
};
class AstNullCheck final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Return LHS after checking that LHS is non-null
ਜਨਤਕ:
    AstNullCheck(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { V3ERROR_NA; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 1; }  // Rarely executes
    string emitVerilog() override { ਵਾਪਸ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override { ਵਾਪਸ fileline() == samep->fileline(); }
};
class AstOneHot final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstOneHot(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstOneHot;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opOneHot(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$onehot(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 4; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstOneHot0 final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstOneHot0(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opOneHot0(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$onehot0(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs() * 3; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstPostDec final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
ਜਨਤਕ:
    AstPostDec(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
ਜਨਤਕ:
    AstPostInc(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ਵਾਪਸ "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
ਜਨਤਕ:
    AstPreDec(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ਵਾਪਸ "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
ਜਨਤਕ:
    AstPreInc(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ਵਾਪਸ "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
ਜਨਤਕ:
    AstRToIRoundS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtype_ਸੈੱਟ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opRToIRoundS(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        ਵਾਪਸ isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstRToIS final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // $rtoi(lhs)
ਜਨਤਕ:
    AstRToIS(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtype_ਸੈੱਟ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opRToIS(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$rtoi(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_RTOI_I_D(%li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }  // Eliminated before matters
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }  // Eliminated before matters
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
ਜਨਤਕ:
    Ast_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ(fl, lhsp) {
        dtype_ਸੈੱਟ_UInt64();
    }
    ASTGEN_MEMBERS_Ast_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f$realtobits(%l)"; }
    string emitC() override { ਵਾਪਸ "VL_CVT_Q_D(%li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }  // Eliminated before matters
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }  // Eliminated before matters
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_DBL; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstRedAnd final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstRedAnd(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opRedAnd(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f(& %l)"; }
    string emitC() override { ਵਾਪਸ "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstRedOr final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstRedOr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opRedOr(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f(| %l)"; }
    string emitC() override { ਵਾਪਸ "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstRedXor final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // @astgen makeDfgVertex
ਜਨਤਕ:
    AstRedXor(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.opRedXor(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%f(^ %l)"; }
    string emitC() override { ਵਾਪਸ "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override {
        ਅਚਲ ਪੂਰਨ w = lhsp()->ਚੌੜਾਈ();
        ਵਾਪਸ (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 1 + V3_ਸੰਖਿਆ::log2b(ਚੌੜਾਈ()); }
};
class AstResizeLValue final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
ਜਨਤਕ:
    AstResizeLValue(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
};
class AstSigned final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // $signed(lhs)
ਜਨਤਕ:
    AstSigned(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(lhs);
        ਬਾਹਰਲਾ.isSigned(false);
    }
    string emitVerilog() override { ਵਾਪਸ "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }  // Eliminated before matters
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }  // Eliminated before matters
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਸਮਾਂ_Import final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
ਜਨਤਕ:
    Ast_ਸਮਾਂ_Import(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ਸਮਾਂ_Import(fl, lhsp) {}
    ASTGEN_MEMBERS_Ast_ਸਮਾਂ_Import;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }
};
class Ast_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // string.tolower()
ਜਨਤਕ:
    Ast_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N(fl, lhsp) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_Ast_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%l.tolower()"; }
    string emitC() override { ਵਾਪਸ "VL_TOLOWER_NN(%li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_ਮੰਜ਼ਿਲ_String_N final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
ਜਨਤਕ:
    Ast_ਮੰਜ਼ਿਲ_String_N(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ਮੰਜ਼ਿਲ_String_N(fl, lhsp) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_Ast_ਮੰਜ਼ਿਲ_String_N;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ਵਾਪਸ "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        ਵਾਪਸ isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class Ast_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // string.toupper()
ਜਨਤਕ:
    Ast_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N(fl, lhsp) {
        dtype_ਸੈੱਟ_String();
    }
    ASTGEN_MEMBERS_Ast_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override { ਬਾਹਰਲਾ.op_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N(lhs); }
    string emitVerilog() override { ਵਾਪਸ "%l.toupper()"; }
    string emitC() override { ਵਾਪਸ "VL_TOUPPER_NN(%li)"; }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ true; }
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstUnsigned final : ਜਨਤਕ Ast_ਗੰਢ_Uniop {
    // $unsigned(lhs)
ਜਨਤਕ:
    AstUnsigned(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(lhs);
        ਬਾਹਰਲਾ.isSigned(false);
    }
    string emitVerilog() override { ਵਾਪਸ "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ false; }
    bool cleanLhs() ਅਚਲ override { ਵਾਪਸ false; }  // Eliminated before matters
    bool ਮਾਪ_Matters_Lhs() ਅਚਲ override { ਵਾਪਸ true; }  // Eliminated before matters
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool isSystemFunc() ਅਚਲ override { ਵਾਪਸ true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstAcosD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::acos(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$acos(%l)"; }
    string emitC() override { ਵਾਪਸ "acos(%li)"; }
};
class AstAcoshD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstAcoshD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::acosh(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$acosh(%l)"; }
    string emitC() override { ਵਾਪਸ "acosh(%li)"; }
};
class AstAsinD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstAsinD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::asin(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$asin(%l)"; }
    string emitC() override { ਵਾਪਸ "asin(%li)"; }
};
class AstAsinhD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstAsinhD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::asinh(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$asinh(%l)"; }
    string emitC() override { ਵਾਪਸ "asinh(%li)"; }
};
class AstAtanD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstAtanD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::atan(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$atan(%l)"; }
    string emitC() override { ਵਾਪਸ "atan(%li)"; }
};
class AstAtanhD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstAtanhD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::atanh(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$atanh(%l)"; }
    string emitC() override { ਵਾਪਸ "atanh(%li)"; }
};
class AstCeilD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstCeilD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::ceil(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$ceil(%l)"; }
    string emitC() override { ਵਾਪਸ "ceil(%li)"; }
};
class AstCosD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstCosD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::ਕੋਜੀਵਾ(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$cos(%l)"; }
    string emitC() override { ਵਾਪਸ "cos(%li)"; }
};
class AstCoshD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstCoshD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::cosh(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$cosh(%l)"; }
    string emitC() override { ਵਾਪਸ "cosh(%li)"; }
};
class AstExpD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstExpD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::ਘਾਤਾਂਕ(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$exp(%l)"; }
    string emitC() override { ਵਾਪਸ "exp(%li)"; }
};
class AstFloorD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstFloorD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::floor(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$floor(%l)"; }
    string emitC() override { ਵਾਪਸ "floor(%li)"; }
};
class AstLog10D final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstLog10D(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::log10(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$log10(%l)"; }
    string emitC() override { ਵਾਪਸ "log10(%li)"; }
};
class AstLogD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstLogD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::ਲਘੂਗਣਕ(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$ln(%l)"; }
    string emitC() override { ਵਾਪਸ "log(%li)"; }
};
class AstSinD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstSinD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::ਜੀਵਾ(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$sin(%l)"; }
    string emitC() override { ਵਾਪਸ "sin(%li)"; }
};
class AstSinhD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstSinhD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::sinh(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$sinh(%l)"; }
    string emitC() override { ਵਾਪਸ "sinh(%li)"; }
};
class AstSqrtD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstSqrtD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::ਵਰਗਮੂਲ(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$sqrt(%l)"; }
    string emitC() override { ਵਾਪਸ "sqrt(%li)"; }
};
class AstTanD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstTanD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::tan(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$tan(%l)"; }
    string emitC() override { ਵਾਪਸ "tan(%li)"; }
};
class AstTanhD final : ਜਨਤਕ Ast_ਗੰਢ_System_Uniop_D {
ਜਨਤਕ:
    AstTanhD(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    ਖਾਲੀ ਸੰਖਿਆ_ਚਲਾਓ(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs) override {
        ਬਾਹਰਲਾ.ਸੈੱਟ_Double(std::tanh(lhs.ਮੰਜ਼ਿਲ_Double()));
    }
    string emitVerilog() override { ਵਾਪਸ "%f$tanh(%l)"; }
    string emitC() override { ਵਾਪਸ "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : ਜਨਤਕ Ast_ਗੰਢ_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
ਜਨਤਕ:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    ਸਤਰਅੰਦਰ AstVarRef(File_ਸਤਰ* fl, AstVar* varp, ਅਚਲ VAccess& access);
    ਸਤਰਅੰਦਰ AstVarRef(File_ਸਤਰ* fl, Ast_ਗੰਢ_Module* classOrPackagep, AstVar* varp,
                     ਅਚਲ VAccess& access);
    // This form only allowed post-link (see above)
    ਸਤਰਅੰਦਰ AstVarRef(File_ਸਤਰ* fl, AstVarScope* varscp, ਅਚਲ VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() ਅਚਲ { ਵਾਪਸ m_name; }
    ਸਤਰਅੰਦਰ string name() ਅਚਲ override;  // * = Var name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    ਸਤਰਅੰਦਰ bool same_ਗੰਢ(ਅਚਲ AstVarRef* samep) ਅਚਲ;
    ਸਤਰਅੰਦਰ bool sameNoLvalue(ਅਚਲ AstVarRef* samep) ਅਚਲ;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstVarXRef final : ਜਨਤਕ Ast_ਗੰਢ_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
ਜਨਤਕ:
    AstVarXRef(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& dotted, ਅਚਲ VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    ਸਤਰਅੰਦਰ AstVarXRef(File_ਸਤਰ* fl, AstVar* varp, ਅਚਲ string& dotted, ਅਚਲ VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    ਖਾਲੀ name(ਅਚਲ std::string& name) override { m_name = name; }  // * = Var name
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string dotted() ਅਚਲ { ਵਾਪਸ m_dotted; }
    ਖਾਲੀ dotted(ਅਚਲ string& dotted) { m_dotted = dotted; }
    string inlinedDots() ਅਚਲ { ਵਾਪਸ m_inlinedDots; }
    ਖਾਲੀ inlinedDots(ਅਚਲ string& flag) { m_inlinedDots = flag; }
    bool ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ() ਅਚਲ { ਵਾਪਸ m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ; }
    ਖਾਲੀ ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ(ਅਚਲ bool flag) { m_ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ = flag; }
    bool readOnlyModport() ਅਚਲ { ਵਾਪਸ m_readOnlyModport; }
    ਖਾਲੀ readOnlyModport(ਅਚਲ bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ਬਾਹਰਲਾ() ਅਚਲ override { ਵਾਪਸ true; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ ਚੌੜਾਈ_Instrs(); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        ਵਾਪਸ (self_ਸੂਚਕ() == asamep->self_ਸੂਚਕ() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#ਜੇ_ਅੰਤ  // Guard
