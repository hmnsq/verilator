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
#ଅନ୍ତର୍ଭାଵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // An expression tree node
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(VNType t, File_ଧାଡ଼ି* fl)
        : Ast_ଗଣ୍ଠି{t, fl} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    // Every expression must have a data type after V3Width
    bool hasDType() ଅଚଳ override final VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଆଭାସୀ string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    ଆଭାସୀ string emitC() = 0;
    ଆଭାସୀ string emitSMT() ଅଚଳ { ଫେରନ୍ତୁ ""; };
    ଆଭାସୀ string emitSimpleOperator() { ଫେରନ୍ତୁ ""; }  // "" means not ok to use
    ଆଭାସୀ bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() { ଫେରନ୍ତୁ false; }  // Check VL_MULS_MAX_WORDS
    ଆଭାସୀ bool cleanOut() ଅଚଳ = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    ଆଭାସୀ bool isMultiCycleSva() ଅଚଳ { ଫେରନ୍ତୁ false; }
    ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getVAccessTargetRecurse() ଅଚଳ;
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getVAccessTargetRecurse() {
        ଫେରନ୍ତୁ const_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() ଅଚଳ;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() ଅଚଳ { ଫେରନ୍ତୁ getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_ବୃକ୍ଷ() ଅଚଳ;
    // Return base var (or const) nodep dereferences
    Ast_ଗଣ୍ଠି* baseFromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    ଧାଡ଼ିଭିତରେ Ast_Stmt_ପଦସମଷ୍ଟି* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    ଆଭାସୀ ଅଚଳ ଅକ୍ଷର* widthMismatch() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string patternString(uint32_t depth = 0) ଅଚଳ;
    bool isFourstateOrNoDtype() ଅଚଳ {
        ଯଦି (!dtypep()) ଫେରନ୍ତୁ true;
        ଫେରନ୍ତୁ dtypep()->skipRefp()->isFourstate();
    }
    ସ୍ଥିର bool isFourstateOrNoDtype(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p) {
        ଫେରନ୍ତୁ expr1p->isFourstateOrNoDtype();
    }
    ସ୍ଥିର bool isFourstateOrNoDtype(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p,
                                     ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr2p) {
        ଫେରନ୍ତୁ expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    ସ୍ଥିର bool isFourstateOrNoDtype(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p,
                                     ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr2p,
                                     ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr3p) {
        ଫେରନ୍ତୁ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    ସ୍ଥିର bool isFourstateOrNoDtype(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr1p,
                                     ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr2p,
                                     ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr3p,
                                     ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ expr4p) {
        ଫେରନ୍ତୁ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_ଗଣ୍ଠି_Biop VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Biop(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    ଆଭାସୀ ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) = 0;
    ଆଭାସୀ bool cleanLhs() ଅଚଳ = 0;  // True if LHS must have extra upper bits zero
    ଆଭାସୀ bool cleanRhs() ଅଚଳ = 0;  // True if RHS must have extra upper bits zero
    ଆଭାସୀ bool sizeMattersLhs() ଅଚଳ = 0;  // True if output result depends on lhs size
    ଆଭାସୀ bool sizeMattersRhs() ଅଚଳ = 0;  // True if output result depends on rhs size
    ଆଭାସୀ bool doubleFlavor() ଅଚଳ { ଫେରନ୍ତୁ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    ଆଭାସୀ bool signedFlavor() ଅଚଳ { ଫେରନ୍ତୁ false; }
    ଆଭାସୀ bool stringFlavor() ଅଚଳ { ଫେରନ୍ତୁ false; }  // N flavor of nodes with both flavors?
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isPure() override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;

ବ୍ୟକ୍ତିଗତ:
    bool getPurityRecurse() ଅଚଳ { ଫେରନ୍ତୁ lhsp()->isPure() && rhsp()->isPure(); }
};
class Ast_ଗଣ୍ଠି_Bi_Com VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Binary expr with commutative properties
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Bi_Com(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhs, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhs)
        : Ast_ଗଣ୍ଠି_Biop{t, fl, lhs, rhs} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Bi_Com;
};
class Ast_ଗଣ୍ଠି_Bi_Com_Asv VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
    // Binary expr with commutative & associative properties
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Bi_Com_Asv(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhs, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhs)
        : Ast_ଗଣ୍ଠି_Bi_Com{t, fl, lhs, rhs} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Bi_Com_Asv;
};
class Ast_ଗଣ୍ଠି_Dist_Biop VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_Dist_Biop(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : Ast_ଗଣ୍ଠି_Biop{t, fl, lhsp, rhsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Dist_Biop;
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL_TRIG; }
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
};
class Ast_ଗଣ୍ଠି_Sel VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Sel(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp)
        : Ast_ଗଣ୍ଠି_Biop{t, fl, fromp, bitp} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Sel;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitConst() ଅଚଳ;
};
class Ast_ଗଣ୍ଠି_Stream VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Stream(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : Ast_ଗଣ୍ଠି_Biop{t, fl, lhsp, rhsp} {
        ଯଦି (lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(lhsp->width(), lhsp->widthMin(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Stream;
};
class Ast_ଗଣ୍ଠି_System_Biop_D VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_System_Biop_D(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : Ast_ଗଣ୍ଠି_Biop{t, fl, lhsp, rhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_System_Biop_D;
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL_TRIG; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_ଗଣ୍ଠି_CCall VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_CCall(VNType t, File_ଧାଡ଼ି* fl, AstCFunc* funcp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp = nullptr)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl}
        , m_funcp{funcp} {
        addArgsp(argsp);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_CCall;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_CALL; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ Ast_ଗଣ୍ଠି_CCall* ଅଚଳ asamep = VN_DBG_AS(samep, ଗଣ୍ଠି_CCall);
        ଫେରନ୍ତୁ (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override;
    bool isOutputter() override { ଫେରନ୍ତୁ !isPure(); }
    AstCFunc* funcp() ଅଚଳ { ଫେରନ୍ତୁ m_funcp; }
    ଶୂନ୍ୟ funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() ଅଚଳ { ଫେରନ୍ତୁ m_argTypes; }
    ଶୂନ୍ୟ argTypes(ଅଚଳ string& str) { m_argTypes = str; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ final override { ଫେରନ୍ତୁ true; }
    bool superReference() ଅଚଳ { ଫେରନ୍ତୁ m_superReference; }
    ଶୂନ୍ୟ superReference(bool flag) { m_superReference = flag; }
};
class Ast_ଗଣ୍ଠି_Cons VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Construct a value of an aggregate type
    friend class Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି;  // For m_knownConstantTree
    mutable bool m_known_Constant_ବୃକ୍ଷ = false;  // Known to be 'this->isConstantTree() == true'

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Cons(VNType t, File_ଧାଡ଼ି* fl)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Cons;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ final override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ final override { ଫେରନ୍ତୁ widthInstrs(); }
};
class Ast_ଗଣ୍ଠି_FTask_Ref VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_FTask_Ref;  // Gen pointers before other members for performance
ବ୍ୟକ୍ତିଗତ:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_FTask_Ref(VNType t, File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstArg* argsp)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl}
        , m_name{name} {
        addArgsp(argsp);
    }

ସର୍ବସାଧାରଣ:
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    bool isGateOptimizable() ଅଚଳ override;
    string dotted() ଅଚଳ { ଫେରନ୍ତୁ m_dotted; }  // * = Scope name or ""
    string inlinedDots() ଅଚଳ { ଫେରନ୍ତୁ m_inlinedDots; }
    ଶୂନ୍ୟ inlinedDots(ଅଚଳ string& flag) { m_inlinedDots = flag; }
    Ast_ଗଣ୍ଠି_FTask* taskp() ଅଚଳ { ଫେରନ୍ତୁ m_taskp; }  // [After Link] Pointer to variable
    ଶୂନ୍ୟ taskp(Ast_ଗଣ୍ଠି_FTask* taskp) { m_taskp = taskp; }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    ଶୂନ୍ୟ dotted(ଅଚଳ string& name) { m_dotted = name; }
    Ast_ଗଣ୍ଠି_Module* classOrPackagep() ଅଚଳ { ଫେରନ୍ତୁ m_classOrPackagep; }
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* nodep) { m_classOrPackagep = nodep; }
    bool containsGenBlock() ଅଚଳ { ଫେରନ୍ତୁ m_containsGenBlock; }
    ଶୂନ୍ୟ containsGenBlock(ଅଚଳ bool flag) { m_containsGenBlock = flag; }
    bool pli() ଅଚଳ { ଫେରନ୍ତୁ m_pli; }
    ଶୂନ୍ୟ pli(bool flag) { m_pli = flag; }
    bool superReference() ଅଚଳ { ଫେରନ୍ତୁ m_superReference; }
    ଶୂନ୍ୟ superReference(bool flag) { m_superReference = flag; }
    bool isPure() override;
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ m_taskp && m_taskp->isUnlikely(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ asamep = VN_DBG_AS(samep, ଗଣ୍ଠି_FTask_Ref);
        ଫେରନ୍ତୁ taskp() == asamep->taskp()  //
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
    bool cleanOut() ଅଚଳ final override { V3ERROR_NA_RETURN(true); }

ବ୍ୟକ୍ତିଗତ:
    bool getPurityRecurse() ଅଚଳ;
};
class Ast_ଗଣ୍ଠି_Pre_Sel VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Pre_Sel(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsp)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Pre_Sel;
    // METHODS
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ final override { V3ERROR_NA_RETURN(true); }
    bool isPure() override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;

ବ୍ୟକ୍ତିଗତ:
    bool getPurityRecurse() ଅଚଳ {
        ଫେରନ୍ତୁ fromp()->isPure() && rhsp()->isPure() && (!thsp() || thsp()->isPure());
    }
};
class Ast_ଗଣ୍ଠି_Quadop VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Quadop(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsp,
                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fhsp)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    ଆଭାସୀ ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs,
                               ଅଚଳ V3_ସଂଖ୍ୟା& ths, ଅଚଳ V3_ସଂଖ୍ୟା& fhs)
        = 0;
    ଆଭାସୀ bool cleanLhs() ଅଚଳ = 0;  // True if LHS must have extra upper bits zero
    ଆଭାସୀ bool cleanRhs() ଅଚଳ = 0;  // True if RHS must have extra upper bits zero
    ଆଭାସୀ bool cleanThs() ଅଚଳ = 0;  // True if THS must have extra upper bits zero
    ଆଭାସୀ bool cleanFhs() ଅଚଳ = 0;  // True if THS must have extra upper bits zero
    ଆଭାସୀ bool sizeMattersLhs() ଅଚଳ = 0;  // True if output result depends on lhs size
    ଆଭାସୀ bool sizeMattersRhs() ଅଚଳ = 0;  // True if output result depends on rhs size
    ଆଭାସୀ bool sizeMattersThs() ଅଚଳ = 0;  // True if output result depends on ths size
    ଆଭାସୀ bool sizeMattersFhs() ଅଚଳ = 0;  // True if output result depends on ths size
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isPure() override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;

ବ୍ୟକ୍ତିଗତ:
    bool getPurityRecurse() ଅଚଳ {
        ଫେରନ୍ତୁ lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure() && fhsp()->isPure();
    }
};
class Ast_ଗଣ୍ଠି_Termop VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Terminal operator -- an operator with no "inputs"
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Termop(VNType t, File_ଧାଡ଼ି* fl)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Termop;
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_ଗଣ୍ଠି_Triop VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Triop(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsp)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Triop;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    // Set out to evaluation of a AstConst'ed
    ଆଭାସୀ ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs,
                               ଅଚଳ V3_ସଂଖ୍ୟା& ths)
        = 0;
    ଆଭାସୀ bool cleanLhs() ଅଚଳ = 0;  // True if LHS must have extra upper bits zero
    ଆଭାସୀ bool cleanRhs() ଅଚଳ = 0;  // True if RHS must have extra upper bits zero
    ଆଭାସୀ bool cleanThs() ଅଚଳ = 0;  // True if THS must have extra upper bits zero
    ଆଭାସୀ bool sizeMattersLhs() ଅଚଳ = 0;  // True if output result depends on lhs size
    ଆଭାସୀ bool sizeMattersRhs() ଅଚଳ = 0;  // True if output result depends on rhs size
    ଆଭାସୀ bool sizeMattersThs() ଅଚଳ = 0;  // True if output result depends on ths size
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isPure() override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;

ବ୍ୟକ୍ତିଗତ:
    bool getPurityRecurse() ଅଚଳ {
        ଫେରନ୍ତୁ lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure();
    }
};
class Ast_ଗଣ୍ଠି_Dist_Triop VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Triop {
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_Dist_Triop(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp,
                     Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsp)
        : Ast_ଗଣ୍ଠି_Triop{t, fl, lhsp, rhsp, thsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Dist_Triop;
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanThs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersThs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL_TRIG; }
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs,
                       ଅଚଳ V3_ସଂଖ୍ୟା& ths) override {
        V3ERROR_NA;
    }
};
class Ast_ଗଣ୍ଠି_Uniop VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Uniop(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Uniop;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    // Set out to evaluation of a AstConst'ed lhs
    ଆଭାସୀ ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) = 0;
    ଆଭାସୀ bool cleanLhs() ଅଚଳ = 0;
    ଆଭାସୀ bool sizeMattersLhs() ଅଚଳ = 0;  // True if output result depends on lhs size
    ଆଭାସୀ bool doubleFlavor() ଅଚଳ { ଫେରନ୍ତୁ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    ଆଭାସୀ bool signedFlavor() ଅଚଳ { ଫେରନ୍ତୁ false; }
    ଆଭାସୀ bool stringFlavor() ଅଚଳ { ଫେରନ୍ତୁ false; }  // N flavor of nodes with both flavors?
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isPure() override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
};
class Ast_ଗଣ୍ଠି_System_Uniop_D VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_System_Uniop_D(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : Ast_ଗଣ୍ଠି_Uniop{t, fl, lhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_System_Uniop_D;
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL_TRIG; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_ଗଣ୍ଠି_Var_Ref VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Var_Ref;  // Gen pointers before other members for performance
ବ୍ୟକ୍ତିଗତ:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Var_Ref(VNType t, File_ଧାଡ଼ି* fl, AstVar* varp, ଅଚଳ VAccess& access)
        : Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    VAccess access() ଅଚଳ { ଫେରନ୍ତୁ m_access; }
    ଶୂନ୍ୟ access(ଅଚଳ VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_varp; }  // [After Link] Pointer to variable
    ଶୂନ୍ୟ varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_ଗଣ୍ଠି*>(varp));
    }
    AstVarScope* varScopep() ଅଚଳ { ଫେରନ୍ତୁ m_varScopep; }
    ଶୂନ୍ୟ varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    ଅଚଳ VSelfPointerText& selfPointer() ଅଚଳ { ଫେରନ୍ତୁ m_selfPointer; }
    ଶୂନ୍ୟ selfPointer(ଅଚଳ VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) ଅଚଳ {
        ଫେରନ୍ତୁ selfPointer().protect(useSelfForThis, protect());
    }
    Ast_ଗଣ୍ଠି_Module* classOrPackagep() ଅଚଳ { ଫେରନ୍ତୁ m_classOrPackagep; }
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
ସର୍ବସାଧାରଣ:
    AstArg(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ std::string& name) override { m_name = name; }
    bool emptyConnectNoNext() ଅଚଳ { ଫେରନ୍ତୁ !exprp() && name() == "" && !nextp(); }
};
class AstWith final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
ବ୍ୟକ୍ତିଗତ:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::set<std::string> m_restrictedNames;

