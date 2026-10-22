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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ASTNODEEXPR_H_
#تعریف_کرو VERILATOR_V3ASTNODEEXPR_H_

#اگر_غیر_تعریف_شدہ VERILATOR_V3AST_H_
#غلطی "Use V3Ast.h as the include"
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کرو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#اگر_ختم

// === Abstract base node types (AstNode*) =====================================

class Ast_گرہ_اظہاریہ VL_NOT_FINAL : عوامی Ast_گرہ {
    // An expression tree node
محفوظ:
    Ast_گرہ_اظہاریہ(VNType t, File_سطر* fl)
        : Ast_گرہ{t, fl} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_اظہاریہ;
    // METHODS
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    // Every expression must have a data type after V3Width
    bool hasDType() ثابت override final VL_MT_محفوظ { واپس true; }
    مجازی string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    مجازی string emitC() = 0;
    مجازی string emitSMT() ثابت { واپس ""; };
    مجازی string emitSimpleOperator() { واپس ""; }  // "" means not ok to use
    مجازی bool emit_Check_Max_الفاظ() { واپس false; }  // Check VL_MULS_MAX_WORDS
    مجازی bool clean_برآمد() ثابت = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    مجازی bool is_Multi_چکر_Sva() ثابت { واپس false; }
    ثابت Ast_گرہ_اظہاریہ* get_VAccess_ہدف_Recurse() ثابت;
    Ast_گرہ_اظہاریہ* get_VAccess_ہدف_Recurse() {
        واپس const_cast<Ast_گرہ_اظہاریہ*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<ثابت Ast_گرہ_اظہاریہ*>(this)->get_VAccess_ہدف_Recurse());
    }
    VAccess getVAccessRecurse() ثابت;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() ثابت { واپس getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_درخت() ثابت;
    // Return base var (or const) nodep dereferences
    Ast_گرہ* بنیاد_Fromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    اندرونی Ast_Stmt_اظہاریہ* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    مجازی ثابت حرف* چوڑائی_Mismatch() ثابت VL_MT_STABLE { واپس nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string نمونہ_String(uint32_t گہرائی = 0) ثابت;
    bool isFourstateOrNoDtype() ثابت {
        اگر (!dtypep()) واپس true;
        واپس dtypep()->skipRefp()->isFourstate();
    }
    ساکن bool isFourstateOrNoDtype(ثابت Ast_گرہ_اظہاریہ* ثابت expr1p) {
        واپس expr1p->isFourstateOrNoDtype();
    }
    ساکن bool isFourstateOrNoDtype(ثابت Ast_گرہ_اظہاریہ* ثابت expr1p,
                                     ثابت Ast_گرہ_اظہاریہ* ثابت expr2p) {
        واپس expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    ساکن bool isFourstateOrNoDtype(ثابت Ast_گرہ_اظہاریہ* ثابت expr1p,
                                     ثابت Ast_گرہ_اظہاریہ* ثابت expr2p,
                                     ثابت Ast_گرہ_اظہاریہ* ثابت expr3p) {
        واپس isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    ساکن bool isFourstateOrNoDtype(ثابت Ast_گرہ_اظہاریہ* ثابت expr1p,
                                     ثابت Ast_گرہ_اظہاریہ* ثابت expr2p,
                                     ثابت Ast_گرہ_اظہاریہ* ثابت expr3p,
                                     ثابت Ast_گرہ_اظہاریہ* ثابت expr4p) {
        واپس isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_گرہ_Biop VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محفوظ:
    Ast_گرہ_Biop(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : Ast_گرہ_اظہاریہ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    مجازی خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) = 0;
    مجازی bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    مجازی bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    مجازی bool سائز_Matters_Lhs() ثابت = 0;  // True if output result depends on lhs size
    مجازی bool سائز_Matters_Rhs() ثابت = 0;  // True if output result depends on rhs size
    مجازی bool doubleFlavor() ثابت { واپس false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    مجازی bool signedFlavor() ثابت { واپس false; }
    مجازی bool stringFlavor() ثابت { واپس false; }  // N flavor of nodes with both flavors?
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;

نجی:
    bool getPurityRecurse() ثابت { واپس lhsp()->is_خالص() && rhsp()->is_خالص(); }
};
class Ast_گرہ_Bi_Com VL_NOT_FINAL : عوامی Ast_گرہ_Biop {
    // Binary expr with commutative properties
محفوظ:
    Ast_گرہ_Bi_Com(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhs, Ast_گرہ_اظہاریہ* rhs)
        : Ast_گرہ_Biop{t, fl, lhs, rhs} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Bi_Com;
};
class Ast_گرہ_Bi_Com_Asv VL_NOT_FINAL : عوامی Ast_گرہ_Bi_Com {
    // Binary expr with commutative & associative properties
محفوظ:
    Ast_گرہ_Bi_Com_Asv(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhs, Ast_گرہ_اظہاریہ* rhs)
        : Ast_گرہ_Bi_Com{t, fl, lhs, rhs} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Bi_Com_Asv;
};
class Ast_گرہ_Dist_Biop VL_NOT_FINAL : عوامی Ast_گرہ_Biop {
عوامی:
    Ast_گرہ_Dist_Biop(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : Ast_گرہ_Biop{t, fl, lhsp, rhsp} {
        dtype_سیٹ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_گرہ_Dist_Biop;
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL_TRIG; }
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
};
class Ast_گرہ_Sel VL_NOT_FINAL : عوامی Ast_گرہ_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
محفوظ:
    Ast_گرہ_Sel(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* bitp)
        : Ast_گرہ_Biop{t, fl, fromp, bitp} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Sel;
    صحیح ثنائی_ہندسہ_Const() ثابت;
};
class Ast_گرہ_دھارا VL_NOT_FINAL : عوامی Ast_گرہ_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
محفوظ:
    Ast_گرہ_دھارا(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : Ast_گرہ_Biop{t, fl, lhsp, rhsp} {
        اگر (lhsp->dtypep()) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(lhsp->چوڑائی(), lhsp->چوڑائی_Min(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_دھارا;
};
class Ast_گرہ_System_Biop_D VL_NOT_FINAL : عوامی Ast_گرہ_Biop {
عوامی:
    Ast_گرہ_System_Biop_D(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : Ast_گرہ_Biop{t, fl, lhsp, rhsp} {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_Ast_گرہ_System_Biop_D;
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL_TRIG; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class Ast_گرہ_CCall VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

محفوظ:
    Ast_گرہ_CCall(VNType t, File_سطر* fl, AstCFunc* funcp, Ast_گرہ_اظہاریہ* argsp = nullptr)
        : Ast_گرہ_اظہاریہ{t, fl}
        , m_funcp{funcp} {
        جوڑو_Argsp(argsp);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_CCall;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_CALL; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت Ast_گرہ_CCall* ثابت asamep = VN_DBG_AS(samep, گرہ_CCall);
        واپس (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override;
    bool isOutputter() override { واپس !is_خالص(); }
    AstCFunc* funcp() ثابت { واپس m_funcp; }
    خالی funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() ثابت { واپس m_argTypes; }
    خالی argTypes(ثابت string& str) { m_argTypes = str; }
    bool isUnlikely() ثابت override { واپس m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت final override { واپس true; }
    bool superReference() ثابت { واپس m_superReference; }
    خالی superReference(bool flag) { m_superReference = flag; }
};
class Ast_گرہ_Cons VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // Construct a value of an aggregate type
    دوست class Ast_گرہ_اظہاریہ;  // For m_knownConstantTree
    mutable bool m_معلوم_Constant_درخت = false;  // Known to be 'this->isConstantTree() == true'

محفوظ:
    Ast_گرہ_Cons(VNType t, File_سطر* fl)
        : Ast_گرہ_اظہاریہ{t, fl} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Cons;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت final override { واپس true; }
    صحیح instr_گنتی() ثابت final override { واپس چوڑائی_Instrs(); }
};
class Ast_گرہ_FTask_Ref VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_گرہ_FTask_Ref;  // Gen pointers before other members for performance
نجی:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_شامل_ہے_Gen_بلاک = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

محفوظ:
    Ast_گرہ_FTask_Ref(VNType t, File_سطر* fl, ثابت string& name, AstArg* argsp)
        : Ast_گرہ_اظہاریہ{t, fl}
        , m_name{name} {
        جوڑو_Argsp(argsp);
    }

عوامی:
    ثابت حرف* broken() ثابت override;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    bool isGateOptimizable() ثابت override;
    string dotted() ثابت { واپس m_dotted; }  // * = Scope name or ""
    string inlinedDots() ثابت { واپس m_inlinedDots; }
    خالی inlinedDots(ثابت string& flag) { m_inlinedDots = flag; }
    Ast_گرہ_FTask* taskp() ثابت { واپس m_taskp; }  // [After Link] Pointer to variable
    خالی taskp(Ast_گرہ_FTask* taskp) { m_taskp = taskp; }
    خالی name(ثابت string& name) override { m_name = name; }
    خالی dotted(ثابت string& name) { m_dotted = name; }
    Ast_گرہ_Module* classOrPackagep() ثابت { واپس m_classOrPackagep; }
    خالی classOrPackagep(Ast_گرہ_Module* nodep) { m_classOrPackagep = nodep; }
    bool شامل_ہے_Gen_بلاک() ثابت { واپس m_شامل_ہے_Gen_بلاک; }
    خالی شامل_ہے_Gen_بلاک(ثابت bool flag) { m_شامل_ہے_Gen_بلاک = flag; }
    bool pli() ثابت { واپس m_pli; }
    خالی pli(bool flag) { m_pli = flag; }
    bool superReference() ثابت { واپس m_superReference; }
    خالی superReference(bool flag) { m_superReference = flag; }
    bool is_خالص() override;
    bool isUnlikely() ثابت override { واپس m_taskp && m_taskp->isUnlikely(); }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت Ast_گرہ_FTask_Ref* ثابت asamep = VN_DBG_AS(samep, گرہ_FTask_Ref);
        واپس taskp() == asamep->taskp()  //
               && classOrPackagep() == asamep->classOrPackagep()  //
               && name() == asamep->name()  //
               && dotted() == asamep->dotted()  //
               && inlinedDots() == asamep->inlinedDots()  //
               && pli() == asamep->pli()  //
               && شامل_ہے_Gen_بلاک() == asamep->شامل_ہے_Gen_بلاک()
               && superReference() == asamep->superReference();
    }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت final override { V3ERROR_NA_RETURN(true); }

نجی:
    bool getPurityRecurse() ثابت;
};
class Ast_گرہ_Pre_Sel VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

محفوظ:
    Ast_گرہ_Pre_Sel(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ_اظہاریہ* thsp)
        : Ast_گرہ_اظہاریہ{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Pre_Sel;
    // METHODS
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت final override { V3ERROR_NA_RETURN(true); }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;

نجی:
    bool getPurityRecurse() ثابت {
        واپس fromp()->is_خالص() && rhsp()->is_خالص() && (!thsp() || thsp()->is_خالص());
    }
};
class Ast_گرہ_Quadop VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محفوظ:
    Ast_گرہ_Quadop(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ_اظہاریہ* thsp,
                  Ast_گرہ_اظہاریہ* fhsp)
        : Ast_گرہ_اظہاریہ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    مجازی خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs,
                               ثابت V3_نمبر& ths, ثابت V3_نمبر& fhs)
        = 0;
    مجازی bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    مجازی bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    مجازی bool cleanThs() ثابت = 0;  // True if THS must have extra upper bits zero
    مجازی bool cleanFhs() ثابت = 0;  // True if THS must have extra upper bits zero
    مجازی bool سائز_Matters_Lhs() ثابت = 0;  // True if output result depends on lhs size
    مجازی bool سائز_Matters_Rhs() ثابت = 0;  // True if output result depends on rhs size
    مجازی bool سائز_Matters_Ths() ثابت = 0;  // True if output result depends on ths size
    مجازی bool سائز_Matters_Fhs() ثابت = 0;  // True if output result depends on ths size
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;

نجی:
    bool getPurityRecurse() ثابت {
        واپس lhsp()->is_خالص() && rhsp()->is_خالص() && thsp()->is_خالص() && fhsp()->is_خالص();
    }
};
class Ast_گرہ_Termop VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // Terminal operator -- an operator with no "inputs"
محفوظ:
    Ast_گرہ_Termop(VNType t, File_سطر* fl)
        : Ast_گرہ_اظہاریہ{t, fl} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Termop;
    bool isSystemFunc() ثابت override { واپس true; }
};
class Ast_گرہ_Triop VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محفوظ:
    Ast_گرہ_Triop(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ_اظہاریہ* thsp)
        : Ast_گرہ_اظہاریہ{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Triop;
    // METHODS
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    // Set out to evaluation of a AstConst'ed
    مجازی خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs,
                               ثابت V3_نمبر& ths)
        = 0;
    مجازی bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    مجازی bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    مجازی bool cleanThs() ثابت = 0;  // True if THS must have extra upper bits zero
    مجازی bool سائز_Matters_Lhs() ثابت = 0;  // True if output result depends on lhs size
    مجازی bool سائز_Matters_Rhs() ثابت = 0;  // True if output result depends on rhs size
    مجازی bool سائز_Matters_Ths() ثابت = 0;  // True if output result depends on ths size
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;

نجی:
    bool getPurityRecurse() ثابت {
        واپس lhsp()->is_خالص() && rhsp()->is_خالص() && thsp()->is_خالص();
    }
};
class Ast_گرہ_Dist_Triop VL_NOT_FINAL : عوامی Ast_گرہ_Triop {
عوامی:
    Ast_گرہ_Dist_Triop(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp,
                     Ast_گرہ_اظہاریہ* thsp)
        : Ast_گرہ_Triop{t, fl, lhsp, rhsp, thsp} {
        dtype_سیٹ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_گرہ_Dist_Triop;
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool cleanThs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool سائز_Matters_Ths() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL_TRIG; }
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs,
                       ثابت V3_نمبر& ths) override {
        V3ERROR_NA;
    }
};
class Ast_گرہ_Uniop VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محفوظ:
    Ast_گرہ_Uniop(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : Ast_گرہ_اظہاریہ{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Uniop;
    // METHODS
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    // Set out to evaluation of a AstConst'ed lhs
    مجازی خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) = 0;
    مجازی bool cleanLhs() ثابت = 0;
    مجازی bool سائز_Matters_Lhs() ثابت = 0;  // True if output result depends on lhs size
    مجازی bool doubleFlavor() ثابت { واپس false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    مجازی bool signedFlavor() ثابت { واپس false; }
    مجازی bool stringFlavor() ثابت { واپس false; }  // N flavor of nodes with both flavors?
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;
};
class Ast_گرہ_System_Uniop_D VL_NOT_FINAL : عوامی Ast_گرہ_Uniop {
عوامی:
    Ast_گرہ_System_Uniop_D(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : Ast_گرہ_Uniop{t, fl, lhsp} {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_Ast_گرہ_System_Uniop_D;
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL_TRIG; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class Ast_گرہ_Var_Ref VL_NOT_FINAL : عوامی Ast_گرہ_اظہاریہ {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_گرہ_Var_Ref;  // Gen pointers before other members for performance
نجی:
    VAccess m_access;  // Left hand side assignment
    VSelf_اشارہ_گر_متن m_self_اشارہ_گر
        = VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::خالی_خانے_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
محفوظ:
    Ast_گرہ_Var_Ref(VNType t, File_سطر* fl, AstVar* varp, ثابت VAccess& access)
        : Ast_گرہ_اظہاریہ{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

عوامی:
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    VAccess access() ثابت { واپس m_access; }
    خالی access(ثابت VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() ثابت VL_MT_STABLE { واپس m_varp; }  // [After Link] Pointer to variable
    خالی varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_گرہ*>(varp));
    }
    AstVarScope* varScopep() ثابت { واپس m_varScopep; }
    خالی varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    ثابت VSelf_اشارہ_گر_متن& self_اشارہ_گر() ثابت { واپس m_self_اشارہ_گر; }
    خالی self_اشارہ_گر(ثابت VSelf_اشارہ_گر_متن& self_اشارہ_گر) { m_self_اشارہ_گر = self_اشارہ_گر; }
    string self_اشارہ_گر_Protect(bool useSelfForThis) ثابت {
        واپس self_اشارہ_گر().protect(useSelfForThis, protect());
    }
    Ast_گرہ_Module* classOrPackagep() ثابت { واپس m_classOrPackagep; }
    خالی classOrPackagep(Ast_گرہ_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : عوامی Ast_گرہ {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
عوامی:
    AstArg(File_سطر* fl, ثابت std::string& name, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت std::string& name) override { m_name = name; }
    bool خالی_خانے_Connect_No_اگلا() ثابت { واپس !exprp() && name() == "" && !nextp(); }
};
class AstWith final : عوامی Ast_گرہ {
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
نجی:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::سیٹ<std::string> m_restricted_نام;

عوامی:
    AstWith(File_سطر* fl, AstLambdaArgRef* اشاریہ_Arg_Refp, AstLambdaArgRef* قدر_Arg_Refp,
            Ast_گرہ* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->اشاریہ_Arg_Refp(اشاریہ_Arg_Refp);
        this->قدر_Arg_Refp(قدر_Arg_Refp);
        جوڑو_Exprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!اشاریہ_Arg_Refp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!قدر_Arg_Refp());  // varp needed to know lambda's arg dtype
        واپس nullptr;
    }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    خالی restricted(bool flag) { m_restricted = flag; }
    bool restricted() ثابت { واپس m_restricted; }
    bool validated() ثابت { واپس m_validated; }
    خالی validated(bool flag) { m_validated = flag; }
    خالی جوڑو_Restricted_Name(ثابت std::string& name) { m_restricted_نام.داخل_کرو(name); }
    ثابت std::سیٹ<std::string>& restricted_نام() ثابت { واپس m_restricted_نام; }
    // True if 'name' binds into the randomize() target class.
    bool name_Resolves_مقصد_ہدف(ثابت std::string& name) ثابت {
        واپس !m_restricted || m_restricted_نام.گنتی(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : عوامی Ast_گرہ_اظہاریہ {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbort_قسم m_قسم{VAbort_قسم::ACCEPT_ON};

عوامی:
    AstAbortOn(File_سطر* fl, VAbort_قسم قسم, Ast_گرہ_اظہاریہ* condp, Ast_گرہ_اظہاریہ* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_قسم{قسم} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    VAbort_قسم قسم() ثابت { واپس m_قسم; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class Ast_Addr_کا_CFunc final : عوامی Ast_گرہ_اظہاریہ {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

عوامی:
    Ast_Addr_کا_CFunc(File_سطر* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_Addr_کا_CFunc(fl)
        , m_funcp{funcp} {
        dtypep(ڈھونڈو_CHandle_DType());
    }

    ASTGEN_MEMBERS_Ast_Addr_کا_CFunc;
    خالی dump(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    AstCFunc* funcp() ثابت { واپس m_funcp; }
};
class Ast_Attr_کا final : عوامی Ast_گرہ_اظہاریہ {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
عوامی:
    Ast_Attr_کا(File_سطر* fl, VAttrType attrtype, Ast_گرہ* fromp = nullptr,
              Ast_گرہ_اظہاریہ* dimp = nullptr)
        : ASTGEN_SUPER_Attr_کا(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_Ast_Attr_کا;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    خالی name(ثابت string& name) override { m_name = name; }
    VAttrType attrType() ثابت { واپس m_attrType; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : عوامی Ast_گرہ_اظہاریہ {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ثابت bool m_خالص;  // Pure optimizable

    خالی init(ثابت string& متن, صحیح setwidth) {
        اگر (!متن.خالی_خانے()) جوڑو(متن);
        اگر (setwidth) {
            dtype_سیٹ_ثنائی_ہندسہ_Sized(setwidth, VSigning::UNSIGNED);
        } ورنہ {
            dtype_سیٹ_Void();  // Caller to override if necessary
        }
    }

عوامی:
    class خالص {};
    explicit AstCExpr(File_سطر* fl, ثابت string& متن = "", صحیح setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_خالص{false} {
        init(متن, setwidth);
    }
    AstCExpr(File_سطر* fl, خالص, ثابت string& متن = "", صحیح setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_خالص{true} {
        init(متن, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    bool clean_برآمد() ثابت override { واپس true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس m_خالص; }
    bool isOutputter() override { واپس true; }
    bool isPredictOptimizable() ثابت override { واپس m_خالص; }
    bool is_خالص() override { واپس m_خالص; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    // Add some text, or a node to this expression
    خالی جوڑو(ثابت std::string& متن) { جوڑو_Nodesp(new Ast_متن{fileline(), متن}); }
    خالی جوڑو(Ast_گرہ* nodep) { جوڑو_Nodesp(nodep); }
};
class AstCExprUser final : عوامی Ast_گرہ_اظہاریہ {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ثابت bool m_خالص;  // Whether the function is pure
عوامی:
    class خالص {};
    explicit AstCExprUser(File_سطر* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_خالص{false} {}
    AstCExprUser(File_سطر* fl, خالص)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_خالص{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    bool clean_برآمد() ثابت override { واپس false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس m_خالص; }
    bool isOutputter() override { واپس true; }
    bool isPredictOptimizable() ثابت override { واپس m_خالص; }
    bool is_خالص() override { واپس m_خالص; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    // Add some text, or a node to this expression
    خالی جوڑو(ثابت std::string& متن) { جوڑو_Nodesp(new Ast_متن{fileline(), متن}); }
    خالی جوڑو(Ast_گرہ* nodep) { جوڑو_Nodesp(nodep); }
};
class AstCFuncHard final : عوامی Ast_گرہ_اظہاریہ {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
عوامی:
    AstCFuncHard(File_سطر* fl, VCFunction function, Ast_گرہ_اظہاریہ* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        جوڑو_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس function().ascii(); }
    bool is_خالص() override {
        اگر (!m_purity.isCached()) m_purity.سیٹ(getPurity());
        واپس m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    VCFunction function() ثابت VL_MT_STABLE { واپس m_function; }

نجی:
    bool getPurity();
};
class AstCMethodHard final : عوامی Ast_گرہ_اظہاریہ {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_طریقہ;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
عوامی:
    AstCMethodHard(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, VCMethod طریقہ, Ast_گرہ_اظہاریہ* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_طریقہ{طریقہ} {
        this->fromp(fromp);
        جوڑو_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس طریقہ().ascii(); }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstCMethodHard* ثابت asamep = VN_DBG_AS(samep, CMethodHard);
        واپس (m_طریقہ == asamep->m_طریقہ);
    }
    bool is_خالص() override {
        اگر (!m_purity.isCached()) m_purity.سیٹ(getPurity());
        واپس m_purity.get();
    }
    صحیح instr_گنتی() ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس m_طریقہ.isClean(); }
    bool usePtr() ثابت { واپس m_usePtr; }
    خالی usePtr(bool flag) { m_usePtr = flag; }
    VCMethod طریقہ() ثابت VL_MT_STABLE { واپس m_طریقہ; }
    خالی طریقہ(VCMethod قدر) { m_طریقہ = قدر; }

نجی:
    bool getPurity();
};
class AstCReset final : عوامی Ast_گرہ_اظہاریہ {
    // Reset variable at startup
    ثابت bool m_constructing;  // Previously cleared by constructor
عوامی:
    AstCReset(File_سطر* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool is_خالص() override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(backp(), گرہ_تفویض));  // V3Emit* assumption
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() ثابت { واپس m_constructing; }
};
class AstCast final : عوامی Ast_گرہ_اظہاریہ {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
عوامی:
    AstCast(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, VFlag_بچہ_DType, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        بچہ_DTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { واپس "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت VL_MT_STABLE { واپس dtypep() ? dtypep() : بچہ_DTypep(); }
};
class AstCastParse final : عوامی Ast_گرہ_اظہاریہ {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
عوامی:
    AstCastParse(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { واپس "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cast_سائز final : عوامی Ast_گرہ_اظہاریہ {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
عوامی:
    Ast_Cast_سائز(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_Cast_سائز(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_Ast_Cast_سائز;
    string emitVerilog() override { واپس "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : عوامی Ast_گرہ_اظہاریہ {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
عوامی:
    AstCellArrayRef(File_سطر* fl, ثابت string& name, Ast_گرہ_اظہاریہ* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        جوڑو_Selp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : عوامی Ast_گرہ_اظہاریہ {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
عوامی:
    AstCellRef(File_سطر* fl, ثابت string& name, Ast_گرہ* cellp, Ast_گرہ* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
عوامی:
    AstClassOrPackageRef(File_سطر* fl, ثابت string& name, Ast_گرہ* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        جوڑو_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_گرہ_Module* classOrPackageSkipp(ثابت bool doRefs = true) ثابت;
    Ast_گرہ* classOrPackageNodep() ثابت { واپس m_classOrPackageNodep; }
    خالی classOrPackageNodep(Ast_گرہ* nodep) { m_classOrPackageNodep = nodep; }
    خالی classOrPackagep(Ast_گرہ_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_گرہ*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : عوامی Ast_گرہ_اظہاریہ {
    // A constant
    V3_نمبر m_عدد;  // Constant value
    خالی init_With_نمبر() {
        اگر (m_عدد.isDouble()) {
            dtype_سیٹ_Double();
        } ورنہ اگر (m_عدد.isString()) {
            dtype_سیٹ_String();
        } ورنہ {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(m_عدد.چوڑائی(), (m_عدد.sized() ? 0 : m_عدد.چوڑائی_مقصد_Fit()),
                                      VSigning::fromBool(m_عدد.isSigned()), m_عدد.isAnyXZ());
        }
        m_عدد.nodep(this);
    }

عوامی:
    ~AstConst() override;
    AstConst(File_سطر* fl, ثابت V3_نمبر& عدد)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{عدد} {
        init_With_نمبر();
    }
    class Widthed_قدر {};  // for creator type-overload selection
    AstConst(File_سطر* fl, Widthed_قدر, صحیح چوڑائی, uint32_t قدر)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, چوڑائی, قدر) {  // Need () constructor
        init_With_نمبر();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_سطر* fl, DTyped, ثابت Ast_گرہ_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{this, nodedtypep} {
        init_With_نمبر();
    }
    class String_مقصد_Parse {};  // for creator type-overload selection
    AstConst(File_سطر* fl, String_مقصد_Parse, ثابت حرف* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{this, V3_نمبر::Verilog_نمبر_Literal{}, sourcep} {
        init_With_نمبر();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_سطر* fl, VerilogStringLiteral, ثابت string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{this, V3_نمبر::VerilogStringLiteral{}, str} {
        init_With_نمبر();
    }
    AstConst(File_سطر* fl, uint32_t عدد)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, 32, عدد) {  // Need () constructor
        dtype_سیٹ_ثنائی_ہندسہ_Unsized(m_عدد.چوڑائی(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_سطر* fl, Unsized32, uint32_t عدد)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, 32, عدد) {  // Need () constructor
        m_عدد.چوڑائی(32, false);
        dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, m_عدد.چوڑائی_مقصد_Fit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_سطر* fl, Signed32, int32_t عدد)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, 32, عدد) {  // Need () constructor
        m_عدد.چوڑائی(32, true);
        dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, m_عدد.چوڑائی_مقصد_Fit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_سطر* fl, Unsized64, uint64_t عدد)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, 64, 0) {  // Need () constructor
        m_عدد.سیٹ_Quad(عدد);
        dtype_سیٹ_ثنائی_ہندسہ_Sized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_سطر* fl, SizedEData, uint64_t عدد)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_عدد.سیٹ_Quad(عدد);
        dtype_سیٹ_ثنائی_ہندسہ_Sized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_سطر* fl, RealDouble, دوہرا عدد)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, 64) {  // Need () constructor
        m_عدد.سیٹ_Double(عدد);
        dtype_سیٹ_Double();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_سطر* fl, String, ثابت string& عدد)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{this, V3_نمبر::String{}, عدد} {
        dtype_سیٹ_String();
    }
    class ثنائی_ہندسہ_False {};
    AstConst(File_سطر* fl, ثنائی_ہندسہ_False)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, 1, 0) {  // Need () constructor
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class ثنائی_ہندسہ_False_Erroring {};
    AstConst(File_سطر* fl, ثنائی_ہندسہ_False_Erroring)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, 1, 0) {  // Need () constructor
        dtype_سیٹ_ثنائی_ہندسہ();
        File_سطر* ثابت newfl = new File_سطر{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class ثنائی_ہندسہ_True {};
    AstConst(File_سطر* fl, ثنائی_ہندسہ_True, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد(this, 1, on) {  // Need () constructor
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    class All0 {};
    AstConst(File_سطر* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{this, V3_نمبر::Verilog_نمبر_Literal{}, "'0"} {
        init_With_نمبر();
        fl->warnOff(V3_Error_رمز::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_سطر* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{this, V3_نمبر::Verilog_نمبر_Literal{}, "'1"} {
        init_With_نمبر();
        fl->warnOff(V3_Error_رمز::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_سطر* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{this, V3_نمبر::Null{}} {
        dtypep(ڈھونڈو_Null_DType());
        init_With_نمبر();
    }
    class One_قدم {};
    AstConst(File_سطر* fl, One_قدم)
        : ASTGEN_SUPER_Const(fl)
        , m_عدد{this, V3_نمبر::One_قدم{}} {
        dtype_سیٹ_ثنائی_ہندسہ_Sized(64, VSigning::UNSIGNED);
        init_With_نمبر();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() ثابت override VL_MT_STABLE { واپس عدد().ascii(); }  // * = Value
    ثابت V3_نمبر& عدد() ثابت VL_MT_محفوظ { واپس m_عدد; }  // * = Value
    V3_نمبر& عدد() { واپس m_عدد; }  // * = Value
    string origParamName() ثابت;
    خالی origParamName(ثابت string& name);
    uint32_t مقصد_UInt() ثابت { واپس عدد().مقصد_UInt(); }
    int32_t مقصد_SInt() ثابت VL_MT_محفوظ { واپس عدد().مقصد_SInt(); }
    uint64_t مقصد_UQuad() ثابت { واپس عدد().مقصد_UQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstConst* ثابت sp = VN_DBG_AS(samep, Const);
        واپس عدد().isCaseEq(sp->عدد());
    }
    خالی cloneRelink() override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(m_عدد.nodep() && m_عدد.nodep() != this);
        واپس nullptr;
    }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool isEqAllOnes() ثابت { واپس عدد().isEqAllOnes(چوڑائی()); }
    bool isEqAllOnesV() ثابت { واپس عدد().isEqAllOnes(چوڑائی_Min_V()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_قدر_Type(ثابت AstConst* samep) ثابت;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    ساکن AstConst* parseParamLiteral(File_سطر* fl, ثابت string& literal);
};
class AstConstraintRef final : عوامی Ast_گرہ_اظہاریہ {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

عوامی:
    AstConstraintRef(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(ڈھونڈو_Constraint_Ref_DType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() ثابت override VL_MT_STABLE;
    AstConstraint* constrp() ثابت VL_MT_STABLE { واپس m_constrp; }
    خالی constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_گرہ_Module* classOrPackagep() ثابت VL_MT_STABLE { واپس m_classOrPackagep; }
    // TODO: this setter is unused
    خالی classOrPackagep(Ast_گرہ_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت final override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cvt_Array_مقصد_Array final : عوامی Ast_گرہ_اظہاریہ {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
نجی:
    ثابت bool m_الٹ;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    ثابت صحیح m_بلاک_سائز;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    ثابت صحیح m_dst_Element_بٹ;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    ثابت صحیح m_src_Element_بٹ;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

عوامی:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_مقصد_Array(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_DType* dtp, bool الٹ,
                       صحیح بلاک_سائز, صحیح dst_Element_بٹ, صحیح src_Element_بٹ)
        : ASTGEN_SUPER_Cvt_Array_مقصد_Array(fl)
        , m_الٹ{الٹ}
        , m_بلاک_سائز{بلاک_سائز}
        , m_dst_Element_بٹ{dst_Element_بٹ}
        , m_src_Element_بٹ{src_Element_بٹ} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_مقصد_Array;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool الٹ() ثابت { واپس m_الٹ; }
    صحیح بلاک_سائز() ثابت { واپس m_بلاک_سائز; }
    صحیح dst_Element_بٹ() ثابت { واپس m_dst_Element_بٹ; }
    صحیح src_Element_بٹ() ثابت { واپس m_src_Element_بٹ; }
};
class Ast_Cvt_Array_مقصد_Packed final : عوامی Ast_گرہ_اظہاریہ {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
عوامی:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_مقصد_Packed(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Array_مقصد_Packed(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_مقصد_Packed;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
};
class Ast_Cvt_Packed_مقصد_Array final : عوامی Ast_گرہ_اظہاریہ {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
عوامی:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Packed_مقصد_Array(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Packed_مقصد_Array(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Packed_مقصد_Array;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
};
class Ast_Cvt_Unpack_مقصد_قطار final : عوامی Ast_گرہ_اظہاریہ {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
عوامی:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Unpack_مقصد_قطار(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_مقصد_قطار(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_مقصد_قطار;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { واپس "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
};
class Ast_Cvt_Unpack_مقصد_Unpack final : عوامی Ast_گرہ_اظہاریہ {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
عوامی:
    Ast_Cvt_Unpack_مقصد_Unpack(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_مقصد_Unpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_مقصد_Unpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
};
class AstDist final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
عوامی:
    AstDist(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_Dist_شے* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        جوڑو_Itemsp(itemsp);
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { واپس "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }  // NA
};
class Ast_Dist_شے final : عوامی Ast_گرہ_اظہاریہ {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_is_پورا = false;  // True for weight ':/', false for ':='
عوامی:
    Ast_Dist_شے(File_سطر* fl, Ast_گرہ_اظہاریہ* rangep, Ast_گرہ_اظہاریہ* weightp)
        : ASTGEN_SUPER_Dist_شے(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_Ast_Dist_شے;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { واپس "%l "s + (m_is_پورا ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }  // NA
    خالی is_پورا(bool flag) { m_is_پورا = flag; }
    bool is_پورا() ثابت { واپس m_is_پورا; }
};
class AstDot final : عوامی Ast_گرہ_اظہاریہ {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    ثابت bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
عوامی:
    AstDot(File_سطر* fl, bool colon, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    ساکن Ast_گرہ_اظہاریہ* newIfPkg(File_سطر* fl, Ast_گرہ_اظہاریہ* packageOrClassp, Ast_گرہ_اظہاریہ* rhsp) {
        اگر (!packageOrClassp) واپس rhsp;
        واپس new AstDot{fl, true, packageOrClassp, rhsp};
    }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool colon() ثابت { واپس m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class Ast_خالی_خانے_قطار final : عوامی Ast_گرہ_اظہاریہ {
عوامی:
    explicit Ast_خالی_خانے_قطار(File_سطر* fl)
        : ASTGEN_SUPER_خالی_خانے_قطار(fl) {}
    ASTGEN_MEMBERS_Ast_خالی_خانے_قطار;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { واپس "{}"; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool clean_برآمد() ثابت override { واپس true; }
};
class Ast_Enum_شے_Ref final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_شامل_ہے_Gen_بلاک = false;  // Contains gen block reference
عوامی:
    Ast_Enum_شے_Ref(File_سطر* fl, Ast_Enum_شے* itemp, Ast_گرہ_Module* classOrPackagep)
        : ASTGEN_SUPER_Enum_شے_Ref(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_Ast_Enum_شے_Ref;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس itemp() ? itemp()->name() : m_name; }
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت Ast_Enum_شے_Ref* ثابت sp = VN_DBG_AS(samep, Enum_شے_Ref);
        واپس itemp() == sp->itemp() && dotted() == sp->dotted()
               && شامل_ہے_Gen_بلاک() == sp->شامل_ہے_Gen_بلاک();
    }
    Ast_Enum_شے* itemp() ثابت VL_MT_STABLE { واپس m_itemp; }
    خالی itemp(Ast_Enum_شے* nodep) { m_itemp = nodep; }
    string dotted() ثابت { واپس m_dotted; }
    خالی dotted(ثابت string& dotted) { m_dotted = dotted; }
    bool شامل_ہے_Gen_بلاک() ثابت { واپس m_شامل_ہے_Gen_بلاک; }
    خالی شامل_ہے_Gen_بلاک(ثابت bool flag) { m_شامل_ہے_Gen_بلاک = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    Ast_گرہ_Module* classOrPackagep() ثابت { واپس m_classOrPackagep; }
    خالی classOrPackagep(Ast_گرہ_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_اظہاریہ_Stmt final : عوامی Ast_گرہ_اظہاریہ {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
نجی:
    bool m_has_نتیجہ = true;  // Returns result via resultp()

عوامی:
    Ast_اظہاریہ_Stmt(File_سطر* fl, Ast_گرہ* stmtsp, Ast_گرہ_اظہاریہ* resultp)
        : ASTGEN_SUPER_اظہاریہ_Stmt(fl) {
        جوڑو_Stmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_اظہاریہ_Stmt;
    // METHODS
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool is_خالص() override {
        اگر (Ast_گرہ::afterCommentp(stmtsp())) واپس false;
        واپس resultp()->is_خالص();
    }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    bool has_نتیجہ() ثابت { واپس m_has_نتیجہ; }
    خالی has_نتیجہ(bool flag) { m_has_نتیجہ = flag; }
};
class AstFError final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
عوامی:
    AstFError(File_سطر* fl, Ast_گرہ_اظہاریہ* filep, Ast_گرہ_اظہاریہ* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { واپس "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 64; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstFOpen final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
عوامی:
    AstFOpen(File_سطر* fl, Ast_گرہ_اظہاریہ* filenamep, Ast_گرہ_اظہاریہ* modep)
        : ASTGEN_SUPER_FOpen(fl) {
        this->filenamep(filenamep);
        this->modep(modep);
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpen;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    string verilogKwd() ثابت override { واپس "$fopen"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstFOpenMcd final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op2 := filenamep : AstNodeExpr
عوامی:
    AstFOpenMcd(File_سطر* fl, Ast_گرہ_اظہاریہ* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    string verilogKwd() ثابت override { واپس "$fopen"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstFRead final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
عوامی:
    AstFRead(File_سطر* fl, Ast_گرہ_اظہاریہ* memp, Ast_گرہ_اظہاریہ* filep, Ast_گرہ_اظہاریہ* startp,
             Ast_گرہ_اظہاریہ* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtype_سیٹ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() ثابت override { واپس "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: makes output
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstFRewind final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := filep : Optional[AstNodeExpr]
عوامی:
    AstFRewind(File_سطر* fl, Ast_گرہ_اظہاریہ* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() ثابت override { واپس "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstFScanF final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_متن;

عوامی:
    AstFScanF(File_سطر* fl, ثابت string& متن, Ast_گرہ_اظہاریہ* filep, Ast_گرہ_اظہاریہ* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_متن{متن} {
        جوڑو_Exprsp(exprsp);
        this->filep(filep);
        dtype_سیٹ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() ثابت override VL_MT_STABLE { واپس m_متن; }
    string verilogKwd() ثابت override { واپس "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: makes output
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس متن() == VN_DBG_AS(samep, FScanF)->متن();
    }
    string متن() ثابت { واپس m_متن; }  // * = Text to display
    خالی متن(ثابت string& متن) { m_متن = متن; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstFSeek final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
عوامی:
    AstFSeek(File_سطر* fl, Ast_گرہ_اظہاریہ* filep, Ast_گرہ_اظہاریہ* انحراف, Ast_گرہ_اظہاریہ* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->انحراف(انحراف);
        this->operation(operation);
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() ثابت override { واپس "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: makes output
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstFTell final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
عوامی:
    AstFTell(File_سطر* fl, Ast_گرہ_اظہاریہ* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() ثابت override { واپس "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstFalling final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
عوامی:
    AstFalling(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { واپس "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstFell final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عوامی:
    AstFell(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_Sen_درخت* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { واپس "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstFuture final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عوامی:
    AstFuture(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_Sen_درخت* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { واپس "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstGatePin final : عوامی Ast_گرہ_اظہاریہ {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
عوامی:
    AstGatePin(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { واپس "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
};
class Ast_Get_Initial_Random_بیج final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $get_initial_random_seed()
عوامی:
    explicit Ast_Get_Initial_Random_بیج(File_سطر* fl)
        : ASTGEN_SUPER_Get_Initial_Random_بیج(fl) {
        dtype_سیٹ_Int();
    }
    ASTGEN_MEMBERS_Ast_Get_Initial_Random_بیج;
    string emitVerilog() override { واپس "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس true; }
    bool is_خالص() override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstImplication final : عوامی Ast_گرہ_اظہاریہ {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

نجی:
    ثابت bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    ثابت bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

عوامی:
    AstImplication(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool is_Multi_چکر_Sva() ثابت override { واپس m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() ثابت { واپس m_isOverlapped; }
    bool isFollowedBy() ثابت { واپس m_isFollowedBy; }
};
class AstInside final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
عوامی:
    AstInside(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        جوڑو_Itemsp(itemsp);
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { واپس "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }  // NA
};
class AstInsideRange final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
عوامی:
    AstInsideRange(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { واپس "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_گرہ_اظہاریہ* newAndFromInside(Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp);
};
class AstLambdaArgRef final : عوامی Ast_گرہ_اظہاریہ {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_اشاریہ;  // Index, not value

عوامی:
    AstLambdaArgRef(File_سطر* fl, ثابت string& name, bool اشاریہ)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_اشاریہ{اشاریہ} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    string emitVerilog() override { واپس name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    خالی name(ثابت string& name) override { m_name = name; }
    bool اشاریہ() ثابت { واپس m_اشاریہ; }
    bool is_اظہاریہ_Coverage_Eligible() ثابت override { واپس false; }
};
class Ast_مطابقت_Masked final : عوامی Ast_گرہ_اظہاریہ {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
عوامی:
    اندرونی Ast_مطابقت_Masked(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_Ast_مطابقت_Masked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { واپس "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    ساکن uint32_t fold(ثابت V3_نمبر& lhs, AstVar* مطابقت_Varp);
};
class AstMatches final : عوامی Ast_گرہ_اظہاریہ {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
عوامی:
    AstMatches(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { واپس "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstMemberSel final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
عوامی:
    AstMemberSel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, VFlag_بچہ_DType, ثابت string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& name) override { m_name = name; }
    VAccess access() ثابت { واپس m_access; }
    خالی access(ثابت VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    AstVar* varp() ثابت { واپس m_varp; }
    خالی varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : عوامی Ast_گرہ_اظہاریہ {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
عوامی:
    AstNewCopy(File_سطر* fl, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { واپس "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
};
class AstNewDynamic final : عوامی Ast_گرہ_اظہاریہ {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
عوامی:
    AstNewDynamic(File_سطر* fl, Ast_گرہ_اظہاریہ* sizep, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { واپس "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
};
class AstPExpr final : عوامی Ast_گرہ_اظہاریہ {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
عوامی:
    explicit AstPExpr(File_سطر* fl, AstBegin* bodyp, Ast_گرہ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_سطر* fl, AstBegin* bodyp, Ast_گرہ_Stmt* finalp, Ast_گرہ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
};
class AstParseHolder final : عوامی Ast_گرہ_اظہاریہ {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
عوامی:
    explicit AstParseHolder(File_سطر* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : عوامی Ast_گرہ_اظہاریہ {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

عوامی:
    AstParseRef(File_سطر* fl, ثابت string& name, Ast_گرہ_اظہاریہ* lhsp = nullptr,
                Ast_گرہ_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    خالی name(ثابت string& name) override { m_name = name; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstParseRef* ثابت asamep = VN_DBG_AS(samep, ParseRef);
        واپس m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

عوامی:
    AstPast(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* ticksp = nullptr,
            Ast_Sen_درخت* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() ثابت { واپس m_propertyTiming; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstPatMember final : عوامی Ast_گرہ_اظہاریہ {
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

عوامی:
    AstPatMember(File_سطر* fl, Ast_گرہ_اظہاریہ* lhssp, Ast_گرہ* keyp, Ast_گرہ_اظہاریہ* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        جوڑو_Lhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { واپس lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 2; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    bool isConcat() ثابت { واپس m_isConcat; }
    خالی isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() ثابت { واپس m_isDefault; }
    خالی isDefault(bool flag) { m_isDefault = flag; }
    Ast_گرہ_اظہاریہ* کلید_Exprp() ثابت { واپس varrefp() ? varrefp() : VN_CAST(keyp(), گرہ_اظہاریہ); }
};
class Ast_نمونہ final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
عوامی:
    Ast_نمونہ(File_سطر* fl, Ast_گرہ* itemsp)
        : ASTGEN_SUPER_نمونہ(fl) {
        جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_نمونہ;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت VL_MT_STABLE { واپس dtypep() ? dtypep() : بچہ_DTypep(); }
};
class Ast_نمونہ_ستارہ final : عوامی Ast_گرہ_اظہاریہ {
    // Pattern wildcard: ".*"
عوامی:
    explicit Ast_نمونہ_ستارہ(File_سطر* fl)
        : ASTGEN_SUPER_نمونہ_ستارہ(fl) {}
    ASTGEN_MEMBERS_Ast_نمونہ_ستارہ;
    string emitVerilog() override { واپس ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_نمونہ_Var final : عوامی Ast_گرہ_اظہاریہ {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
عوامی:
    Ast_نمونہ_Var(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_نمونہ_Var(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_نمونہ_Var;
    string emitVerilog() override { واپس ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_name == VN_DBG_AS(samep, نمونہ_Var)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
};
class AstPropAlways final : عوامی Ast_گرہ_اظہاریہ {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    ثابت bool m_isStrong = false;  // s_always
عوامی:
    AstPropAlways(File_سطر* fl, Ast_گرہ_اظہاریہ* propp, Ast_گرہ_اظہاریہ* loBoundp, Ast_گرہ_اظہاریہ* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { V3ERROR_NA_RETURN(0); }
    bool isStrong() ثابت { واپس m_isStrong; }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstRand final : عوامی Ast_گرہ_اظہاریہ {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    ثابت bool m_urandom;  // $urandom vs $random
عوامی:
    class Reset {};
    AstRand(File_سطر* fl, Ast_گرہ_اظہاریہ* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_سطر* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtype_سیٹ_UInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override {
        واپس seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        اگر (seedp()) {
            اگر (urandom()) واپس "VL_URANDOM_SEEDED_%nq%lq(%li)";
            واپس "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        اگر (isWide()) واپس "VL_RANDOM_%nq(%nw, %P)";
        واپس "VL_RANDOM_%nq()";
    }
    bool clean_برآمد() ثابت override { واپس false; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس !seedp(); }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool combinable(ثابت AstRand* samep) ثابت {
        واپس !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() ثابت { واپس m_urandom; }
};
class AstRandRNG final : عوامی Ast_گرہ_اظہاریہ {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
عوامی:
    AstRandRNG(File_سطر* fl, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { واپس "%f$rngrandom()"; }
    string emitC() override {
        واپس isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool clean_برآمد() ثابت override { واپس false; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstRising final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
عوامی:
    AstRising(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { واپس "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstRose final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عوامی:
    AstRose(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_Sen_درخت* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { واپس "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstSClocked final : عوامی Ast_گرہ_اظہاریہ {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
عوامی:
    AstSClocked(File_سطر* fl, Ast_Sen_شے* sensesp, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : عوامی Ast_گرہ_اظہاریہ {
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
    ثابت bool m_unbounded = false;  // True when the upper bound is $
عوامی:
    // Exact [*N]
    AstSConsRep(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* countp, Ast_گرہ_اظہاریہ* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { V3ERROR_NA_RETURN(0); }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {  // LCOV_EXCL_LINE
        واپس m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() ثابت { واپس m_unbounded; }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstSEventually final : عوامی Ast_گرہ_اظہاریہ {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
عوامی:
    explicit AstSEventually(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : عوامی Ast_گرہ_اظہاریہ {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
عوامی:
    explicit AstSExpr(File_سطر* fl, Ast_گرہ_اظہاریہ* preExprp, Ast_گرہ_Stmt* delayp, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_سطر* fl, Ast_گرہ_Stmt* delayp, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstSFormatArg final : عوامی Ast_گرہ_اظہاریہ {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

عوامی:
    AstSFormatArg(File_سطر* fl, VFormatAttr formatAttr, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() ثابت override { واپس "$sformatarg"; }
    string emitVerilog() override { واپس "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && پہلا_Abovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        واپس nullptr;
    }
    VFormatAttr formatAttr() ثابت { واپس m_formatAttr; }
    خالی formatAttr(ثابت VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    ساکن VFormatAttr formatAttrDefauled(ثابت AstSFormatArg* nodep, ثابت Ast_گرہ_DType* dtypep);
};
class AstSFormatF final : عوامی Ast_گرہ_اظہاریہ {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_متن;
    bool m_hidden;  // Under display, etc
    bool m_اظہاریہ_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    ثابت حرف m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
عوامی:
    class اظہاریہ_Format {};
    AstSFormatF(File_سطر* fl, ثابت string& متن, bool hidden, Ast_گرہ_اظہاریہ* exprsp,
                حرف missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_متن{متن}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtype_سیٹ_String();
        جوڑو_Exprsp(exprsp);
    }
    AstSFormatF(File_سطر* fl, اظہاریہ_Format, Ast_گرہ_اظہاریہ* exprsp, حرف missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_متن{""}
        , m_hidden{hidden}
        , m_اظہاریہ_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtype_سیٹ_String();
        جوڑو_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_متن; }
    خالی name(ثابت string& name) override { m_متن = name; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس متن() == VN_DBG_AS(samep, SFormatF)->متن();
    }
    string verilogKwd() ثابت override { واپس "$sformatf"; }
    string متن() ثابت { واپس m_متن; }  // * = Text to display
    خالی متن(ثابت string& متن) { m_متن = متن; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(متن() != "" && اظہاریہ_Format_2());  // Expr format means no literal format
        واپس nullptr;
    }
    bool formatScopeTracking() ثابت {  // Track scopeNamep();  Ok if false positive
        اگر (اظہاریہ_Format_2() || name().ڈھونڈو("%m") != string::npos || name().ڈھونڈو("%M") != string::npos)
            واپس true;
        برائے (ثابت Ast_گرہ* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            اگر (ثابت AstConst* ثابت fmtp = VN_CAST(exprp, Const)) {
                اگر (fmtp->عدد().isFromString()) {
                    ثابت string str = fmtp->عدد().مقصد_String();
                    اگر (str.ڈھونڈو("%m") != string::npos || str.ڈھونڈو("%M") != string::npos)
                        واپس true;
                }
            }
        }
        واپس false;
    }
    bool hidden() ثابت { واپس m_hidden; }
    خالی hidden(bool flag) { m_hidden = flag; }
    bool اظہاریہ_Format_2() ثابت { واپس m_اظہاریہ_Format; }
    خالی اظہاریہ_Format_2(bool flag) { m_اظہاریہ_Format = flag; }
    bool optionalFormat() ثابت { واپس m_optionalFormat; }
    خالی optionalFormat(bool flag) { m_optionalFormat = flag; }
    حرف missingArgChar() ثابت { واپس m_missingArgChar; }
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstSGotoRep final : عوامی Ast_گرہ_اظہاریہ {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
عوامی:
    // Exact [->N]
    AstSGotoRep(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* countp, Ast_گرہ_اظہاریہ* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstSNonConsRep final : عوامی Ast_گرہ_اظہاریہ {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
عوامی:
    // Exact [=N]
    AstSNonConsRep(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* countp, Ast_گرہ_اظہاریہ* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstSScanF final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_متن;
    VTimescale m_timeunit;  // Parent module time unit

عوامی:
    AstSScanF(File_سطر* fl, ثابت string& متن, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_متن{متن} {
        جوڑو_Exprsp(exprsp);
        this->fromp(fromp);
        dtype_سیٹ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() ثابت override VL_MT_STABLE { واپس m_متن; }
    string verilogKwd() ثابت override { واپس "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: makes output
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstSScanF* ثابت asamep = VN_DBG_AS(samep, SScanF);
        واپس متن() == asamep->متن() && timeunit() == asamep->timeunit();
    }
    string متن() ثابت { واپس m_متن; }  // * = Text to display
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstSampled final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_اندرونی : 1;  // Internally created, not from a source $sampled
عوامی:
    AstSampled(File_سطر* fl, Ast_گرہ* exprp, Ast_گرہ_DType* dtypep, bool اندرونی_3 = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_اندرونی{اندرونی_3} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { واپس "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_اندرونی == VN_DBG_AS(samep, Sampled)->m_اندرونی;
    }
    bool اندرونی_3() ثابت { واپس m_اندرونی; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstScopeName final : عوامی Ast_گرہ_اظہاریہ {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    ثابت bool m_forFormat;  // Is for a format %m
    ساکن std::string scopeNameFormatter(ثابت std::string& متن);
    ساکن std::string scopePrettyNameFormatter(ثابت std::string& متن);

عوامی:
    class ForFormat {};
    AstScopeName(File_سطر* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtype_سیٹ_UInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstScopeName* ثابت sp = VN_DBG_AS(samep, ScopeName);
        واپس m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    // ACCESSORS
    ثابت std::string& scopeAttr() ثابت { واپس m_scopeAttr; }
    خالی scopeAttr(ثابت std::string& val) { m_scopeAttr = val; }
    ثابت std::string& scopeEntr() ثابت { واپس m_scopeEntr; }
    خالی scopeEntr(ثابت std::string& val) { m_scopeEntr = val; }
    bool dpiExport() ثابت { واپس m_dpiExport; }
    خالی dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() ثابت { واپس m_selfRelative; }
    خالی selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() ثابت { واپس m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() ثابت { واپس scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() ثابت { واپس scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() ثابت { واپس scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() ثابت { واپس scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عوامی:
    AstStable(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_Sen_درخت* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { واپس "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class Ast_ڈھیر_Trace_F final : عوامی Ast_گرہ_اظہاریہ {
    // $stacktrace used as function
عوامی:
    explicit Ast_ڈھیر_Trace_F(File_سطر* fl)
        : ASTGEN_SUPER_ڈھیر_Trace_F(fl) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_Ast_ڈھیر_Trace_F;
    string verilogKwd() ثابت override { واپس "$stacktrace"; }
    string emitVerilog() override { واپس verilogKwd(); }
    string emitC() override { واپس "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstSteady final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
عوامی:
    AstSteady(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { واپس "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstStructSel final : عوامی Ast_گرہ_اظہاریہ {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
عوامی:
    AstStructSel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, ثابت string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override {
        // Not a union
        واپس VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstStructSel* ثابت sp = VN_DBG_AS(samep, StructSel);
        واپس m_name == sp->m_name;
    }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
};
class AstSysIgnore final : عوامی Ast_گرہ_اظہاریہ {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
عوامی:
    AstSysIgnore(File_سطر* fl, Ast_گرہ_اظہاریہ* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        جوڑو_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() ثابت override { واپس "$ignored"; }
    bool isGateOptimizable() ثابت override { واپس false; }  // Though deleted before opt
    bool isPredictOptimizable() ثابت override { واپس false; }  // Though deleted before opt
    bool is_خالص() override { واپس false; }  // Though deleted before opt
    bool isOutputter() override { واپس true; }  // Though deleted before opt
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : عوامی Ast_گرہ_اظہاریہ {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
عوامی:
    AstSystemF(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtype_سیٹ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() ثابت override { واپس "$system"; }
    string emitVerilog() override { واپس verilogKwd(); }
    string emitC() override { واپس "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
    bool isUnlikely() ثابت override { واپس true; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class Ast_Tagged_اظہاریہ final : عوامی Ast_گرہ_اظہاریہ {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
عوامی:
    Ast_Tagged_اظہاریہ(File_سطر* fl, ثابت string& name, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_Tagged_اظہاریہ(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_اظہاریہ;
    string emitVerilog() override { واپس "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_name == VN_DBG_AS(samep, Tagged_اظہاریہ)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
};
class Ast_Tagged_نمونہ final : عوامی Ast_گرہ_اظہاریہ {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
عوامی:
    Ast_Tagged_نمونہ(File_سطر* fl, ثابت string& name, Ast_گرہ* patternp)
        : ASTGEN_SUPER_Tagged_نمونہ(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_نمونہ;
    string emitVerilog() override { واپس "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_name == VN_DBG_AS(samep, Tagged_نمونہ)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
};
class Ast_Test_Plus_دلائل final : عوامی Ast_گرہ_اظہاریہ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
عوامی:
    Ast_Test_Plus_دلائل(File_سطر* fl, Ast_گرہ_اظہاریہ* searchp)
        : ASTGEN_SUPER_Test_Plus_دلائل(fl) {
        this->searchp(searchp);
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_Test_Plus_دلائل;
    string verilogKwd() ثابت override { واپس "$test$plusargs"; }
    string emitVerilog() override { واپس verilogKwd(); }
    string emitC() override { واپس "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    // but isPure() true
    bool isSystemFunc() ثابت override { واپس true; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstThisRef final : عوامی Ast_گرہ_اظہاریہ {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
عوامی:
    AstThisRef(File_سطر* fl, VFlag_بچہ_DType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        بچہ_DTypep(dtypep);
    }
    AstThisRef(File_سطر* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { واپس "this"; }
    string emitVerilog() override { واپس "this"; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool clean_برآمد() ثابت override { واپس true; }
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
};
class Ast_وقت_Precision final : عوامی Ast_گرہ_اظہاریہ {
    // Verilog $timeprecision
عوامی:
    explicit Ast_وقت_Precision(File_سطر* fl)
        : ASTGEN_SUPER_وقت_Precision(fl) {
        dtype_سیٹ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_وقت_Precision;
    string emitVerilog() override { واپس "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_وقت_Unit final : عوامی Ast_گرہ_اظہاریہ {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
عوامی:
    explicit Ast_وقت_Unit(File_سطر* fl)
        : ASTGEN_SUPER_وقت_Unit(fl) {
        dtype_سیٹ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_وقت_Unit;
    string emitVerilog() override { واپس "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : عوامی Ast_گرہ_اظہاریہ {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    ثابت std::سمتیہ<uint32_t> m_indices;  // Indices of the triggers tested
عوامی:
    AstTriggered(File_سطر* fl, AstVarScope* trigVscp, ثابت std::سمتیہ<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    ثابت std::سمتیہ<uint32_t>& indices() ثابت { واپس m_indices; }
};
class AstUnbounded final : عوامی Ast_گرہ_اظہاریہ {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
عوامی:
    explicit AstUnbounded(File_سطر* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtype_سیٹ_Int();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { واپس "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
};
class AstUnlinkedRef final : عوامی Ast_گرہ_اظہاریہ {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
عوامی:
    AstUnlinkedRef(File_سطر* fl, Ast_گرہ* refp, ثابت string& name, Ast_گرہ* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : عوامی Ast_گرہ_اظہاریہ {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    ثابت bool m_strong;  // 's_' preffix
    ثابت bool m_overlapping;  // '_with` suffix
عوامی:
    AstUntil(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() ثابت override {
        واپس (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
    bool isStrong() ثابت { واپس m_strong; }
    bool isOverlapping() ثابت { واپس m_overlapping; }
};
class Ast_قدر_Plus_دلائل final : عوامی Ast_گرہ_اظہاریہ {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
عوامی:
    Ast_قدر_Plus_دلائل(File_سطر* fl, Ast_گرہ_اظہاریہ* searchp, Ast_گرہ_اظہاریہ* outp)
        : ASTGEN_SUPER_قدر_Plus_دلائل(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_قدر_Plus_دلائل;
    string verilogKwd() ثابت override { واپس "$value$plusargs"; }
    string emitVerilog() override { واپس "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس !outp(); }
    bool isSystemFunc() ثابت override { واپس true; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstWithParse final : عوامی Ast_گرہ_اظہاریہ {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
نجی:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

عوامی:
    AstWithParse(File_سطر* fl, Ast_گرہ_اظہاریہ* funcrefp, Ast_گرہ_اظہاریہ* exprsp,
                 Ast_گرہ* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        جوڑو_Exprsp(exprsp);
        جوڑو_Constraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool restricted() ثابت { واپس m_restricted; }
    خالی restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : عوامی Ast_گرہ_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
عوامی:
    AstBufIf1(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { واپس "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstCastDynamic final : عوامی Ast_گرہ_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
عوامی:
    AstCastDynamic(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Unsized(32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { واپس "%f$cast(%r, %l)"; }
    string emitC() override { واپس "VL_DYNAMIC_CAST(%r, %l)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 20; }
    bool is_خالص() override { واپس false; }
};
class AstCompareNN final : عوامی Ast_گرہ_Biop {
    // Verilog str.compare() and str.icompare()
    ثابت bool m_ignoreCase;  // True for str.icompare()
عوامی:
    AstCompareNN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtype_سیٹ_Int();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() ثابت override VL_MT_STABLE { واپس m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        واپس m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        واپس m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool ignoreCase() ثابت { واپس m_ignoreCase; }
};
class AstConcat final : عوامی Ast_گرہ_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
عوامی:
    AstConcat(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        اگر (lhsp->dtypep() && rhsp->dtypep()) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Sized(lhsp->dtypep()->چوڑائی() + rhsp->dtypep()->چوڑائی(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { واپس "%f{%l, %k%r}"; }
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opConcat(lhs, rhs);
    }
    string emitC() override { واپس "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(concat %l %r)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 2; }
};
class AstConcatN final : عوامی Ast_گرہ_Biop {
    // String concatenate
عوامی:
    AstConcatN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { واپس "%f{%l, %k%r}"; }
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opConcatN(lhs, rhs);
    }
    string emitC() override { واپس "VL_CONCATN_NNN(%li, %ri)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
    bool stringFlavor() ثابت override { واپس true; }
};
class AstDiv final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstDiv(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opDiv(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f/ %r)"; }
    string emitC() override { واپس "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvudiv %l %r)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_DIV; }
};
class AstDivD final : عوامی Ast_گرہ_Biop {
عوامی:
    AstDivD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_AstDivD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opDivD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "/"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL_DIV; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstDivS final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstDivS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opDivS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f/ %r)"; }
    string emitC() override { واپس "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvsdiv %l %r)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_DIV; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstEqWild final : عوامی Ast_گرہ_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
عوامی:
    AstEqWild(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    ساکن Ast_گرہ_Biop* newTyped(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp);
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f==? %r)"; }
    string emitC() override {
        اگر (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ورنہ {
            واپس "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() ثابت override { واپس "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        اگر (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ورنہ {
            واپس "==";
        }
    }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstFGetS final : عوامی Ast_گرہ_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
عوامی:
    AstFGetS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtype_سیٹ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%f$fgets(%l,%r)"; }
    string emitC() override {
        واپس strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 64; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : عوامی Ast_گرہ_Biop {
عوامی:
    AstFUngetC(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        واپس "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 64; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    Ast_گرہ* filep() ثابت { واپس lhsp(); }
    Ast_گرہ* charp() ثابت { واپس rhsp(); }
};
class AstGetcN final : عوامی Ast_گرہ_Biop {
    // Verilog string.getc()
عوامی:
    AstGetcN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGetcN(lhs, rhs);
    }
    string name() ثابت override VL_MT_STABLE { واپس "getc"; }
    string emitVerilog() override { واپس "%k(%l.getc(%r))"; }
    string emitC() override { واپس "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstGetcRefN final : عوامی Ast_گرہ_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
عوامی:
    AstGetcRefN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstGt final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstGt(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGt(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f> %r)"; }
    string emitC() override { واپس "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { واپس ">"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstGtD final : عوامی Ast_گرہ_Biop {
عوامی:
    AstGtD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstGtD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGtD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس ">"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstGtN final : عوامی Ast_گرہ_Biop {
عوامی:
    AstGtN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstGtN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGtN(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس ">"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
    bool stringFlavor() ثابت override { واپس true; }
};
class AstGtS final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstGtS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGtS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f> %r)"; }
    string emitC() override { واپس "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstGte final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstGte(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGte(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f>= %r)"; }
    string emitC() override { واپس "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { واپس ">="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstGteD final : عوامی Ast_گرہ_Biop {
عوامی:
    AstGteD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstGteD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGteD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس ">="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstGteN final : عوامی Ast_گرہ_Biop {
عوامی:
    AstGteN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstGteN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGteN(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس ">="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
    bool stringFlavor() ثابت override { واپس true; }
};
class AstGteS final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstGteS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opGteS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f>= %r)"; }
    string emitC() override { واپس "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstLogAnd final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstLogAnd(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f&& %r)"; }
    string emitC() override { واپس "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvand %l %r)"; }
    string emitSimpleOperator() override { واپس "&&"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
};
class AstLogIf final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstLogIf(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f-> %r)"; }
    string emitC() override { واپس "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { واپس "->"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
};
class AstLogOr final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstLogOr(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f|| %r)"; }
    string emitC() override { واپس "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvor %l %r)"; }
    string emitSimpleOperator() override { واپس "||"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
};
class AstLt final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstLt(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLt(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f< %r)"; }
    string emitC() override { واپس "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { واپس "<"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstLtD final : عوامی Ast_گرہ_Biop {
عوامی:
    AstLtD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstLtD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLtD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "<"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstLtN final : عوامی Ast_گرہ_Biop {
عوامی:
    AstLtN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstLtN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLtN(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "<"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
    bool stringFlavor() ثابت override { واپس true; }
};
class AstLtS final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstLtS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLtS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f< %r)"; }
    string emitC() override { واپس "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstLte final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstLte(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLte(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f<= %r)"; }
    string emitC() override { واپس "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { واپس "<="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstLteD final : عوامی Ast_گرہ_Biop {
عوامی:
    AstLteD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstLteD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLteD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "<="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstLteN final : عوامی Ast_گرہ_Biop {
عوامی:
    AstLteN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstLteN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLteN(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "<="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
    bool stringFlavor() ثابت override { واپس true; }
};
class AstLteS final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstLteS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLteS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f<= %r)"; }
    string emitC() override { واپس "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstModDiv final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstModDiv(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f%% %r)"; }
    string emitC() override { واپس "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvurem %l %r)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_DIV; }
};
class AstModDivS final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstModDivS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f%% %r)"; }
    string emitC() override { واپس "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvsmod %l %r)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_DIV; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstNeqWild final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstNeqWild(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f!=? %r)"; }
    string emitC() override {
        اگر (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ورنہ {
            واپس "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        اگر (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } ورنہ {
            واپس "!=";
        }
    }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstPow final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstPow(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opPow(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f** %r)"; }
    string emitC() override { واپس "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_MUL * 10; }
};
class AstPowD final : عوامی Ast_گرہ_Biop {
عوامی:
    AstPowD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_AstPowD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opPowD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f** %r)"; }
    string emitC() override { واپس "pow(%li,%ri)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL_DIV * 5; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstPowSS final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstPowSS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f** %r)"; }
    string emitC() override { واپس "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_MUL * 10; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstPowSU final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstPowSU(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f** %r)"; }
    string emitC() override { واپس "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_MUL * 10; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstPowUS final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstPowUS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f** %r)"; }
    string emitC() override { واپس "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_MUL * 10; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstReplicate final : عوامی Ast_گرہ_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
عوامی:
    AstReplicate(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        اگر (lhsp) {
            اگر (ثابت AstConst* ثابت constp = VN_CAST(rhsp, Const)) {
                اگر (constp->عدد().isFourState()
                    || (constp->dtypep()->isSigned() && constp->عدد().isNegative())) {
                    dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Sized(lhsp->چوڑائی(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } ورنہ {
                    dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Sized(lhsp->چوڑائی() * constp->مقصد_SInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, uint32_t rep_گنتی)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_گنتی}} {}
    ASTGEN_MEMBERS_AstReplicate;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opRepl(lhs, rhs);
    }
    string emitVerilog() override { واپس "%f{%r{%k%l}}"; }
    string emitC() override { واپس "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override {
        واپس "((_ repeat " + cvt_مقصد_Str(چوڑائی() / lhsp()->چوڑائی()) + ") %l)";
    }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 2; }
};
class AstReplicateN final : عوامی Ast_گرہ_Biop {
    // String replicate
عوامی:
    AstReplicateN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opReplN(lhs, rhs);
    }
    string emitVerilog() override { واپس "%f{%r{%k%l}}"; }
    string emitC() override { واپس "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 2; }
    bool stringFlavor() ثابت override { واپس true; }
};
class AstSAnd final : عوامی Ast_گرہ_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    ثابت bool m_propertyControl;  // Parser-generated property if/case branch conjunction
عوامی:
    AstSAnd(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstSAnd;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {  // LCOV_EXCL_LINE
        واپس m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() ثابت { واپس m_propertyControl; }
};
class AstSIntersect final : عوامی Ast_گرہ_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
عوامی:
    AstSIntersect(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
    // LCOV_EXCL_STOP
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstSOr final : عوامی Ast_گرہ_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
عوامی:
    AstSOr(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstSOr;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstSThroughout final : عوامی Ast_گرہ_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
عوامی:
    AstSThroughout(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    // LCOV_EXCL_STOP
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstSWithin final : عوامی Ast_گرہ_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
عوامی:
    AstSWithin(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    // LCOV_EXCL_STOP
    bool is_Multi_چکر_Sva() ثابت override { واپس true; }
};
class AstSel final : عوامی Ast_گرہ_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    صحیح m_decl_El_چوڑائی;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    صحیح m_چوڑائی_Const;

عوامی:
    AstSel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* lsbp, صحیح bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_decl_El_چوڑائی{1}
        , m_چوڑائی_Const{bitwidth} {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Sized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, صحیح lsb, صحیح bitwidth, صحیح bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_decl_El_چوڑائی{1}
        , m_چوڑائی_Const{bitwidth} {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& from, ثابت V3_نمبر& ثنائی_ہندسہ_2) override {
        برآمد.opSel(from, ثنائی_ہندسہ_2.مقصد_UInt() + چوڑائی_Const() - 1, ثنائی_ہندسہ_2.مقصد_UInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        واپس چوڑائی_Const() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() ثابت override { واپس "((_ extract %t %r) %l)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* otherp) ثابت override {
        واپس چوڑائی_Const() == VN_DBG_AS(otherp, Sel)->چوڑائی_Const();
    }
    صحیح instr_گنتی() ثابت override {
        واپس چوڑائی_Instrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    صحیح چوڑائی_Const() ثابت { واپس m_چوڑائی_Const; }
    خالی چوڑائی_Const(صحیح قدر) { m_چوڑائی_Const = قدر; }
    صحیح lsbConst() ثابت { واپس VN_AS(lsbp(), Const)->مقصد_SInt(); }
    صحیح msbConst() ثابت { واپس lsbConst() + چوڑائی_Const() - 1; }
    VNumRange& declRange() VL_MT_STABLE { واپس m_declRange; }
    ثابت VNumRange& declRange() ثابت VL_MT_STABLE { واپس m_declRange; }
    خالی declRange(ثابت VNumRange& flag) { m_declRange = flag; }
    صحیح decl_El_چوڑائی() ثابت { واپس m_decl_El_چوڑائی; }
    خالی decl_El_چوڑائی(صحیح flag) { m_decl_El_چوڑائی = flag; }
};
class Ast_سرکاؤ_L final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    Ast_سرکاؤ_L(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, صحیح setwidth = 0)
        : ASTGEN_SUPER_سرکاؤ_L(fl, lhsp, rhsp) {
        اگر (setwidth) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ورنہ اگر (ثابت Ast_گرہ_DType* ثابت dtypep = lhsp->dtypep()) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(dtypep->چوڑائی(), dtypep->چوڑائی_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرکاؤ_L;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_سرکاؤ_L(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f<< %r)"; }
    string emitC() override { واپس "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        واپس (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class Ast_سرکاؤ_LOvr final : عوامی Ast_گرہ_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
عوامی:
    Ast_سرکاؤ_LOvr(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, صحیح setwidth = 0)
        : ASTGEN_SUPER_سرکاؤ_LOvr(fl, lhsp, rhsp) {
        اگر (setwidth) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ورنہ {
            ثابت Ast_گرہ_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(dtypep->چوڑائی(), dtypep->چوڑائی_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرکاؤ_LOvr;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_سرکاؤ_L(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f<< %r)"; }
    string emitC() override { واپس "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class Ast_سرکاؤ_R final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    Ast_سرکاؤ_R(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, صحیح setwidth = 0)
        : ASTGEN_SUPER_سرکاؤ_R(fl, lhsp, rhsp) {
        اگر (setwidth) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ورنہ اگر (ثابت Ast_گرہ_DType* ثابت dtypep = lhsp->dtypep()) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(dtypep->چوڑائی(), dtypep->چوڑائی_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرکاؤ_R;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_سرکاؤ_R(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f>> %r)"; }
    string emitC() override { واپس "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        واپس (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    // LHS size might be > output size, so don't want to force size
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class Ast_سرکاؤ_ROvr final : عوامی Ast_گرہ_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
عوامی:
    Ast_سرکاؤ_ROvr(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, صحیح setwidth = 0)
        : ASTGEN_SUPER_سرکاؤ_ROvr(fl, lhsp, rhsp) {
        اگر (setwidth) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ورنہ {
            ثابت Ast_گرہ_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(dtypep->چوڑائی(), dtypep->چوڑائی_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرکاؤ_ROvr;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_سرکاؤ_R(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f>> %r)"; }
    string emitC() override { واپس "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    // LHS size might be > output size, so don't want to force size
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class Ast_سرکاؤ_RS final : عوامی Ast_گرہ_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
عوامی:
    Ast_سرکاؤ_RS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, صحیح setwidth = 0)
        : ASTGEN_SUPER_سرکاؤ_RS(fl, lhsp, rhsp) {
        اگر (setwidth) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ورنہ اگر (ثابت Ast_گرہ_DType* ثابت dtypep = lhsp->dtypep()) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(dtypep->چوڑائی(), dtypep->چوڑائی_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرکاؤ_RS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_سرکاؤ_RS(lhs, rhs, lhsp()->چوڑائی_Min_V());
    }
    string emitVerilog() override { واپس "%k(%l %f>>> %r)"; }
    string emitSMT() ثابت override { واپس "(bvashr %l %r)"; }
    string emitC() override { واپس "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool signedFlavor() ثابت override { واپس true; }
};
class Ast_سرکاؤ_RSOvr final : عوامی Ast_گرہ_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
عوامی:
    Ast_سرکاؤ_RSOvr(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, صحیح setwidth = 0)
        : ASTGEN_SUPER_سرکاؤ_RSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        اگر (setwidth) {
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } ورنہ {
            ثابت Ast_گرہ_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(dtypep->چوڑائی(), dtypep->چوڑائی_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرکاؤ_RSOvr;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_سرکاؤ_RS(lhs, rhs, lhsp()->چوڑائی_Min_V());
    }
    string emitVerilog() override { واپس "%k(%l %f>>> %r)"; }
    string emitC() override { واپس "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstSub final : عوامی Ast_گرہ_Biop {
    // @astgen makeDfgVertex
عوامی:
    AstSub(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opSub(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f- %r)"; }
    string emitC() override { واپس "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvsub %l %r)"; }
    string emitSimpleOperator() override { واپس "-"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
};
class AstSubD final : عوامی Ast_گرہ_Biop {
عوامی:
    AstSubD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_AstSubD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opSubD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "-"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstURandomRange final : عوامی Ast_گرہ_Biop {
    // $urandom_range
عوامی:
    AstURandomRange(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtype_سیٹ_UInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%f$urandom_range(%l, %r)"; }
    string emitC() override { واپس "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : عوامی Ast_گرہ_Bi_Com {
    // @astgen makeDfgVertex
عوامی:
    AstEq(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    ساکن Ast_گرہ_Biop* newTyped(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp);
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opEq(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f== %r)"; }
    string emitC() override { واپس "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { واپس "=="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstEqCase final : عوامی Ast_گرہ_Bi_Com {
    // @astgen makeDfgVertex
عوامی:
    AstEqCase(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstEqCase;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f=== %r)"; }
    string emitC() override { واپس "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { واپس "=="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstEqD final : عوامی Ast_گرہ_Bi_Com {
عوامی:
    AstEqD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstEqD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opEqD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "=="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstEqN final : عوامی Ast_گرہ_Bi_Com {
عوامی:
    AstEqN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstEqN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opEqN(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "=="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
    bool stringFlavor() ثابت override { واپس true; }
};
class AstEqT final : عوامی Ast_گرہ_Bi_Com {
    // Equal (==) for data types
عوامی:
    AstEqT(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstEqT;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "=="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
};
class AstLogEq final : عوامی Ast_گرہ_Bi_Com {
    // @astgen makeDfgVertex
عوامی:
    AstLogEq(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f<-> %r)"; }
    string emitC() override { واپس "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { واپس "<->"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() + INSTR_گنتی_BRANCH; }
};
class AstNeq final : عوامی Ast_گرہ_Bi_Com {
    // @astgen makeDfgVertex
عوامی:
    AstNeq(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    ساکن Ast_گرہ_Biop* newTyped(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp);
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opNeq(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f!= %r)"; }
    string emitC() override { واپس "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { واپس "!="; }
    string emitSMT() ثابت override { واپس "(__Vbv (not (= %l %r)))"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstNeqCase final : عوامی Ast_گرہ_Bi_Com {
    // @astgen makeDfgVertex
عوامی:
    AstNeqCase(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f!== %r)"; }
    string emitC() override { واپس "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { واپس "!="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
};
class AstNeqD final : عوامی Ast_گرہ_Bi_Com {
عوامی:
    AstNeqD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstNeqD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "!="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstNeqN final : عوامی Ast_گرہ_Bi_Com {
عوامی:
    AstNeqN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstNeqN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "!="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
    bool stringFlavor() ثابت override { واپس true; }
};
class AstNeqT final : عوامی Ast_گرہ_Bi_Com {
    // Not-equal (!=) for data types
عوامی:
    AstNeqT(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstNeqT;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "!="; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_STR; }
};

// === AstNodeBiComAsv ===
class Ast_جوڑو final : عوامی Ast_گرہ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عوامی:
    Ast_جوڑو(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_جوڑو(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_جوڑو;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_جوڑو(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f+ %r)"; }
    string emitC() override { واپس "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvadd %l %r)"; }
    string emitSimpleOperator() override { واپس "+"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
};
class Ast_جوڑو_D final : عوامی Ast_گرہ_Bi_Com_Asv {
عوامی:
    Ast_جوڑو_D(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_جوڑو_D(fl, lhsp, rhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_Ast_جوڑو_D;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_جوڑو_D(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "+"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstAnd final : عوامی Ast_گرہ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عوامی:
    AstAnd(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opAnd(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f& %r)"; }
    string emitC() override { واپس "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvand %l %r)"; }
    string emitSimpleOperator() override { واپس "&"; }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    ثابت حرف* چوڑائی_Mismatch() ثابت override VL_MT_STABLE;
};
class AstMul final : عوامی Ast_گرہ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عوامی:
    AstMul(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opMul(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f* %r)"; }
    string emitC() override { واپس "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvmul %l %r)"; }
    string emitSimpleOperator() override { واپس "*"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_MUL; }
};
class AstMulD final : عوامی Ast_گرہ_Bi_Com_Asv {
عوامی:
    AstMulD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_AstMulD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opMulD(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "*"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstMulS final : عوامی Ast_گرہ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عوامی:
    AstMulS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opMulS(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f* %r)"; }
    string emitC() override { واپس "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvmul %l %r)"; }
    string emitSimpleOperator() override { واپس ""; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    bool سائز_Matters_Rhs() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * INSTR_گنتی_INT_MUL; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstOr final : عوامی Ast_گرہ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عوامی:
    AstOr(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opOr(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f| %r)"; }
    string emitC() override { واپس "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvor %l %r)"; }
    string emitSimpleOperator() override { واپس "|"; }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    ثابت حرف* چوڑائی_Mismatch() ثابت override VL_MT_STABLE;
};
class AstXor final : عوامی Ast_گرہ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عوامی:
    AstXor(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.opXor(lhs, rhs);
    }
    string emitVerilog() override { واپس "%k(%l %f^ %r)"; }
    string emitC() override { واپس "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { واپس "(bvxor %l %r)"; }
    string emitSimpleOperator() override { واپس "^"; }
    bool clean_برآمد() ثابت override { واپس false; }  // Lclean && Rclean
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    ثابت حرف* چوڑائی_Mismatch() ثابت override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class Ast_Dist_Chi_مربع final : عوامی Ast_گرہ_Dist_Biop {
عوامی:
    Ast_Dist_Chi_مربع(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Dist_Chi_مربع(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_Chi_مربع;
    string emitVerilog() override { واپس "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { واپس "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class Ast_Dist_قوت_نمائی final : عوامی Ast_گرہ_Dist_Biop {
عوامی:
    Ast_Dist_قوت_نمائی(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Dist_قوت_نمائی(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_قوت_نمائی;
    string emitVerilog() override { واپس "%f$dist_exponential(%l, %r)"; }
    string emitC() override { واپس "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class Ast_Dist_پوآسون final : عوامی Ast_گرہ_Dist_Biop {
عوامی:
    Ast_Dist_پوآسون(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Dist_پوآسون(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_پوآسون;
    string emitVerilog() override { واپس "%f$dist_poisson(%l, %r)"; }
    string emitC() override { واپس "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : عوامی Ast_گرہ_Dist_Biop {
عوامی:
    AstDistT(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { واپس "%f$dist_t(%l, %r)"; }
    string emitC() override { واپس "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : عوامی Ast_گرہ_Sel {
    // @astgen makeDfgVertex
    خالی init(ثابت Ast_گرہ* fromp) {
        اگر (fromp && VN_IS(fromp->dtypep()->skipRefp(), گرہ_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), گرہ_Array_DType)->subDTypep());
        }
    }

عوامی:
    AstArraySel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, صحیح ثنائی_ہندسہ_2)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, ثنائی_ہندسہ_2)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() ثابت override { واپس "(select %l %r)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isGateOptimizable() ثابت override { واپس true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
};
class AstAssocSel final : عوامی Ast_گرہ_Sel {
    خالی init(ثابت Ast_گرہ* fromp) {
        اگر (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

عوامی:
    AstAssocSel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%k(%l%f[%r])"; }
    string emitC() override { واپس "%li%k[%ri]"; }
    string emitSMT() ثابت override { واپس "(select %l %r)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isGateOptimizable() ثابت override {
        واپس !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس !isLValue(); }  // AssocSel creates on miss
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
};
class AstWildcardSel final : عوامی Ast_گرہ_Sel {
    خالی init(ثابت Ast_گرہ* fromp) {
        اگر (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

عوامی:
    AstWildcardSel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%k(%l%f[%r])"; }
    string emitC() override { واپس "%li%k[%ri]"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool isGateOptimizable() ثابت override { واپس true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
};
class Ast_لفظ_Sel final : عوامی Ast_گرہ_Sel {
    // Select a single word from a multi-word wide value
عوامی:
    Ast_لفظ_Sel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* bitp)
        : ASTGEN_SUPER_لفظ_Sel(fl, fromp, bitp) {
        dtype_سیٹ_UInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_Ast_لفظ_Sel;
    خالی نمبر_عمل_کرو(V3_نمبر&, ثابت V3_نمبر&, ثابت V3_نمبر&) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};

// === AstNodeStream ===
class Ast_دھارا_L final : عوامی Ast_گرہ_دھارا {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
عوامی:
    Ast_دھارا_L(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_دھارا_L(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_دھارا_L;
    string emitVerilog() override { واپس "%f{ << %r %k{%l} }"; }
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_دھارا_L(lhs, rhs);
    }
    string emitC() override { واپس "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 2; }
};
class Ast_دھارا_R final : عوامی Ast_گرہ_دھارا {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
عوامی:
    Ast_دھارا_R(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_دھارا_R(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_دھارا_R;
    string emitVerilog() override { واپس "%f{ >> %r %k{%l} }"; }
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.op_تفویض(lhs);
    }
    string emitC() override { واپس isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : عوامی Ast_گرہ_System_Biop_D {
عوامی:
    AstAtan2D(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.سیٹ_Double(std::atan2(lhs.مقصد_Double(), rhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$atan2(%l,%r)"; }
    string emitC() override { واپس "atan2(%li,%ri)"; }
};
class AstHypotD final : عوامی Ast_گرہ_System_Biop_D {
عوامی:
    AstHypotD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) override {
        برآمد.سیٹ_Double(std::hypot(lhs.مقصد_Double(), rhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$hypot(%l,%r)"; }
    string emitC() override { واپس "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : عوامی Ast_گرہ_CCall {
    // C++ function call
    VSelf_اشارہ_گر_متن m_self_اشارہ_گر
        = VSelf_اشارہ_گر_متن{VSelf_اشارہ_گر_متن::خالی_خانے_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
عوامی:
    AstCCall(File_سطر* fl, AstCFunc* funcp, Ast_گرہ_اظہاریہ* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    ثابت VSelf_اشارہ_گر_متن& self_اشارہ_گر() ثابت { واپس m_self_اشارہ_گر; }
    خالی self_اشارہ_گر(ثابت VSelf_اشارہ_گر_متن& self_اشارہ_گر) { m_self_اشارہ_گر = self_اشارہ_گر; }
    string self_اشارہ_گر_Protect(bool useSelfForThis) ثابت {
        واپس self_اشارہ_گر().protect(useSelfForThis, protect());
    }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
};
class AstCMethodCall final : عوامی Ast_گرہ_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
عوامی:
    AstCMethodCall(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, AstCFunc* funcp, Ast_گرہ_اظہاریہ* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : عوامی Ast_گرہ_CCall {
    // C++ new() call
عوامی:
    AstCNew(File_سطر* fl, AstCFunc* funcp, Ast_گرہ_اظہاریہ* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : عوامی Ast_گرہ_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, Ast_Cons_Array_شے*> m_itemps;  // The item at each index given

عوامی:
    AstConsArray(File_سطر* fl, Ast_گرہ_DType* newDTypep, Ast_گرہ_اظہاریہ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    ثابت حرف* broken() ثابت override;
    خالی cloneRelink() override;
    اندرونی Ast_Cons_Array_شے* itemsp() ثابت VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_گرہ_اظہاریہ* valuep(uint64_t اشاریہ) ثابت;
    // Add the value of the element at the index, which must not be given yet
    خالی valuep(uint64_t اشاریہ, Ast_گرہ_اظہاریہ* newp);
    // Add the elements of the array 'arrayp', starting at the index
    خالی splice_اقدار(uint64_t اشاریہ, Ast_گرہ_اظہاریہ* arrayp);
};
class AstConsAssoc final : عوامی Ast_گرہ_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
عوامی:
    explicit AstConsAssoc(File_سطر* fl, Ast_گرہ_اظہاریہ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        واپس nullptr;
    }
};
class AstConsDynArray final : عوامی Ast_گرہ_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
عوامی:
    explicit AstConsDynArray(File_سطر* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    ثابت حرف* broken() ثابت override;
};
class Ast_Cons_قطار final : عوامی Ast_گرہ_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
عوامی:
    explicit Ast_Cons_قطار(File_سطر* fl)
        : ASTGEN_SUPER_Cons_قطار(fl) {}
    ASTGEN_MEMBERS_Ast_Cons_قطار;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), قطار_DType));
        واپس nullptr;
    }
};
class AstConsStruct final : عوامی Ast_گرہ_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
عوامی:
    AstConsStruct(File_سطر* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    ثابت حرف* broken() ثابت override;
    // The value of the member called 'name'
    Ast_گرہ_اظہاریہ* memberValuep(ثابت string& name) ثابت;
};
class AstConsWildcard final : عوامی Ast_گرہ_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
عوامی:
    explicit AstConsWildcard(File_سطر* fl, Ast_گرہ_اظہاریہ* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        واپس nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : عوامی Ast_گرہ_FTask_Ref {
    // A reference to a function
عوامی:
    اندرونی AstFuncRef(File_سطر* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_سطر* fl, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class Ast_طریقہ_Call final : عوامی Ast_گرہ_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
عوامی:
    Ast_طریقہ_Call(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, VFlag_بچہ_DType, ثابت string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_طریقہ_Call(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_طریقہ_Call(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_طریقہ_Call(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_Ast_طریقہ_Call;
};
class AstNew final : عوامی Ast_گرہ_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
عوامی:
    AstNew(File_سطر* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool isImplicit() ثابت { واپس m_isImplicit; }
    خالی isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() ثابت { واپس m_isScoped; }
    خالی isScoped(bool flag) { m_isScoped = flag; }
    bool is_خالص() override { واپس false; }
};
class AstTaskRef final : عوامی Ast_گرہ_FTask_Ref {
    // A reference to a task
عوامی:
    اندرونی AstTaskRef(File_سطر* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_سطر* fl, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtype_سیٹ_Void();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class Ast_Sel_ثنائی_ہندسہ final : عوامی Ast_گرہ_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
نجی:
    VAccess m_access;  // Left hand side assignment
عوامی:
    Ast_Sel_ثنائی_ہندسہ(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* bitp)
        : ASTGEN_SUPER_Sel_ثنائی_ہندسہ(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_Ast_Sel_ثنائی_ہندسہ;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    VAccess access() ثابت { واپس m_access; }
    خالی access(ثابت VAccess& flag) { m_access = flag; }
};
class Ast_Sel_نکالو final : عوامی Ast_گرہ_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
عوامی:
    Ast_Sel_نکالو(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* msbp, Ast_گرہ_اظہاریہ* lsbp)
        : ASTGEN_SUPER_Sel_نکالو(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_Ast_Sel_نکالو;
};
class AstSelMinus final : عوامی Ast_گرہ_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
عوامی:
    AstSelMinus(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* bitp, Ast_گرہ_اظہاریہ* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : عوامی Ast_گرہ_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
عوامی:
    AstSelPlus(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_اظہاریہ* bitp, Ast_گرہ_اظہاریہ* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_گنتی_بٹ final : عوامی Ast_گرہ_Quadop {
    // Number of bits set in vector
عوامی:
    Ast_گنتی_بٹ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* ctrl1p)
        : ASTGEN_SUPER_گنتی_بٹ(fl, exprp, ctrl1p, ctrl1p->clone_درخت_خالص(false),
                                 ctrl1p->clone_درخت_خالص(false)) {
        dtype_سیٹ_Int();
    }
    Ast_گنتی_بٹ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* ctrl1p, Ast_گرہ_اظہاریہ* ctrl2p)
        : ASTGEN_SUPER_گنتی_بٹ(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_درخت_خالص(false)) {
        dtype_سیٹ_Int();
    }
    Ast_گنتی_بٹ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_گرہ_اظہاریہ* ctrl1p, Ast_گرہ_اظہاریہ* ctrl2p,
                 Ast_گرہ_اظہاریہ* ctrl3p)
        : ASTGEN_SUPER_گنتی_بٹ(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtype_سیٹ_Int();
    }
    ASTGEN_MEMBERS_Ast_گنتی_بٹ;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& اظہاریہ, ثابت V3_نمبر& ctrl1,
                       ثابت V3_نمبر& ctrl2, ثابت V3_نمبر& ctrl3) override {
        برآمد.op_گنتی_بٹ(اظہاریہ, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { واپس "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool cleanThs() ثابت override { واپس true; }
    bool cleanFhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool سائز_Matters_Ths() ثابت override { واپس false; }
    bool سائز_Matters_Fhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : عوامی Ast_گرہ_Termop {
عوامی:
    explicit AstInferredDisable(File_سطر* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { واپس "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool isSystemFunc() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_وقت final : عوامی Ast_گرہ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
عوامی:
    Ast_وقت(File_سطر* fl, ثابت VTimescale& timeunit)
        : ASTGEN_SUPER_وقت(fl)
        , m_timeunit{timeunit} {
        dtype_سیٹ_UInt64();
    }
    ASTGEN_MEMBERS_Ast_وقت;
    string emitVerilog() override { واپس "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_وقت; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class Ast_وقت_D final : عوامی Ast_گرہ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
عوامی:
    Ast_وقت_D(File_سطر* fl, ثابت VTimescale& timeunit)
        : ASTGEN_SUPER_وقت_D(fl)
        , m_timeunit{timeunit} {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_Ast_وقت_D;
    string emitVerilog() override { واپس "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_وقت; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : عوامی Ast_گرہ_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
عوامی:
    AstCond(File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ_اظہاریہ* thenp, Ast_گرہ_اظہاریہ* elsep);
    ASTGEN_MEMBERS_AstCond;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs,
                       ثابت V3_نمبر& ths) override {
        برآمد.op_تفویض(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { واپس "%k(%l %f? %r %k: %t)"; }
    string emitC() override { واپس "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() ثابت override { واپس "(ite (__Vbool %l) %r %t)"; }
    bool clean_برآمد() ثابت override { واپس false; }  // clean if e1 & e2 clean
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس false; }
    bool cleanThs() ثابت override { واپس false; }  // Propagates up
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool سائز_Matters_Ths() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_BRANCH; }
};
class AstPutcN final : عوامی Ast_گرہ_Triop {
    // Verilog string.putc()
عوامی:
    AstPutcN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ_اظہاریہ* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_AstPutcN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs,
                       ثابت V3_نمبر& ths) override {
        برآمد.opPutcN(lhs, rhs, ths);
    }
    string name() ثابت override VL_MT_STABLE { واپس "putc"; }
    string emitVerilog() override { واپس "%k(%l.putc(%r,%t))"; }
    string emitC() override { واپس "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool cleanThs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool سائز_Matters_Ths() ثابت override { واپس false; }
};
class Ast_قاش_Sel final : عوامی Ast_گرہ_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
عوامی:
    Ast_قاش_Sel(File_سطر* fl, Ast_گرہ_اظہاریہ* fromp, ثابت VNumRange& declRange)
        : ASTGEN_SUPER_قاش_Sel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_Ast_قاش_Sel;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& from, ثابت V3_نمبر& lo,
                       ثابت V3_نمبر& چوڑائی) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool cleanRhs() ثابت override { واپس true; }
    bool cleanThs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool سائز_Matters_Ths() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { واپس m_declRange; }
    ثابت VNumRange& declRange() ثابت VL_MT_STABLE { واپس m_declRange; }
};
class AstSubstrN final : عوامی Ast_گرہ_Triop {
    // Verilog string.substr()
عوامی:
    AstSubstrN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ_اظہاریہ* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs,
                       ثابت V3_نمبر& ths) override {
        برآمد.opSubstrN(lhs, rhs, ths);
    }
    string name() ثابت override VL_MT_STABLE { واپس "substr"; }
    string emitVerilog() override { واپس "%k(%l.substr(%r,%t))"; }
    string emitC() override { واپس "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { واپس ""; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool cleanRhs() ثابت override { واپس true; }
    bool cleanThs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool سائز_Matters_Rhs() ثابت override { واپس false; }
    bool سائز_Matters_Ths() ثابت override { واپس false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : عوامی Ast_گرہ_Dist_Triop {
عوامی:
    AstDistErlang(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ_اظہاریہ* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { واپس "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { واپس "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class Ast_Dist_معمول final : عوامی Ast_گرہ_Dist_Triop {
عوامی:
    Ast_Dist_معمول(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ_اظہاریہ* thsp)
        : ASTGEN_SUPER_Dist_معمول(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_معمول;
    string emitVerilog() override { واپس "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { واپس "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class Ast_Dist_یکساں final : عوامی Ast_گرہ_Dist_Triop {
عوامی:
    Ast_Dist_یکساں(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp, Ast_گرہ_اظہاریہ* thsp)
        : ASTGEN_SUPER_Dist_یکساں(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_یکساں;
    string emitVerilog() override { واپس "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { واپس "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : عوامی Ast_گرہ_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
عوامی:
    شمار FmtType : صحیح { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

نجی:
    // dist-ast-dump-suppress  // Part of name()
    ثابت FmtType m_fmtType;  // Operation type
عوامی:
    AstAtoN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtype_سیٹ_Double() : dtype_سیٹ_Integer();
    }
    ASTGEN_MEMBERS_AstAtoN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opAtoN(lhs, m_fmtType); }
    string name() ثابت override VL_MT_STABLE {
        انتخاب (m_fmtType) {
        حالت ATOI: واپس "atoi";
        حالت ATOHEX: واپس "atohex";
        حالت ATOOCT: واپس "atooct";
        حالت ATOBIN: واپس "atobin";
        حالت ATOREAL: واپس "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { واپس "%l." + name() + "()"; }
    string emitC() override {
        انتخاب (m_fmtType) {
        حالت ATOI: واپس "VL_ATOI_N(%li, 10)";
        حالت ATOHEX: واپس "VL_ATOI_N(%li, 16)";
        حالت ATOOCT: واپس "VL_ATOI_N(%li, 8)";
        حالت ATOBIN: واپس "VL_ATOI_N(%li, 2)";
        حالت ATOREAL: واپس "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    FmtType fmtType() ثابت { واپس m_fmtType; }
};
class Ast_بٹ_مقصد_Real_D final : عوامی Ast_گرہ_Uniop {
عوامی:
    Ast_بٹ_مقصد_Real_D(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_بٹ_مقصد_Real_D(fl, lhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_Ast_بٹ_مقصد_Real_D;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_بٹ_مقصد_Real_D(lhs); }
    string emitVerilog() override { واپس "%f$bitstoreal(%l)"; }
    string emitC() override { واپس "VL_CVT_D_Q(%li)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }  // Eliminated before matters
    bool سائز_Matters_Lhs() ثابت override { واپس false; }  // Eliminated before matters
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstCCast final : عوامی Ast_گرہ_Uniop {
    // Cast to C-based data type
    صحیح m_سائز;

عوامی:
    AstCCast(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, صحیح setwidth, صحیح minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_سائز = setwidth;
        اگر (setwidth) {
            اگر (minwidth == -1) minwidth = setwidth;
            dtype_سیٹ_ثنائی_ہندسہ_Unsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_سائز = چوڑائی();
    }
    ASTGEN_MEMBERS_AstCCast;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_تفویض(lhs); }
    string emitVerilog() override { واپس "%f$_CAST(%l)"; }
    string emitC() override { واپس "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }  // Special cased in V3Cast
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس سائز() == VN_DBG_AS(samep, CCast)->سائز();
    }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    //
    صحیح سائز() ثابت { واپس m_سائز; }
};
class AstCLog2 final : عوامی Ast_گرہ_Uniop {
عوامی:
    AstCLog2(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtype_سیٹ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opCLog2(lhs); }
    string emitVerilog() override { واپس "%f$clog2(%l)"; }
    string emitC() override { واپس "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 16; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstCastWrap final : عوامی Ast_گرہ_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
عوامی:
    AstCastWrap(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_تفویض(lhs); }
    string emitVerilog() override { واپس "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس 0; }
};
class Ast_گنتی_Ones final : عوامی Ast_گرہ_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
عوامی:
    Ast_گنتی_Ones(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_گنتی_Ones(fl, lhsp) {
        dtype_سیٹ_Int();
    }
    ASTGEN_MEMBERS_Ast_گنتی_Ones;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_گنتی_Ones(lhs); }
    string emitVerilog() override { واپس "%f$countones(%l)"; }
    string emitC() override { واپس "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 16; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstCvtPackString final : عوامی Ast_گرہ_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
عوامی:
    AstCvtPackString(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstExtend final : عوامی Ast_گرہ_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
عوامی:
    AstExtend(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, صحیح چوڑائی, صحیح چوڑائی_Min = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(چوڑائی, چوڑائی_Min ? چوڑائی_Min : چوڑائی, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_تفویض(lhs); }
    string emitVerilog() override { واپس "%l"; }
    string emitC() override { واپس "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override {
        واپس "((_ zero_extend " + cvt_مقصد_Str(چوڑائی() - lhsp()->چوڑائی()) + ") %l)";
    }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override {
        واپس false;  // Because the EXTEND operator self-casts
    }
    صحیح instr_گنتی() ثابت override { واپس 0; }
};
class AstExtendS final : عوامی Ast_گرہ_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
عوامی:
    AstExtendS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, صحیح چوڑائی)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Sized(چوڑائی, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.opExtendS(lhs, lhsp()->چوڑائی_Min_V());
    }
    string emitVerilog() override { واپس "%l"; }
    string emitC() override { واپس "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override {
        واپس "((_ sign_extend " + cvt_مقصد_Str(چوڑائی() - lhsp()->چوڑائی()) + ") %l)";
    }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override {
        واپس false;  // Because the EXTEND operator self-casts
    }
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool signedFlavor() ثابت override { واپس true; }
};
class AstFEof final : عوامی Ast_گرہ_Uniop {
عوامی:
    AstFEof(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%f$feof(%l)"; }
    string emitC() override { واپس "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 16; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ثابت override { واپس true; }
    Ast_گرہ* filep() ثابت { واپس lhsp(); }
};
class AstFGetC final : عوامی Ast_گرہ_Uniop {
عوامی:
    AstFGetC(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtype_سیٹ_Logic2_State_Unsized(
            32, v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::MATCHES_چوڑائی ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { واپس "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 64; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    Ast_گرہ* filep() ثابت { واپس lhsp(); }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstISToRD final : عوامی Ast_گرہ_Uniop {
    // $itor where lhs is signed
عوامی:
    AstISToRD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_AstISToRD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opISToRD(lhs); }
    string emitVerilog() override { واپس "%f$itor($signed(%l))"; }
    string emitC() override { واپس "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_الفاظ() override { واپس true; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstIToRD final : عوامی Ast_گرہ_Uniop {
    // $itor where lhs is unsigned
عوامی:
    AstIToRD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_AstIToRD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opIToRD(lhs); }
    string emitVerilog() override { واپس "%f$itor(%l)"; }
    string emitC() override { واپس "VL_ITOR_D_%lq(%lw, %li)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstIsUnbounded final : عوامی Ast_گرہ_Uniop {
    // True if is unbounded ($)
عوامی:
    AstIsUnbounded(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر&) override {
        // Any constant isn't unbounded
        برآمد.سیٹ_Zero();
    }
    string emitVerilog() override { واپس "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstIsUnknown final : عوامی Ast_گرہ_Uniop {
    // True if any unknown bits
عوامی:
    AstIsUnknown(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opIsUnknown(lhs); }
    string emitVerilog() override { واپس "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstLenN final : عوامی Ast_گرہ_Uniop {
    // Length of a string
عوامی:
    AstLenN(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtype_سیٹ_Int();
    }
    ASTGEN_MEMBERS_AstLenN;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opLenN(lhs); }
    string emitVerilog() override { واپس "%f(%l)"; }
    string emitC() override { واپس "VL_LEN_IN(%li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
};
class AstLogNot final : عوامی Ast_گرہ_Uniop {
    // @astgen makeDfgVertex
نجی:
    ثابت bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
عوامی:
    AstLogNot(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opLogNot(lhs); }
    string emitVerilog() override { واپس "%f(! %l)"; }
    string emitC() override { واپس "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override { واپس "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { واپس "!"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool fromProperty() ثابت { واپس m_fromProperty; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class Ast_Most_سیٹ_ثنائی_ہندسہ_P1 final : عوامی Ast_گرہ_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
عوامی:
    Ast_Most_سیٹ_ثنائی_ہندسہ_P1(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Most_سیٹ_ثنائی_ہندسہ_P1(fl, lhsp) {
        dtype_سیٹ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_Most_سیٹ_ثنائی_ہندسہ_P1;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_Most_سیٹ_ثنائی_ہندسہ_P1(lhs); }
    string emitVerilog() override { واپس "%f$mostsetbitp1(%l)"; }
    string emitC() override { واپس "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 16; }
};
class AstNToI final : عوامی Ast_گرہ_Uniop {
    // String to any-size integral
عوامی:
    AstNToI(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opNToI(lhs); }
    string emitVerilog() override { واپس "'(%l)"; }
    string emitC() override { واپس "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
};
class AstNegate final : عوامی Ast_گرہ_Uniop {
    // @astgen makeDfgVertex
عوامی:
    AstNegate(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opNegate(lhs); }
    string emitVerilog() override { واپس "%f(- %l)"; }
    string emitC() override { واپس "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() ثابت override { واپس "(bvneg %l)"; }
    string emitSimpleOperator() override { واپس "-"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
};
class AstNegateD final : عوامی Ast_گرہ_Uniop {
عوامی:
    AstNegateD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtype_سیٹ_Double();
    }
    ASTGEN_MEMBERS_AstNegateD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opNegateD(lhs); }
    string emitVerilog() override { واپس "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { واپس "-"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool doubleFlavor() ثابت override { واپس true; }
};
class AstNot final : عوامی Ast_گرہ_Uniop {
    // @astgen makeDfgVertex
عوامی:
    AstNot(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opNot(lhs); }
    string emitVerilog() override { واپس "%f(~ %l)"; }
    string emitC() override { واپس "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() ثابت override { واپس "(bvnot %l)"; }
    string emitSimpleOperator() override { واپس "~"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس true; }
    ثابت حرف* چوڑائی_Mismatch() ثابت override VL_MT_STABLE;
};
class AstNullCheck final : عوامی Ast_گرہ_Uniop {
    // Return LHS after checking that LHS is non-null
عوامی:
    AstNullCheck(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { V3ERROR_NA; }
    صحیح instr_گنتی() ثابت override { واپس 1; }  // Rarely executes
    string emitVerilog() override { واپس "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override { واپس fileline() == samep->fileline(); }
};
class AstOneHot final : عوامی Ast_گرہ_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
عوامی:
    AstOneHot(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstOneHot;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opOneHot(lhs); }
    string emitVerilog() override { واپس "%f$onehot(%l)"; }
    string emitC() override { واپس "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 4; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstOneHot0 final : عوامی Ast_گرہ_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
عوامی:
    AstOneHot0(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opOneHot0(lhs); }
    string emitVerilog() override { واپس "%f$onehot0(%l)"; }
    string emitC() override { واپس "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs() * 3; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstPostDec final : عوامی Ast_گرہ_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
عوامی:
    AstPostDec(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { واپس "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool سائز_Matters_Lhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : عوامی Ast_گرہ_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
عوامی:
    AstPostInc(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { واپس "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool سائز_Matters_Lhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : عوامی Ast_گرہ_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
عوامی:
    AstPreDec(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { واپس "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool سائز_Matters_Lhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : عوامی Ast_گرہ_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
عوامی:
    AstPreInc(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { واپس "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool سائز_Matters_Lhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : عوامی Ast_گرہ_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
عوامی:
    AstRToIRoundS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtype_سیٹ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opRToIRoundS(lhs); }
    string emitVerilog() override { واپس "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        واپس isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstRToIS final : عوامی Ast_گرہ_Uniop {
    // $rtoi(lhs)
عوامی:
    AstRToIS(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtype_سیٹ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opRToIS(lhs); }
    string emitVerilog() override { واپس "%f$rtoi(%l)"; }
    string emitC() override { واپس "VL_RTOI_I_D(%li)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }  // Eliminated before matters
    bool سائز_Matters_Lhs() ثابت override { واپس false; }  // Eliminated before matters
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class Ast_Real_مقصد_بٹ final : عوامی Ast_گرہ_Uniop {
عوامی:
    Ast_Real_مقصد_بٹ(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Real_مقصد_بٹ(fl, lhsp) {
        dtype_سیٹ_UInt64();
    }
    ASTGEN_MEMBERS_Ast_Real_مقصد_بٹ;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_Real_مقصد_بٹ(lhs); }
    string emitVerilog() override { واپس "%f$realtobits(%l)"; }
    string emitC() override { واپس "VL_CVT_Q_D(%li)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }  // Eliminated before matters
    bool سائز_Matters_Lhs() ثابت override { واپس false; }  // Eliminated before matters
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_DBL; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class AstRedAnd final : عوامی Ast_گرہ_Uniop {
    // @astgen makeDfgVertex
عوامی:
    AstRedAnd(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opRedAnd(lhs); }
    string emitVerilog() override { واپس "%f(& %l)"; }
    string emitC() override { واپس "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
};
class AstRedOr final : عوامی Ast_گرہ_Uniop {
    // @astgen makeDfgVertex
عوامی:
    AstRedOr(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opRedOr(lhs); }
    string emitVerilog() override { واپس "%f(| %l)"; }
    string emitC() override { واپس "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
};
class AstRedXor final : عوامی Ast_گرہ_Uniop {
    // @astgen makeDfgVertex
عوامی:
    AstRedXor(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.opRedXor(lhs); }
    string emitVerilog() override { واپس "%f(^ %l)"; }
    string emitC() override { واپس "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override {
        ثابت صحیح w = lhsp()->چوڑائی();
        واپس (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس 1 + V3_نمبر::log2b(چوڑائی()); }
};
class AstResizeLValue final : عوامی Ast_گرہ_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
عوامی:
    AstResizeLValue(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    صحیح instr_گنتی() ثابت override { واپس 0; }
};
class AstSigned final : عوامی Ast_گرہ_Uniop {
    // $signed(lhs)
عوامی:
    AstSigned(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.op_تفویض(lhs);
        برآمد.isSigned(false);
    }
    string emitVerilog() override { واپس "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }  // Eliminated before matters
    bool سائز_Matters_Lhs() ثابت override { واپس true; }  // Eliminated before matters
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool isSystemFunc() ثابت override { واپس true; }
};
class Ast_وقت_Import final : عوامی Ast_گرہ_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
عوامی:
    Ast_وقت_Import(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_وقت_Import(fl, lhsp) {}
    ASTGEN_MEMBERS_Ast_وقت_Import;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class Ast_مقصد_زیریں_N final : عوامی Ast_گرہ_Uniop {
    // string.tolower()
عوامی:
    Ast_مقصد_زیریں_N(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_مقصد_زیریں_N(fl, lhsp) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_Ast_مقصد_زیریں_N;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_مقصد_زیریں_N(lhs); }
    string emitVerilog() override { واپس "%l.tolower()"; }
    string emitC() override { واپس "VL_TOLOWER_NN(%li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
};
class Ast_مقصد_String_N final : عوامی Ast_گرہ_Uniop {
عوامی:
    Ast_مقصد_String_N(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_مقصد_String_N(fl, lhsp) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_Ast_مقصد_String_N;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { واپس "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        واپس isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
};
class Ast_مقصد_بالائی_N final : عوامی Ast_گرہ_Uniop {
    // string.toupper()
عوامی:
    Ast_مقصد_بالائی_N(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_مقصد_بالائی_N(fl, lhsp) {
        dtype_سیٹ_String();
    }
    ASTGEN_MEMBERS_Ast_مقصد_بالائی_N;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override { برآمد.op_مقصد_بالائی_N(lhs); }
    string emitVerilog() override { واپس "%l.toupper()"; }
    string emitC() override { واپس "VL_TOUPPER_NN(%li)"; }
    bool clean_برآمد() ثابت override { واپس true; }
    bool cleanLhs() ثابت override { واپس true; }
    bool سائز_Matters_Lhs() ثابت override { واپس false; }
};
class AstUnsigned final : عوامی Ast_گرہ_Uniop {
    // $unsigned(lhs)
عوامی:
    AstUnsigned(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.op_تفویض(lhs);
        برآمد.isSigned(false);
    }
    string emitVerilog() override { واپس "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس false; }
    bool cleanLhs() ثابت override { واپس false; }  // Eliminated before matters
    bool سائز_Matters_Lhs() ثابت override { واپس true; }  // Eliminated before matters
    صحیح instr_گنتی() ثابت override { واپس 0; }
    bool isSystemFunc() ثابت override { واپس true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstAcosD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::acos(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$acos(%l)"; }
    string emitC() override { واپس "acos(%li)"; }
};
class AstAcoshD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstAcoshD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::acosh(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$acosh(%l)"; }
    string emitC() override { واپس "acosh(%li)"; }
};
class AstAsinD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstAsinD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::asin(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$asin(%l)"; }
    string emitC() override { واپس "asin(%li)"; }
};
class AstAsinhD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstAsinhD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::asinh(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$asinh(%l)"; }
    string emitC() override { واپس "asinh(%li)"; }
};
class AstAtanD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstAtanD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::atan(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$atan(%l)"; }
    string emitC() override { واپس "atan(%li)"; }
};
class AstAtanhD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstAtanhD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::atanh(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$atanh(%l)"; }
    string emitC() override { واپس "atanh(%li)"; }
};
class AstCeilD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstCeilD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::ceil(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$ceil(%l)"; }
    string emitC() override { واپس "ceil(%li)"; }
};
class AstCosD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstCosD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::جیب_تمام(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$cos(%l)"; }
    string emitC() override { واپس "cos(%li)"; }
};
class AstCoshD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstCoshD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::cosh(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$cosh(%l)"; }
    string emitC() override { واپس "cosh(%li)"; }
};
class AstExpD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstExpD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::قوت_نما(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$exp(%l)"; }
    string emitC() override { واپس "exp(%li)"; }
};
class AstFloorD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstFloorD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::floor(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$floor(%l)"; }
    string emitC() override { واپس "floor(%li)"; }
};
class AstLog10D final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstLog10D(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::log10(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$log10(%l)"; }
    string emitC() override { واپس "log10(%li)"; }
};
class AstLogD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstLogD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::لوگارتھم(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$ln(%l)"; }
    string emitC() override { واپس "log(%li)"; }
};
class AstSinD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstSinD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::جیب(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$sin(%l)"; }
    string emitC() override { واپس "sin(%li)"; }
};
class AstSinhD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstSinhD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::sinh(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$sinh(%l)"; }
    string emitC() override { واپس "sinh(%li)"; }
};
class AstSqrtD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstSqrtD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::جذر(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$sqrt(%l)"; }
    string emitC() override { واپس "sqrt(%li)"; }
};
class AstTanD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstTanD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::tan(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$tan(%l)"; }
    string emitC() override { واپس "tan(%li)"; }
};
class AstTanhD final : عوامی Ast_گرہ_System_Uniop_D {
عوامی:
    AstTanhD(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    خالی نمبر_عمل_کرو(V3_نمبر& برآمد, ثابت V3_نمبر& lhs) override {
        برآمد.سیٹ_Double(std::tanh(lhs.مقصد_Double()));
    }
    string emitVerilog() override { واپس "%f$tanh(%l)"; }
    string emitC() override { واپس "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : عوامی Ast_گرہ_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
عوامی:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    اندرونی AstVarRef(File_سطر* fl, AstVar* varp, ثابت VAccess& access);
    اندرونی AstVarRef(File_سطر* fl, Ast_گرہ_Module* classOrPackagep, AstVar* varp,
                     ثابت VAccess& access);
    // This form only allowed post-link (see above)
    اندرونی AstVarRef(File_سطر* fl, AstVarScope* varscp, ثابت VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() ثابت { واپس m_name; }
    اندرونی string name() ثابت override;  // * = Var name
    خالی name(ثابت string& name) override { m_name = name; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    اندرونی bool same_گرہ(ثابت AstVarRef* samep) ثابت;
    اندرونی bool sameNoLvalue(ثابت AstVarRef* samep) ثابت;
    صحیح instr_گنتی() ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
};
class AstVarXRef final : عوامی Ast_گرہ_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_شامل_ہے_Gen_بلاک = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
عوامی:
    AstVarXRef(File_سطر* fl, ثابت string& name, ثابت string& dotted, ثابت VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    اندرونی AstVarXRef(File_سطر* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    خالی name(ثابت std::string& name) override { m_name = name; }  // * = Var name
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string dotted() ثابت { واپس m_dotted; }
    خالی dotted(ثابت string& dotted) { m_dotted = dotted; }
    string inlinedDots() ثابت { واپس m_inlinedDots; }
    خالی inlinedDots(ثابت string& flag) { m_inlinedDots = flag; }
    bool شامل_ہے_Gen_بلاک() ثابت { واپس m_شامل_ہے_Gen_بلاک; }
    خالی شامل_ہے_Gen_بلاک(ثابت bool flag) { m_شامل_ہے_Gen_بلاک = flag; }
    bool readOnlyModport() ثابت { واپس m_readOnlyModport; }
    خالی readOnlyModport(ثابت bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_برآمد() ثابت override { واپس true; }
    صحیح instr_گنتی() ثابت override { واپس چوڑائی_Instrs(); }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        واپس (self_اشارہ_گر() == asamep->self_اشارہ_گر() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#اگر_ختم  // Guard
