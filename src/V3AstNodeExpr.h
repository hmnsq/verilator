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
#અન્તર્ભાવ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_ગાંઠ_પદાવલિ VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // An expression tree node
સુરક્ષિત:
    Ast_ગાંઠ_પદાવલિ(VNType t, File_લીટી* fl)
        : Ast_ગાંઠ{t, fl} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_પદાવલિ;
    // METHODS
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    // Every expression must have a data type after V3Width
    bool hasDType() અચલ override final VL_MT_SAFE { પાછા true; }
    આભાસી string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    આભાસી string emitC() = 0;
    આભાસી string emitSMT() અચલ { પાછા ""; };
    આભાસી string emitSimpleOperator() { પાછા ""; }  // "" means not ok to use
    આભાસી bool emit_Check_Max_શબ્દો() { પાછા false; }  // Check VL_MULS_MAX_WORDS
    આભાસી bool cleanOut() અચલ = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    આભાસી bool isMultiCycleSva() અચલ { પાછા false; }
    અચલ Ast_ગાંઠ_પદાવલિ* getVAccessTargetRecurse() અચલ;
    Ast_ગાંઠ_પદાવલિ* getVAccessTargetRecurse() {
        પાછા const_cast<Ast_ગાંઠ_પદાવલિ*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<અચલ Ast_ગાંઠ_પદાવલિ*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() અચલ;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() અચલ { પાછા getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_વૃક્ષ() અચલ;
    // Return base var (or const) nodep dereferences
    Ast_ગાંઠ* baseFromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    અંતઃરેખ Ast_Stmt_પદાવલિ* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    આભાસી અચલ અક્ષર* widthMismatch() અચલ VL_MT_STABLE { પાછા nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string patternString(uint32_t depth = 0) અચલ;
    bool isFourstateOrNoDtype() અચલ {
        જો (!dtypep()) પાછા true;
        પાછા dtypep()->skipRefp()->isFourstate();
    }
    સ્થિર bool isFourstateOrNoDtype(અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr1p) {
        પાછા expr1p->isFourstateOrNoDtype();
    }
    સ્થિર bool isFourstateOrNoDtype(અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr1p,
                                     અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr2p) {
        પાછા expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    સ્થિર bool isFourstateOrNoDtype(અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr1p,
                                     અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr2p,
                                     અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr3p) {
        પાછા isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    સ્થિર bool isFourstateOrNoDtype(અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr1p,
                                     અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr2p,
                                     અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr3p,
                                     અચલ Ast_ગાંઠ_પદાવલિ* અચલ expr4p) {
        પાછા isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_ગાંઠ_Biop VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

સુરક્ષિત:
    Ast_ગાંઠ_Biop(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : Ast_ગાંઠ_પદાવલિ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    આભાસી શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) = 0;
    આભાસી bool cleanLhs() અચલ = 0;  // True if LHS must have extra upper bits zero
    આભાસી bool cleanRhs() અચલ = 0;  // True if RHS must have extra upper bits zero
    આભાસી bool sizeMattersLhs() અચલ = 0;  // True if output result depends on lhs size
    આભાસી bool sizeMattersRhs() અચલ = 0;  // True if output result depends on rhs size
    આભાસી bool doubleFlavor() અચલ { પાછા false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    આભાસી bool signedFlavor() અચલ { પાછા false; }
    આભાસી bool stringFlavor() અચલ { પાછા false; }  // N flavor of nodes with both flavors?
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    bool isPure() override;
    અચલ અક્ષર* broken() અચલ override;

ખાનગી:
    bool getPurityRecurse() અચલ { પાછા lhsp()->isPure() && rhsp()->isPure(); }
};
class Ast_ગાંઠ_Bi_Com VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Biop {
    // Binary expr with commutative properties
સુરક્ષિત:
    Ast_ગાંઠ_Bi_Com(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhs, Ast_ગાંઠ_પદાવલિ* rhs)
        : Ast_ગાંઠ_Biop{t, fl, lhs, rhs} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Bi_Com;
};
class Ast_ગાંઠ_Bi_Com_Asv VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Bi_Com {
    // Binary expr with commutative & associative properties
સુરક્ષિત:
    Ast_ગાંઠ_Bi_Com_Asv(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhs, Ast_ગાંઠ_પદાવલિ* rhs)
        : Ast_ગાંઠ_Bi_Com{t, fl, lhs, rhs} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Bi_Com_Asv;
};
class Ast_ગાંઠ_Dist_Biop VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    Ast_ગાંઠ_Dist_Biop(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : Ast_ગાંઠ_Biop{t, fl, lhsp, rhsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_ગાંઠ_Dist_Biop;
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL_TRIG; }
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
};
class Ast_ગાંઠ_Sel VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
સુરક્ષિત:
    Ast_ગાંઠ_Sel(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* bitp)
        : Ast_ગાંઠ_Biop{t, fl, fromp, bitp} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Sel;
    પૂર્ણાંક bitConst() અચલ;
};
class Ast_ગાંઠ_Stream VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
સુરક્ષિત:
    Ast_ગાંઠ_Stream(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : Ast_ગાંઠ_Biop{t, fl, lhsp, rhsp} {
        જો (lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(lhsp->width(), lhsp->widthMin(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Stream;
};
class Ast_ગાંઠ_System_Biop_D VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    Ast_ગાંઠ_System_Biop_D(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : Ast_ગાંઠ_Biop{t, fl, lhsp, rhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_ગાંઠ_System_Biop_D;
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL_TRIG; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class Ast_ગાંઠ_CCall VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

સુરક્ષિત:
    Ast_ગાંઠ_CCall(VNType t, File_લીટી* fl, AstCFunc* funcp, Ast_ગાંઠ_પદાવલિ* argsp = nullptr)
        : Ast_ગાંઠ_પદાવલિ{t, fl}
        , m_funcp{funcp} {
        addArgsp(argsp);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_CCall;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_CALL; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ Ast_ગાંઠ_CCall* અચલ asamep = VN_DBG_AS(samep, ગાંઠ_CCall);
        પાછા (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override;
    bool isOutputter() override { પાછા !isPure(); }
    AstCFunc* funcp() અચલ { પાછા m_funcp; }
    શૂન્ય funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() અચલ { પાછા m_argTypes; }
    શૂન્ય argTypes(અચલ string& str) { m_argTypes = str; }
    bool isUnlikely() અચલ override { પાછા m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ final override { પાછા true; }
    bool superReference() અચલ { પાછા m_superReference; }
    શૂન્ય superReference(bool flag) { m_superReference = flag; }
};
class Ast_ગાંઠ_Cons VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Construct a value of an aggregate type
    friend class Ast_ગાંઠ_પદાવલિ;  // For m_knownConstantTree
    mutable bool m_known_Constant_વૃક્ષ = false;  // Known to be 'this->isConstantTree() == true'

સુરક્ષિત:
    Ast_ગાંઠ_Cons(VNType t, File_લીટી* fl)
        : Ast_ગાંઠ_પદાવલિ{t, fl} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Cons;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ final override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ final override { પાછા widthInstrs(); }
};
class Ast_ગાંઠ_FTask_Ref VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_ગાંઠ_FTask_Ref;  // Gen pointers before other members for performance
ખાનગી:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

સુરક્ષિત:
    Ast_ગાંઠ_FTask_Ref(VNType t, File_લીટી* fl, અચલ string& name, AstArg* argsp)
        : Ast_ગાંઠ_પદાવલિ{t, fl}
        , m_name{name} {
        addArgsp(argsp);
    }

જાહેર:
    અચલ અક્ષર* broken() અચલ override;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    bool isGateOptimizable() અચલ override;
    string dotted() અચલ { પાછા m_dotted; }  // * = Scope name or ""
    string inlinedDots() અચલ { પાછા m_inlinedDots; }
    શૂન્ય inlinedDots(અચલ string& flag) { m_inlinedDots = flag; }
    Ast_ગાંઠ_FTask* taskp() અચલ { પાછા m_taskp; }  // [After Link] Pointer to variable
    શૂન્ય taskp(Ast_ગાંઠ_FTask* taskp) { m_taskp = taskp; }
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    શૂન્ય dotted(અચલ string& name) { m_dotted = name; }
    Ast_ગાંઠ_Module* classOrPackagep() અચલ { પાછા m_classOrPackagep; }
    શૂન્ય classOrPackagep(Ast_ગાંઠ_Module* nodep) { m_classOrPackagep = nodep; }
    bool containsGenBlock() અચલ { પાછા m_containsGenBlock; }
    શૂન્ય containsGenBlock(અચલ bool flag) { m_containsGenBlock = flag; }
    bool pli() અચલ { પાછા m_pli; }
    શૂન્ય pli(bool flag) { m_pli = flag; }
    bool superReference() અચલ { પાછા m_superReference; }
    શૂન્ય superReference(bool flag) { m_superReference = flag; }
    bool isPure() override;
    bool isUnlikely() અચલ override { પાછા m_taskp && m_taskp->isUnlikely(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ Ast_ગાંઠ_FTask_Ref* અચલ asamep = VN_DBG_AS(samep, ગાંઠ_FTask_Ref);
        પાછા taskp() == asamep->taskp()  //
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
    bool cleanOut() અચલ final override { V3ERROR_NA_RETURN(true); }

ખાનગી:
    bool getPurityRecurse() અચલ;
};
class Ast_ગાંઠ_Pre_Sel VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

સુરક્ષિત:
    Ast_ગાંઠ_Pre_Sel(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ_પદાવલિ* thsp)
        : Ast_ગાંઠ_પદાવલિ{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Pre_Sel;
    // METHODS
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ final override { V3ERROR_NA_RETURN(true); }
    bool isPure() override;
    અચલ અક્ષર* broken() અચલ override;

ખાનગી:
    bool getPurityRecurse() અચલ {
        પાછા fromp()->isPure() && rhsp()->isPure() && (!thsp() || thsp()->isPure());
    }
};
class Ast_ગાંઠ_Quadop VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

સુરક્ષિત:
    Ast_ગાંઠ_Quadop(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ_પદાવલિ* thsp,
                  Ast_ગાંઠ_પદાવલિ* fhsp)
        : Ast_ગાંઠ_પદાવલિ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    આભાસી શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs,
                               અચલ V3_સંખ્યા& ths, અચલ V3_સંખ્યા& fhs)
        = 0;
    આભાસી bool cleanLhs() અચલ = 0;  // True if LHS must have extra upper bits zero
    આભાસી bool cleanRhs() અચલ = 0;  // True if RHS must have extra upper bits zero
    આભાસી bool cleanThs() અચલ = 0;  // True if THS must have extra upper bits zero
    આભાસી bool cleanFhs() અચલ = 0;  // True if THS must have extra upper bits zero
    આભાસી bool sizeMattersLhs() અચલ = 0;  // True if output result depends on lhs size
    આભાસી bool sizeMattersRhs() અચલ = 0;  // True if output result depends on rhs size
    આભાસી bool sizeMattersThs() અચલ = 0;  // True if output result depends on ths size
    આભાસી bool sizeMattersFhs() અચલ = 0;  // True if output result depends on ths size
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    bool isPure() override;
    અચલ અક્ષર* broken() અચલ override;

ખાનગી:
    bool getPurityRecurse() અચલ {
        પાછા lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure() && fhsp()->isPure();
    }
};
class Ast_ગાંઠ_Termop VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Terminal operator -- an operator with no "inputs"
સુરક્ષિત:
    Ast_ગાંઠ_Termop(VNType t, File_લીટી* fl)
        : Ast_ગાંઠ_પદાવલિ{t, fl} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Termop;
    bool isSystemFunc() અચલ override { પાછા true; }
};
class Ast_ગાંઠ_Triop VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

સુરક્ષિત:
    Ast_ગાંઠ_Triop(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ_પદાવલિ* thsp)
        : Ast_ગાંઠ_પદાવલિ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Triop;
    // METHODS
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    // Set out to evaluation of a AstConst'ed
    આભાસી શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs,
                               અચલ V3_સંખ્યા& ths)
        = 0;
    આભાસી bool cleanLhs() અચલ = 0;  // True if LHS must have extra upper bits zero
    આભાસી bool cleanRhs() અચલ = 0;  // True if RHS must have extra upper bits zero
    આભાસી bool cleanThs() અચલ = 0;  // True if THS must have extra upper bits zero
    આભાસી bool sizeMattersLhs() અચલ = 0;  // True if output result depends on lhs size
    આભાસી bool sizeMattersRhs() અચલ = 0;  // True if output result depends on rhs size
    આભાસી bool sizeMattersThs() અચલ = 0;  // True if output result depends on ths size
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    bool isPure() override;
    અચલ અક્ષર* broken() અચલ override;

ખાનગી:
    bool getPurityRecurse() અચલ {
        પાછા lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure();
    }
};
class Ast_ગાંઠ_Dist_Triop VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Triop {
જાહેર:
    Ast_ગાંઠ_Dist_Triop(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp,
                     Ast_ગાંઠ_પદાવલિ* thsp)
        : Ast_ગાંઠ_Triop{t, fl, lhsp, rhsp, thsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_ગાંઠ_Dist_Triop;
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool cleanThs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool sizeMattersThs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL_TRIG; }
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs,
                       અચલ V3_સંખ્યા& ths) override {
        V3ERROR_NA;
    }
};
class Ast_ગાંઠ_Uniop VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

સુરક્ષિત:
    Ast_ગાંઠ_Uniop(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : Ast_ગાંઠ_પદાવલિ{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Uniop;
    // METHODS
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    // Set out to evaluation of a AstConst'ed lhs
    આભાસી શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) = 0;
    આભાસી bool cleanLhs() અચલ = 0;
    આભાસી bool sizeMattersLhs() અચલ = 0;  // True if output result depends on lhs size
    આભાસી bool doubleFlavor() અચલ { પાછા false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    આભાસી bool signedFlavor() અચલ { પાછા false; }
    આભાસી bool stringFlavor() અચલ { પાછા false; }  // N flavor of nodes with both flavors?
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    bool isPure() override;
    અચલ અક્ષર* broken() અચલ override;
};
class Ast_ગાંઠ_System_Uniop_D VL_NOT_FINAL : જાહેર Ast_ગાંઠ_Uniop {
જાહેર:
    Ast_ગાંઠ_System_Uniop_D(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : Ast_ગાંઠ_Uniop{t, fl, lhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_ગાંઠ_System_Uniop_D;
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL_TRIG; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class Ast_ગાંઠ_Var_Ref VL_NOT_FINAL : જાહેર Ast_ગાંઠ_પદાવલિ {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_ગાંઠ_Var_Ref;  // Gen pointers before other members for performance
ખાનગી:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
સુરક્ષિત:
    Ast_ગાંઠ_Var_Ref(VNType t, File_લીટી* fl, AstVar* varp, અચલ VAccess& access)
        : Ast_ગાંઠ_પદાવલિ{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

જાહેર:
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    VAccess access() અચલ { પાછા m_access; }
    શૂન્ય access(અચલ VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() અચલ VL_MT_STABLE { પાછા m_varp; }  // [After Link] Pointer to variable
    શૂન્ય varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_ગાંઠ*>(varp));
    }
    AstVarScope* varScopep() અચલ { પાછા m_varScopep; }
    શૂન્ય varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    અચલ VSelfPointerText& selfPointer() અચલ { પાછા m_selfPointer; }
    શૂન્ય selfPointer(અચલ VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) અચલ {
        પાછા selfPointer().protect(useSelfForThis, protect());
    }
    Ast_ગાંઠ_Module* classOrPackagep() અચલ { પાછા m_classOrPackagep; }
    શૂન્ય classOrPackagep(Ast_ગાંઠ_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : જાહેર Ast_ગાંઠ {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
જાહેર:
    AstArg(File_લીટી* fl, અચલ std::string& name, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ std::string& name) override { m_name = name; }
    bool emptyConnectNoNext() અચલ { પાછા !exprp() && name() == "" && !nextp(); }
};
class AstWith final : જાહેર Ast_ગાંઠ {
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
ખાનગી:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::set<std::string> m_restrictedNames;

જાહેર:
    AstWith(File_લીટી* fl, AstLambdaArgRef* indexArgRefp, AstLambdaArgRef* કિંમત_Arg_Refp,
            Ast_ગાંઠ* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->indexArgRefp(indexArgRefp);
        this->કિંમત_Arg_Refp(કિંમત_Arg_Refp);
        addExprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!indexArgRefp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!કિંમત_Arg_Refp());  // varp needed to know lambda's arg dtype
        પાછા nullptr;
    }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    શૂન્ય restricted(bool flag) { m_restricted = flag; }
    bool restricted() અચલ { પાછા m_restricted; }
    bool validated() અચલ { પાછા m_validated; }
    શૂન્ય validated(bool flag) { m_validated = flag; }
    શૂન્ય addRestrictedName(અચલ std::string& name) { m_restrictedNames.ઉમેરો(name); }
    અચલ std::set<std::string>& restrictedNames() અચલ { પાછા m_restrictedNames; }
    // True if 'name' binds into the randomize() target class.
    bool nameResolvesToTarget(અચલ std::string& name) અચલ {
        પાછા !m_restricted || m_restrictedNames.ગણતરી(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbortKind m_kind{VAbortKind::ACCEPT_ON};

જાહેર:
    AstAbortOn(File_લીટી* fl, VAbortKind kind, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ_પદાવલિ* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_kind{kind} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    VAbortKind kind() અચલ { પાછા m_kind; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstAddrOfCFunc final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

જાહેર:
    AstAddrOfCFunc(File_લીટી* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_AstAddrOfCFunc;
    શૂન્ય dump(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    AstCFunc* funcp() અચલ { પાછા m_funcp; }
};
class AstAttrOf final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
જાહેર:
    AstAttrOf(File_લીટી* fl, VAttrType attrtype, Ast_ગાંઠ* fromp = nullptr,
              Ast_ગાંઠ_પદાવલિ* dimp = nullptr)
        : ASTGEN_SUPER_AttrOf(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_AstAttrOf;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    VAttrType attrType() અચલ { પાછા m_attrType; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    અચલ bool m_pure;  // Pure optimizable

    શૂન્ય init(અચલ string& text, પૂર્ણાંક setwidth) {
        જો (!text.empty()) add(text);
        જો (setwidth) {
            dtypeSetBitSized(setwidth, VSigning::UNSIGNED);
        } નહીંતર {
            dtypeSetVoid();  // Caller to override if necessary
        }
    }

જાહેર:
    class Pure {};
    explicit AstCExpr(File_લીટી* fl, અચલ string& text = "", પૂર્ણાંક setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{false} {
        init(text, setwidth);
    }
    AstCExpr(File_લીટી* fl, Pure, અચલ string& text = "", પૂર્ણાંક setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    bool cleanOut() અચલ override { પાછા true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા m_pure; }
    bool isOutputter() override { પાછા true; }
    bool isPredictOptimizable() અચલ override { પાછા m_pure; }
    bool isPure() override { પાછા m_pure; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    // Add some text, or a node to this expression
    શૂન્ય add(અચલ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    શૂન્ય add(Ast_ગાંઠ* nodep) { addNodesp(nodep); }
};
class AstCExprUser final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    અચલ bool m_pure;  // Whether the function is pure
જાહેર:
    class Pure {};
    explicit AstCExprUser(File_લીટી* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{false} {}
    AstCExprUser(File_લીટી* fl, Pure)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    bool cleanOut() અચલ override { પાછા false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા m_pure; }
    bool isOutputter() override { પાછા true; }
    bool isPredictOptimizable() અચલ override { પાછા m_pure; }
    bool isPure() override { પાછા m_pure; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    // Add some text, or a node to this expression
    શૂન્ય add(અચલ std::string& text) { addNodesp(new AstText{fileline(), text}); }
    શૂન્ય add(Ast_ગાંઠ* nodep) { addNodesp(nodep); }
};
class AstCFuncHard final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
જાહેર:
    AstCFuncHard(File_લીટી* fl, VCFunction function, Ast_ગાંઠ_પદાવલિ* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા function().ascii(); }
    bool isPure() override {
        જો (!m_purity.isCached()) m_purity.set(getPurity());
        પાછા m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    VCFunction function() અચલ VL_MT_STABLE { પાછા m_function; }

ખાનગી:
    bool getPurity();
};
class AstCMethodHard final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_method;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
જાહેર:
    AstCMethodHard(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, VCMethod method, Ast_ગાંઠ_પદાવલિ* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_method{method} {
        this->fromp(fromp);
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા method().ascii(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstCMethodHard* અચલ asamep = VN_DBG_AS(samep, CMethodHard);
        પાછા (m_method == asamep->m_method);
    }
    bool isPure() override {
        જો (!m_purity.isCached()) m_purity.set(getPurity());
        પાછા m_purity.get();
    }
    પૂર્ણાંક instr_ગણતરી() અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા m_method.isClean(); }
    bool usePtr() અચલ { પાછા m_usePtr; }
    શૂન્ય usePtr(bool flag) { m_usePtr = flag; }
    VCMethod method() અચલ VL_MT_STABLE { પાછા m_method; }
    શૂન્ય method(VCMethod કિંમત) { m_method = કિંમત; }

ખાનગી:
    bool getPurity();
};
class AstCReset final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Reset variable at startup
    અચલ bool m_constructing;  // Previously cleared by constructor
જાહેર:
    AstCReset(File_લીટી* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isPure() override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!VN_IS(backp(), ગાંઠ_Assign));  // V3Emit* assumption
        પાછા nullptr;
    }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() અચલ { પાછા m_constructing; }
};
class AstCast final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
જાહેર:
    AstCast(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, VFlagChildDType, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { પાછા "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
    Ast_ગાંઠ_DType* getChildDTypep() અચલ override { પાછા childDTypep(); }
    Ast_ગાંઠ_DType* subDTypep() અચલ VL_MT_STABLE { પાછા dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
જાહેર:
    AstCastParse(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { પાછા "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstCastSize final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
જાહેર:
    AstCastSize(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_CastSize(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCastSize;
    string emitVerilog() override { પાછા "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
જાહેર:
    AstCellArrayRef(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ_પદાવલિ* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        addSelp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
જાહેર:
    AstCellRef(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* cellp, Ast_ગાંઠ* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
જાહેર:
    AstClassOrPackageRef(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_ગાંઠ_Module* classOrPackageSkipp(અચલ bool doRefs = true) અચલ;
    Ast_ગાંઠ* classOrPackageNodep() અચલ { પાછા m_classOrPackageNodep; }
    શૂન્ય classOrPackageNodep(Ast_ગાંઠ* nodep) { m_classOrPackageNodep = nodep; }
    શૂન્ય classOrPackagep(Ast_ગાંઠ_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_ગાંઠ*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A constant
    V3_સંખ્યા m_અંક;  // Constant value
    શૂન્ય init_With_સંખ્યા() {
        જો (m_અંક.isDouble()) {
            dtypeSetDouble();
        } નહીંતર જો (m_અંક.isString()) {
            dtypeSetString();
        } નહીંતર {
            dtypeSetBitOrLogicUnsized(m_અંક.width(), (m_અંક.sized() ? 0 : m_અંક.widthToFit()),
                                      VSigning::fromBool(m_અંક.isSigned()), m_અંક.isAnyXZ());
        }
        m_અંક.nodep(this);
    }

જાહેર:
    ~AstConst() override;
    AstConst(File_લીટી* fl, અચલ V3_સંખ્યા& અંક)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{અંક} {
        init_With_સંખ્યા();
    }
    class Widthed_કિંમત {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, Widthed_કિંમત, પૂર્ણાંક width, uint32_t કિંમત)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, width, કિંમત) {  // Need () constructor
        init_With_સંખ્યા();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_લીટી* fl, DTyped, અચલ Ast_ગાંઠ_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{this, nodedtypep} {
        init_With_સંખ્યા();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, StringToParse, અચલ અક્ષર* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{this, V3_સંખ્યા::Verilog_સંખ્યા_Literal{}, sourcep} {
        init_With_સંખ્યા();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, VerilogStringLiteral, અચલ string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{this, V3_સંખ્યા::VerilogStringLiteral{}, str} {
        init_With_સંખ્યા();
    }
    AstConst(File_લીટી* fl, uint32_t અંક)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, 32, અંક) {  // Need () constructor
        dtypeSetBitUnsized(m_અંક.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, Unsized32, uint32_t અંક)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, 32, અંક) {  // Need () constructor
        m_અંક.width(32, false);
        dtypeSetBitUnsized(32, m_અંક.widthToFit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, Signed32, int32_t અંક)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, 32, અંક) {  // Need () constructor
        m_અંક.width(32, true);
        dtypeSetBitUnsized(32, m_અંક.widthToFit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, Unsized64, uint64_t અંક)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, 64, 0) {  // Need () constructor
        m_અંક.setQuad(અંક);
        dtypeSetBitSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, SizedEData, uint64_t અંક)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_અંક.setQuad(અંક);
        dtypeSetBitSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, RealDouble, દ્વિગુણ અંક)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, 64) {  // Need () constructor
        m_અંક.setDouble(અંક);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_લીટી* fl, String, અચલ string& અંક)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{this, V3_સંખ્યા::String{}, અંક} {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(File_લીટી* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_લીટી* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
        File_લીટી* અચલ newfl = new File_લીટી{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_લીટી* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક(this, 1, on) {  // Need () constructor
        dtypeSetBit();
    }
    class All0 {};
    AstConst(File_લીટી* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{this, V3_સંખ્યા::Verilog_સંખ્યા_Literal{}, "'0"} {
        init_With_સંખ્યા();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_લીટી* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{this, V3_સંખ્યા::Verilog_સંખ્યા_Literal{}, "'1"} {
        init_With_સંખ્યા();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_લીટી* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{this, V3_સંખ્યા::Null{}} {
        dtypep(findNullDType());
        init_With_સંખ્યા();
    }
    class OneStep {};
    AstConst(File_લીટી* fl, OneStep)
        : ASTGEN_SUPER_Const(fl)
        , m_અંક{this, V3_સંખ્યા::OneStep{}} {
        dtypeSetBitSized(64, VSigning::UNSIGNED);
        init_With_સંખ્યા();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() અચલ override VL_MT_STABLE { પાછા અંક().ascii(); }  // * = Value
    અચલ V3_સંખ્યા& અંક() અચલ VL_MT_SAFE { પાછા m_અંક; }  // * = Value
    V3_સંખ્યા& અંક() { પાછા m_અંક; }  // * = Value
    string origParamName() અચલ;
    શૂન્ય origParamName(અચલ string& name);
    uint32_t toUInt() અચલ { પાછા અંક().toUInt(); }
    int32_t toSInt() અચલ VL_MT_SAFE { પાછા અંક().toSInt(); }
    uint64_t toUQuad() અચલ { પાછા અંક().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstConst* અચલ sp = VN_DBG_AS(samep, Const);
        પાછા અંક().isCaseEq(sp->અંક());
    }
    શૂન્ય cloneRelink() override;
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(m_અંક.nodep() && m_અંક.nodep() != this);
        પાછા nullptr;
    }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool isEqAllOnes() અચલ { પાછા અંક().isEqAllOnes(width()); }
    bool isEqAllOnesV() અચલ { પાછા અંક().isEqAllOnes(widthMinV()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_કિંમત_Type(અચલ AstConst* samep) અચલ;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    સ્થિર AstConst* parseParamLiteral(File_લીટી* fl, અચલ string& literal);
};
class AstConstraintRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

જાહેર:
    AstConstraintRef(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() અચલ override VL_MT_STABLE;
    AstConstraint* constrp() અચલ VL_MT_STABLE { પાછા m_constrp; }
    શૂન્ય constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_ગાંઠ_Module* classOrPackagep() અચલ VL_MT_STABLE { પાછા m_classOrPackagep; }
    // TODO: this setter is unused
    શૂન્ય classOrPackagep(Ast_ગાંઠ_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ final override { V3ERROR_NA_RETURN(true); }
};
class AstCvtArrayToArray final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
ખાનગી:
    અચલ bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    અચલ પૂર્ણાંક m_blockSize;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    અચલ પૂર્ણાંક m_dstElementBits;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    અચલ પૂર્ણાંક m_srcElementBits;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

જાહેર:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToArray(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_DType* dtp, bool reverse,
                       પૂર્ણાંક blockSize, પૂર્ણાંક dstElementBits, પૂર્ણાંક srcElementBits)
        : ASTGEN_SUPER_CvtArrayToArray(fl)
        , m_reverse{reverse}
        , m_blockSize{blockSize}
        , m_dstElementBits{dstElementBits}
        , m_srcElementBits{srcElementBits} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToArray;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool reverse() અચલ { પાછા m_reverse; }
    પૂર્ણાંક blockSize() અચલ { પાછા m_blockSize; }
    પૂર્ણાંક dstElementBits() અચલ { પાછા m_dstElementBits; }
    પૂર્ણાંક srcElementBits() અચલ { પાછા m_srcElementBits; }
};
class AstCvtArrayToPacked final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
જાહેર:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToPacked(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_CvtArrayToPacked(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToPacked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
};
class AstCvtPackedToArray final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
જાહેર:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtPackedToArray(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_CvtPackedToArray(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtPackedToArray;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
};
class AstCvtUnpackToQueue final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
જાહેર:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtUnpackToQueue(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToQueue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToQueue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { પાછા "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool cleanOut() અચલ override { પાછા true; }
};
class AstCvtUnpackToUnpack final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
જાહેર:
    AstCvtUnpackToUnpack(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToUnpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToUnpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
};
class AstDist final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
જાહેર:
    AstDist(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { પાછા "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }  // NA
};
class AstDistItem final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_isWhole = false;  // True for weight ':/', false for ':='
જાહેર:
    AstDistItem(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* rangep, Ast_ગાંઠ_પદાવલિ* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { પાછા "%l "s + (m_isWhole ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }  // NA
    શૂન્ય isWhole(bool flag) { m_isWhole = flag; }
    bool isWhole() અચલ { પાછા m_isWhole; }
};
class AstDot final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    અચલ bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
જાહેર:
    AstDot(File_લીટી* fl, bool colon, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    સ્થિર Ast_ગાંઠ_પદાવલિ* newIfPkg(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* packageOrClassp, Ast_ગાંઠ_પદાવલિ* rhsp) {
        જો (!packageOrClassp) પાછા rhsp;
        પાછા new AstDot{fl, true, packageOrClassp, rhsp};
    }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool colon() અચલ { પાછા m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstEmptyQueue final : જાહેર Ast_ગાંઠ_પદાવલિ {
જાહેર:
    explicit AstEmptyQueue(File_લીટી* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_AstEmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { પાછા "{}"; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool cleanOut() અચલ override { પાછા true; }
};
class AstEnumItemRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_containsGenBlock = false;  // Contains gen block reference
જાહેર:
    AstEnumItemRef(File_લીટી* fl, AstEnumItem* itemp, Ast_ગાંઠ_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    અચલ અક્ષર* broken() અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા itemp() ? itemp()->name() : m_name; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstEnumItemRef* અચલ sp = VN_DBG_AS(samep, EnumItemRef);
        પાછા itemp() == sp->itemp() && dotted() == sp->dotted()
               && containsGenBlock() == sp->containsGenBlock();
    }
    AstEnumItem* itemp() અચલ VL_MT_STABLE { પાછા m_itemp; }
    શૂન્ય itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() અચલ { પાછા m_dotted; }
    શૂન્ય dotted(અચલ string& dotted) { m_dotted = dotted; }
    bool containsGenBlock() અચલ { પાછા m_containsGenBlock; }
    શૂન્ય containsGenBlock(અચલ bool flag) { m_containsGenBlock = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    Ast_ગાંઠ_Module* classOrPackagep() અચલ { પાછા m_classOrPackagep; }
    શૂન્ય classOrPackagep(Ast_ગાંઠ_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_પદાવલિ_Stmt final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
ખાનગી:
    bool m_hasResult = true;  // Returns result via resultp()

જાહેર:
    Ast_પદાવલિ_Stmt(File_લીટી* fl, Ast_ગાંઠ* stmtsp, Ast_ગાંઠ_પદાવલિ* resultp)
        : ASTGEN_SUPER_પદાવલિ_Stmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_પદાવલિ_Stmt;
    // METHODS
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool isPure() override {
        જો (Ast_ગાંઠ::afterCommentp(stmtsp())) પાછા false;
        પાછા resultp()->isPure();
    }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    bool hasResult() અચલ { પાછા m_hasResult; }
    શૂન્ય hasResult(bool flag) { m_hasResult = flag; }
};
class AstFError final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
જાહેર:
    AstFError(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* filep, Ast_ગાંઠ_પદાવલિ* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { પાછા "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 64; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstFOpen final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
જાહેર:
    AstFOpen(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* filenamep, Ast_ગાંઠ_પદાવલિ* modep)
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
    bool cleanOut() અચલ override { પાછા true; }
    string verilogKwd() અચલ override { પાછા "$fopen"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstFOpenMcd final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op2 := filenamep : AstNodeExpr
જાહેર:
    AstFOpenMcd(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    string verilogKwd() અચલ override { પાછા "$fopen"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstFRead final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
જાહેર:
    AstFRead(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* memp, Ast_ગાંઠ_પદાવલિ* filep, Ast_ગાંઠ_પદાવલિ* startp,
             Ast_ગાંઠ_પદાવલિ* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() અચલ override { પાછા "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: makes output
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstFRewind final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := filep : Optional[AstNodeExpr]
જાહેર:
    AstFRewind(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() અચલ override { પાછા "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstFScanF final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

જાહેર:
    AstFScanF(File_લીટી* fl, અચલ string& text, Ast_ગાંઠ_પદાવલિ* filep, Ast_ગાંઠ_પદાવલિ* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() અચલ override VL_MT_STABLE { પાછા m_text; }
    string verilogKwd() અચલ override { પાછા "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: makes output
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() અચલ { પાછા m_text; }  // * = Text to display
    શૂન્ય text(અચલ string& text) { m_text = text; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstFSeek final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
જાહેર:
    AstFSeek(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* filep, Ast_ગાંઠ_પદાવલિ* offset, Ast_ગાંઠ_પદાવલિ* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() અચલ override { પાછા "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: makes output
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstFTell final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
જાહેર:
    AstFTell(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() અચલ override { પાછા "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstFalling final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
જાહેર:
    AstFalling(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { પાછા "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstFell final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
જાહેર:
    AstFell(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_Sen_વૃક્ષ* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { પાછા "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstFuture final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
જાહેર:
    AstFuture(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_Sen_વૃક્ષ* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { પાછા "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstGatePin final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
જાહેર:
    AstGatePin(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { પાછા "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
};
class AstGetInitialRandomSeed final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $get_initial_random_seed()
જાહેર:
    explicit AstGetInitialRandomSeed(File_લીટી* fl)
        : ASTGEN_SUPER_GetInitialRandomSeed(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstGetInitialRandomSeed;
    string emitVerilog() override { પાછા "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા true; }
    bool isPure() override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstImplication final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

ખાનગી:
    અચલ bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    અચલ bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

જાહેર:
    AstImplication(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool isMultiCycleSva() અચલ override { પાછા m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() અચલ { પાછા m_isOverlapped; }
    bool isFollowedBy() અચલ { પાછા m_isFollowedBy; }
};
class AstInside final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
જાહેર:
    AstInside(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { પાછા "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }  // NA
};
class AstInsideRange final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
જાહેર:
    AstInsideRange(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { પાછા "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_ગાંઠ_પદાવલિ* newAndFromInside(Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp);
};
class AstLambdaArgRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_index;  // Index, not value

જાહેર:
    AstLambdaArgRef(File_લીટી* fl, અચલ string& name, bool index)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_index{index} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    string emitVerilog() override { પાછા name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    bool index() અચલ { પાછા m_index; }
    bool is_પદાવલિ_Coverage_Eligible() અચલ override { પાછા false; }
};
class AstMatchMasked final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
જાહેર:
    અંતઃરેખ AstMatchMasked(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_AstMatchMasked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { પાછા "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool cleanOut() અચલ override { પાછા true; }
    સ્થિર uint32_t fold(અચલ V3_સંખ્યા& lhs, AstVar* matchVarp);
};
class AstMatches final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
જાહેર:
    AstMatches(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { પાછા "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstMemberSel final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
જાહેર:
    AstMemberSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, VFlagChildDType, અચલ string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    VAccess access() અચલ { પાછા m_access; }
    શૂન્ય access(અચલ VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    AstVar* varp() અચલ { પાછા m_varp; }
    શૂન્ય varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
જાહેર:
    AstNewCopy(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { પાછા "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
};
class AstNewDynamic final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
જાહેર:
    AstNewDynamic(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* sizep, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { પાછા "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
};
class AstPExpr final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
જાહેર:
    explicit AstPExpr(File_લીટી* fl, AstBegin* bodyp, Ast_ગાંઠ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_લીટી* fl, AstBegin* bodyp, Ast_ગાંઠ_Stmt* finalp, Ast_ગાંઠ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
};
class AstParseHolder final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
જાહેર:
    explicit AstParseHolder(File_લીટી* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

જાહેર:
    AstParseRef(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ_પદાવલિ* lhsp = nullptr,
                Ast_ગાંઠ_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstParseRef* અચલ asamep = VN_DBG_AS(samep, ParseRef);
        પાછા m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

જાહેર:
    AstPast(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* ticksp = nullptr,
            Ast_Sen_વૃક્ષ* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() અચલ { પાછા m_propertyTiming; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstPatMember final : જાહેર Ast_ગાંઠ_પદાવલિ {
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

જાહેર:
    AstPatMember(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhssp, Ast_ગાંઠ* keyp, Ast_ગાંઠ_પદાવલિ* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { પાછા lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 2; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    bool isConcat() અચલ { પાછા m_isConcat; }
    શૂન્ય isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() અચલ { પાછા m_isDefault; }
    શૂન્ય isDefault(bool flag) { m_isDefault = flag; }
    Ast_ગાંઠ_પદાવલિ* ચાવી_Exprp() અચલ { પાછા varrefp() ? varrefp() : VN_CAST(keyp(), ગાંઠ_પદાવલિ); }
};
class AstPattern final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
જાહેર:
    AstPattern(File_લીટી* fl, Ast_ગાંઠ* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstPattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    Ast_ગાંઠ_DType* getChildDTypep() અચલ override { પાછા childDTypep(); }
    Ast_ગાંઠ_DType* subDTypep() અચલ VL_MT_STABLE { પાછા dtypep() ? dtypep() : childDTypep(); }
};
class AstPatternStar final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Pattern wildcard: ".*"
જાહેર:
    explicit AstPatternStar(File_લીટી* fl)
        : ASTGEN_SUPER_PatternStar(fl) {}
    ASTGEN_MEMBERS_AstPatternStar;
    string emitVerilog() override { પાછા ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstPatternVar final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
જાહેર:
    AstPatternVar(File_લીટી* fl, અચલ string& name)
        : ASTGEN_SUPER_PatternVar(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPatternVar;
    string emitVerilog() override { પાછા ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_name == VN_DBG_AS(samep, PatternVar)->m_name;
    }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
};
class AstPropAlways final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    અચલ bool m_isStrong = false;  // s_always
જાહેર:
    AstPropAlways(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* propp, Ast_ગાંઠ_પદાવલિ* loBoundp, Ast_ગાંઠ_પદાવલિ* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { V3ERROR_NA_RETURN(0); }
    bool isStrong() અચલ { પાછા m_isStrong; }
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstRand final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    અચલ bool m_urandom;  // $urandom vs $random
જાહેર:
    class Reset {};
    AstRand(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_લીટી* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtypeSetUInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override {
        પાછા seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        જો (seedp()) {
            જો (urandom()) પાછા "VL_URANDOM_SEEDED_%nq%lq(%li)";
            પાછા "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        જો (isWide()) પાછા "VL_RANDOM_%nq(%nw, %P)";
        પાછા "VL_RANDOM_%nq()";
    }
    bool cleanOut() અચલ override { પાછા false; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા !seedp(); }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool combinable(અચલ AstRand* samep) અચલ {
        પાછા !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() અચલ { પાછા m_urandom; }
};
class AstRandRNG final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
જાહેર:
    AstRandRNG(File_લીટી* fl, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { પાછા "%f$rngrandom()"; }
    string emitC() override {
        પાછા isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool cleanOut() અચલ override { પાછા false; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstRising final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
જાહેર:
    AstRising(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { પાછા "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstRose final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
જાહેર:
    AstRose(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_Sen_વૃક્ષ* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { પાછા "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstSClocked final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
જાહેર:
    AstSClocked(File_લીટી* fl, AstSenItem* sensesp, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : જાહેર Ast_ગાંઠ_પદાવલિ {
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
    અચલ bool m_unbounded = false;  // True when the upper bound is $
જાહેર:
    // Exact [*N]
    AstSConsRep(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* countp, Ast_ગાંઠ_પદાવલિ* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { V3ERROR_NA_RETURN(0); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {  // LCOV_EXCL_LINE
        પાછા m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() અચલ { પાછા m_unbounded; }
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstSEventually final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
જાહેર:
    explicit AstSEventually(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
જાહેર:
    explicit AstSExpr(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* preExprp, Ast_ગાંઠ_Stmt* delayp, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_લીટી* fl, Ast_ગાંઠ_Stmt* delayp, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstSFormatArg final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

જાહેર:
    AstSFormatArg(File_લીટી* fl, VFormatAttr formatAttr, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() અચલ override { પાછા "$sformatarg"; }
    string emitVerilog() override { પાછા "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        પાછા nullptr;
    }
    VFormatAttr formatAttr() અચલ { પાછા m_formatAttr; }
    શૂન્ય formatAttr(અચલ VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    સ્થિર VFormatAttr formatAttrDefauled(અચલ AstSFormatArg* nodep, અચલ Ast_ગાંઠ_DType* dtypep);
};
class AstSFormatF final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_પદાવલિ_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    અચલ અક્ષર m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
જાહેર:
    class પદાવલિ_Format {};
    AstSFormatF(File_લીટી* fl, અચલ string& text, bool hidden, Ast_ગાંઠ_પદાવલિ* exprsp,
                અક્ષર missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    AstSFormatF(File_લીટી* fl, પદાવલિ_Format, Ast_ગાંઠ_પદાવલિ* exprsp, અક્ષર missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_પદાવલિ_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_text; }
    શૂન્ય name(અચલ string& name) override { m_text = name; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() અચલ override { પાછા "$sformatf"; }
    string text() અચલ { પાછા m_text; }  // * = Text to display
    શૂન્ય text(અચલ string& text) { m_text = text; }
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(text() != "" && પદાવલિ_Format_2());  // Expr format means no literal format
        પાછા nullptr;
    }
    bool formatScopeTracking() અચલ {  // Track scopeNamep();  Ok if false positive
        જો (પદાવલિ_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            પાછા true;
        માટે (અચલ Ast_ગાંઠ* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            જો (અચલ AstConst* અચલ fmtp = VN_CAST(exprp, Const)) {
                જો (fmtp->અંક().isFromString()) {
                    અચલ string str = fmtp->અંક().toString();
                    જો (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        પાછા true;
                }
            }
        }
        પાછા false;
    }
    bool hidden() અચલ { પાછા m_hidden; }
    શૂન્ય hidden(bool flag) { m_hidden = flag; }
    bool પદાવલિ_Format_2() અચલ { પાછા m_પદાવલિ_Format; }
    શૂન્ય પદાવલિ_Format_2(bool flag) { m_પદાવલિ_Format = flag; }
    bool optionalFormat() અચલ { પાછા m_optionalFormat; }
    શૂન્ય optionalFormat(bool flag) { m_optionalFormat = flag; }
    અક્ષર missingArgChar() અચલ { પાછા m_missingArgChar; }
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstSGotoRep final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
જાહેર:
    // Exact [->N]
    AstSGotoRep(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* countp, Ast_ગાંઠ_પદાવલિ* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstSNonConsRep final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
જાહેર:
    // Exact [=N]
    AstSNonConsRep(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* countp, Ast_ગાંઠ_પદાવલિ* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstSScanF final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

જાહેર:
    AstSScanF(File_લીટી* fl, અચલ string& text, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->fromp(fromp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() અચલ override VL_MT_STABLE { પાછા m_text; }
    string verilogKwd() અચલ override { પાછા "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: makes output
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstSScanF* અચલ asamep = VN_DBG_AS(samep, SScanF);
        પાછા text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() અચલ { પાછા m_text; }  // * = Text to display
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstSampled final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_internal : 1;  // Internally created, not from a source $sampled
જાહેર:
    AstSampled(File_લીટી* fl, Ast_ગાંઠ* exprp, Ast_ગાંઠ_DType* dtypep, bool internal = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_internal{internal} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { પાછા "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_internal == VN_DBG_AS(samep, Sampled)->m_internal;
    }
    bool internal() અચલ { પાછા m_internal; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstScopeName final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    અચલ bool m_forFormat;  // Is for a format %m
    સ્થિર std::string scopeNameFormatter(અચલ std::string& text);
    સ્થિર std::string scopePrettyNameFormatter(અચલ std::string& text);

જાહેર:
    class ForFormat {};
    AstScopeName(File_લીટી* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstScopeName* અચલ sp = VN_DBG_AS(samep, ScopeName);
        પાછા m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    // ACCESSORS
    અચલ std::string& scopeAttr() અચલ { પાછા m_scopeAttr; }
    શૂન્ય scopeAttr(અચલ std::string& val) { m_scopeAttr = val; }
    અચલ std::string& scopeEntr() અચલ { પાછા m_scopeEntr; }
    શૂન્ય scopeEntr(અચલ std::string& val) { m_scopeEntr = val; }
    bool dpiExport() અચલ { પાછા m_dpiExport; }
    શૂન્ય dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() અચલ { પાછા m_selfRelative; }
    શૂન્ય selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() અચલ { પાછા m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() અચલ { પાછા scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() અચલ { પાછા scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() અચલ { પાછા scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() અચલ { પાછા scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
જાહેર:
    AstStable(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_Sen_વૃક્ષ* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { પાછા "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class Ast_ઢગલો_Trace_F final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // $stacktrace used as function
જાહેર:
    explicit Ast_ઢગલો_Trace_F(File_લીટી* fl)
        : ASTGEN_SUPER_ઢગલો_Trace_F(fl) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_Ast_ઢગલો_Trace_F;
    string verilogKwd() અચલ override { પાછા "$stacktrace"; }
    string emitVerilog() override { પાછા verilogKwd(); }
    string emitC() override { પાછા "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool cleanOut() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstSteady final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
જાહેર:
    AstSteady(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { પાછા "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstStructSel final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
જાહેર:
    AstStructSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, અચલ string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override {
        // Not a union
        પાછા VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstStructSel* અચલ sp = VN_DBG_AS(samep, StructSel);
        પાછા m_name == sp->m_name;
    }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
};
class AstSysIgnore final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
જાહેર:
    AstSysIgnore(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() અચલ override { પાછા "$ignored"; }
    bool isGateOptimizable() અચલ override { પાછા false; }  // Though deleted before opt
    bool isPredictOptimizable() અચલ override { પાછા false; }  // Though deleted before opt
    bool isPure() override { પાછા false; }  // Though deleted before opt
    bool isOutputter() override { પાછા true; }  // Though deleted before opt
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
જાહેર:
    AstSystemF(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() અચલ override { પાછા "$system"; }
    string emitVerilog() override { પાછા verilogKwd(); }
    string emitC() override { પાછા "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
    bool isUnlikely() અચલ override { પાછા true; }
    bool cleanOut() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class Ast_Tagged_પદાવલિ final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
જાહેર:
    Ast_Tagged_પદાવલિ(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_Tagged_પદાવલિ(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_પદાવલિ;
    string emitVerilog() override { પાછા "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_name == VN_DBG_AS(samep, Tagged_પદાવલિ)->m_name;
    }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
};
class AstTaggedPattern final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
જાહેર:
    AstTaggedPattern(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* patternp)
        : ASTGEN_SUPER_TaggedPattern(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstTaggedPattern;
    string emitVerilog() override { પાછા "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_name == VN_DBG_AS(samep, TaggedPattern)->m_name;
    }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
};
class AstTestPlusArgs final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
જાહેર:
    AstTestPlusArgs(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* searchp)
        : ASTGEN_SUPER_TestPlusArgs(fl) {
        this->searchp(searchp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstTestPlusArgs;
    string verilogKwd() અચલ override { પાછા "$test$plusargs"; }
    string emitVerilog() override { પાછા verilogKwd(); }
    string emitC() override { પાછા "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    // but isPure() true
    bool isSystemFunc() અચલ override { પાછા true; }
    bool cleanOut() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstThisRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
જાહેર:
    AstThisRef(File_લીટી* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_લીટી* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { પાછા "this"; }
    string emitVerilog() override { પાછા "this"; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool cleanOut() અચલ override { પાછા true; }
    Ast_ગાંઠ_DType* getChildDTypep() અચલ override { પાછા childDTypep(); }
};
class AstTimePrecision final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Verilog $timeprecision
જાહેર:
    explicit AstTimePrecision(File_લીટી* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { પાછા "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstTimeUnit final : જાહેર Ast_ગાંઠ_પદાવલિ {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
જાહેર:
    explicit AstTimeUnit(File_લીટી* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { પાછા "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    અચલ std::vector<uint32_t> m_indices;  // Indices of the triggers tested
જાહેર:
    AstTriggered(File_લીટી* fl, AstVarScope* trigVscp, અચલ std::vector<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    અચલ std::vector<uint32_t>& indices() અચલ { પાછા m_indices; }
};
class AstUnbounded final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
જાહેર:
    explicit AstUnbounded(File_લીટી* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { પાછા "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
};
class AstUnlinkedRef final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
જાહેર:
    AstUnlinkedRef(File_લીટી* fl, Ast_ગાંઠ* refp, અચલ string& name, Ast_ગાંઠ* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    અચલ bool m_strong;  // 's_' preffix
    અચલ bool m_overlapping;  // '_with` suffix
જાહેર:
    AstUntil(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() અચલ override {
        પાછા (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isMultiCycleSva() અચલ override { પાછા true; }
    bool isStrong() અચલ { પાછા m_strong; }
    bool isOverlapping() અચલ { પાછા m_overlapping; }
};
class Ast_કિંમત_Plus_Args final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
જાહેર:
    Ast_કિંમત_Plus_Args(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* searchp, Ast_ગાંઠ_પદાવલિ* outp)
        : ASTGEN_SUPER_કિંમત_Plus_Args(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_કિંમત_Plus_Args;
    string verilogKwd() અચલ override { પાછા "$value$plusargs"; }
    string emitVerilog() override { પાછા "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા !outp(); }
    bool isSystemFunc() અચલ override { પાછા true; }
    bool cleanOut() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstWithParse final : જાહેર Ast_ગાંઠ_પદાવલિ {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
ખાનગી:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

જાહેર:
    AstWithParse(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* funcrefp, Ast_ગાંઠ_પદાવલિ* exprsp,
                 Ast_ગાંઠ* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        addExprsp(exprsp);
        addConstraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool restricted() અચલ { પાછા m_restricted; }
    શૂન્ય restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : જાહેર Ast_ગાંઠ_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
જાહેર:
    AstBufIf1(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { પાછા "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstCastDynamic final : જાહેર Ast_ગાંઠ_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
જાહેર:
    AstCastDynamic(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtypeSetBitUnsized(32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { પાછા "%f$cast(%r, %l)"; }
    string emitC() override { પાછા "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 20; }
    bool isPure() override { પાછા false; }
};
class AstCompareNN final : જાહેર Ast_ગાંઠ_Biop {
    // Verilog str.compare() and str.icompare()
    અચલ bool m_ignoreCase;  // True for str.icompare()
જાહેર:
    AstCompareNN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() અચલ override VL_MT_STABLE { પાછા m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        પાછા m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        પાછા m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool ignoreCase() અચલ { પાછા m_ignoreCase; }
};
class AstConcat final : જાહેર Ast_ગાંઠ_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
જાહેર:
    AstConcat(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        જો (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetBitOrLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { પાછા "%f{%l, %k%r}"; }
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { પાછા "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(concat %l %r)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 2; }
};
class AstConcatN final : જાહેર Ast_ગાંઠ_Biop {
    // String concatenate
જાહેર:
    AstConcatN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { પાછા "%f{%l, %k%r}"; }
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { પાછા "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
    bool stringFlavor() અચલ override { પાછા true; }
};
class AstDiv final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstDiv(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f/ %r)"; }
    string emitC() override { પાછા "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvudiv %l %r)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_DIV; }
};
class AstDivD final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstDivD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstDivD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "/"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL_DIV; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstDivS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstDivS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f/ %r)"; }
    string emitC() override { પાછા "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvsdiv %l %r)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_DIV; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstEqWild final : જાહેર Ast_ગાંઠ_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
જાહેર:
    AstEqWild(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    સ્થિર Ast_ગાંઠ_Biop* newTyped(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp);
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f==? %r)"; }
    string emitC() override {
        જો (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } નહીંતર {
            પાછા "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() અચલ override { પાછા "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        જો (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } નહીંતર {
            પાછા "==";
        }
    }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstFGetS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
જાહેર:
    AstFGetS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%f$fgets(%l,%r)"; }
    string emitC() override {
        પાછા strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 64; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstFUngetC(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        પાછા "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 64; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ગાંઠ* filep() અચલ { પાછા lhsp(); }
    Ast_ગાંઠ* charp() અચલ { પાછા rhsp(); }
};
class AstGetcN final : જાહેર Ast_ગાંઠ_Biop {
    // Verilog string.getc()
જાહેર:
    AstGetcN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() અચલ override VL_MT_STABLE { પાછા "getc"; }
    string emitVerilog() override { પાછા "%k(%l.getc(%r))"; }
    string emitC() override { પાછા "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstGetcRefN final : જાહેર Ast_ગાંઠ_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
જાહેર:
    AstGetcRefN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstGt final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstGt(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f> %r)"; }
    string emitC() override { પાછા "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { પાછા ">"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstGtD final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstGtD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા ">"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstGtN final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstGtN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા ">"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
    bool stringFlavor() અચલ override { પાછા true; }
};
class AstGtS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstGtS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f> %r)"; }
    string emitC() override { પાછા "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstGte final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstGte(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f>= %r)"; }
    string emitC() override { પાછા "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { પાછા ">="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstGteD final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstGteD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા ">="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstGteN final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstGteN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા ">="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
    bool stringFlavor() અચલ override { પાછા true; }
};
class AstGteS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstGteS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f>= %r)"; }
    string emitC() override { પાછા "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstLogAnd final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstLogAnd(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f&& %r)"; }
    string emitC() override { પાછા "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvand %l %r)"; }
    string emitSimpleOperator() override { પાછા "&&"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() + INSTR_ગણતરી_BRANCH; }
};
class AstLogIf final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstLogIf(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f-> %r)"; }
    string emitC() override { પાછા "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { પાછા "->"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() + INSTR_ગણતરી_BRANCH; }
};
class AstLogOr final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstLogOr(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f|| %r)"; }
    string emitC() override { પાછા "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvor %l %r)"; }
    string emitSimpleOperator() override { પાછા "||"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() + INSTR_ગણતરી_BRANCH; }
};
class AstLt final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstLt(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f< %r)"; }
    string emitC() override { પાછા "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { પાછા "<"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstLtD final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstLtD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "<"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstLtN final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstLtN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "<"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
    bool stringFlavor() અચલ override { પાછા true; }
};
class AstLtS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstLtS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f< %r)"; }
    string emitC() override { પાછા "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstLte final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstLte(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f<= %r)"; }
    string emitC() override { પાછા "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { પાછા "<="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstLteD final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstLteD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "<="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstLteN final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstLteN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "<="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
    bool stringFlavor() અચલ override { પાછા true; }
};
class AstLteS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstLteS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f<= %r)"; }
    string emitC() override { પાછા "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstModDiv final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstModDiv(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f%% %r)"; }
    string emitC() override { પાછા "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvurem %l %r)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_DIV; }
};
class AstModDivS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstModDivS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f%% %r)"; }
    string emitC() override { પાછા "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvsmod %l %r)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_DIV; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstNeqWild final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstNeqWild(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f!=? %r)"; }
    string emitC() override {
        જો (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } નહીંતર {
            પાછા "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        જો (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } નહીંતર {
            પાછા "!=";
        }
    }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstPow final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstPow(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f** %r)"; }
    string emitC() override { પાછા "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_MUL * 10; }
};
class AstPowD final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstPowD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstPowD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f** %r)"; }
    string emitC() override { પાછા "pow(%li,%ri)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL_DIV * 5; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstPowSS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstPowSS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f** %r)"; }
    string emitC() override { પાછા "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_MUL * 10; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstPowSU final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstPowSU(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f** %r)"; }
    string emitC() override { પાછા "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_MUL * 10; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstPowUS final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstPowUS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f** %r)"; }
    string emitC() override { પાછા "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_MUL * 10; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstReplicate final : જાહેર Ast_ગાંઠ_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
જાહેર:
    AstReplicate(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        જો (lhsp) {
            જો (અચલ AstConst* અચલ constp = VN_CAST(rhsp, Const)) {
                જો (constp->અંક().isFourState()
                    || (constp->dtypep()->isSigned() && constp->અંક().isNegative())) {
                    dtypeSetBitOrLogicSized(lhsp->width(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } નહીંતર {
                    dtypeSetBitOrLogicSized(lhsp->width() * constp->toSInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, uint32_t rep_ગણતરી)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_ગણતરી}} {}
    ASTGEN_MEMBERS_AstReplicate;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%f{%r{%k%l}}"; }
    string emitC() override { પાછા "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override {
        પાછા "((_ repeat " + cvtToStr(width() / lhsp()->width()) + ") %l)";
    }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 2; }
};
class AstReplicateN final : જાહેર Ast_ગાંઠ_Biop {
    // String replicate
જાહેર:
    AstReplicateN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%f{%r{%k%l}}"; }
    string emitC() override { પાછા "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 2; }
    bool stringFlavor() અચલ override { પાછા true; }
};
class AstSAnd final : જાહેર Ast_ગાંઠ_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    અચલ bool m_propertyControl;  // Parser-generated property if/case branch conjunction
જાહેર:
    AstSAnd(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() + INSTR_ગણતરી_BRANCH; }
    bool isMultiCycleSva() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {  // LCOV_EXCL_LINE
        પાછા m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() અચલ { પાછા m_propertyControl; }
};
class AstSIntersect final : જાહેર Ast_ગાંઠ_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
જાહેર:
    AstSIntersect(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() + INSTR_ગણતરી_BRANCH; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstSOr final : જાહેર Ast_ગાંઠ_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
જાહેર:
    AstSOr(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSOr;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() + INSTR_ગણતરી_BRANCH; }
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstSThroughout final : જાહેર Ast_ગાંઠ_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
જાહેર:
    AstSThroughout(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstSWithin final : જાહેર Ast_ગાંઠ_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
જાહેર:
    AstSWithin(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() અચલ override { પાછા true; }
};
class AstSel final : જાહેર Ast_ગાંઠ_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    પૂર્ણાંક m_declElWidth;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    પૂર્ણાંક m_widthConst;

જાહેર:
    AstSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* lsbp, પૂર્ણાંક bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicSized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, પૂર્ણાંક lsb, પૂર્ણાંક bitwidth, પૂર્ણાંક bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicUnsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& from, અચલ V3_સંખ્યા& bit) override {
        out.opSel(from, bit.toUInt() + widthConst() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        પાછા widthConst() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() અચલ override { પાછા "((_ extract %t %r) %l)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* otherp) અચલ override {
        પાછા widthConst() == VN_DBG_AS(otherp, Sel)->widthConst();
    }
    પૂર્ણાંક instr_ગણતરી() અચલ override {
        પાછા widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    પૂર્ણાંક widthConst() અચલ { પાછા m_widthConst; }
    શૂન્ય widthConst(પૂર્ણાંક કિંમત) { m_widthConst = કિંમત; }
    પૂર્ણાંક lsbConst() અચલ { પાછા VN_AS(lsbp(), Const)->toSInt(); }
    પૂર્ણાંક msbConst() અચલ { પાછા lsbConst() + widthConst() - 1; }
    VNumRange& declRange() VL_MT_STABLE { પાછા m_declRange; }
    અચલ VNumRange& declRange() અચલ VL_MT_STABLE { પાછા m_declRange; }
    શૂન્ય declRange(અચલ VNumRange& flag) { m_declRange = flag; }
    પૂર્ણાંક declElWidth() અચલ { પાછા m_declElWidth; }
    શૂન્ય declElWidth(પૂર્ણાંક flag) { m_declElWidth = flag; }
};
class AstShiftL final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstShiftL(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, પૂર્ણાંક setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        જો (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } નહીંતર જો (અચલ Ast_ગાંઠ_DType* અચલ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftL;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f<< %r)"; }
    string emitC() override { પાછા "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        પાછા (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstShiftLOvr final : જાહેર Ast_ગાંઠ_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
જાહેર:
    AstShiftLOvr(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, પૂર્ણાંક setwidth = 0)
        : ASTGEN_SUPER_ShiftLOvr(fl, lhsp, rhsp) {
        જો (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } નહીંતર {
            અચલ Ast_ગાંઠ_DType* અચલ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftLOvr;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f<< %r)"; }
    string emitC() override { પાછા "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstShiftR final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstShiftR(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, પૂર્ણાંક setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        જો (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } નહીંતર જો (અચલ Ast_ગાંઠ_DType* અચલ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftR;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f>> %r)"; }
    string emitC() override { પાછા "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        પાછા (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstShiftROvr final : જાહેર Ast_ગાંઠ_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
જાહેર:
    AstShiftROvr(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, પૂર્ણાંક setwidth = 0)
        : ASTGEN_SUPER_ShiftROvr(fl, lhsp, rhsp) {
        જો (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } નહીંતર {
            અચલ Ast_ગાંઠ_DType* અચલ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftROvr;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f>> %r)"; }
    string emitC() override { પાછા "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstShiftRS final : જાહેર Ast_ગાંઠ_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
જાહેર:
    AstShiftRS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, પૂર્ણાંક setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        જો (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } નહીંતર જો (અચલ Ast_ગાંઠ_DType* અચલ dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { પાછા "%k(%l %f>>> %r)"; }
    string emitSMT() અચલ override { પાછા "(bvashr %l %r)"; }
    string emitC() override { પાછા "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstShiftRSOvr final : જાહેર Ast_ગાંઠ_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
જાહેર:
    AstShiftRSOvr(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, પૂર્ણાંક setwidth = 0)
        : ASTGEN_SUPER_ShiftRSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        જો (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } નહીંતર {
            અચલ Ast_ગાંઠ_DType* અચલ dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRSOvr;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { પાછા "%k(%l %f>>> %r)"; }
    string emitC() override { પાછા "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstSub final : જાહેર Ast_ગાંઠ_Biop {
    // @astgen makeDfgVertex
જાહેર:
    AstSub(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f- %r)"; }
    string emitC() override { પાછા "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvsub %l %r)"; }
    string emitSimpleOperator() override { પાછા "-"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
};
class AstSubD final : જાહેર Ast_ગાંઠ_Biop {
જાહેર:
    AstSubD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstSubD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "-"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstURandomRange final : જાહેર Ast_ગાંઠ_Biop {
    // $urandom_range
જાહેર:
    AstURandomRange(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%f$urandom_range(%l, %r)"; }
    string emitC() override { પાછા "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : જાહેર Ast_ગાંઠ_Bi_Com {
    // @astgen makeDfgVertex
જાહેર:
    AstEq(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    સ્થિર Ast_ગાંઠ_Biop* newTyped(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp);
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f== %r)"; }
    string emitC() override { પાછા "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { પાછા "=="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstEqCase final : જાહેર Ast_ગાંઠ_Bi_Com {
    // @astgen makeDfgVertex
જાહેર:
    AstEqCase(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f=== %r)"; }
    string emitC() override { પાછા "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { પાછા "=="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstEqD final : જાહેર Ast_ગાંઠ_Bi_Com {
જાહેર:
    AstEqD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "=="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstEqN final : જાહેર Ast_ગાંઠ_Bi_Com {
જાહેર:
    AstEqN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "=="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
    bool stringFlavor() અચલ override { પાછા true; }
};
class AstEqT final : જાહેર Ast_ગાંઠ_Bi_Com {
    // Equal (==) for data types
જાહેર:
    AstEqT(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqT;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "=="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
};
class AstLogEq final : જાહેર Ast_ગાંઠ_Bi_Com {
    // @astgen makeDfgVertex
જાહેર:
    AstLogEq(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f<-> %r)"; }
    string emitC() override { પાછા "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { પાછા "<->"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() + INSTR_ગણતરી_BRANCH; }
};
class AstNeq final : જાહેર Ast_ગાંઠ_Bi_Com {
    // @astgen makeDfgVertex
જાહેર:
    AstNeq(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    સ્થિર Ast_ગાંઠ_Biop* newTyped(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp);
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f!= %r)"; }
    string emitC() override { પાછા "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { પાછા "!="; }
    string emitSMT() અચલ override { પાછા "(__Vbv (not (= %l %r)))"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstNeqCase final : જાહેર Ast_ગાંઠ_Bi_Com {
    // @astgen makeDfgVertex
જાહેર:
    AstNeqCase(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f!== %r)"; }
    string emitC() override { પાછા "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { પાછા "!="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
};
class AstNeqD final : જાહેર Ast_ગાંઠ_Bi_Com {
જાહેર:
    AstNeqD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "!="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstNeqN final : જાહેર Ast_ગાંઠ_Bi_Com {
જાહેર:
    AstNeqN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "!="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
    bool stringFlavor() અચલ override { પાછા true; }
};
class AstNeqT final : જાહેર Ast_ગાંઠ_Bi_Com {
    // Not-equal (!=) for data types
જાહેર:
    AstNeqT(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "!="; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_STR; }
};

// === AstNodeBiComAsv ===
class AstAdd final : જાહેર Ast_ગાંઠ_Bi_Com_Asv {
    // @astgen makeDfgVertex
જાહેર:
    AstAdd(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAdd;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f+ %r)"; }
    string emitC() override { પાછા "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvadd %l %r)"; }
    string emitSimpleOperator() override { પાછા "+"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
};
class AstAddD final : જાહેર Ast_ગાંઠ_Bi_Com_Asv {
જાહેર:
    AstAddD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstAddD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "+"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstAnd final : જાહેર Ast_ગાંઠ_Bi_Com_Asv {
    // @astgen makeDfgVertex
જાહેર:
    AstAnd(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f& %r)"; }
    string emitC() override { પાછા "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvand %l %r)"; }
    string emitSimpleOperator() override { પાછા "&"; }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    અચલ અક્ષર* widthMismatch() અચલ override VL_MT_STABLE;
};
class AstMul final : જાહેર Ast_ગાંઠ_Bi_Com_Asv {
    // @astgen makeDfgVertex
જાહેર:
    AstMul(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f* %r)"; }
    string emitC() override { પાછા "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvmul %l %r)"; }
    string emitSimpleOperator() override { પાછા "*"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_MUL; }
};
class AstMulD final : જાહેર Ast_ગાંઠ_Bi_Com_Asv {
જાહેર:
    AstMulD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstMulD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "*"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstMulS final : જાહેર Ast_ગાંઠ_Bi_Com_Asv {
    // @astgen makeDfgVertex
જાહેર:
    AstMulS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f* %r)"; }
    string emitC() override { પાછા "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvmul %l %r)"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    bool sizeMattersRhs() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * INSTR_ગણતરી_INT_MUL; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstOr final : જાહેર Ast_ગાંઠ_Bi_Com_Asv {
    // @astgen makeDfgVertex
જાહેર:
    AstOr(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f| %r)"; }
    string emitC() override { પાછા "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvor %l %r)"; }
    string emitSimpleOperator() override { પાછા "|"; }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    અચલ અક્ષર* widthMismatch() અચલ override VL_MT_STABLE;
};
class AstXor final : જાહેર Ast_ગાંઠ_Bi_Com_Asv {
    // @astgen makeDfgVertex
જાહેર:
    AstXor(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { પાછા "%k(%l %f^ %r)"; }
    string emitC() override { પાછા "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() અચલ override { પાછા "(bvxor %l %r)"; }
    string emitSimpleOperator() override { પાછા "^"; }
    bool cleanOut() અચલ override { પાછા false; }  // Lclean && Rclean
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    અચલ અક્ષર* widthMismatch() અચલ override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class AstDistChiSquare final : જાહેર Ast_ગાંઠ_Dist_Biop {
જાહેર:
    AstDistChiSquare(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_DistChiSquare(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistChiSquare;
    string emitVerilog() override { પાછા "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { પાછા "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class AstDistExponential final : જાહેર Ast_ગાંઠ_Dist_Biop {
જાહેર:
    AstDistExponential(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_DistExponential(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistExponential;
    string emitVerilog() override { પાછા "%f$dist_exponential(%l, %r)"; }
    string emitC() override { પાછા "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class AstDistPoisson final : જાહેર Ast_ગાંઠ_Dist_Biop {
જાહેર:
    AstDistPoisson(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_DistPoisson(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistPoisson;
    string emitVerilog() override { પાછા "%f$dist_poisson(%l, %r)"; }
    string emitC() override { પાછા "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : જાહેર Ast_ગાંઠ_Dist_Biop {
જાહેર:
    AstDistT(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { પાછા "%f$dist_t(%l, %r)"; }
    string emitC() override { પાછા "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : જાહેર Ast_ગાંઠ_Sel {
    // @astgen makeDfgVertex
    શૂન્ય init(અચલ Ast_ગાંઠ* fromp) {
        જો (fromp && VN_IS(fromp->dtypep()->skipRefp(), ગાંઠ_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), ગાંઠ_Array_DType)->subDTypep());
        }
    }

જાહેર:
    AstArraySel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, પૂર્ણાંક bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() અચલ override { પાછા "(select %l %r)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isGateOptimizable() અચલ override { પાછા true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
};
class AstAssocSel final : જાહેર Ast_ગાંઠ_Sel {
    શૂન્ય init(અચલ Ast_ગાંઠ* fromp) {
        જો (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

જાહેર:
    AstAssocSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%k(%l%f[%r])"; }
    string emitC() override { પાછા "%li%k[%ri]"; }
    string emitSMT() અચલ override { પાછા "(select %l %r)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isGateOptimizable() અચલ override {
        પાછા !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા !isLValue(); }  // AssocSel creates on miss
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
};
class AstWildcardSel final : જાહેર Ast_ગાંઠ_Sel {
    શૂન્ય init(અચલ Ast_ગાંઠ* fromp) {
        જો (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

જાહેર:
    AstWildcardSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%k(%l%f[%r])"; }
    string emitC() override { પાછા "%li%k[%ri]"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool isGateOptimizable() અચલ override { પાછા true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
};
class AstWordSel final : જાહેર Ast_ગાંઠ_Sel {
    // Select a single word from a multi-word wide value
જાહેર:
    AstWordSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા&, અચલ V3_સંખ્યા&, અચલ V3_સંખ્યા&) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};

// === AstNodeStream ===
class AstStreamL final : જાહેર Ast_ગાંઠ_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
જાહેર:
    AstStreamL(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamL;
    string emitVerilog() override { પાછા "%f{ << %r %k{%l} }"; }
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { પાછા "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 2; }
};
class AstStreamR final : જાહેર Ast_ગાંઠ_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
જાહેર:
    AstStreamR(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamR;
    string emitVerilog() override { પાછા "%f{ >> %r %k{%l} }"; }
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { પાછા isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : જાહેર Ast_ગાંઠ_System_Biop_D {
જાહેર:
    AstAtan2D(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$atan2(%l,%r)"; }
    string emitC() override { પાછા "atan2(%li,%ri)"; }
};
class AstHypotD final : જાહેર Ast_ગાંઠ_System_Biop_D {
જાહેર:
    AstHypotD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$hypot(%l,%r)"; }
    string emitC() override { પાછા "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : જાહેર Ast_ગાંઠ_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
જાહેર:
    AstCCall(File_લીટી* fl, AstCFunc* funcp, Ast_ગાંઠ_પદાવલિ* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    અચલ VSelfPointerText& selfPointer() અચલ { પાછા m_selfPointer; }
    શૂન્ય selfPointer(અચલ VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) અચલ {
        પાછા selfPointer().protect(useSelfForThis, protect());
    }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
};
class AstCMethodCall final : જાહેર Ast_ગાંઠ_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
જાહેર:
    AstCMethodCall(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, AstCFunc* funcp, Ast_ગાંઠ_પદાવલિ* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : જાહેર Ast_ગાંઠ_CCall {
    // C++ new() call
જાહેર:
    AstCNew(File_લીટી* fl, AstCFunc* funcp, Ast_ગાંઠ_પદાવલિ* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : જાહેર Ast_ગાંઠ_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

જાહેર:
    AstConsArray(File_લીટી* fl, Ast_ગાંઠ_DType* newDTypep, Ast_ગાંઠ_પદાવલિ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    અચલ અક્ષર* broken() અચલ override;
    શૂન્ય cloneRelink() override;
    અંતઃરેખ AstConsArrayItem* itemsp() અચલ VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_ગાંઠ_પદાવલિ* valuep(uint64_t index) અચલ;
    // Add the value of the element at the index, which must not be given yet
    શૂન્ય valuep(uint64_t index, Ast_ગાંઠ_પદાવલિ* newp);
    // Add the elements of the array 'arrayp', starting at the index
    શૂન્ય splice_કિંમતો(uint64_t index, Ast_ગાંઠ_પદાવલિ* arrayp);
};
class AstConsAssoc final : જાહેર Ast_ગાંઠ_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
જાહેર:
    explicit AstConsAssoc(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        પાછા nullptr;
    }
};
class AstConsDynArray final : જાહેર Ast_ગાંઠ_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
જાહેર:
    explicit AstConsDynArray(File_લીટી* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    અચલ અક્ષર* broken() અચલ override;
};
class AstConsQueue final : જાહેર Ast_ગાંઠ_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
જાહેર:
    explicit AstConsQueue(File_લીટી* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        પાછા nullptr;
    }
};
class AstConsStruct final : જાહેર Ast_ગાંઠ_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
જાહેર:
    AstConsStruct(File_લીટી* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    અચલ અક્ષર* broken() અચલ override;
    // The value of the member called 'name'
    Ast_ગાંઠ_પદાવલિ* memberValuep(અચલ string& name) અચલ;
};
class AstConsWildcard final : જાહેર Ast_ગાંઠ_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
જાહેર:
    explicit AstConsWildcard(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        પાછા nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : જાહેર Ast_ગાંઠ_FTask_Ref {
    // A reference to a function
જાહેર:
    અંતઃરેખ AstFuncRef(File_લીટી* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_લીટી* fl, અચલ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class AstMethodCall final : જાહેર Ast_ગાંઠ_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
જાહેર:
    AstMethodCall(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, VFlagChildDType, અચલ string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMethodCall(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, અચલ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_AstMethodCall;
};
class AstNew final : જાહેર Ast_ગાંઠ_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
જાહેર:
    AstNew(File_લીટી* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool isImplicit() અચલ { પાછા m_isImplicit; }
    શૂન્ય isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() અચલ { પાછા m_isScoped; }
    શૂન્ય isScoped(bool flag) { m_isScoped = flag; }
    bool isPure() override { પાછા false; }
};
class AstTaskRef final : જાહેર Ast_ગાંઠ_FTask_Ref {
    // A reference to a task
જાહેર:
    અંતઃરેખ AstTaskRef(File_લીટી* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_લીટી* fl, અચલ string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtypeSetVoid();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : જાહેર Ast_ગાંઠ_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
ખાનગી:
    VAccess m_access;  // Left hand side assignment
જાહેર:
    AstSelBit(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    VAccess access() અચલ { પાછા m_access; }
    શૂન્ય access(અચલ VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : જાહેર Ast_ગાંઠ_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
જાહેર:
    AstSelExtract(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* msbp, Ast_ગાંઠ_પદાવલિ* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : જાહેર Ast_ગાંઠ_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
જાહેર:
    AstSelMinus(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* bitp, Ast_ગાંઠ_પદાવલિ* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : જાહેર Ast_ગાંઠ_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
જાહેર:
    AstSelPlus(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* bitp, Ast_ગાંઠ_પદાવલિ* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_ગણતરી_Bits final : જાહેર Ast_ગાંઠ_Quadop {
    // Number of bits set in vector
જાહેર:
    Ast_ગણતરી_Bits(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* ctrl1p)
        : ASTGEN_SUPER_ગણતરી_Bits(fl, exprp, ctrl1p, ctrl1p->clone_વૃક્ષ_Pure(false),
                                 ctrl1p->clone_વૃક્ષ_Pure(false)) {
        dtypeSetInt();
    }
    Ast_ગણતરી_Bits(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* ctrl1p, Ast_ગાંઠ_પદાવલિ* ctrl2p)
        : ASTGEN_SUPER_ગણતરી_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_વૃક્ષ_Pure(false)) {
        dtypeSetInt();
    }
    Ast_ગણતરી_Bits(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* ctrl1p, Ast_ગાંઠ_પદાવલિ* ctrl2p,
                 Ast_ગાંઠ_પદાવલિ* ctrl3p)
        : ASTGEN_SUPER_ગણતરી_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_ગણતરી_Bits;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& પદાવલિ, અચલ V3_સંખ્યા& ctrl1,
                       અચલ V3_સંખ્યા& ctrl2, અચલ V3_સંખ્યા& ctrl3) override {
        out.op_ગણતરી_Bits(પદાવલિ, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { પાછા "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool cleanThs() અચલ override { પાછા true; }
    bool cleanFhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool sizeMattersThs() અચલ override { પાછા false; }
    bool sizeMattersFhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : જાહેર Ast_ગાંઠ_Termop {
જાહેર:
    explicit AstInferredDisable(File_લીટી* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { પાછા "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool isSystemFunc() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstTime final : જાહેર Ast_ગાંઠ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
જાહેર:
    AstTime(File_લીટી* fl, અચલ VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { પાછા "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_TIME; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : જાહેર Ast_ગાંઠ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
જાહેર:
    AstTimeD(File_લીટી* fl, અચલ VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { પાછા "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_TIME; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : જાહેર Ast_ગાંઠ_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
જાહેર:
    AstCond(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ_પદાવલિ* thenp, Ast_ગાંઠ_પદાવલિ* elsep);
    ASTGEN_MEMBERS_AstCond;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs,
                       અચલ V3_સંખ્યા& ths) override {
        out.opAssign(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { પાછા "%k(%l %f? %r %k: %t)"; }
    string emitC() override { પાછા "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() અચલ override { પાછા "(ite (__Vbool %l) %r %t)"; }
    bool cleanOut() અચલ override { પાછા false; }  // clean if e1 & e2 clean
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા false; }
    bool cleanThs() અચલ override { પાછા false; }  // Propagates up
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool sizeMattersThs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_BRANCH; }
};
class AstPutcN final : જાહેર Ast_ગાંઠ_Triop {
    // Verilog string.putc()
જાહેર:
    AstPutcN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ_પદાવલિ* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstPutcN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs,
                       અચલ V3_સંખ્યા& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() અચલ override VL_MT_STABLE { પાછા "putc"; }
    string emitVerilog() override { પાછા "%k(%l.putc(%r,%t))"; }
    string emitC() override { પાછા "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool cleanThs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool sizeMattersThs() અચલ override { પાછા false; }
};
class AstSliceSel final : જાહેર Ast_ગાંઠ_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
જાહેર:
    AstSliceSel(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* fromp, અચલ VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_AstSliceSel;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& from, અચલ V3_સંખ્યા& lo,
                       અચલ V3_સંખ્યા& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool cleanThs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool sizeMattersThs() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { પાછા m_declRange; }
    અચલ VNumRange& declRange() અચલ VL_MT_STABLE { પાછા m_declRange; }
};
class AstSubstrN final : જાહેર Ast_ગાંઠ_Triop {
    // Verilog string.substr()
જાહેર:
    AstSubstrN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ_પદાવલિ* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs,
                       અચલ V3_સંખ્યા& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() અચલ override VL_MT_STABLE { પાછા "substr"; }
    string emitVerilog() override { પાછા "%k(%l.substr(%r,%t))"; }
    string emitC() override { પાછા "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { પાછા ""; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool cleanRhs() અચલ override { પાછા true; }
    bool cleanThs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool sizeMattersRhs() અચલ override { પાછા false; }
    bool sizeMattersThs() અચલ override { પાછા false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : જાહેર Ast_ગાંઠ_Dist_Triop {
જાહેર:
    AstDistErlang(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ_પદાવલિ* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { પાછા "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { પાછા "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class AstDistNormal final : જાહેર Ast_ગાંઠ_Dist_Triop {
જાહેર:
    AstDistNormal(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ_પદાવલિ* thsp)
        : ASTGEN_SUPER_DistNormal(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistNormal;
    string emitVerilog() override { પાછા "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { પાછા "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class AstDistUniform final : જાહેર Ast_ગાંઠ_Dist_Triop {
જાહેર:
    AstDistUniform(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp, Ast_ગાંઠ_પદાવલિ* thsp)
        : ASTGEN_SUPER_DistUniform(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistUniform;
    string emitVerilog() override { પાછા "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { પાછા "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : જાહેર Ast_ગાંઠ_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
જાહેર:
    ગણના FmtType : પૂર્ણાંક { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

ખાનગી:
    // dist-ast-dump-suppress  // Part of name()
    અચલ FmtType m_fmtType;  // Operation type
જાહેર:
    AstAtoN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtypeSetDouble() : dtypeSetInteger();
    }
    ASTGEN_MEMBERS_AstAtoN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opAtoN(lhs, m_fmtType); }
    string name() અચલ override VL_MT_STABLE {
        પસંદગી (m_fmtType) {
        સ્થિતિ ATOI: પાછા "atoi";
        સ્થિતિ ATOHEX: પાછા "atohex";
        સ્થિતિ ATOOCT: પાછા "atooct";
        સ્થિતિ ATOBIN: પાછા "atobin";
        સ્થિતિ ATOREAL: પાછા "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { પાછા "%l." + name() + "()"; }
    string emitC() override {
        પસંદગી (m_fmtType) {
        સ્થિતિ ATOI: પાછા "VL_ATOI_N(%li, 10)";
        સ્થિતિ ATOHEX: પાછા "VL_ATOI_N(%li, 16)";
        સ્થિતિ ATOOCT: પાછા "VL_ATOI_N(%li, 8)";
        સ્થિતિ ATOBIN: પાછા "VL_ATOI_N(%li, 2)";
        સ્થિતિ ATOREAL: પાછા "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    FmtType fmtType() અચલ { પાછા m_fmtType; }
};
class AstBitsToRealD final : જાહેર Ast_ગાંઠ_Uniop {
જાહેર:
    AstBitsToRealD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstBitsToRealD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { પાછા "%f$bitstoreal(%l)"; }
    string emitC() override { પાછા "VL_CVT_D_Q(%li)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }  // Eliminated before matters
    bool sizeMattersLhs() અચલ override { પાછા false; }  // Eliminated before matters
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstCCast final : જાહેર Ast_ગાંઠ_Uniop {
    // Cast to C-based data type
    પૂર્ણાંક m_size;

જાહેર:
    AstCCast(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, પૂર્ણાંક setwidth, પૂર્ણાંક minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_size = setwidth;
        જો (setwidth) {
            જો (minwidth == -1) minwidth = setwidth;
            dtypeSetBitUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_size = width();
    }
    ASTGEN_MEMBERS_AstCCast;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { પાછા "%f$_CAST(%l)"; }
    string emitC() override { પાછા "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }  // Special cased in V3Cast
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા size() == VN_DBG_AS(samep, CCast)->size();
    }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    //
    પૂર્ણાંક size() અચલ { પાછા m_size; }
};
class AstCLog2 final : જાહેર Ast_ગાંઠ_Uniop {
જાહેર:
    AstCLog2(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { પાછા "%f$clog2(%l)"; }
    string emitC() override { પાછા "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 16; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstCastWrap final : જાહેર Ast_ગાંઠ_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
જાહેર:
    AstCastWrap(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { પાછા "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
};
class Ast_ગણતરી_Ones final : જાહેર Ast_ગાંઠ_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
જાહેર:
    Ast_ગણતરી_Ones(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_ગણતરી_Ones(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_ગણતરી_Ones;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.op_ગણતરી_Ones(lhs); }
    string emitVerilog() override { પાછા "%f$countones(%l)"; }
    string emitC() override { પાછા "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 16; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstCvtPackString final : જાહેર Ast_ગાંઠ_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
જાહેર:
    AstCvtPackString(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstExtend final : જાહેર Ast_ગાંઠ_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
જાહેર:
    AstExtend(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, પૂર્ણાંક width, પૂર્ણાંક widthMin = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetBitOrLogicUnsized(width, widthMin ? widthMin : width, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { પાછા "%l"; }
    string emitC() override { પાછા "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() અચલ override {
        પાછા "((_ zero_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override {
        પાછા false;  // Because the EXTEND operator self-casts
    }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
};
class AstExtendS final : જાહેર Ast_ગાંઠ_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
જાહેર:
    AstExtendS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, પૂર્ણાંક width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { પાછા "%l"; }
    string emitC() override { પાછા "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() અચલ override {
        પાછા "((_ sign_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override {
        પાછા false;  // Because the EXTEND operator self-casts
    }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool signedFlavor() અચલ override { પાછા true; }
};
class AstFEof final : જાહેર Ast_ગાંઠ_Uniop {
જાહેર:
    AstFEof(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%f$feof(%l)"; }
    string emitC() override { પાછા "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 16; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() અચલ override { પાછા true; }
    Ast_ગાંઠ* filep() અચલ { પાછા lhsp(); }
};
class AstFGetC final : જાહેર Ast_ગાંઠ_Uniop {
જાહેર:
    AstFGetC(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { પાછા "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 64; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ગાંઠ* filep() અચલ { પાછા lhsp(); }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstISToRD final : જાહેર Ast_ગાંઠ_Uniop {
    // $itor where lhs is signed
જાહેર:
    AstISToRD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstISToRD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { પાછા "%f$itor($signed(%l))"; }
    string emitC() override { પાછા "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_શબ્દો() override { પાછા true; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstIToRD final : જાહેર Ast_ગાંઠ_Uniop {
    // $itor where lhs is unsigned
જાહેર:
    AstIToRD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstIToRD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { પાછા "%f$itor(%l)"; }
    string emitC() override { પાછા "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstIsUnbounded final : જાહેર Ast_ગાંઠ_Uniop {
    // True if is unbounded ($)
જાહેર:
    AstIsUnbounded(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { પાછા "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstIsUnknown final : જાહેર Ast_ગાંઠ_Uniop {
    // True if any unknown bits
જાહેર:
    AstIsUnknown(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { પાછા "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstLenN final : જાહેર Ast_ગાંઠ_Uniop {
    // Length of a string
જાહેર:
    AstLenN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstLenN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { પાછા "%f(%l)"; }
    string emitC() override { પાછા "VL_LEN_IN(%li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
};
class AstLogNot final : જાહેર Ast_ગાંઠ_Uniop {
    // @astgen makeDfgVertex
ખાનગી:
    અચલ bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
જાહેર:
    AstLogNot(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { પાછા "%f(! %l)"; }
    string emitC() override { પાછા "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() અચલ override { પાછા "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { પાછા "!"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool fromProperty() અચલ { પાછા m_fromProperty; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstMostSetBitP1 final : જાહેર Ast_ગાંઠ_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
જાહેર:
    AstMostSetBitP1(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_MostSetBitP1(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstMostSetBitP1;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opMostSetBitP1(lhs); }
    string emitVerilog() override { પાછા "%f$mostsetbitp1(%l)"; }
    string emitC() override { પાછા "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 16; }
};
class AstNToI final : જાહેર Ast_ગાંઠ_Uniop {
    // String to any-size integral
જાહેર:
    AstNToI(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opNToI(lhs); }
    string emitVerilog() override { પાછા "'(%l)"; }
    string emitC() override { પાછા "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
};
class AstNegate final : જાહેર Ast_ગાંઠ_Uniop {
    // @astgen makeDfgVertex
જાહેર:
    AstNegate(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { પાછા "%f(- %l)"; }
    string emitC() override { પાછા "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() અચલ override { પાછા "(bvneg %l)"; }
    string emitSimpleOperator() override { પાછા "-"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
};
class AstNegateD final : જાહેર Ast_ગાંઠ_Uniop {
જાહેર:
    AstNegateD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstNegateD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { પાછા "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { પાછા "-"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool doubleFlavor() અચલ override { પાછા true; }
};
class AstNot final : જાહેર Ast_ગાંઠ_Uniop {
    // @astgen makeDfgVertex
જાહેર:
    AstNot(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { પાછા "%f(~ %l)"; }
    string emitC() override { પાછા "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() અચલ override { પાછા "(bvnot %l)"; }
    string emitSimpleOperator() override { પાછા "~"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા true; }
    અચલ અક્ષર* widthMismatch() અચલ override VL_MT_STABLE;
};
class AstNullCheck final : જાહેર Ast_ગાંઠ_Uniop {
    // Return LHS after checking that LHS is non-null
જાહેર:
    AstNullCheck(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { V3ERROR_NA; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 1; }  // Rarely executes
    string emitVerilog() override { પાછા "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override { પાછા fileline() == samep->fileline(); }
};
class AstOneHot final : જાહેર Ast_ગાંઠ_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
જાહેર:
    AstOneHot(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { પાછા "%f$onehot(%l)"; }
    string emitC() override { પાછા "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 4; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstOneHot0 final : જાહેર Ast_ગાંઠ_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
જાહેર:
    AstOneHot0(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { પાછા "%f$onehot0(%l)"; }
    string emitC() override { પાછા "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs() * 3; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstPostDec final : જાહેર Ast_ગાંઠ_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
જાહેર:
    AstPostDec(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { પાછા "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() અચલ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : જાહેર Ast_ગાંઠ_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
જાહેર:
    AstPostInc(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { પાછા "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() અચલ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : જાહેર Ast_ગાંઠ_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
જાહેર:
    AstPreDec(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { પાછા "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() અચલ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : જાહેર Ast_ગાંઠ_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
જાહેર:
    AstPreInc(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { પાછા "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() અચલ override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() અચલ override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : જાહેર Ast_ગાંઠ_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
જાહેર:
    AstRToIRoundS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { પાછા "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        પાછા isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstRToIS final : જાહેર Ast_ગાંઠ_Uniop {
    // $rtoi(lhs)
જાહેર:
    AstRToIS(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { પાછા "%f$rtoi(%l)"; }
    string emitC() override { પાછા "VL_RTOI_I_D(%li)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }  // Eliminated before matters
    bool sizeMattersLhs() અચલ override { પાછા false; }  // Eliminated before matters
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstRealToBits final : જાહેર Ast_ગાંઠ_Uniop {
જાહેર:
    AstRealToBits(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstRealToBits;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { પાછા "%f$realtobits(%l)"; }
    string emitC() override { પાછા "VL_CVT_Q_D(%li)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }  // Eliminated before matters
    bool sizeMattersLhs() અચલ override { પાછા false; }  // Eliminated before matters
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_DBL; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstRedAnd final : જાહેર Ast_ગાંઠ_Uniop {
    // @astgen makeDfgVertex
જાહેર:
    AstRedAnd(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { પાછા "%f(& %l)"; }
    string emitC() override { પાછા "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
};
class AstRedOr final : જાહેર Ast_ગાંઠ_Uniop {
    // @astgen makeDfgVertex
જાહેર:
    AstRedOr(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { પાછા "%f(| %l)"; }
    string emitC() override { પાછા "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
};
class AstRedXor final : જાહેર Ast_ગાંઠ_Uniop {
    // @astgen makeDfgVertex
જાહેર:
    AstRedXor(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { પાછા "%f(^ %l)"; }
    string emitC() override { પાછા "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override {
        અચલ પૂર્ણાંક w = lhsp()->width();
        પાછા (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 1 + V3_સંખ્યા::log2b(width()); }
};
class AstResizeLValue final : જાહેર Ast_ગાંઠ_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
જાહેર:
    AstResizeLValue(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
};
class AstSigned final : જાહેર Ast_ગાંઠ_Uniop {
    // $signed(lhs)
જાહેર:
    AstSigned(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { પાછા "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }  // Eliminated before matters
    bool sizeMattersLhs() અચલ override { પાછા true; }  // Eliminated before matters
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool isSystemFunc() અચલ override { પાછા true; }
};
class AstTimeImport final : જાહેર Ast_ગાંઠ_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
જાહેર:
    AstTimeImport(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }
};
class AstToLowerN final : જાહેર Ast_ગાંઠ_Uniop {
    // string.tolower()
જાહેર:
    AstToLowerN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToLowerN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { પાછા "%l.tolower()"; }
    string emitC() override { પાછા "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
};
class AstToStringN final : જાહેર Ast_ગાંઠ_Uniop {
જાહેર:
    AstToStringN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_ToStringN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToStringN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { પાછા "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        પાછા isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
};
class AstToUpperN final : જાહેર Ast_ગાંઠ_Uniop {
    // string.toupper()
જાહેર:
    AstToUpperN(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToUpperN;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { પાછા "%l.toupper()"; }
    string emitC() override { પાછા "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() અચલ override { પાછા true; }
    bool cleanLhs() અચલ override { પાછા true; }
    bool sizeMattersLhs() અચલ override { પાછા false; }
};
class AstUnsigned final : જાહેર Ast_ગાંઠ_Uniop {
    // $unsigned(lhs)
જાહેર:
    AstUnsigned(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { પાછા "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા false; }
    bool cleanLhs() અચલ override { પાછા false; }  // Eliminated before matters
    bool sizeMattersLhs() અચલ override { પાછા true; }  // Eliminated before matters
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 0; }
    bool isSystemFunc() અચલ override { પાછા true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstAcosD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$acos(%l)"; }
    string emitC() override { પાછા "acos(%li)"; }
};
class AstAcoshD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstAcoshD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$acosh(%l)"; }
    string emitC() override { પાછા "acosh(%li)"; }
};
class AstAsinD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstAsinD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$asin(%l)"; }
    string emitC() override { પાછા "asin(%li)"; }
};
class AstAsinhD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstAsinhD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$asinh(%l)"; }
    string emitC() override { પાછા "asinh(%li)"; }
};
class AstAtanD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstAtanD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$atan(%l)"; }
    string emitC() override { પાછા "atan(%li)"; }
};
class AstAtanhD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstAtanhD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$atanh(%l)"; }
    string emitC() override { પાછા "atanh(%li)"; }
};
class AstCeilD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstCeilD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::ceil(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$ceil(%l)"; }
    string emitC() override { પાછા "ceil(%li)"; }
};
class AstCosD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstCosD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$cos(%l)"; }
    string emitC() override { પાછા "cos(%li)"; }
};
class AstCoshD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstCoshD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$cosh(%l)"; }
    string emitC() override { પાછા "cosh(%li)"; }
};
class AstExpD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstExpD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$exp(%l)"; }
    string emitC() override { પાછા "exp(%li)"; }
};
class AstFloorD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstFloorD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::floor(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$floor(%l)"; }
    string emitC() override { પાછા "floor(%li)"; }
};
class AstLog10D final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstLog10D(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$log10(%l)"; }
    string emitC() override { પાછા "log10(%li)"; }
};
class AstLogD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstLogD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$ln(%l)"; }
    string emitC() override { પાછા "log(%li)"; }
};
class AstSinD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstSinD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$sin(%l)"; }
    string emitC() override { પાછા "sin(%li)"; }
};
class AstSinhD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstSinhD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$sinh(%l)"; }
    string emitC() override { પાછા "sinh(%li)"; }
};
class AstSqrtD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstSqrtD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::sqrt(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$sqrt(%l)"; }
    string emitC() override { પાછા "sqrt(%li)"; }
};
class AstTanD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstTanD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$tan(%l)"; }
    string emitC() override { પાછા "tan(%li)"; }
};
class AstTanhD final : જાહેર Ast_ગાંઠ_System_Uniop_D {
જાહેર:
    AstTanhD(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    શૂન્ય સંખ્યા_Operate(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { પાછા "%f$tanh(%l)"; }
    string emitC() override { પાછા "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : જાહેર Ast_ગાંઠ_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
જાહેર:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    અંતઃરેખ AstVarRef(File_લીટી* fl, AstVar* varp, અચલ VAccess& access);
    અંતઃરેખ AstVarRef(File_લીટી* fl, Ast_ગાંઠ_Module* classOrPackagep, AstVar* varp,
                     અચલ VAccess& access);
    // This form only allowed post-link (see above)
    અંતઃરેખ AstVarRef(File_લીટી* fl, AstVarScope* varscp, અચલ VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() અચલ { પાછા m_name; }
    અંતઃરેખ string name() અચલ override;  // * = Var name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    અચલ અક્ષર* broken() અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override;
    અંતઃરેખ bool same_ગાંઠ(અચલ AstVarRef* samep) અચલ;
    અંતઃરેખ bool sameNoLvalue(અચલ AstVarRef* samep) અચલ;
    પૂર્ણાંક instr_ગણતરી() અચલ override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
};
class AstVarXRef final : જાહેર Ast_ગાંઠ_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
જાહેર:
    AstVarXRef(File_લીટી* fl, અચલ string& name, અચલ string& dotted, અચલ VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    અંતઃરેખ AstVarXRef(File_લીટી* fl, AstVar* varp, અચલ string& dotted, અચલ VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    શૂન્ય name(અચલ std::string& name) override { m_name = name; }  // * = Var name
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string dotted() અચલ { પાછા m_dotted; }
    શૂન્ય dotted(અચલ string& dotted) { m_dotted = dotted; }
    string inlinedDots() અચલ { પાછા m_inlinedDots; }
    શૂન્ય inlinedDots(અચલ string& flag) { m_inlinedDots = flag; }
    bool containsGenBlock() અચલ { પાછા m_containsGenBlock; }
    શૂન્ય containsGenBlock(અચલ bool flag) { m_containsGenBlock = flag; }
    bool readOnlyModport() અચલ { પાછા m_readOnlyModport; }
    શૂન્ય readOnlyModport(અચલ bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() અચલ override { પાછા true; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા widthInstrs(); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        પાછા (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#endif  // Guard