ସର୍ବସାଧାରଣ:
    AstWith(File_ଧାଡ଼ି* fl, AstLambdaArgRef* indexArgRefp, AstLambdaArgRef* ମୂଲ୍ୟ_Arg_Refp,
            Ast_ଗଣ୍ଠି* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->indexArgRefp(indexArgRefp);
        this->ମୂଲ୍ୟ_Arg_Refp(ମୂଲ୍ୟ_Arg_Refp);
        addExprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!indexArgRefp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!ମୂଲ୍ୟ_Arg_Refp());  // varp needed to know lambda's arg dtype
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ restricted(bool flag) { m_restricted = flag; }
    bool restricted() ଅଚଳ { ଫେରନ୍ତୁ m_restricted; }
    bool validated() ଅଚଳ { ଫେରନ୍ତୁ m_validated; }
    ଶୂନ୍ୟ validated(bool flag) { m_validated = flag; }
    ଶୂନ୍ୟ addRestrictedName(ଅଚଳ std::string& name) { m_restrictedNames.ଭର୍ତ୍ତି(name); }
    ଅଚଳ std::set<std::string>& restrictedNames() ଅଚଳ { ଫେରନ୍ତୁ m_restrictedNames; }
    // True if 'name' binds into the randomize() target class.
    bool nameResolvesToTarget(ଅଚଳ std::string& name) ଅଚଳ {
        ଫେରନ୍ତୁ !m_restricted || m_restrictedNames.ଗଣନା_2(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbortKind m_kind{VAbortKind::ACCEPT_ON};

ସର୍ବସାଧାରଣ:
    AstAbortOn(File_ଧାଡ଼ି* fl, VAbortKind kind, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_kind{kind} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    VAbortKind kind() ଅଚଳ { ଫେରନ୍ତୁ m_kind; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstAddrOfCFunc final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

ସର୍ବସାଧାରଣ:
    AstAddrOfCFunc(File_ଧାଡ଼ି* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_AstAddrOfCFunc;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    AstCFunc* funcp() ଅଚଳ { ଫେରନ୍ତୁ m_funcp; }
};
class AstAttrOf final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
ସର୍ବସାଧାରଣ:
    AstAttrOf(File_ଧାଡ଼ି* fl, VAttrType attrtype, Ast_ଗଣ୍ଠି* fromp = nullptr,
              Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* dimp = nullptr)
        : ASTGEN_SUPER_AttrOf(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_AstAttrOf;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    VAttrType attrType() ଅଚଳ { ଫେରନ୍ତୁ m_attrType; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ଅଚଳ bool m_pure;  // Pure optimizable

    ଶୂନ୍ୟ init(ଅଚଳ string& text, ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth) {
        ଯଦି (!text.empty()) add(text);
        ଯଦି (setwidth) {
            dtypeSetBitSized(setwidth, VSigning::UNSIGNED);
        } ଅନ୍ୟଥା {
            dtypeSetVoid();  // Caller to override if necessary
        }
    }

ସର୍ବସାଧାରଣ:
    class Pure {};
    explicit AstCExpr(File_ଧାଡ଼ି* fl, ଅଚଳ string& text = "", ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{false} {
        init(text, setwidth);
    }
    AstCExpr(File_ଧାଡ଼ି* fl, Pure, ଅଚଳ string& text = "", ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ m_pure; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ m_pure; }
    bool isPure() override { ଫେରନ୍ତୁ m_pure; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    // Add some text, or a node to this expression
    ଶୂନ୍ୟ add(ଅଚଳ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ଶୂନ୍ୟ add(Ast_ଗଣ୍ଠି* nodep) { addNodesp(nodep); }
};
class AstCExprUser final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ଅଚଳ bool m_pure;  // Whether the function is pure
ସର୍ବସାଧାରଣ:
    class Pure {};
    explicit AstCExprUser(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{false} {}
    AstCExprUser(File_ଧାଡ଼ି* fl, Pure)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ m_pure; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ m_pure; }
    bool isPure() override { ଫେରନ୍ତୁ m_pure; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    // Add some text, or a node to this expression
    ଶୂନ୍ୟ add(ଅଚଳ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ଶୂନ୍ୟ add(Ast_ଗଣ୍ଠି* nodep) { addNodesp(nodep); }
};
class AstCFuncHard final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
ସର୍ବସାଧାରଣ:
    AstCFuncHard(File_ଧାଡ଼ି* fl, VCFunction function, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ function().ascii(); }
    bool isPure() override {
        ଯଦି (!m_purity.isCached()) m_purity.set(getPurity());
        ଫେରନ୍ତୁ m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    VCFunction function() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_function; }

ବ୍ୟକ୍ତିଗତ:
    bool getPurity();
};
class AstCMethodHard final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_method;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
ସର୍ବସାଧାରଣ:
    AstCMethodHard(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, VCMethod method, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_method{method} {
        this->fromp(fromp);
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ method().ascii(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstCMethodHard* ଅଚଳ asamep = VN_DBG_AS(samep, CMethodHard);
        ଫେରନ୍ତୁ (m_method == asamep->m_method);
    }
    bool isPure() override {
        ଯଦି (!m_purity.isCached()) m_purity.set(getPurity());
        ଫେରନ୍ତୁ m_purity.get();
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ m_method.isClean(); }
    bool usePtr() ଅଚଳ { ଫେରନ୍ତୁ m_usePtr; }
    ଶୂନ୍ୟ usePtr(bool flag) { m_usePtr = flag; }
    VCMethod method() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_method; }
    ଶୂନ୍ୟ method(VCMethod ମୂଲ୍ୟ) { m_method = ମୂଲ୍ୟ; }

ବ୍ୟକ୍ତିଗତ:
    bool getPurity();
};
class AstCReset final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Reset variable at startup
    ଅଚଳ bool m_constructing;  // Previously cleared by constructor
ସର୍ବସାଧାରଣ:
    AstCReset(File_ଧାଡ଼ି* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isPure() override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!VN_IS(backp(), ଗଣ୍ଠି_Assign));  // V3Emit* assumption
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() ଅଚଳ { ଫେରନ୍ତୁ m_constructing; }
};
class AstCast final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
ସର୍ବସାଧାରଣ:
    AstCast(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { ଫେରନ୍ତୁ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
ସର୍ବସାଧାରଣ:
    AstCastParse(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { ଫେରନ୍ତୁ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstCastSize final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
ସର୍ବସାଧାରଣ:
    AstCastSize(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_CastSize(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCastSize;
    string emitVerilog() override { ଫେରନ୍ତୁ "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
ସର୍ବସାଧାରଣ:
    AstCellArrayRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        addSelp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
ସର୍ବସାଧାରଣ:
    AstCellRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* cellp, Ast_ଗଣ୍ଠି* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
ସର୍ବସାଧାରଣ:
    AstClassOrPackageRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_ଗଣ୍ଠି_Module* classOrPackageSkipp(ଅଚଳ bool doRefs = true) ଅଚଳ;
    Ast_ଗଣ୍ଠି* classOrPackageNodep() ଅଚଳ { ଫେରନ୍ତୁ m_classOrPackageNodep; }
    ଶୂନ୍ୟ classOrPackageNodep(Ast_ଗଣ୍ଠି* nodep) { m_classOrPackageNodep = nodep; }
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_ଗଣ୍ଠି*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A constant
    V3_ସଂଖ୍ୟା m_ଅଙ୍କ;  // Constant value
    ଶୂନ୍ୟ init_With_ସଂଖ୍ୟା() {
        ଯଦି (m_ଅଙ୍କ.isDouble()) {
            dtypeSetDouble();
        } ଅନ୍ୟଥା ଯଦି (m_ଅଙ୍କ.isString()) {
            dtypeSetString();
        } ଅନ୍ୟଥା {
            dtypeSetBitOrLogicUnsized(m_ଅଙ୍କ.width(), (m_ଅଙ୍କ.sized() ? 0 : m_ଅଙ୍କ.widthToFit()),
                                      VSigning::fromBool(m_ଅଙ୍କ.isSigned()), m_ଅଙ୍କ.isAnyXZ());
        }
        m_ଅଙ୍କ.nodep(this);
    }

ସର୍ବସାଧାରଣ:
    ~AstConst() override;
    AstConst(File_ଧାଡ଼ି* fl, ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{ଅଙ୍କ} {
        init_With_ସଂଖ୍ୟା();
    }
    class Widthed_ମୂଲ୍ୟ {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, Widthed_ମୂଲ୍ୟ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, uint32_t ମୂଲ୍ୟ)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, width, ମୂଲ୍ୟ) {  // Need () constructor
        init_With_ସଂଖ୍ୟା();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_ଧାଡ଼ି* fl, DTyped, ଅଚଳ Ast_ଗଣ୍ଠି_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{this, nodedtypep} {
        init_With_ସଂଖ୍ୟା();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, StringToParse, ଅଚଳ ଅକ୍ଷର* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{this, V3_ସଂଖ୍ୟା::Verilog_ସଂଖ୍ୟା_Literal{}, sourcep} {
        init_With_ସଂଖ୍ୟା();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, VerilogStringLiteral, ଅଚଳ string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{this, V3_ସଂଖ୍ୟା::VerilogStringLiteral{}, str} {
        init_With_ସଂଖ୍ୟା();
    }
    AstConst(File_ଧାଡ଼ି* fl, uint32_t ଅଙ୍କ)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, 32, ଅଙ୍କ) {  // Need () constructor
        dtypeSetBitUnsized(m_ଅଙ୍କ.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, Unsized32, uint32_t ଅଙ୍କ)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, 32, ଅଙ୍କ) {  // Need () constructor
        m_ଅଙ୍କ.width(32, false);
        dtypeSetBitUnsized(32, m_ଅଙ୍କ.widthToFit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, Signed32, int32_t ଅଙ୍କ)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, 32, ଅଙ୍କ) {  // Need () constructor
        m_ଅଙ୍କ.width(32, true);
        dtypeSetBitUnsized(32, m_ଅଙ୍କ.widthToFit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, Unsized64, uint64_t ଅଙ୍କ)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, 64, 0) {  // Need () constructor
        m_ଅଙ୍କ.setQuad(ଅଙ୍କ);
        dtypeSetBitSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, SizedEData, uint64_t ଅଙ୍କ)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_ଅଙ୍କ.setQuad(ଅଙ୍କ);
        dtypeSetBitSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, RealDouble, ଦ୍ୱିଗୁଣ ଅଙ୍କ)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, 64) {  // Need () constructor
        m_ଅଙ୍କ.setDouble(ଅଙ୍କ);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_ଧାଡ଼ି* fl, String, ଅଚଳ string& ଅଙ୍କ)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{this, V3_ସଂଖ୍ୟା::String{}, ଅଙ୍କ} {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(File_ଧାଡ଼ି* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_ଧାଡ଼ି* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
        File_ଧାଡ଼ି* ଅଚଳ newfl = new File_ଧାଡ଼ି{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_ଧାଡ଼ି* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ(this, 1, on) {  // Need () constructor
        dtypeSetBit();
    }
    class All0 {};
    AstConst(File_ଧାଡ଼ି* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{this, V3_ସଂଖ୍ୟା::Verilog_ସଂଖ୍ୟା_Literal{}, "'0"} {
        init_With_ସଂଖ୍ୟା();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_ଧାଡ଼ି* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{this, V3_ସଂଖ୍ୟା::Verilog_ସଂଖ୍ୟା_Literal{}, "'1"} {
        init_With_ସଂଖ୍ୟା();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_ଧାଡ଼ି* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{this, V3_ସଂଖ୍ୟା::Null{}} {
        dtypep(findNullDType());
        init_With_ସଂଖ୍ୟା();
    }
    class OneStep {};
    AstConst(File_ଧାଡ଼ି* fl, OneStep)
        : ASTGEN_SUPER_Const(fl)
        , m_ଅଙ୍କ{this, V3_ସଂଖ୍ୟା::OneStep{}} {
        dtypeSetBitSized(64, VSigning::UNSIGNED);
        init_With_ସଂଖ୍ୟା();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ ଅଙ୍କ().ascii(); }  // * = Value
    ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_ଅଙ୍କ; }  // * = Value
    V3_ସଂଖ୍ୟା& ଅଙ୍କ() { ଫେରନ୍ତୁ m_ଅଙ୍କ; }  // * = Value
    string origParamName() ଅଚଳ;
    ଶୂନ୍ୟ origParamName(ଅଚଳ string& name);
    uint32_t toUInt() ଅଚଳ { ଫେରନ୍ତୁ ଅଙ୍କ().toUInt(); }
    int32_t toSInt() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ ଅଙ୍କ().toSInt(); }
    uint64_t toUQuad() ଅଚଳ { ଫେରନ୍ତୁ ଅଙ୍କ().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstConst* ଅଚଳ sp = VN_DBG_AS(samep, Const);
        ଫେରନ୍ତୁ ଅଙ୍କ().isCaseEq(sp->ଅଙ୍କ());
    }
    ଶୂନ୍ୟ cloneRelink() override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(m_ଅଙ୍କ.nodep() && m_ଅଙ୍କ.nodep() != this);
        ଫେରନ୍ତୁ nullptr;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool isEqAllOnes() ଅଚଳ { ଫେରନ୍ତୁ ଅଙ୍କ().isEqAllOnes(width()); }
    bool isEqAllOnesV() ଅଚଳ { ଫେରନ୍ତୁ ଅଙ୍କ().isEqAllOnes(widthMinV()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_ମୂଲ୍ୟ_Type(ଅଚଳ AstConst* samep) ଅଚଳ;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    ସ୍ଥିର AstConst* parseParamLiteral(File_ଧାଡ଼ି* fl, ଅଚଳ string& literal);
};
class AstConstraintRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

ସର୍ବସାଧାରଣ:
    AstConstraintRef(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() ଅଚଳ override VL_MT_STABLE;
    AstConstraint* constrp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_constrp; }
    ଶୂନ୍ୟ constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_ଗଣ୍ଠି_Module* classOrPackagep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_classOrPackagep; }
    // TODO: this setter is unused
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ final override { V3ERROR_NA_RETURN(true); }
};
class AstCvtArrayToArray final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_blockSize;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_dstElementBits;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_srcElementBits;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

ସର୍ବସାଧାରଣ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToArray(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_DType* dtp, bool reverse,
                       ପୂର୍ଣ୍ଣସଂଖ୍ୟା blockSize, ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstElementBits, ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcElementBits)
        : ASTGEN_SUPER_CvtArrayToArray(fl)
        , m_reverse{reverse}
        , m_blockSize{blockSize}
        , m_dstElementBits{dstElementBits}
        , m_srcElementBits{srcElementBits} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToArray;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool reverse() ଅଚଳ { ଫେରନ୍ତୁ m_reverse; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା blockSize() ଅଚଳ { ଫେରନ୍ତୁ m_blockSize; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା dstElementBits() ଅଚଳ { ଫେରନ୍ତୁ m_dstElementBits; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା srcElementBits() ଅଚଳ { ଫେରନ୍ତୁ m_srcElementBits; }
};
class AstCvtArrayToPacked final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToPacked(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_CvtArrayToPacked(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToPacked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCvtPackedToArray final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtPackedToArray(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_CvtPackedToArray(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtPackedToArray;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCvtUnpackToQueue final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtUnpackToQueue(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToQueue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToQueue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { ଫେରନ୍ତୁ "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCvtUnpackToUnpack final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstCvtUnpackToUnpack(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToUnpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToUnpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstDist final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
ସର୍ବସାଧାରଣ:
    AstDist(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { ଫେରନ୍ତୁ "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // NA
};
class AstDistItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_isWhole = false;  // True for weight ':/', false for ':='
ସର୍ବସାଧାରଣ:
    AstDistItem(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rangep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { ଫେରନ୍ତୁ "%l "s + (m_isWhole ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // NA
    ଶୂନ୍ୟ isWhole(bool flag) { m_isWhole = flag; }
    bool isWhole() ଅଚଳ { ଫେରନ୍ତୁ m_isWhole; }
};
class AstDot final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    ଅଚଳ bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
ସର୍ବସାଧାରଣ:
    AstDot(File_ଧାଡ଼ି* fl, bool colon, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newIfPkg(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* packageOrClassp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        ଯଦି (!packageOrClassp) ଫେରନ୍ତୁ rhsp;
        ଫେରନ୍ତୁ new AstDot{fl, true, packageOrClassp, rhsp};
    }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool colon() ଅଚଳ { ଫେରନ୍ତୁ m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstEmptyQueue final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
ସର୍ବସାଧାରଣ:
    explicit AstEmptyQueue(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_AstEmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { ଫେରନ୍ତୁ "{}"; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstEnumItemRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_containsGenBlock = false;  // Contains gen block reference
ସର୍ବସାଧାରଣ:
    AstEnumItemRef(File_ଧାଡ଼ି* fl, AstEnumItem* itemp, Ast_ଗଣ୍ଠି_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ itemp() ? itemp()->name() : m_name; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstEnumItemRef* ଅଚଳ sp = VN_DBG_AS(samep, EnumItemRef);
        ଫେରନ୍ତୁ itemp() == sp->itemp() && dotted() == sp->dotted()
               && containsGenBlock() == sp->containsGenBlock();
    }
    AstEnumItem* itemp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_itemp; }
    ଶୂନ୍ୟ itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() ଅଚଳ { ଫେରନ୍ତୁ m_dotted; }
    ଶୂନ୍ୟ dotted(ଅଚଳ string& dotted) { m_dotted = dotted; }
    bool containsGenBlock() ଅଚଳ { ଫେରନ୍ତୁ m_containsGenBlock; }
    ଶୂନ୍ୟ containsGenBlock(ଅଚଳ bool flag) { m_containsGenBlock = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_Module* classOrPackagep() ଅଚଳ { ଫେରନ୍ତୁ m_classOrPackagep; }
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_ପଦସମଷ୍ଟି_Stmt final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
ବ୍ୟକ୍ତିଗତ:
    bool m_hasResult = true;  // Returns result via resultp()

ସର୍ବସାଧାରଣ:
    Ast_ପଦସମଷ୍ଟି_Stmt(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp)
        : ASTGEN_SUPER_ପଦସମଷ୍ଟି_Stmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_ପଦସମଷ୍ଟି_Stmt;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isPure() override {
        ଯଦି (Ast_ଗଣ୍ଠି::afterCommentp(stmtsp())) ଫେରନ୍ତୁ false;
        ଫେରନ୍ତୁ resultp()->isPure();
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool hasResult() ଅଚଳ { ଫେରନ୍ତୁ m_hasResult; }
    ଶୂନ୍ୟ hasResult(bool flag) { m_hasResult = flag; }
};
class AstFError final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstFError(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 64; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFOpen final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstFOpen(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filenamep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* modep)
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
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$fopen"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFOpenMcd final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op2 := filenamep : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstFOpenMcd(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$fopen"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFRead final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
ସର୍ବସାଧାରଣ:
    AstFRead(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* memp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* startp,
             Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: makes output
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFRewind final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := filep : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    AstFRewind(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFScanF final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

ସର୍ବସାଧାରଣ:
    AstFScanF(File_ଧାଡ଼ି* fl, ଅଚଳ string& text, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_text; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: makes output
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() ଅଚଳ { ଫେରନ୍ତୁ m_text; }  // * = Text to display
    ଶୂନ୍ୟ text(ଅଚଳ string& text) { m_text = text; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFSeek final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    AstFSeek(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* offset, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: makes output
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFTell final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
ସର୍ବସାଧାରଣ:
    AstFTell(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFalling final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstFalling(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { ଫେରନ୍ତୁ "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFell final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ସର୍ବସାଧାରଣ:
    AstFell(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_Sen_ବୃକ୍ଷ* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { ଫେରନ୍ତୁ "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFuture final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ସର୍ବସାଧାରଣ:
    AstFuture(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_Sen_ବୃକ୍ଷ* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { ଫେରନ୍ତୁ "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstGatePin final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
ସର୍ବସାଧାରଣ:
    AstGatePin(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { ଫେରନ୍ତୁ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstGetInitialRandomSeed final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $get_initial_random_seed()
ସର୍ବସାଧାରଣ:
    explicit AstGetInitialRandomSeed(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_GetInitialRandomSeed(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstGetInitialRandomSeed;
    string emitVerilog() override { ଫେରନ୍ତୁ "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isPure() override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstImplication final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    ଅଚଳ bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

ସର୍ବସାଧାରଣ:
    AstImplication(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() ଅଚଳ { ଫେରନ୍ତୁ m_isOverlapped; }
    bool isFollowedBy() ଅଚଳ { ଫେରନ୍ତୁ m_isFollowedBy; }
};
class AstInside final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    AstInside(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { ଫେରନ୍ତୁ "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // NA
};
class AstInsideRange final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstInsideRange(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { ଫେରନ୍ତୁ "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newAndFromInside(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp);
};
class AstLambdaArgRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_index;  // Index, not value

ସର୍ବସାଧାରଣ:
    AstLambdaArgRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, bool index)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_index{index} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    string emitVerilog() override { ଫେରନ୍ତୁ name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    bool index() ଅଚଳ { ଫେରନ୍ତୁ m_index; }
    bool is_ପଦସମଷ୍ଟି_Coverage_Eligible() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstMatchMasked final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
ସର୍ବସାଧାରଣ:
    ଧାଡ଼ିଭିତରେ AstMatchMasked(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_AstMatchMasked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { ଫେରନ୍ତୁ "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ସ୍ଥିର uint32_t fold(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, AstVar* matchVarp);
};
class AstMatches final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
ସର୍ବସାଧାରଣ:
    AstMatches(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { ଫେରନ୍ତୁ "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstMemberSel final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
ସର୍ବସାଧାରଣ:
    AstMemberSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, VFlagChildDType, ଅଚଳ string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    VAccess access() ଅଚଳ { ଫେରନ୍ତୁ m_access; }
    ଶୂନ୍ୟ access(ଅଚଳ VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    AstVar* varp() ଅଚଳ { ଫେରନ୍ତୁ m_varp; }
    ଶୂନ୍ୟ varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstNewCopy(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { ଫେରନ୍ତୁ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
};
class AstNewDynamic final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    AstNewDynamic(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sizep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { ଫେରନ୍ତୁ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
};
class AstPExpr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
ସର୍ବସାଧାରଣ:
    explicit AstPExpr(File_ଧାଡ଼ି* fl, AstBegin* bodyp, Ast_ଗଣ୍ଠି_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_ଧାଡ଼ି* fl, AstBegin* bodyp, Ast_ଗଣ୍ଠି_Stmt* finalp, Ast_ଗଣ୍ଠି_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
};
class AstParseHolder final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
ସର୍ବସାଧାରଣ:
    explicit AstParseHolder(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

ସର୍ବସାଧାରଣ:
    AstParseRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nullptr,
                Ast_ଗଣ୍ଠି_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstParseRef* ଅଚଳ asamep = VN_DBG_AS(samep, ParseRef);
        ଫେରନ୍ତୁ m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

ସର୍ବସାଧାରଣ:
    AstPast(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ticksp = nullptr,
            Ast_Sen_ବୃକ୍ଷ* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() ଅଚଳ { ଫେରନ୍ତୁ m_propertyTiming; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstPatMember final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
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

ସର୍ବସାଧାରଣ:
    AstPatMember(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhssp, Ast_ଗଣ୍ଠି* keyp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { ଫେରନ୍ତୁ lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 2; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    bool isConcat() ଅଚଳ { ଫେରନ୍ତୁ m_isConcat; }
    ଶୂନ୍ୟ isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() ଅଚଳ { ଫେରନ୍ତୁ m_isDefault; }
    ଶୂନ୍ୟ isDefault(bool flag) { m_isDefault = flag; }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଚାବି_Exprp() ଅଚଳ { ଫେରନ୍ତୁ varrefp() ? varrefp() : VN_CAST(keyp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି); }
};
class AstPattern final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
ସର୍ବସାଧାରଣ:
    AstPattern(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstPattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ dtypep() ? dtypep() : childDTypep(); }
};
class AstPatternStar final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Pattern wildcard: ".*"
ସର୍ବସାଧାରଣ:
    explicit AstPatternStar(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_PatternStar(fl) {}
    ASTGEN_MEMBERS_AstPatternStar;
    string emitVerilog() override { ଫେରନ୍ତୁ ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstPatternVar final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
ସର୍ବସାଧାରଣ:
    AstPatternVar(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_PatternVar(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPatternVar;
    string emitVerilog() override { ଫେରନ୍ତୁ ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_name == VN_DBG_AS(samep, PatternVar)->m_name;
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
};
class AstPropAlways final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    ଅଚଳ bool m_isStrong = false;  // s_always
ସର୍ବସାଧାରଣ:
    AstPropAlways(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* propp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* loBoundp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { V3ERROR_NA_RETURN(0); }
    bool isStrong() ଅଚଳ { ଫେରନ୍ତୁ m_isStrong; }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstRand final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    ଅଚଳ bool m_urandom;  // $urandom vs $random
ସର୍ବସାଧାରଣ:
    class Reset {};
    AstRand(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_ଧାଡ଼ି* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtypeSetUInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override {
        ଫେରନ୍ତୁ seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        ଯଦି (seedp()) {
            ଯଦି (urandom()) ଫେରନ୍ତୁ "VL_URANDOM_SEEDED_%nq%lq(%li)";
            ଫେରନ୍ତୁ "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        ଯଦି (isWide()) ଫେରନ୍ତୁ "VL_RANDOM_%nq(%nw, %P)";
        ଫେରନ୍ତୁ "VL_RANDOM_%nq()";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ !seedp(); }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool combinable(ଅଚଳ AstRand* samep) ଅଚଳ {
        ଫେରନ୍ତୁ !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() ଅଚଳ { ଫେରନ୍ତୁ m_urandom; }
};
class AstRandRNG final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
ସର୍ବସାଧାରଣ:
    AstRandRNG(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$rngrandom()"; }
    string emitC() override {
        ଫେରନ୍ତୁ isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstRising final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstRising(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { ଫେରନ୍ତୁ "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstRose final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ସର୍ବସାଧାରଣ:
    AstRose(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_Sen_ବୃକ୍ଷ* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { ଫେରନ୍ତୁ "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSClocked final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstSClocked(File_ଧାଡ଼ି* fl, AstSenItem* sensesp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
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
    ଅଚଳ bool m_unbounded = false;  // True when the upper bound is $
ସର୍ବସାଧାରଣ:
    // Exact [*N]
    AstSConsRep(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { V3ERROR_NA_RETURN(0); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {  // LCOV_EXCL_LINE
        ଫେରନ୍ତୁ m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() ଅଚଳ { ଫେରନ୍ତୁ m_unbounded; }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSEventually final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    explicit AstSEventually(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    explicit AstSExpr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* preExprp, Ast_ଗଣ୍ଠି_Stmt* delayp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_Stmt* delayp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSFormatArg final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

ସର୍ବସାଧାରଣ:
    AstSFormatArg(File_ଧାଡ଼ି* fl, VFormatAttr formatAttr, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$sformatarg"; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        ଫେରନ୍ତୁ nullptr;
    }
    VFormatAttr formatAttr() ଅଚଳ { ଫେରନ୍ତୁ m_formatAttr; }
    ଶୂନ୍ୟ formatAttr(ଅଚଳ VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    ସ୍ଥିର VFormatAttr formatAttrDefauled(ଅଚଳ AstSFormatArg* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep);
};
class AstSFormatF final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_ପଦସମଷ୍ଟି_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    ଅଚଳ ଅକ୍ଷର m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
ସର୍ବସାଧାରଣ:
    class ପଦସମଷ୍ଟି_Format {};
    AstSFormatF(File_ଧାଡ଼ି* fl, ଅଚଳ string& text, bool hidden, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp,
                ଅକ୍ଷର missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    AstSFormatF(File_ଧାଡ଼ି* fl, ପଦସମଷ୍ଟି_Format, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp, ଅକ୍ଷର missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_ପଦସମଷ୍ଟି_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_text; }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_text = name; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$sformatf"; }
    string text() ଅଚଳ { ଫେରନ୍ତୁ m_text; }  // * = Text to display
    ଶୂନ୍ୟ text(ଅଚଳ string& text) { m_text = text; }
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(text() != "" && ପଦସମଷ୍ଟି_Format_2());  // Expr format means no literal format
        ଫେରନ୍ତୁ nullptr;
    }
    bool formatScopeTracking() ଅଚଳ {  // Track scopeNamep();  Ok if false positive
        ଯଦି (ପଦସମଷ୍ଟି_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            ଫେରନ୍ତୁ true;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ fmtp = VN_CAST(exprp, Const)) {
                ଯଦି (fmtp->ଅଙ୍କ().isFromString()) {
                    ଅଚଳ string str = fmtp->ଅଙ୍କ().toString();
                    ଯଦି (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        ଫେରନ୍ତୁ true;
                }
            }
        }
        ଫେରନ୍ତୁ false;
    }
    bool hidden() ଅଚଳ { ଫେରନ୍ତୁ m_hidden; }
    ଶୂନ୍ୟ hidden(bool flag) { m_hidden = flag; }
    bool ପଦସମଷ୍ଟି_Format_2() ଅଚଳ { ଫେରନ୍ତୁ m_ପଦସମଷ୍ଟି_Format; }
    ଶୂନ୍ୟ ପଦସମଷ୍ଟି_Format_2(bool flag) { m_ପଦସମଷ୍ଟି_Format = flag; }
    bool optionalFormat() ଅଚଳ { ଫେରନ୍ତୁ m_optionalFormat; }
    ଶୂନ୍ୟ optionalFormat(bool flag) { m_optionalFormat = flag; }
    ଅକ୍ଷର missingArgChar() ଅଚଳ { ଫେରନ୍ତୁ m_missingArgChar; }
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSGotoRep final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    // Exact [->N]
    AstSGotoRep(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSNonConsRep final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    // Exact [=N]
    AstSNonConsRep(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSScanF final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

ସର୍ବସାଧାରଣ:
    AstSScanF(File_ଧାଡ଼ି* fl, ଅଚଳ string& text, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->fromp(fromp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_text; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: makes output
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstSScanF* ଅଚଳ asamep = VN_DBG_AS(samep, SScanF);
        ଫେରନ୍ତୁ text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() ଅଚଳ { ଫେରନ୍ତୁ m_text; }  // * = Text to display
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSampled final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_internal : 1;  // Internally created, not from a source $sampled
ସର୍ବସାଧାରଣ:
    AstSampled(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* exprp, Ast_ଗଣ୍ଠି_DType* dtypep, bool internal = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_internal{internal} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { ଫେରନ୍ତୁ "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_internal == VN_DBG_AS(samep, Sampled)->m_internal;
    }
    bool internal() ଅଚଳ { ଫେରନ୍ତୁ m_internal; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstScopeName final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    ଅଚଳ bool m_forFormat;  // Is for a format %m
    ସ୍ଥିର std::string scopeNameFormatter(ଅଚଳ std::string& text);
    ସ୍ଥିର std::string scopePrettyNameFormatter(ଅଚଳ std::string& text);

ସର୍ବସାଧାରଣ:
    class ForFormat {};
    AstScopeName(File_ଧାଡ଼ି* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstScopeName* ଅଚଳ sp = VN_DBG_AS(samep, ScopeName);
        ଫେରନ୍ତୁ m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    // ACCESSORS
    ଅଚଳ std::string& scopeAttr() ଅଚଳ { ଫେରନ୍ତୁ m_scopeAttr; }
    ଶୂନ୍ୟ scopeAttr(ଅଚଳ std::string& val) { m_scopeAttr = val; }
    ଅଚଳ std::string& scopeEntr() ଅଚଳ { ଫେରନ୍ତୁ m_scopeEntr; }
    ଶୂନ୍ୟ scopeEntr(ଅଚଳ std::string& val) { m_scopeEntr = val; }
    bool dpiExport() ଅଚଳ { ଫେରନ୍ତୁ m_dpiExport; }
    ଶୂନ୍ୟ dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() ଅଚଳ { ଫେରନ୍ତୁ m_selfRelative; }
    ଶୂନ୍ୟ selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() ଅଚଳ { ଫେରନ୍ତୁ m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() ଅଚଳ { ଫେରନ୍ତୁ scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() ଅଚଳ { ଫେରନ୍ତୁ scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() ଅଚଳ { ଫେରନ୍ତୁ scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() ଅଚଳ { ଫେରନ୍ତୁ scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
ସର୍ବସାଧାରଣ:
    AstStable(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_Sen_ବୃକ୍ଷ* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { ଫେରନ୍ତୁ "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_ଗଦା_Trace_F final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // $stacktrace used as function
ସର୍ବସାଧାରଣ:
    explicit Ast_ଗଦା_Trace_F(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_ଗଦା_Trace_F(fl) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_Ast_ଗଦା_Trace_F;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$stacktrace"; }
    string emitVerilog() override { ଫେରନ୍ତୁ verilogKwd(); }
    string emitC() override { ଫେରନ୍ତୁ "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSteady final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstSteady(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { ଫେରନ୍ତୁ "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstStructSel final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
ସର୍ବସାଧାରଣ:
    AstStructSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, ଅଚଳ string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override {
        // Not a union
        ଫେରନ୍ତୁ VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstStructSel* ଅଚଳ sp = VN_DBG_AS(samep, StructSel);
        ଫେରନ୍ତୁ m_name == sp->m_name;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
};
class AstSysIgnore final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
ସର୍ବସାଧାରଣ:
    AstSysIgnore(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$ignored"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Though deleted before opt
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Though deleted before opt
    bool isPure() override { ଫେରନ୍ତୁ false; }  // Though deleted before opt
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // Though deleted before opt
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstSystemF(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$system"; }
    string emitVerilog() override { ଫେରନ୍ତୁ verilogKwd(); }
    string emitC() override { ଫେରନ୍ତୁ "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_Tagged_ପଦସମଷ୍ଟି final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
ସର୍ବସାଧାରଣ:
    Ast_Tagged_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_Tagged_ପଦସମଷ୍ଟି(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_ପଦସମଷ୍ଟି;
    string emitVerilog() override { ଫେରନ୍ତୁ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_name == VN_DBG_AS(samep, Tagged_ପଦସମଷ୍ଟି)->m_name;
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
};
class AstTaggedPattern final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
ସର୍ବସାଧାରଣ:
    AstTaggedPattern(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* patternp)
        : ASTGEN_SUPER_TaggedPattern(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstTaggedPattern;
    string emitVerilog() override { ଫେରନ୍ତୁ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_name == VN_DBG_AS(samep, TaggedPattern)->m_name;
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
};
class AstTestPlusArgs final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
ସର୍ବସାଧାରଣ:
    AstTestPlusArgs(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* searchp)
        : ASTGEN_SUPER_TestPlusArgs(fl) {
        this->searchp(searchp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstTestPlusArgs;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$test$plusargs"; }
    string emitVerilog() override { ଫେରନ୍ତୁ verilogKwd(); }
    string emitC() override { ଫେରନ୍ତୁ "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    // but isPure() true
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstThisRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
ସର୍ବସାଧାରଣ:
    AstThisRef(File_ଧାଡ଼ି* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_ଧାଡ଼ି* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { ଫେରନ୍ତୁ "this"; }
    string emitVerilog() override { ଫେରନ୍ତୁ "this"; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
};
class AstTimePrecision final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Verilog $timeprecision
ସର୍ବସାଧାରଣ:
    explicit AstTimePrecision(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { ଫେରନ୍ତୁ "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstTimeUnit final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
ସର୍ବସାଧାରଣ:
    explicit AstTimeUnit(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { ଫେରନ୍ତୁ "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    ଅଚଳ std::vector<uint32_t> m_indices;  // Indices of the triggers tested
ସର୍ବସାଧାରଣ:
    AstTriggered(File_ଧାଡ଼ି* fl, AstVarScope* trigVscp, ଅଚଳ std::vector<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    ଅଚଳ std::vector<uint32_t>& indices() ଅଚଳ { ଫେରନ୍ତୁ m_indices; }
};
class AstUnbounded final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
ସର୍ବସାଧାରଣ:
    explicit AstUnbounded(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { ଫେରନ୍ତୁ "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstUnlinkedRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
ସର୍ବସାଧାରଣ:
    AstUnlinkedRef(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* refp, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    ଅଚଳ bool m_strong;  // 's_' preffix
    ଅଚଳ bool m_overlapping;  // '_with` suffix
ସର୍ବସାଧାରଣ:
    AstUntil(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() ଅଚଳ override {
        ଫେରନ୍ତୁ (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isStrong() ଅଚଳ { ଫେରନ୍ତୁ m_strong; }
    bool isOverlapping() ଅଚଳ { ଫେରନ୍ତୁ m_overlapping; }
};
class Ast_ମୂଲ୍ୟ_Plus_Args final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
ସର୍ବସାଧାରଣ:
    Ast_ମୂଲ୍ୟ_Plus_Args(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* searchp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* outp)
        : ASTGEN_SUPER_ମୂଲ୍ୟ_Plus_Args(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_ମୂଲ୍ୟ_Plus_Args;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$value$plusargs"; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ !outp(); }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstWithParse final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
ବ୍ୟକ୍ତିଗତ:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

ସର୍ବସାଧାରଣ:
    AstWithParse(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* funcrefp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp,
                 Ast_ଗଣ୍ଠି* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        addExprsp(exprsp);
        addConstraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool restricted() ଅଚଳ { ଫେରନ୍ତୁ m_restricted; }
    ଶୂନ୍ୟ restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
ସର୍ବସାଧାରଣ:
    AstBufIf1(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstCastDynamic final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
ସର୍ବସାଧାରଣ:
    AstCastDynamic(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtypeSetBitUnsized(32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$cast(%r, %l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 20; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
};
class AstCompareNN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Verilog str.compare() and str.icompare()
    ଅଚଳ bool m_ignoreCase;  // True for str.icompare()
ସର୍ବସାଧାରଣ:
    AstCompareNN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        ଫେରନ୍ତୁ m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        ଫେରନ୍ତୁ m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool ignoreCase() ଅଚଳ { ଫେରନ୍ତୁ m_ignoreCase; }
};
class AstConcat final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstConcat(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        ଯଦି (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetBitOrLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f{%l, %k%r}"; }
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { ଫେରନ୍ତୁ "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(concat %l %r)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 2; }
};
class AstConcatN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // String concatenate
ସର୍ବସାଧାରଣ:
    AstConcatN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f{%l, %k%r}"; }
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { ଫେରନ୍ତୁ "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
    bool stringFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstDiv final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstDiv(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f/ %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvudiv %l %r)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_DIV; }
};
class AstDivD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstDivD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstDivD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "/"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL_DIV; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstDivS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstDivS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f/ %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvsdiv %l %r)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_DIV; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstEqWild final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstEqWild(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Biop* newTyped(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp);
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f==? %r)"; }
    string emitC() override {
        ଯଦି (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        ଯଦି (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ "==";
        }
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstFGetS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
ସର୍ବସାଧାରଣ:
    AstFGetS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$fgets(%l,%r)"; }
    string emitC() override {
        ଫେରନ୍ତୁ strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 64; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstFUngetC(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        ଫେରନ୍ତୁ "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 64; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ଗଣ୍ଠି* filep() ଅଚଳ { ଫେରନ୍ତୁ lhsp(); }
    Ast_ଗଣ୍ଠି* charp() ଅଚଳ { ଫେରନ୍ତୁ rhsp(); }
};
class AstGetcN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Verilog string.getc()
ସର୍ବସାଧାରଣ:
    AstGetcN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ "getc"; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l.getc(%r))"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstGetcRefN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
ସର୍ବସାଧାରଣ:
    AstGetcRefN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstGt final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstGt(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f> %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ">"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstGtD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstGtD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ">"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstGtN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstGtN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ">"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
    bool stringFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstGtS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstGtS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f> %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstGte final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstGte(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f>= %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ">="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstGteD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstGteD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ">="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstGteN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstGteN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ">="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
    bool stringFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstGteS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstGteS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f>= %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstLogAnd final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstLogAnd(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f&& %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvand %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "&&"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() + INSTR_ଗଣନା_BRANCH; }
};
class AstLogIf final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstLogIf(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f-> %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "->"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() + INSTR_ଗଣନା_BRANCH; }
};
class AstLogOr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstLogOr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f|| %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvor %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "||"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() + INSTR_ଗଣନା_BRANCH; }
};
class AstLt final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstLt(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f< %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "<"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstLtD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstLtD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "<"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstLtN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstLtN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "<"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
    bool stringFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstLtS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstLtS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f< %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstLte final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstLte(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f<= %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "<="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstLteD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstLteD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "<="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstLteN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstLteN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "<="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
    bool stringFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstLteS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstLteS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f<= %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstModDiv final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstModDiv(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f%% %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvurem %l %r)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_DIV; }
};
class AstModDivS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstModDivS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f%% %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvsmod %l %r)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_DIV; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstNeqWild final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstNeqWild(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f!=? %r)"; }
    string emitC() override {
        ଯଦି (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        ଯଦି (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ "!=";
        }
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstPow final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstPow(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f** %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_MUL * 10; }
};
class AstPowD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstPowD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstPowD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f** %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "pow(%li,%ri)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL_DIV * 5; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstPowSS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstPowSS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f** %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_MUL * 10; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstPowSU final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstPowSU(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f** %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_MUL * 10; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstPowUS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstPowUS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f** %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_MUL * 10; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstReplicate final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
ସର୍ବସାଧାରଣ:
    AstReplicate(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        ଯଦି (lhsp) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(rhsp, Const)) {
                ଯଦି (constp->ଅଙ୍କ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ଅଙ୍କ().isNegative())) {
                    dtypeSetBitOrLogicSized(lhsp->width(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } ଅନ୍ୟଥା {
                    dtypeSetBitOrLogicSized(lhsp->width() * constp->toSInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, uint32_t rep_ଗଣନା)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_ଗଣନା}} {}
    ASTGEN_MEMBERS_AstReplicate;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f{%r{%k%l}}"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override {
        ଫେରନ୍ତୁ "((_ repeat " + cvtToStr(width() / lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 2; }
};
class AstReplicateN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // String replicate
ସର୍ବସାଧାରଣ:
    AstReplicateN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f{%r{%k%l}}"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 2; }
    bool stringFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSAnd final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    ଅଚଳ bool m_propertyControl;  // Parser-generated property if/case branch conjunction
ସର୍ବସାଧାରଣ:
    AstSAnd(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() + INSTR_ଗଣନା_BRANCH; }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {  // LCOV_EXCL_LINE
        ଫେରନ୍ତୁ m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() ଅଚଳ { ଫେରନ୍ତୁ m_propertyControl; }
};
class AstSIntersect final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
ସର୍ବସାଧାରଣ:
    AstSIntersect(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() + INSTR_ଗଣନା_BRANCH; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSOr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
ସର୍ବସାଧାରଣ:
    AstSOr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSOr;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() + INSTR_ଗଣନା_BRANCH; }
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSThroughout final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
ସର୍ବସାଧାରଣ:
    AstSThroughout(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSWithin final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
ସର୍ବସାଧାରଣ:
    AstSWithin(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSel final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_declElWidth;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_widthConst;

ସର୍ବସାଧାରଣ:
    AstSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicSized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitwidth, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicUnsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& from, ଅଚଳ V3_ସଂଖ୍ୟା& bit) override {
        out.opSel(from, bit.toUInt() + widthConst() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        ଫେରନ୍ତୁ widthConst() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "((_ extract %t %r) %l)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* otherp) ଅଚଳ override {
        ଫେରନ୍ତୁ widthConst() == VN_DBG_AS(otherp, Sel)->widthConst();
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override {
        ଫେରନ୍ତୁ widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthConst() ଅଚଳ { ଫେରନ୍ତୁ m_widthConst; }
    ଶୂନ୍ୟ widthConst(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) { m_widthConst = ମୂଲ୍ୟ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsbConst() ଅଚଳ { ଫେରନ୍ତୁ VN_AS(lsbp(), Const)->toSInt(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା msbConst() ଅଚଳ { ଫେରନ୍ତୁ lsbConst() + widthConst() - 1; }
    VNumRange& declRange() VL_MT_STABLE { ଫେରନ୍ତୁ m_declRange; }
    ଅଚଳ VNumRange& declRange() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_declRange; }
    ଶୂନ୍ୟ declRange(ଅଚଳ VNumRange& flag) { m_declRange = flag; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା declElWidth() ଅଚଳ { ଫେରନ୍ତୁ m_declElWidth; }
    ଶୂନ୍ୟ declElWidth(ପୂର୍ଣ୍ଣସଂଖ୍ୟା flag) { m_declElWidth = flag; }
};
class AstShiftL final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstShiftL(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        ଯଦି (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftL;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f<< %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        ଫେରନ୍ତୁ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstShiftLOvr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
ସର୍ବସାଧାରଣ:
    AstShiftLOvr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth = 0)
        : ASTGEN_SUPER_ShiftLOvr(fl, lhsp, rhsp) {
        ଯଦି (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ଅନ୍ୟଥା {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftLOvr;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f<< %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstShiftR final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstShiftR(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        ଯଦି (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftR;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f>> %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        ଫେରନ୍ତୁ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstShiftROvr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
ସର୍ବସାଧାରଣ:
    AstShiftROvr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth = 0)
        : ASTGEN_SUPER_ShiftROvr(fl, lhsp, rhsp) {
        ଯଦି (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ଅନ୍ୟଥା {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftROvr;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f>> %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstShiftRS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstShiftRS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        ଯଦି (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f>>> %r)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvashr %l %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstShiftRSOvr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
ସର୍ବସାଧାରଣ:
    AstShiftRSOvr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth = 0)
        : ASTGEN_SUPER_ShiftRSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        ଯଦି (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ଅନ୍ୟଥା {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRSOvr;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f>>> %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSub final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstSub(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f- %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvsub %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "-"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstSubD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
ସର୍ବସାଧାରଣ:
    AstSubD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstSubD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "-"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstURandomRange final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Biop {
    // $urandom_range
ସର୍ବସାଧାରଣ:
    AstURandomRange(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$urandom_range(%l, %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstEq(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Biop* newTyped(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp);
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f== %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "=="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstEqCase final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstEqCase(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f=== %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "=="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstEqD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
ସର୍ବସାଧାରଣ:
    AstEqD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "=="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstEqN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
ସର୍ବସାଧାରଣ:
    AstEqN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "=="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
    bool stringFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstEqT final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
    // Equal (==) for data types
ସର୍ବସାଧାରଣ:
    AstEqT(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqT;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "=="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
};
class AstLogEq final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstLogEq(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f<-> %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "<->"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() + INSTR_ଗଣନା_BRANCH; }
};
class AstNeq final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstNeq(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Biop* newTyped(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp);
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f!= %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "!="; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (not (= %l %r)))"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstNeqCase final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstNeqCase(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f!== %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "!="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstNeqD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
ସର୍ବସାଧାରଣ:
    AstNeqD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "!="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstNeqN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
ସର୍ବସାଧାରଣ:
    AstNeqN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "!="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
    bool stringFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstNeqT final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com {
    // Not-equal (!=) for data types
ସର୍ବସାଧାରଣ:
    AstNeqT(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "!="; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_STR; }
};

// === AstNodeBiComAsv ===
class AstAdd final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com_Asv {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstAdd(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAdd;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f+ %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvadd %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "+"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstAddD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com_Asv {
ସର୍ବସାଧାରଣ:
    AstAddD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstAddD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "+"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstAnd final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com_Asv {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstAnd(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f& %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvand %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "&"; }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ଅଚଳ ଅକ୍ଷର* widthMismatch() ଅଚଳ override VL_MT_STABLE;
};
class AstMul final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com_Asv {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstMul(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f* %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "*"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_MUL; }
};
class AstMulD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com_Asv {
ସର୍ବସାଧାରଣ:
    AstMulD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstMulD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "*"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstMulS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com_Asv {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstMulS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f* %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * INSTR_ଗଣନା_INT_MUL; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstOr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com_Asv {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstOr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f| %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvor %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "|"; }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ଅଚଳ ଅକ୍ଷର* widthMismatch() ଅଚଳ override VL_MT_STABLE;
};
class AstXor final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Bi_Com_Asv {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstXor(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f^ %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvxor %l %r)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "^"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Lclean && Rclean
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ଅଚଳ ଅକ୍ଷର* widthMismatch() ଅଚଳ override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class AstDistChiSquare final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Dist_Biop {
ସର୍ବସାଧାରଣ:
    AstDistChiSquare(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_DistChiSquare(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistChiSquare;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class AstDistExponential final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Dist_Biop {
ସର୍ବସାଧାରଣ:
    AstDistExponential(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_DistExponential(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistExponential;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$dist_exponential(%l, %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class AstDistPoisson final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Dist_Biop {
ସର୍ବସାଧାରଣ:
    AstDistPoisson(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_DistPoisson(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistPoisson;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$dist_poisson(%l, %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Dist_Biop {
ସର୍ବସାଧାରଣ:
    AstDistT(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$dist_t(%l, %r)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Sel {
    // @astgen makeDfgVertex
    ଶୂନ୍ୟ init(ଅଚଳ Ast_ଗଣ୍ଠି* fromp) {
        ଯଦି (fromp && VN_IS(fromp->dtypep()->skipRefp(), ଗଣ୍ଠି_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), ଗଣ୍ଠି_Array_DType)->subDTypep());
        }
    }

ସର୍ବସାଧାରଣ:
    AstArraySel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(select %l %r)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
};
class AstAssocSel final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Sel {
    ଶୂନ୍ୟ init(ଅଚଳ Ast_ଗଣ୍ଠି* fromp) {
        ଯଦି (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

ସର୍ବସାଧାରଣ:
    AstAssocSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l%f[%r])"; }
    string emitC() override { ଫେରନ୍ତୁ "%li%k[%ri]"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(select %l %r)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isGateOptimizable() ଅଚଳ override {
        ଫେରନ୍ତୁ !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ !isLValue(); }  // AssocSel creates on miss
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
};
class AstWildcardSel final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Sel {
    ଶୂନ୍ୟ init(ଅଚଳ Ast_ଗଣ୍ଠି* fromp) {
        ଯଦି (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

ସର୍ବସାଧାରଣ:
    AstWildcardSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l%f[%r])"; }
    string emitC() override { ଫେରନ୍ତୁ "%li%k[%ri]"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
};
class AstWordSel final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Sel {
    // Select a single word from a multi-word wide value
ସର୍ବସାଧାରଣ:
    AstWordSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&, ଅଚଳ V3_ସଂଖ୍ୟା&) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};

// === AstNodeStream ===
class AstStreamL final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstStreamL(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamL;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f{ << %r %k{%l} }"; }
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { ଫେରନ୍ତୁ "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 2; }
};
class AstStreamR final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstStreamR(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamR;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f{ >> %r %k{%l} }"; }
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { ଫେରନ୍ତୁ isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Biop_D {
ସର୍ବସାଧାରଣ:
    AstAtan2D(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$atan2(%l,%r)"; }
    string emitC() override { ଫେରନ୍ତୁ "atan2(%li,%ri)"; }
};
class AstHypotD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Biop_D {
ସର୍ବସାଧାରଣ:
    AstHypotD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$hypot(%l,%r)"; }
    string emitC() override { ଫେରନ୍ତୁ "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
ସର୍ବସାଧାରଣ:
    AstCCall(File_ଧାଡ଼ି* fl, AstCFunc* funcp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    ଅଚଳ VSelfPointerText& selfPointer() ଅଚଳ { ଫେରନ୍ତୁ m_selfPointer; }
    ଶୂନ୍ୟ selfPointer(ଅଚଳ VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) ଅଚଳ {
        ଫେରନ୍ତୁ selfPointer().protect(useSelfForThis, protect());
    }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
};
class AstCMethodCall final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstCMethodCall(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, AstCFunc* funcp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_CCall {
    // C++ new() call
ସର୍ବସାଧାରଣ:
    AstCNew(File_ଧାଡ଼ି* fl, AstCFunc* funcp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

ସର୍ବସାଧାରଣ:
    AstConsArray(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* newDTypep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ଶୂନ୍ୟ cloneRelink() override;
    ଧାଡ଼ିଭିତରେ AstConsArrayItem* itemsp() ଅଚଳ VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep(uint64_t index) ଅଚଳ;
    // Add the value of the element at the index, which must not be given yet
    ଶୂନ୍ୟ valuep(uint64_t index, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp);
    // Add the elements of the array 'arrayp', starting at the index
    ଶୂନ୍ୟ splice_ମୂଲ୍ୟଗୁଡ଼ିକ(uint64_t index, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* arrayp);
};
class AstConsAssoc final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
ସର୍ବସାଧାରଣ:
    explicit AstConsAssoc(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        ଫେରନ୍ତୁ nullptr;
    }
};
class AstConsDynArray final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
ସର୍ବସାଧାରଣ:
    explicit AstConsDynArray(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
};
class AstConsQueue final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
ସର୍ବସାଧାରଣ:
    explicit AstConsQueue(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        ଫେରନ୍ତୁ nullptr;
    }
};
class AstConsStruct final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
ସର୍ବସାଧାରଣ:
    AstConsStruct(File_ଧାଡ଼ି* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    // The value of the member called 'name'
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* memberValuep(ଅଚଳ string& name) ଅଚଳ;
};
class AstConsWildcard final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
ସର୍ବସାଧାରଣ:
    explicit AstConsWildcard(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        ଫେରନ୍ତୁ nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask_Ref {
    // A reference to a function
ସର୍ବସାଧାରଣ:
    ଧାଡ଼ିଭିତରେ AstFuncRef(File_ଧାଡ଼ି* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class AstMethodCall final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
ସର୍ବସାଧାରଣ:
    AstMethodCall(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, VFlagChildDType, ଅଚଳ string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMethodCall(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, ଅଚଳ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_AstMethodCall;
};
class AstNew final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
ସର୍ବସାଧାରଣ:
    AstNew(File_ଧାଡ଼ି* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool isImplicit() ଅଚଳ { ଫେରନ୍ତୁ m_isImplicit; }
    ଶୂନ୍ୟ isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() ଅଚଳ { ଫେରନ୍ତୁ m_isScoped; }
    ଶୂନ୍ୟ isScoped(bool flag) { m_isScoped = flag; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
};
class AstTaskRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask_Ref {
    // A reference to a task
ସର୍ବସାଧାରଣ:
    ଧାଡ଼ିଭିତରେ AstTaskRef(File_ଧାଡ଼ି* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtypeSetVoid();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
ବ୍ୟକ୍ତିଗତ:
    VAccess m_access;  // Left hand side assignment
ସର୍ବସାଧାରଣ:
    AstSelBit(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    VAccess access() ଅଚଳ { ଫେରନ୍ତୁ m_access; }
    ଶୂନ୍ୟ access(ଅଚଳ VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
ସର୍ବସାଧାରଣ:
    AstSelExtract(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* msbp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
ସର୍ବସାଧାରଣ:
    AstSelMinus(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
ସର୍ବସାଧାରଣ:
    AstSelPlus(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bitp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_ଗଣନା_Bits final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Quadop {
    // Number of bits set in vector
ସର୍ବସାଧାରଣ:
    Ast_ଗଣନା_Bits(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ctrl1p)
        : ASTGEN_SUPER_ଗଣନା_Bits(fl, exprp, ctrl1p, ctrl1p->clone_ବୃକ୍ଷ_Pure(false),
                                 ctrl1p->clone_ବୃକ୍ଷ_Pure(false)) {
        dtypeSetInt();
    }
    Ast_ଗଣନା_Bits(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ctrl1p, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ctrl2p)
        : ASTGEN_SUPER_ଗଣନା_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_ବୃକ୍ଷ_Pure(false)) {
        dtypeSetInt();
    }
    Ast_ଗଣନା_Bits(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ctrl1p, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ctrl2p,
                 Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ctrl3p)
        : ASTGEN_SUPER_ଗଣନା_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_ଗଣନା_Bits;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& ପଦସମଷ୍ଟି, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl1,
                       ଅଚଳ V3_ସଂଖ୍ୟା& ctrl2, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl3) override {
        out.op_ଗଣନା_Bits(ପଦସମଷ୍ଟି, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanThs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanFhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersThs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersFhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Termop {
ସର୍ବସାଧାରଣ:
    explicit AstInferredDisable(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstTime final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Termop {
    VTimescale m_timeunit;  // Parent module time unit
ସର୍ବସାଧାରଣ:
    AstTime(File_ଧାଡ଼ି* fl, ଅଚଳ VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_TIME; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Termop {
    VTimescale m_timeunit;  // Parent module time unit
ସର୍ବସାଧାରଣ:
    AstTimeD(File_ଧାଡ଼ି* fl, ଅଚଳ VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_TIME; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstCond(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thenp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* elsep);
    ASTGEN_MEMBERS_AstCond;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs,
                       ଅଚଳ V3_ସଂଖ୍ୟା& ths) override {
        out.opAssign(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l %f? %r %k: %t)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(ite (__Vbool %l) %r %t)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // clean if e1 & e2 clean
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanThs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Propagates up
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersThs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_BRANCH; }
};
class AstPutcN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Triop {
    // Verilog string.putc()
ସର୍ବସାଧାରଣ:
    AstPutcN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstPutcN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs,
                       ଅଚଳ V3_ସଂଖ୍ୟା& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ "putc"; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l.putc(%r,%t))"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanThs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersThs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstSliceSel final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
ସର୍ବସାଧାରଣ:
    AstSliceSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, ଅଚଳ VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_AstSliceSel;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& from, ଅଚଳ V3_ସଂଖ୍ୟା& lo,
                       ଅଚଳ V3_ସଂଖ୍ୟା& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanThs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersThs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { ଫେରନ୍ତୁ m_declRange; }
    ଅଚଳ VNumRange& declRange() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_declRange; }
};
class AstSubstrN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Triop {
    // Verilog string.substr()
ସର୍ବସାଧାରଣ:
    AstSubstrN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs,
                       ଅଚଳ V3_ସଂଖ୍ୟା& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ "substr"; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l.substr(%r,%t))"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ ""; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanRhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanThs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersRhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersThs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Dist_Triop {
ସର୍ବସାଧାରଣ:
    AstDistErlang(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class AstDistNormal final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Dist_Triop {
ସର୍ବସାଧାରଣ:
    AstDistNormal(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsp)
        : ASTGEN_SUPER_DistNormal(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistNormal;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class AstDistUniform final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Dist_Triop {
ସର୍ବସାଧାରଣ:
    AstDistUniform(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsp)
        : ASTGEN_SUPER_DistUniform(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistUniform;
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
ସର୍ବସାଧାରଣ:
    ଗଣନା FmtType : ପୂର୍ଣ୍ଣସଂଖ୍ୟା { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

ବ୍ୟକ୍ତିଗତ:
    // dist-ast-dump-suppress  // Part of name()
    ଅଚଳ FmtType m_fmtType;  // Operation type
ସର୍ବସାଧାରଣ:
    AstAtoN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtypeSetDouble() : dtypeSetInteger();
    }
    ASTGEN_MEMBERS_AstAtoN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opAtoN(lhs, m_fmtType); }
    string name() ଅଚଳ override VL_MT_STABLE {
        ବାଛ (m_fmtType) {
        ସ୍ଥିତି ATOI: ଫେରନ୍ତୁ "atoi";
        ସ୍ଥିତି ATOHEX: ଫେରନ୍ତୁ "atohex";
        ସ୍ଥିତି ATOOCT: ଫେରନ୍ତୁ "atooct";
        ସ୍ଥିତି ATOBIN: ଫେରନ୍ତୁ "atobin";
        ସ୍ଥିତି ATOREAL: ଫେରନ୍ତୁ "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%l." + name() + "()"; }
    string emitC() override {
        ବାଛ (m_fmtType) {
        ସ୍ଥିତି ATOI: ଫେରନ୍ତୁ "VL_ATOI_N(%li, 10)";
        ସ୍ଥିତି ATOHEX: ଫେରନ୍ତୁ "VL_ATOI_N(%li, 16)";
        ସ୍ଥିତି ATOOCT: ଫେରନ୍ତୁ "VL_ATOI_N(%li, 8)";
        ସ୍ଥିତି ATOBIN: ଫେରନ୍ତୁ "VL_ATOI_N(%li, 2)";
        ସ୍ଥିତି ATOREAL: ଫେରନ୍ତୁ "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    FmtType fmtType() ଅଚଳ { ଫେରନ୍ତୁ m_fmtType; }
};
class AstBitsToRealD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
ସର୍ବସାଧାରଣ:
    AstBitsToRealD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstBitsToRealD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$bitstoreal(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_CVT_D_Q(%li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Eliminated before matters
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Eliminated before matters
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCCast final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Cast to C-based data type
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_size;

ସର୍ବସାଧାରଣ:
    AstCCast(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା setwidth, ପୂର୍ଣ୍ଣସଂଖ୍ୟା minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_size = setwidth;
        ଯଦି (setwidth) {
            ଯଦି (minwidth == -1) minwidth = setwidth;
            dtypeSetBitUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_size = width();
    }
    ASTGEN_MEMBERS_AstCCast;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$_CAST(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Special cased in V3Cast
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ size() == VN_DBG_AS(samep, CCast)->size();
    }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    //
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା size() ଅଚଳ { ଫେରନ୍ତୁ m_size; }
};
class AstCLog2 final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
ସର୍ବସାଧାରଣ:
    AstCLog2(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$clog2(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 16; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCastWrap final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
ସର୍ବସାଧାରଣ:
    AstCastWrap(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
};
class Ast_ଗଣନା_Ones final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    Ast_ଗଣନା_Ones(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_ଗଣନା_Ones(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_ଗଣନା_Ones;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.op_ଗଣନା_Ones(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$countones(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 16; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCvtPackString final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
ସର୍ବସାଧାରଣ:
    AstCvtPackString(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstExtend final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstExtend(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetBitOrLogicUnsized(width, widthMin ? widthMin : width, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%l"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ଅଚଳ override {
        ଫେରନ୍ତୁ "((_ zero_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override {
        ଫେରନ୍ତୁ false;  // Because the EXTEND operator self-casts
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
};
class AstExtendS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstExtendS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%l"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ଅଚଳ override {
        ଫେରନ୍ତୁ "((_ sign_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override {
        ଫେରନ୍ତୁ false;  // Because the EXTEND operator self-casts
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool signedFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstFEof final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
ସର୍ବସାଧାରଣ:
    AstFEof(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$feof(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 16; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି* filep() ଅଚଳ { ଫେରନ୍ତୁ lhsp(); }
};
class AstFGetC final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
ସର୍ବସାଧାରଣ:
    AstFGetC(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { ଫେରନ୍ତୁ "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 64; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ଗଣ୍ଠି* filep() ଅଚଳ { ଫେରନ୍ତୁ lhsp(); }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstISToRD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // $itor where lhs is signed
ସର୍ବସାଧାରଣ:
    AstISToRD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstISToRD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$itor($signed(%l))"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_ଶବ୍ଦଗୁଡ଼ିକ() override { ଫେରନ୍ତୁ true; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstIToRD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // $itor where lhs is unsigned
ସର୍ବସାଧାରଣ:
    AstIToRD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstIToRD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$itor(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstIsUnbounded final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // True if is unbounded ($)
ସର୍ବସାଧାରଣ:
    AstIsUnbounded(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstIsUnknown final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // True if any unknown bits
ସର୍ବସାଧାରଣ:
    AstIsUnknown(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstLenN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Length of a string
ସର୍ବସାଧାରଣ:
    AstLenN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstLenN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LEN_IN(%li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstLogNot final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // @astgen makeDfgVertex
ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
ସର୍ବସାଧାରଣ:
    AstLogNot(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f(! %l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "!"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool fromProperty() ଅଚଳ { ଫେରନ୍ତୁ m_fromProperty; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstMostSetBitP1 final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
ସର୍ବସାଧାରଣ:
    AstMostSetBitP1(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_MostSetBitP1(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstMostSetBitP1;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opMostSetBitP1(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$mostsetbitp1(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 16; }
};
class AstNToI final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // String to any-size integral
ସର୍ବସାଧାରଣ:
    AstNToI(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opNToI(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "'(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstNegate final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstNegate(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f(- %l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvneg %l)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "-"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstNegateD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
ସର୍ବସାଧାରଣ:
    AstNegateD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstNegateD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "-"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool doubleFlavor() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstNot final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstNot(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f(~ %l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() ଅଚଳ override { ଫେରନ୍ତୁ "(bvnot %l)"; }
    string emitSimpleOperator() override { ଫେରନ୍ତୁ "~"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଅଚଳ ଅକ୍ଷର* widthMismatch() ଅଚଳ override VL_MT_STABLE;
};
class AstNullCheck final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Return LHS after checking that LHS is non-null
ସର୍ବସାଧାରଣ:
    AstNullCheck(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { V3ERROR_NA; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 1; }  // Rarely executes
    string emitVerilog() override { ଫେରନ୍ତୁ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override { ଫେରନ୍ତୁ fileline() == samep->fileline(); }
};
class AstOneHot final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstOneHot(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$onehot(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 4; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstOneHot0 final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstOneHot0(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$onehot0(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs() * 3; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstPostDec final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
ସର୍ବସାଧାରଣ:
    AstPostDec(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
ସର୍ବସାଧାରଣ:
    AstPostInc(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
ସର୍ବସାଧାରଣ:
    AstPreDec(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
ସର୍ବସାଧାରଣ:
    AstPreInc(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ଅଚଳ override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
ସର୍ବସାଧାରଣ:
    AstRToIRoundS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        ଫେରନ୍ତୁ isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstRToIS final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // $rtoi(lhs)
ସର୍ବସାଧାରଣ:
    AstRToIS(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$rtoi(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_RTOI_I_D(%li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Eliminated before matters
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Eliminated before matters
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstRealToBits final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
ସର୍ବସାଧାରଣ:
    AstRealToBits(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstRealToBits;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$realtobits(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_CVT_Q_D(%li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Eliminated before matters
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Eliminated before matters
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_DBL; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstRedAnd final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstRedAnd(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f(& %l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstRedOr final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstRedOr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f(| %l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstRedXor final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // @astgen makeDfgVertex
ସର୍ବସାଧାରଣ:
    AstRedXor(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f(^ %l)"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = lhsp()->width();
        ଫେରନ୍ତୁ (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 1 + V3_ସଂଖ୍ୟା::log2b(width()); }
};
class AstResizeLValue final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
ସର୍ବସାଧାରଣ:
    AstResizeLValue(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
};
class AstSigned final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // $signed(lhs)
ସର୍ବସାଧାରଣ:
    AstSigned(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Eliminated before matters
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }  // Eliminated before matters
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstTimeImport final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
ସର୍ବସାଧାରଣ:
    AstTimeImport(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }
};
class AstToLowerN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // string.tolower()
ସର୍ବସାଧାରଣ:
    AstToLowerN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToLowerN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%l.tolower()"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstToStringN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
ସର୍ବସାଧାରଣ:
    AstToStringN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_ToStringN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToStringN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { ଫେରନ୍ତୁ "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        ଫେରନ୍ତୁ isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstToUpperN final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // string.toupper()
ସର୍ବସାଧାରଣ:
    AstToUpperN(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToUpperN;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { ଫେରନ୍ତୁ "%l.toupper()"; }
    string emitC() override { ଫେରନ୍ତୁ "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstUnsigned final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Uniop {
    // $unsigned(lhs)
ସର୍ବସାଧାରଣ:
    AstUnsigned(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool cleanLhs() ଅଚଳ override { ଫେରନ୍ତୁ false; }  // Eliminated before matters
    bool sizeMattersLhs() ଅଚଳ override { ଫେରନ୍ତୁ true; }  // Eliminated before matters
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool isSystemFunc() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstAcosD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$acos(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "acos(%li)"; }
};
class AstAcoshD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstAcoshD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$acosh(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "acosh(%li)"; }
};
class AstAsinD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstAsinD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$asin(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "asin(%li)"; }
};
class AstAsinhD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstAsinhD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$asinh(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "asinh(%li)"; }
};
class AstAtanD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstAtanD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$atan(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "atan(%li)"; }
};
class AstAtanhD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstAtanhD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$atanh(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "atanh(%li)"; }
};
class AstCeilD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstCeilD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::ceil(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$ceil(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "ceil(%li)"; }
};
class AstCosD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstCosD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$cos(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "cos(%li)"; }
};
class AstCoshD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstCoshD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$cosh(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "cosh(%li)"; }
};
class AstExpD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstExpD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$exp(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "exp(%li)"; }
};
class AstFloorD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstFloorD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::floor(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$floor(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "floor(%li)"; }
};
class AstLog10D final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstLog10D(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$log10(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "log10(%li)"; }
};
class AstLogD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstLogD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$ln(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "log(%li)"; }
};
class AstSinD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstSinD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$sin(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "sin(%li)"; }
};
class AstSinhD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstSinhD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$sinh(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "sinh(%li)"; }
};
class AstSqrtD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstSqrtD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::sqrt(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$sqrt(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "sqrt(%li)"; }
};
class AstTanD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstTanD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$tan(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "tan(%li)"; }
};
class AstTanhD final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_System_Uniop_D {
ସର୍ବସାଧାରଣ:
    AstTanhD(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    ଶୂନ୍ୟ ସଂଖ୍ୟା_Operate(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { ଫେରନ୍ତୁ "%f$tanh(%l)"; }
    string emitC() override { ଫେରନ୍ତୁ "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
ସର୍ବସାଧାରଣ:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    ଧାଡ଼ିଭିତରେ AstVarRef(File_ଧାଡ଼ି* fl, AstVar* varp, ଅଚଳ VAccess& access);
    ଧାଡ଼ିଭିତରେ AstVarRef(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_Module* classOrPackagep, AstVar* varp,
                     ଅଚଳ VAccess& access);
    // This form only allowed post-link (see above)
    ଧାଡ଼ିଭିତରେ AstVarRef(File_ଧାଡ଼ି* fl, AstVarScope* varscp, ଅଚଳ VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
    ଧାଡ଼ିଭିତରେ string name() ଅଚଳ override;  // * = Var name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    ଧାଡ଼ିଭିତରେ bool same_ଗଣ୍ଠି(ଅଚଳ AstVarRef* samep) ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool sameNoLvalue(ଅଚଳ AstVarRef* samep) ଅଚଳ;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstVarXRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
ସର୍ବସାଧାରଣ:
    AstVarXRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& dotted, ଅଚଳ VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    ଧାଡ଼ିଭିତରେ AstVarXRef(File_ଧାଡ଼ି* fl, AstVar* varp, ଅଚଳ string& dotted, ଅଚଳ VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    ଶୂନ୍ୟ name(ଅଚଳ std::string& name) override { m_name = name; }  // * = Var name
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string dotted() ଅଚଳ { ଫେରନ୍ତୁ m_dotted; }
    ଶୂନ୍ୟ dotted(ଅଚଳ string& dotted) { m_dotted = dotted; }
    string inlinedDots() ଅଚଳ { ଫେରନ୍ତୁ m_inlinedDots; }
    ଶୂନ୍ୟ inlinedDots(ଅଚଳ string& flag) { m_inlinedDots = flag; }
    bool containsGenBlock() ଅଚଳ { ଫେରନ୍ତୁ m_containsGenBlock; }
    ଶୂନ୍ୟ containsGenBlock(ଅଚଳ bool flag) { m_containsGenBlock = flag; }
    bool readOnlyModport() ଅଚଳ { ଫେରନ୍ତୁ m_readOnlyModport; }
    ଶୂନ୍ୟ readOnlyModport(ଅଚଳ bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ widthInstrs(); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        ଫେରନ୍ତୁ (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#endif  // Guard
