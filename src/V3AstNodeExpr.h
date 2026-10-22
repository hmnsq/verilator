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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ASTNODEEXPR_H_
#وصف_ڪريو VERILATOR_V3ASTNODEEXPR_H_

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3AST_H_
#غلطي "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#وصف_ڪريو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#جيڪڏهن_پورو

// === Abstract base node types (AstNode*) =====================================

class Ast_ڳنڍ_اظهار VL_NOT_FINAL : عام Ast_ڳنڍ {
    // An expression tree node
محمي:
    Ast_ڳنڍ_اظهار(VNType t, File_سٽ* fl)
        : Ast_ڳنڍ{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_اظهار;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // Every expression must have a data type after V3Width
    bool hasDType() ثابت override final VL_MT_محفوظ { أرجع true; }
    مجازي string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    مجازي string emitC() = 0;
    مجازي string emitSMT() ثابت { أرجع ""; };
    مجازي string emitSimpleOperator() { أرجع ""; }  // "" means not ok to use
    مجازي bool emit_Check_Max_لفظ() { أرجع false; }  // Check VL_MULS_MAX_WORDS
    مجازي bool clean_ٻاهريون() ثابت = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    مجازي bool is_Multi_چڪر_Sva() ثابت { أرجع false; }
    ثابت Ast_ڳنڍ_اظهار* get_VAccess_هدف_Recurse() ثابت;
    Ast_ڳنڍ_اظهار* get_VAccess_هدف_Recurse() {
        أرجع const_cast<Ast_ڳنڍ_اظهار*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<ثابت Ast_ڳنڍ_اظهار*>(this)->get_VAccess_هدف_Recurse());
    }
    VAccess getVAccessRecurse() ثابت;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() ثابت { أرجع getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_وڻ() ثابت;
    // Return base var (or const) nodep dereferences
    Ast_ڳنڍ* بنياد_Fromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    مضمن Ast_Stmt_اظهار* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    مجازي ثابت حرف* ويڪر_Mismatch() ثابت VL_MT_STABLE { أرجع nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string نمونو_String(uint32_t اونهائي = 0) ثابت;
    bool isFourstateOrNoDtype() ثابت {
        إذا (!dtypep()) أرجع true;
        أرجع dtypep()->skipRefp()->isFourstate();
    }
    ساكن bool isFourstateOrNoDtype(ثابت Ast_ڳنڍ_اظهار* ثابت expr1p) {
        أرجع expr1p->isFourstateOrNoDtype();
    }
    ساكن bool isFourstateOrNoDtype(ثابت Ast_ڳنڍ_اظهار* ثابت expr1p,
                                     ثابت Ast_ڳنڍ_اظهار* ثابت expr2p) {
        أرجع expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    ساكن bool isFourstateOrNoDtype(ثابت Ast_ڳنڍ_اظهار* ثابت expr1p,
                                     ثابت Ast_ڳنڍ_اظهار* ثابت expr2p,
                                     ثابت Ast_ڳنڍ_اظهار* ثابت expr3p) {
        أرجع isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    ساكن bool isFourstateOrNoDtype(ثابت Ast_ڳنڍ_اظهار* ثابت expr1p,
                                     ثابت Ast_ڳنڍ_اظهار* ثابت expr2p,
                                     ثابت Ast_ڳنڍ_اظهار* ثابت expr3p,
                                     ثابت Ast_ڳنڍ_اظهار* ثابت expr4p) {
        أرجع isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_ڳنڍ_Biop VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محمي:
    Ast_ڳنڍ_Biop(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : Ast_ڳنڍ_اظهار{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    مجازي فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) = 0;
    مجازي bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    مجازي bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    مجازي bool سائيز_Matters_Lhs() ثابت = 0;  // True if output result depends on lhs size
    مجازي bool سائيز_Matters_Rhs() ثابت = 0;  // True if output result depends on rhs size
    مجازي bool doubleFlavor() ثابت { أرجع false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    مجازي bool signedFlavor() ثابت { أرجع false; }
    مجازي bool stringFlavor() ثابت { أرجع false; }  // N flavor of nodes with both flavors?
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع true; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;

خاص:
    bool getPurityRecurse() ثابت { أرجع lhsp()->is_خالص() && rhsp()->is_خالص(); }
};
class Ast_ڳنڍ_Bi_Com VL_NOT_FINAL : عام Ast_ڳنڍ_Biop {
    // Binary expr with commutative properties
محمي:
    Ast_ڳنڍ_Bi_Com(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhs, Ast_ڳنڍ_اظهار* rhs)
        : Ast_ڳنڍ_Biop{t, fl, lhs, rhs} {}

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Bi_Com;
};
class Ast_ڳنڍ_Bi_Com_Asv VL_NOT_FINAL : عام Ast_ڳنڍ_Bi_Com {
    // Binary expr with commutative & associative properties
محمي:
    Ast_ڳنڍ_Bi_Com_Asv(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhs, Ast_ڳنڍ_اظهار* rhs)
        : Ast_ڳنڍ_Bi_Com{t, fl, lhs, rhs} {}

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Bi_Com_Asv;
};
class Ast_ڳنڍ_Dist_Biop VL_NOT_FINAL : عام Ast_ڳنڍ_Biop {
عام:
    Ast_ڳنڍ_Dist_Biop(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : Ast_ڳنڍ_Biop{t, fl, lhsp, rhsp} {
        dtype_سيٽ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ڳنڍ_Dist_Biop;
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL_TRIG; }
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
};
class Ast_ڳنڍ_Sel VL_NOT_FINAL : عام Ast_ڳنڍ_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
محمي:
    Ast_ڳنڍ_Sel(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* bitp)
        : Ast_ڳنڍ_Biop{t, fl, fromp, bitp} {}

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Sel;
    صحيح ٻٽو_انگ_Const() ثابت;
};
class Ast_ڳنڍ_وهڪرو VL_NOT_FINAL : عام Ast_ڳنڍ_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
محمي:
    Ast_ڳنڍ_وهڪرو(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : Ast_ڳنڍ_Biop{t, fl, lhsp, rhsp} {
        إذا (lhsp->dtypep()) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(lhsp->ويڪر(), lhsp->ويڪر_Min(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_وهڪرو;
};
class Ast_ڳنڍ_System_Biop_D VL_NOT_FINAL : عام Ast_ڳنڍ_Biop {
عام:
    Ast_ڳنڍ_System_Biop_D(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : Ast_ڳنڍ_Biop{t, fl, lhsp, rhsp} {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_Ast_ڳنڍ_System_Biop_D;
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL_TRIG; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class Ast_ڳنڍ_CCall VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

محمي:
    Ast_ڳنڍ_CCall(VNType t, File_سٽ* fl, AstCFunc* funcp, Ast_ڳنڍ_اظهار* argsp = nullptr)
        : Ast_ڳنڍ_اظهار{t, fl}
        , m_funcp{funcp} {
        جوڙيو_Argsp(argsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_CCall;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_CALL; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت Ast_ڳنڍ_CCall* ثابت asamep = VN_DBG_AS(samep, ڳنڍ_CCall);
        أرجع (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override;
    bool isOutputter() override { أرجع !is_خالص(); }
    AstCFunc* funcp() ثابت { أرجع m_funcp; }
    فراغ funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() ثابت { أرجع m_argTypes; }
    فراغ argTypes(ثابت string& str) { m_argTypes = str; }
    bool isUnlikely() ثابت override { أرجع m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت final override { أرجع true; }
    bool superReference() ثابت { أرجع m_superReference; }
    فراغ superReference(bool flag) { m_superReference = flag; }
};
class Ast_ڳنڍ_Cons VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // Construct a value of an aggregate type
    دوست class Ast_ڳنڍ_اظهار;  // For m_knownConstantTree
    mutable bool m_معلوم_Constant_وڻ = false;  // Known to be 'this->isConstantTree() == true'

محمي:
    Ast_ڳنڍ_Cons(VNType t, File_سٽ* fl)
        : Ast_ڳنڍ_اظهار{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Cons;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت final override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت final override { أرجع ويڪر_Instrs(); }
};
class Ast_ڳنڍ_FTask_Ref VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_ڳنڍ_FTask_Ref;  // Gen pointers before other members for performance
خاص:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_شامل_آهي_Gen_بلاڪ = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

محمي:
    Ast_ڳنڍ_FTask_Ref(VNType t, File_سٽ* fl, ثابت string& name, AstArg* argsp)
        : Ast_ڳنڍ_اظهار{t, fl}
        , m_name{name} {
        جوڙيو_Argsp(argsp);
    }

عام:
    ثابت حرف* broken() ثابت override;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    bool isGateOptimizable() ثابت override;
    string dotted() ثابت { أرجع m_dotted; }  // * = Scope name or ""
    string inlinedDots() ثابت { أرجع m_inlinedDots; }
    فراغ inlinedDots(ثابت string& flag) { m_inlinedDots = flag; }
    Ast_ڳنڍ_FTask* taskp() ثابت { أرجع m_taskp; }  // [After Link] Pointer to variable
    فراغ taskp(Ast_ڳنڍ_FTask* taskp) { m_taskp = taskp; }
    فراغ name(ثابت string& name) override { m_name = name; }
    فراغ dotted(ثابت string& name) { m_dotted = name; }
    Ast_ڳنڍ_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_ڳنڍ_Module* nodep) { m_classOrPackagep = nodep; }
    bool شامل_آهي_Gen_بلاڪ() ثابت { أرجع m_شامل_آهي_Gen_بلاڪ; }
    فراغ شامل_آهي_Gen_بلاڪ(ثابت bool flag) { m_شامل_آهي_Gen_بلاڪ = flag; }
    bool pli() ثابت { أرجع m_pli; }
    فراغ pli(bool flag) { m_pli = flag; }
    bool superReference() ثابت { أرجع m_superReference; }
    فراغ superReference(bool flag) { m_superReference = flag; }
    bool is_خالص() override;
    bool isUnlikely() ثابت override { أرجع m_taskp && m_taskp->isUnlikely(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت Ast_ڳنڍ_FTask_Ref* ثابت asamep = VN_DBG_AS(samep, ڳنڍ_FTask_Ref);
        أرجع taskp() == asamep->taskp()  //
               && classOrPackagep() == asamep->classOrPackagep()  //
               && name() == asamep->name()  //
               && dotted() == asamep->dotted()  //
               && inlinedDots() == asamep->inlinedDots()  //
               && pli() == asamep->pli()  //
               && شامل_آهي_Gen_بلاڪ() == asamep->شامل_آهي_Gen_بلاڪ()
               && superReference() == asamep->superReference();
    }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت final override { V3ERROR_NA_RETURN(true); }

خاص:
    bool getPurityRecurse() ثابت;
};
class Ast_ڳنڍ_Pre_Sel VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

محمي:
    Ast_ڳنڍ_Pre_Sel(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* rhsp, Ast_ڳنڍ_اظهار* thsp)
        : Ast_ڳنڍ_اظهار{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Pre_Sel;
    // METHODS
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت final override { V3ERROR_NA_RETURN(true); }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;

خاص:
    bool getPurityRecurse() ثابت {
        أرجع fromp()->is_خالص() && rhsp()->is_خالص() && (!thsp() || thsp()->is_خالص());
    }
};
class Ast_ڳنڍ_Quadop VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محمي:
    Ast_ڳنڍ_Quadop(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, Ast_ڳنڍ_اظهار* thsp,
                  Ast_ڳنڍ_اظهار* fhsp)
        : Ast_ڳنڍ_اظهار{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    مجازي فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,
                               ثابت V3_عدد& ths, ثابت V3_عدد& fhs)
        = 0;
    مجازي bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    مجازي bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    مجازي bool cleanThs() ثابت = 0;  // True if THS must have extra upper bits zero
    مجازي bool cleanFhs() ثابت = 0;  // True if THS must have extra upper bits zero
    مجازي bool سائيز_Matters_Lhs() ثابت = 0;  // True if output result depends on lhs size
    مجازي bool سائيز_Matters_Rhs() ثابت = 0;  // True if output result depends on rhs size
    مجازي bool سائيز_Matters_Ths() ثابت = 0;  // True if output result depends on ths size
    مجازي bool سائيز_Matters_Fhs() ثابت = 0;  // True if output result depends on ths size
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع true; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;

خاص:
    bool getPurityRecurse() ثابت {
        أرجع lhsp()->is_خالص() && rhsp()->is_خالص() && thsp()->is_خالص() && fhsp()->is_خالص();
    }
};
class Ast_ڳنڍ_Termop VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // Terminal operator -- an operator with no "inputs"
محمي:
    Ast_ڳنڍ_Termop(VNType t, File_سٽ* fl)
        : Ast_ڳنڍ_اظهار{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Termop;
    bool isSystemFunc() ثابت override { أرجع true; }
};
class Ast_ڳنڍ_Triop VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محمي:
    Ast_ڳنڍ_Triop(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, Ast_ڳنڍ_اظهار* thsp)
        : Ast_ڳنڍ_اظهار{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Triop;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // Set out to evaluation of a AstConst'ed
    مجازي فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,
                               ثابت V3_عدد& ths)
        = 0;
    مجازي bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    مجازي bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    مجازي bool cleanThs() ثابت = 0;  // True if THS must have extra upper bits zero
    مجازي bool سائيز_Matters_Lhs() ثابت = 0;  // True if output result depends on lhs size
    مجازي bool سائيز_Matters_Rhs() ثابت = 0;  // True if output result depends on rhs size
    مجازي bool سائيز_Matters_Ths() ثابت = 0;  // True if output result depends on ths size
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع true; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;

خاص:
    bool getPurityRecurse() ثابت {
        أرجع lhsp()->is_خالص() && rhsp()->is_خالص() && thsp()->is_خالص();
    }
};
class Ast_ڳنڍ_Dist_Triop VL_NOT_FINAL : عام Ast_ڳنڍ_Triop {
عام:
    Ast_ڳنڍ_Dist_Triop(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp,
                     Ast_ڳنڍ_اظهار* thsp)
        : Ast_ڳنڍ_Triop{t, fl, lhsp, rhsp, thsp} {
        dtype_سيٽ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ڳنڍ_Dist_Triop;
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool cleanThs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Ths() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL_TRIG; }
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,
                       ثابت V3_عدد& ths) override {
        V3ERROR_NA;
    }
};
class Ast_ڳنڍ_Uniop VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محمي:
    Ast_ڳنڍ_Uniop(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : Ast_ڳنڍ_اظهار{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Uniop;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // Set out to evaluation of a AstConst'ed lhs
    مجازي فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) = 0;
    مجازي bool cleanLhs() ثابت = 0;
    مجازي bool سائيز_Matters_Lhs() ثابت = 0;  // True if output result depends on lhs size
    مجازي bool doubleFlavor() ثابت { أرجع false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    مجازي bool signedFlavor() ثابت { أرجع false; }
    مجازي bool stringFlavor() ثابت { أرجع false; }  // N flavor of nodes with both flavors?
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع true; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;
};
class Ast_ڳنڍ_System_Uniop_D VL_NOT_FINAL : عام Ast_ڳنڍ_Uniop {
عام:
    Ast_ڳنڍ_System_Uniop_D(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : Ast_ڳنڍ_Uniop{t, fl, lhsp} {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_Ast_ڳنڍ_System_Uniop_D;
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL_TRIG; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class Ast_ڳنڍ_Var_Ref VL_NOT_FINAL : عام Ast_ڳنڍ_اظهار {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_ڳنڍ_Var_Ref;  // Gen pointers before other members for performance
خاص:
    VAccess m_access;  // Left hand side assignment
    VSelf_اشارو_متن m_self_اشارو
        = VSelf_اشارو_متن{VSelf_اشارو_متن::خالي_جايون_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
محمي:
    Ast_ڳنڍ_Var_Ref(VNType t, File_سٽ* fl, AstVar* varp, ثابت VAccess& access)
        : Ast_ڳنڍ_اظهار{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

عام:
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    VAccess access() ثابت { أرجع m_access; }
    فراغ access(ثابت VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() ثابت VL_MT_STABLE { أرجع m_varp; }  // [After Link] Pointer to variable
    فراغ varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_ڳنڍ*>(varp));
    }
    AstVarScope* varScopep() ثابت { أرجع m_varScopep; }
    فراغ varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    ثابت VSelf_اشارو_متن& self_اشارو() ثابت { أرجع m_self_اشارو; }
    فراغ self_اشارو(ثابت VSelf_اشارو_متن& self_اشارو) { m_self_اشارو = self_اشارو; }
    string self_اشارو_Protect(bool useSelfForThis) ثابت {
        أرجع self_اشارو().protect(useSelfForThis, protect());
    }
    Ast_ڳنڍ_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_ڳنڍ_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : عام Ast_ڳنڍ {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
عام:
    AstArg(File_سٽ* fl, ثابت std::string& name, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت std::string& name) override { m_name = name; }
    bool خالي_جايون_Connect_No_ايندڙ() ثابت { أرجع !exprp() && name() == "" && !nextp(); }
};
class AstWith final : عام Ast_ڳنڍ {
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
خاص:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::سيٽ<std::string> m_restricted_نالا;

عام:
    AstWith(File_سٽ* fl, AstLambdaArgRef* اشاريو_Arg_Refp, AstLambdaArgRef* قدر_Arg_Refp,
            Ast_ڳنڍ* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->اشاريو_Arg_Refp(اشاريو_Arg_Refp);
        this->قدر_Arg_Refp(قدر_Arg_Refp);
        جوڙيو_Exprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!اشاريو_Arg_Refp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!قدر_Arg_Refp());  // varp needed to know lambda's arg dtype
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ restricted(bool flag) { m_restricted = flag; }
    bool restricted() ثابت { أرجع m_restricted; }
    bool validated() ثابت { أرجع m_validated; }
    فراغ validated(bool flag) { m_validated = flag; }
    فراغ جوڙيو_Restricted_Name(ثابت std::string& name) { m_restricted_نالا.وجهو(name); }
    ثابت std::سيٽ<std::string>& restricted_نالا() ثابت { أرجع m_restricted_نالا; }
    // True if 'name' binds into the randomize() target class.
    bool name_Resolves_منزل_هدف(ثابت std::string& name) ثابت {
        أرجع !m_restricted || m_restricted_نالا.ڳڻپ(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : عام Ast_ڳنڍ_اظهار {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbort_قسم m_قسم{VAbort_قسم::ACCEPT_ON};

عام:
    AstAbortOn(File_سٽ* fl, VAbort_قسم قسم, Ast_ڳنڍ_اظهار* condp, Ast_ڳنڍ_اظهار* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_قسم{قسم} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VAbort_قسم قسم() ثابت { أرجع m_قسم; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class Ast_Addr_جو_CFunc final : عام Ast_ڳنڍ_اظهار {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

عام:
    Ast_Addr_جو_CFunc(File_سٽ* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_Addr_جو_CFunc(fl)
        , m_funcp{funcp} {
        dtypep(ڳوليو_CHandle_DType());
    }

    ASTGEN_MEMBERS_Ast_Addr_جو_CFunc;
    فراغ dump(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    AstCFunc* funcp() ثابت { أرجع m_funcp; }
};
class Ast_Attr_جو final : عام Ast_ڳنڍ_اظهار {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
عام:
    Ast_Attr_جو(File_سٽ* fl, VAttrType attrtype, Ast_ڳنڍ* fromp = nullptr,
              Ast_ڳنڍ_اظهار* dimp = nullptr)
        : ASTGEN_SUPER_Attr_جو(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_Ast_Attr_جو;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ name(ثابت string& name) override { m_name = name; }
    VAttrType attrType() ثابت { أرجع m_attrType; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : عام Ast_ڳنڍ_اظهار {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ثابت bool m_خالص;  // Pure optimizable

    فراغ init(ثابت string& متن, صحيح setwidth) {
        إذا (!متن.خالي_جايون()) جوڙيو(متن);
        إذا (setwidth) {
            dtype_سيٽ_ٻٽو_انگ_Sized(setwidth, VSigning::UNSIGNED);
        } وإلا {
            dtype_سيٽ_Void();  // Caller to override if necessary
        }
    }

عام:
    class خالص {};
    explicit AstCExpr(File_سٽ* fl, ثابت string& متن = "", صحيح setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_خالص{false} {
        init(متن, setwidth);
    }
    AstCExpr(File_سٽ* fl, خالص, ثابت string& متن = "", صحيح setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_خالص{true} {
        init(متن, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع m_خالص; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع m_خالص; }
    bool is_خالص() override { أرجع m_خالص; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    // Add some text, or a node to this expression
    فراغ جوڙيو(ثابت std::string& متن) { جوڙيو_Nodesp(new Ast_متن{fileline(), متن}); }
    فراغ جوڙيو(Ast_ڳنڍ* nodep) { جوڙيو_Nodesp(nodep); }
};
class AstCExprUser final : عام Ast_ڳنڍ_اظهار {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ثابت bool m_خالص;  // Whether the function is pure
عام:
    class خالص {};
    explicit AstCExprUser(File_سٽ* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_خالص{false} {}
    AstCExprUser(File_سٽ* fl, خالص)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_خالص{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع m_خالص; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع m_خالص; }
    bool is_خالص() override { أرجع m_خالص; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    // Add some text, or a node to this expression
    فراغ جوڙيو(ثابت std::string& متن) { جوڙيو_Nodesp(new Ast_متن{fileline(), متن}); }
    فراغ جوڙيو(Ast_ڳنڍ* nodep) { جوڙيو_Nodesp(nodep); }
};
class AstCFuncHard final : عام Ast_ڳنڍ_اظهار {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
عام:
    AstCFuncHard(File_سٽ* fl, VCFunction function, Ast_ڳنڍ_اظهار* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        جوڙيو_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع function().ascii(); }
    bool is_خالص() override {
        إذا (!m_purity.isCached()) m_purity.سيٽ(getPurity());
        أرجع m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    VCFunction function() ثابت VL_MT_STABLE { أرجع m_function; }

خاص:
    bool getPurity();
};
class AstCMethodHard final : عام Ast_ڳنڍ_اظهار {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_طريقو;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
عام:
    AstCMethodHard(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, VCMethod طريقو, Ast_ڳنڍ_اظهار* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_طريقو{طريقو} {
        this->fromp(fromp);
        جوڙيو_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع طريقو().ascii(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstCMethodHard* ثابت asamep = VN_DBG_AS(samep, CMethodHard);
        أرجع (m_طريقو == asamep->m_طريقو);
    }
    bool is_خالص() override {
        إذا (!m_purity.isCached()) m_purity.سيٽ(getPurity());
        أرجع m_purity.get();
    }
    صحيح instr_ڳڻپ() ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع m_طريقو.isClean(); }
    bool usePtr() ثابت { أرجع m_usePtr; }
    فراغ usePtr(bool flag) { m_usePtr = flag; }
    VCMethod طريقو() ثابت VL_MT_STABLE { أرجع m_طريقو; }
    فراغ طريقو(VCMethod قدر) { m_طريقو = قدر; }

خاص:
    bool getPurity();
};
class AstCReset final : عام Ast_ڳنڍ_اظهار {
    // Reset variable at startup
    ثابت bool m_constructing;  // Previously cleared by constructor
عام:
    AstCReset(File_سٽ* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool is_خالص() override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(backp(), ڳنڍ_سونپيو));  // V3Emit* assumption
        أرجع nullptr;
    }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() ثابت { أرجع m_constructing; }
};
class AstCast final : عام Ast_ڳنڍ_اظهار {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
عام:
    AstCast(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, VFlag_ٻار_DType, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        ٻار_DTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { أرجع "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
    Ast_ڳنڍ_DType* get_ٻار_DTypep() ثابت override { أرجع ٻار_DTypep(); }
    Ast_ڳنڍ_DType* subDTypep() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep() : ٻار_DTypep(); }
};
class AstCastParse final : عام Ast_ڳنڍ_اظهار {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
عام:
    AstCastParse(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { أرجع "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cast_سائيز final : عام Ast_ڳنڍ_اظهار {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
عام:
    Ast_Cast_سائيز(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_Cast_سائيز(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_Ast_Cast_سائيز;
    string emitVerilog() override { أرجع "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : عام Ast_ڳنڍ_اظهار {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
عام:
    AstCellArrayRef(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ_اظهار* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        جوڙيو_Selp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : عام Ast_ڳنڍ_اظهار {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
عام:
    AstCellRef(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* cellp, Ast_ڳنڍ* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
عام:
    AstClassOrPackageRef(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        جوڙيو_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_ڳنڍ_Module* classOrPackageSkipp(ثابت bool doRefs = true) ثابت;
    Ast_ڳنڍ* classOrPackageNodep() ثابت { أرجع m_classOrPackageNodep; }
    فراغ classOrPackageNodep(Ast_ڳنڍ* nodep) { m_classOrPackageNodep = nodep; }
    فراغ classOrPackagep(Ast_ڳنڍ_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_ڳنڍ*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : عام Ast_ڳنڍ_اظهار {
    // A constant
    V3_عدد m_انگ;  // Constant value
    فراغ init_With_عدد() {
        إذا (m_انگ.isDouble()) {
            dtype_سيٽ_Double();
        } وإلا إذا (m_انگ.isString()) {
            dtype_سيٽ_String();
        } وإلا {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(m_انگ.ويڪر(), (m_انگ.sized() ? 0 : m_انگ.ويڪر_منزل_Fit()),
                                      VSigning::fromBool(m_انگ.isSigned()), m_انگ.isAnyXZ());
        }
        m_انگ.nodep(this);
    }

عام:
    ~AstConst() override;
    AstConst(File_سٽ* fl, ثابت V3_عدد& انگ)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{انگ} {
        init_With_عدد();
    }
    class Widthed_قدر {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, Widthed_قدر, صحيح ويڪر, uint32_t قدر)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, ويڪر, قدر) {  // Need () constructor
        init_With_عدد();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_سٽ* fl, DTyped, ثابت Ast_ڳنڍ_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{this, nodedtypep} {
        init_With_عدد();
    }
    class String_منزل_Parse {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, String_منزل_Parse, ثابت حرف* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{this, V3_عدد::Verilog_عدد_Literal{}, sourcep} {
        init_With_عدد();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, VerilogStringLiteral, ثابت string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{this, V3_عدد::VerilogStringLiteral{}, str} {
        init_With_عدد();
    }
    AstConst(File_سٽ* fl, uint32_t انگ)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, 32, انگ) {  // Need () constructor
        dtype_سيٽ_ٻٽو_انگ_Unsized(m_انگ.ويڪر(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, Unsized32, uint32_t انگ)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, 32, انگ) {  // Need () constructor
        m_انگ.ويڪر(32, false);
        dtype_سيٽ_ٻٽو_انگ_Unsized(32, m_انگ.ويڪر_منزل_Fit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, Signed32, int32_t انگ)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, 32, انگ) {  // Need () constructor
        m_انگ.ويڪر(32, true);
        dtype_سيٽ_ٻٽو_انگ_Unsized(32, m_انگ.ويڪر_منزل_Fit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, Unsized64, uint64_t انگ)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, 64, 0) {  // Need () constructor
        m_انگ.سيٽ_Quad(انگ);
        dtype_سيٽ_ٻٽو_انگ_Sized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, SizedEData, uint64_t انگ)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_انگ.سيٽ_Quad(انگ);
        dtype_سيٽ_ٻٽو_انگ_Sized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, RealDouble, مزدوج انگ)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, 64) {  // Need () constructor
        m_انگ.سيٽ_Double(انگ);
        dtype_سيٽ_Double();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_سٽ* fl, String, ثابت string& انگ)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{this, V3_عدد::String{}, انگ} {
        dtype_سيٽ_String();
    }
    class ٻٽو_انگ_False {};
    AstConst(File_سٽ* fl, ٻٽو_انگ_False)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, 1, 0) {  // Need () constructor
        dtype_سيٽ_ٻٽو_انگ();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class ٻٽو_انگ_False_Erroring {};
    AstConst(File_سٽ* fl, ٻٽو_انگ_False_Erroring)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, 1, 0) {  // Need () constructor
        dtype_سيٽ_ٻٽو_انگ();
        File_سٽ* ثابت newfl = new File_سٽ{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class ٻٽو_انگ_True {};
    AstConst(File_سٽ* fl, ٻٽو_انگ_True, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ(this, 1, on) {  // Need () constructor
        dtype_سيٽ_ٻٽو_انگ();
    }
    class All0 {};
    AstConst(File_سٽ* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{this, V3_عدد::Verilog_عدد_Literal{}, "'0"} {
        init_With_عدد();
        fl->warnOff(V3_Error_ڪوڊ::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_سٽ* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{this, V3_عدد::Verilog_عدد_Literal{}, "'1"} {
        init_With_عدد();
        fl->warnOff(V3_Error_ڪوڊ::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_سٽ* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{this, V3_عدد::Null{}} {
        dtypep(ڳوليو_Null_DType());
        init_With_عدد();
    }
    class One_قدم {};
    AstConst(File_سٽ* fl, One_قدم)
        : ASTGEN_SUPER_Const(fl)
        , m_انگ{this, V3_عدد::One_قدم{}} {
        dtype_سيٽ_ٻٽو_انگ_Sized(64, VSigning::UNSIGNED);
        init_With_عدد();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() ثابت override VL_MT_STABLE { أرجع انگ().ascii(); }  // * = Value
    ثابت V3_عدد& انگ() ثابت VL_MT_محفوظ { أرجع m_انگ; }  // * = Value
    V3_عدد& انگ() { أرجع m_انگ; }  // * = Value
    string origParamName() ثابت;
    فراغ origParamName(ثابت string& name);
    uint32_t منزل_UInt() ثابت { أرجع انگ().منزل_UInt(); }
    int32_t منزل_SInt() ثابت VL_MT_محفوظ { أرجع انگ().منزل_SInt(); }
    uint64_t منزل_UQuad() ثابت { أرجع انگ().منزل_UQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstConst* ثابت sp = VN_DBG_AS(samep, Const);
        أرجع انگ().isCaseEq(sp->انگ());
    }
    فراغ cloneRelink() override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(m_انگ.nodep() && m_انگ.nodep() != this);
        أرجع nullptr;
    }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool isEqAllOnes() ثابت { أرجع انگ().isEqAllOnes(ويڪر()); }
    bool isEqAllOnesV() ثابت { أرجع انگ().isEqAllOnes(ويڪر_Min_V()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_قدر_Type(ثابت AstConst* samep) ثابت;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    ساكن AstConst* parseParamLiteral(File_سٽ* fl, ثابت string& literal);
};
class AstConstraintRef final : عام Ast_ڳنڍ_اظهار {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

عام:
    AstConstraintRef(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(ڳوليو_Constraint_Ref_DType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() ثابت override VL_MT_STABLE;
    AstConstraint* constrp() ثابت VL_MT_STABLE { أرجع m_constrp; }
    فراغ constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_ڳنڍ_Module* classOrPackagep() ثابت VL_MT_STABLE { أرجع m_classOrPackagep; }
    // TODO: this setter is unused
    فراغ classOrPackagep(Ast_ڳنڍ_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت final override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cvt_Array_منزل_Array final : عام Ast_ڳنڍ_اظهار {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
خاص:
    ثابت bool m_ابتو;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    ثابت صحيح m_بلاڪ_سائيز;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    ثابت صحيح m_dst_Element_بٽ;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    ثابت صحيح m_src_Element_بٽ;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

عام:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_منزل_Array(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_DType* dtp, bool ابتو,
                       صحيح بلاڪ_سائيز, صحيح dst_Element_بٽ, صحيح src_Element_بٽ)
        : ASTGEN_SUPER_Cvt_Array_منزل_Array(fl)
        , m_ابتو{ابتو}
        , m_بلاڪ_سائيز{بلاڪ_سائيز}
        , m_dst_Element_بٽ{dst_Element_بٽ}
        , m_src_Element_بٽ{src_Element_بٽ} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_منزل_Array;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool ابتو() ثابت { أرجع m_ابتو; }
    صحيح بلاڪ_سائيز() ثابت { أرجع m_بلاڪ_سائيز; }
    صحيح dst_Element_بٽ() ثابت { أرجع m_dst_Element_بٽ; }
    صحيح src_Element_بٽ() ثابت { أرجع m_src_Element_بٽ; }
};
class Ast_Cvt_Array_منزل_Packed final : عام Ast_ڳنڍ_اظهار {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
عام:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_منزل_Packed(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Array_منزل_Packed(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_منزل_Packed;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
};
class Ast_Cvt_Packed_منزل_Array final : عام Ast_ڳنڍ_اظهار {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
عام:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Packed_منزل_Array(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Packed_منزل_Array(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Packed_منزل_Array;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
};
class Ast_Cvt_Unpack_منزل_قطار final : عام Ast_ڳنڍ_اظهار {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
عام:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Unpack_منزل_قطار(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_منزل_قطار(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_منزل_قطار;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { أرجع "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
};
class Ast_Cvt_Unpack_منزل_Unpack final : عام Ast_ڳنڍ_اظهار {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
عام:
    Ast_Cvt_Unpack_منزل_Unpack(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_منزل_Unpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_منزل_Unpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
};
class AstDist final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
عام:
    AstDist(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_Dist_شي* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        جوڙيو_Itemsp(itemsp);
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { أرجع "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }  // NA
};
class Ast_Dist_شي final : عام Ast_ڳنڍ_اظهار {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_is_سڄو = false;  // True for weight ':/', false for ':='
عام:
    Ast_Dist_شي(File_سٽ* fl, Ast_ڳنڍ_اظهار* rangep, Ast_ڳنڍ_اظهار* weightp)
        : ASTGEN_SUPER_Dist_شي(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_Ast_Dist_شي;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { أرجع "%l "s + (m_is_سڄو ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }  // NA
    فراغ is_سڄو(bool flag) { m_is_سڄو = flag; }
    bool is_سڄو() ثابت { أرجع m_is_سڄو; }
};
class AstDot final : عام Ast_ڳنڍ_اظهار {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    ثابت bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
عام:
    AstDot(File_سٽ* fl, bool colon, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    ساكن Ast_ڳنڍ_اظهار* newIfPkg(File_سٽ* fl, Ast_ڳنڍ_اظهار* packageOrClassp, Ast_ڳنڍ_اظهار* rhsp) {
        إذا (!packageOrClassp) أرجع rhsp;
        أرجع new AstDot{fl, true, packageOrClassp, rhsp};
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool colon() ثابت { أرجع m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class Ast_خالي_جايون_قطار final : عام Ast_ڳنڍ_اظهار {
عام:
    explicit Ast_خالي_جايون_قطار(File_سٽ* fl)
        : ASTGEN_SUPER_خالي_جايون_قطار(fl) {}
    ASTGEN_MEMBERS_Ast_خالي_جايون_قطار;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { أرجع "{}"; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
};
class Ast_Enum_شي_Ref final : عام Ast_ڳنڍ_اظهار {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_شامل_آهي_Gen_بلاڪ = false;  // Contains gen block reference
عام:
    Ast_Enum_شي_Ref(File_سٽ* fl, Ast_Enum_شي* itemp, Ast_ڳنڍ_Module* classOrPackagep)
        : ASTGEN_SUPER_Enum_شي_Ref(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_Ast_Enum_شي_Ref;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع itemp() ? itemp()->name() : m_name; }
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت Ast_Enum_شي_Ref* ثابت sp = VN_DBG_AS(samep, Enum_شي_Ref);
        أرجع itemp() == sp->itemp() && dotted() == sp->dotted()
               && شامل_آهي_Gen_بلاڪ() == sp->شامل_آهي_Gen_بلاڪ();
    }
    Ast_Enum_شي* itemp() ثابت VL_MT_STABLE { أرجع m_itemp; }
    فراغ itemp(Ast_Enum_شي* nodep) { m_itemp = nodep; }
    string dotted() ثابت { أرجع m_dotted; }
    فراغ dotted(ثابت string& dotted) { m_dotted = dotted; }
    bool شامل_آهي_Gen_بلاڪ() ثابت { أرجع m_شامل_آهي_Gen_بلاڪ; }
    فراغ شامل_آهي_Gen_بلاڪ(ثابت bool flag) { m_شامل_آهي_Gen_بلاڪ = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    Ast_ڳنڍ_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_ڳنڍ_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_اظهار_Stmt final : عام Ast_ڳنڍ_اظهار {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
خاص:
    bool m_has_نتيجو = true;  // Returns result via resultp()

عام:
    Ast_اظهار_Stmt(File_سٽ* fl, Ast_ڳنڍ* stmtsp, Ast_ڳنڍ_اظهار* resultp)
        : ASTGEN_SUPER_اظهار_Stmt(fl) {
        جوڙيو_Stmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_اظهار_Stmt;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool is_خالص() override {
        إذا (Ast_ڳنڍ::afterCommentp(stmtsp())) أرجع false;
        أرجع resultp()->is_خالص();
    }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع true; }
    bool has_نتيجو() ثابت { أرجع m_has_نتيجو; }
    فراغ has_نتيجو(bool flag) { m_has_نتيجو = flag; }
};
class AstFError final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
عام:
    AstFError(File_سٽ* fl, Ast_ڳنڍ_اظهار* filep, Ast_ڳنڍ_اظهار* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { أرجع "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 64; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstFOpen final : عام Ast_ڳنڍ_اظهار {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
عام:
    AstFOpen(File_سٽ* fl, Ast_ڳنڍ_اظهار* filenamep, Ast_ڳنڍ_اظهار* modep)
        : ASTGEN_SUPER_FOpen(fl) {
        this->filenamep(filenamep);
        this->modep(modep);
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpen;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    string verilogKwd() ثابت override { أرجع "$fopen"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFOpenMcd final : عام Ast_ڳنڍ_اظهار {
    // @astgen op2 := filenamep : AstNodeExpr
عام:
    AstFOpenMcd(File_سٽ* fl, Ast_ڳنڍ_اظهار* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    string verilogKwd() ثابت override { أرجع "$fopen"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstFRead final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
عام:
    AstFRead(File_سٽ* fl, Ast_ڳنڍ_اظهار* memp, Ast_ڳنڍ_اظهار* filep, Ast_ڳنڍ_اظهار* startp,
             Ast_ڳنڍ_اظهار* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtype_سيٽ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() ثابت override { أرجع "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: makes output
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFRewind final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := filep : Optional[AstNodeExpr]
عام:
    AstFRewind(File_سٽ* fl, Ast_ڳنڍ_اظهار* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() ثابت override { أرجع "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstFScanF final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_متن;

عام:
    AstFScanF(File_سٽ* fl, ثابت string& متن, Ast_ڳنڍ_اظهار* filep, Ast_ڳنڍ_اظهار* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_متن{متن} {
        جوڙيو_Exprsp(exprsp);
        this->filep(filep);
        dtype_سيٽ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() ثابت override VL_MT_STABLE { أرجع m_متن; }
    string verilogKwd() ثابت override { أرجع "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: makes output
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع متن() == VN_DBG_AS(samep, FScanF)->متن();
    }
    string متن() ثابت { أرجع m_متن; }  // * = Text to display
    فراغ متن(ثابت string& متن) { m_متن = متن; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFSeek final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
عام:
    AstFSeek(File_سٽ* fl, Ast_ڳنڍ_اظهار* filep, Ast_ڳنڍ_اظهار* وٿي, Ast_ڳنڍ_اظهار* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->وٿي(وٿي);
        this->operation(operation);
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() ثابت override { أرجع "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: makes output
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFTell final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
عام:
    AstFTell(File_سٽ* fl, Ast_ڳنڍ_اظهار* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() ثابت override { أرجع "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFalling final : عام Ast_ڳنڍ_اظهار {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
عام:
    AstFalling(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { أرجع "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFell final : عام Ast_ڳنڍ_اظهار {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عام:
    AstFell(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_Sen_وڻ* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { أرجع "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFuture final : عام Ast_ڳنڍ_اظهار {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عام:
    AstFuture(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_Sen_وڻ* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { أرجع "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstGatePin final : عام Ast_ڳنڍ_اظهار {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
عام:
    AstGatePin(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
};
class Ast_Get_Initial_Random_ٻج final : عام Ast_ڳنڍ_اظهار {
    // Verilog $get_initial_random_seed()
عام:
    explicit Ast_Get_Initial_Random_ٻج(File_سٽ* fl)
        : ASTGEN_SUPER_Get_Initial_Random_ٻج(fl) {
        dtype_سيٽ_Int();
    }
    ASTGEN_MEMBERS_Ast_Get_Initial_Random_ٻج;
    string emitVerilog() override { أرجع "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع true; }
    bool is_خالص() override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_PLI; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstImplication final : عام Ast_ڳنڍ_اظهار {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

خاص:
    ثابت bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    ثابت bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

عام:
    AstImplication(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() ثابت { أرجع m_isOverlapped; }
    bool isFollowedBy() ثابت { أرجع m_isFollowedBy; }
};
class AstInside final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
عام:
    AstInside(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        جوڙيو_Itemsp(itemsp);
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { أرجع "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }  // NA
};
class AstInsideRange final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
عام:
    AstInsideRange(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { أرجع "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_ڳنڍ_اظهار* newAndFromInside(Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp);
};
class AstLambdaArgRef final : عام Ast_ڳنڍ_اظهار {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_اشاريو;  // Index, not value

عام:
    AstLambdaArgRef(File_سٽ* fl, ثابت string& name, bool اشاريو)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_اشاريو{اشاريو} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    string emitVerilog() override { أرجع name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ name(ثابت string& name) override { m_name = name; }
    bool اشاريو() ثابت { أرجع m_اشاريو; }
    bool is_اظهار_Coverage_Eligible() ثابت override { أرجع false; }
};
class Ast_ميلاپ_Masked final : عام Ast_ڳنڍ_اظهار {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
عام:
    مضمن Ast_ميلاپ_Masked(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_Ast_ميلاپ_Masked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { أرجع "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    ساكن uint32_t fold(ثابت V3_عدد& lhs, AstVar* ميلاپ_Varp);
};
class AstMatches final : عام Ast_ڳنڍ_اظهار {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
عام:
    AstMatches(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { أرجع "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstMemberSel final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
عام:
    AstMemberSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, VFlag_ٻار_DType, ثابت string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    VAccess access() ثابت { أرجع m_access; }
    فراغ access(ثابت VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override;
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    AstVar* varp() ثابت { أرجع m_varp; }
    فراغ varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : عام Ast_ڳنڍ_اظهار {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
عام:
    AstNewCopy(File_سٽ* fl, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { أرجع "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
};
class AstNewDynamic final : عام Ast_ڳنڍ_اظهار {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
عام:
    AstNewDynamic(File_سٽ* fl, Ast_ڳنڍ_اظهار* sizep, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { أرجع "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
};
class AstPExpr final : عام Ast_ڳنڍ_اظهار {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
عام:
    explicit AstPExpr(File_سٽ* fl, AstBegin* bodyp, Ast_ڳنڍ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_سٽ* fl, AstBegin* bodyp, Ast_ڳنڍ_Stmt* finalp, Ast_ڳنڍ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
};
class AstParseHolder final : عام Ast_ڳنڍ_اظهار {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
عام:
    explicit AstParseHolder(File_سٽ* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : عام Ast_ڳنڍ_اظهار {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

عام:
    AstParseRef(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ_اظهار* lhsp = nullptr,
                Ast_ڳنڍ_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ name(ثابت string& name) override { m_name = name; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstParseRef* ثابت asamep = VN_DBG_AS(samep, ParseRef);
        أرجع m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : عام Ast_ڳنڍ_اظهار {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

عام:
    AstPast(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* ticksp = nullptr,
            Ast_Sen_وڻ* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() ثابت { أرجع m_propertyTiming; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstPatMember final : عام Ast_ڳنڍ_اظهار {
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

عام:
    AstPatMember(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhssp, Ast_ڳنڍ* keyp, Ast_ڳنڍ_اظهار* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        جوڙيو_Lhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { أرجع lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 2; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool isConcat() ثابت { أرجع m_isConcat; }
    فراغ isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() ثابت { أرجع m_isDefault; }
    فراغ isDefault(bool flag) { m_isDefault = flag; }
    Ast_ڳنڍ_اظهار* ڪنجي_Exprp() ثابت { أرجع varrefp() ? varrefp() : VN_CAST(keyp(), ڳنڍ_اظهار); }
};
class Ast_نمونو final : عام Ast_ڳنڍ_اظهار {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
عام:
    Ast_نمونو(File_سٽ* fl, Ast_ڳنڍ* itemsp)
        : ASTGEN_SUPER_نمونو(fl) {
        جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_نمونو;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    Ast_ڳنڍ_DType* get_ٻار_DTypep() ثابت override { أرجع ٻار_DTypep(); }
    Ast_ڳنڍ_DType* subDTypep() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep() : ٻار_DTypep(); }
};
class Ast_نمونو_تارو final : عام Ast_ڳنڍ_اظهار {
    // Pattern wildcard: ".*"
عام:
    explicit Ast_نمونو_تارو(File_سٽ* fl)
        : ASTGEN_SUPER_نمونو_تارو(fl) {}
    ASTGEN_MEMBERS_Ast_نمونو_تارو;
    string emitVerilog() override { أرجع ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_نمونو_Var final : عام Ast_ڳنڍ_اظهار {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
عام:
    Ast_نمونو_Var(File_سٽ* fl, ثابت string& name)
        : ASTGEN_SUPER_نمونو_Var(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_نمونو_Var;
    string emitVerilog() override { أرجع ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع m_name == VN_DBG_AS(samep, نمونو_Var)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstPropAlways final : عام Ast_ڳنڍ_اظهار {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    ثابت bool m_isStrong = false;  // s_always
عام:
    AstPropAlways(File_سٽ* fl, Ast_ڳنڍ_اظهار* propp, Ast_ڳنڍ_اظهار* loBoundp, Ast_ڳنڍ_اظهار* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { V3ERROR_NA_RETURN(0); }
    bool isStrong() ثابت { أرجع m_isStrong; }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstRand final : عام Ast_ڳنڍ_اظهار {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    ثابت bool m_urandom;  // $urandom vs $random
عام:
    class Reset {};
    AstRand(File_سٽ* fl, Ast_ڳنڍ_اظهار* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_سٽ* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtype_سيٽ_UInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override {
        أرجع seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        إذا (seedp()) {
            إذا (urandom()) أرجع "VL_URANDOM_SEEDED_%nq%lq(%li)";
            أرجع "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        إذا (isWide()) أرجع "VL_RANDOM_%nq(%nw, %P)";
        أرجع "VL_RANDOM_%nq()";
    }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع !seedp(); }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_PLI; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool combinable(ثابت AstRand* samep) ثابت {
        أرجع !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() ثابت { أرجع m_urandom; }
};
class AstRandRNG final : عام Ast_ڳنڍ_اظهار {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
عام:
    AstRandRNG(File_سٽ* fl, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { أرجع "%f$rngrandom()"; }
    string emitC() override {
        أرجع isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_PLI; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstRising final : عام Ast_ڳنڍ_اظهار {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
عام:
    AstRising(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { أرجع "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstRose final : عام Ast_ڳنڍ_اظهار {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عام:
    AstRose(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_Sen_وڻ* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { أرجع "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstSClocked final : عام Ast_ڳنڍ_اظهار {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
عام:
    AstSClocked(File_سٽ* fl, Ast_Sen_شي* sensesp, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : عام Ast_ڳنڍ_اظهار {
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
عام:
    // Exact [*N]
    AstSConsRep(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* countp, Ast_ڳنڍ_اظهار* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { V3ERROR_NA_RETURN(0); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {  // LCOV_EXCL_LINE
        أرجع m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() ثابت { أرجع m_unbounded; }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstSEventually final : عام Ast_ڳنڍ_اظهار {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
عام:
    explicit AstSEventually(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : عام Ast_ڳنڍ_اظهار {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
عام:
    explicit AstSExpr(File_سٽ* fl, Ast_ڳنڍ_اظهار* preExprp, Ast_ڳنڍ_Stmt* delayp, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_سٽ* fl, Ast_ڳنڍ_Stmt* delayp, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstSFormatArg final : عام Ast_ڳنڍ_اظهار {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

عام:
    AstSFormatArg(File_سٽ* fl, VFormatAttr formatAttr, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() ثابت override { أرجع "$sformatarg"; }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && پهريون_Abovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        أرجع nullptr;
    }
    VFormatAttr formatAttr() ثابت { أرجع m_formatAttr; }
    فراغ formatAttr(ثابت VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    ساكن VFormatAttr formatAttrDefauled(ثابت AstSFormatArg* nodep, ثابت Ast_ڳنڍ_DType* dtypep);
};
class AstSFormatF final : عام Ast_ڳنڍ_اظهار {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_متن;
    bool m_hidden;  // Under display, etc
    bool m_اظهار_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    ثابت حرف m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
عام:
    class اظهار_Format {};
    AstSFormatF(File_سٽ* fl, ثابت string& متن, bool hidden, Ast_ڳنڍ_اظهار* exprsp,
                حرف missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_متن{متن}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtype_سيٽ_String();
        جوڙيو_Exprsp(exprsp);
    }
    AstSFormatF(File_سٽ* fl, اظهار_Format, Ast_ڳنڍ_اظهار* exprsp, حرف missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_متن{""}
        , m_hidden{hidden}
        , m_اظهار_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtype_سيٽ_String();
        جوڙيو_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_متن; }
    فراغ name(ثابت string& name) override { m_متن = name; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_PLI; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع متن() == VN_DBG_AS(samep, SFormatF)->متن();
    }
    string verilogKwd() ثابت override { أرجع "$sformatf"; }
    string متن() ثابت { أرجع m_متن; }  // * = Text to display
    فراغ متن(ثابت string& متن) { m_متن = متن; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(متن() != "" && اظهار_Format_2());  // Expr format means no literal format
        أرجع nullptr;
    }
    bool formatScopeTracking() ثابت {  // Track scopeNamep();  Ok if false positive
        إذا (اظهار_Format_2() || name().ڳوليو("%m") != string::npos || name().ڳوليو("%M") != string::npos)
            أرجع true;
        لكل (ثابت Ast_ڳنڍ* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            إذا (ثابت AstConst* ثابت fmtp = VN_CAST(exprp, Const)) {
                إذا (fmtp->انگ().isFromString()) {
                    ثابت string str = fmtp->انگ().منزل_String();
                    إذا (str.ڳوليو("%m") != string::npos || str.ڳوليو("%M") != string::npos)
                        أرجع true;
                }
            }
        }
        أرجع false;
    }
    bool hidden() ثابت { أرجع m_hidden; }
    فراغ hidden(bool flag) { m_hidden = flag; }
    bool اظهار_Format_2() ثابت { أرجع m_اظهار_Format; }
    فراغ اظهار_Format_2(bool flag) { m_اظهار_Format = flag; }
    bool optionalFormat() ثابت { أرجع m_optionalFormat; }
    فراغ optionalFormat(bool flag) { m_optionalFormat = flag; }
    حرف missingArgChar() ثابت { أرجع m_missingArgChar; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstSGotoRep final : عام Ast_ڳنڍ_اظهار {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
عام:
    // Exact [->N]
    AstSGotoRep(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* countp, Ast_ڳنڍ_اظهار* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstSNonConsRep final : عام Ast_ڳنڍ_اظهار {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
عام:
    // Exact [=N]
    AstSNonConsRep(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* countp, Ast_ڳنڍ_اظهار* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstSScanF final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_متن;
    VTimescale m_timeunit;  // Parent module time unit

عام:
    AstSScanF(File_سٽ* fl, ثابت string& متن, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_متن{متن} {
        جوڙيو_Exprsp(exprsp);
        this->fromp(fromp);
        dtype_سيٽ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() ثابت override VL_MT_STABLE { أرجع m_متن; }
    string verilogKwd() ثابت override { أرجع "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: makes output
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstSScanF* ثابت asamep = VN_DBG_AS(samep, SScanF);
        أرجع متن() == asamep->متن() && timeunit() == asamep->timeunit();
    }
    string متن() ثابت { أرجع m_متن; }  // * = Text to display
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstSampled final : عام Ast_ڳنڍ_اظهار {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_اندروني : 1;  // Internally created, not from a source $sampled
عام:
    AstSampled(File_سٽ* fl, Ast_ڳنڍ* exprp, Ast_ڳنڍ_DType* dtypep, bool اندروني_2 = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_اندروني{اندروني_2} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { أرجع "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع m_اندروني == VN_DBG_AS(samep, Sampled)->m_اندروني;
    }
    bool اندروني_2() ثابت { أرجع m_اندروني; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstScopeName final : عام Ast_ڳنڍ_اظهار {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    ثابت bool m_forFormat;  // Is for a format %m
    ساكن std::string scopeNameFormatter(ثابت std::string& متن);
    ساكن std::string scopePrettyNameFormatter(ثابت std::string& متن);

عام:
    class ForFormat {};
    AstScopeName(File_سٽ* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtype_سيٽ_UInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstScopeName* ثابت sp = VN_DBG_AS(samep, ScopeName);
        أرجع m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    // ACCESSORS
    ثابت std::string& scopeAttr() ثابت { أرجع m_scopeAttr; }
    فراغ scopeAttr(ثابت std::string& val) { m_scopeAttr = val; }
    ثابت std::string& scopeEntr() ثابت { أرجع m_scopeEntr; }
    فراغ scopeEntr(ثابت std::string& val) { m_scopeEntr = val; }
    bool dpiExport() ثابت { أرجع m_dpiExport; }
    فراغ dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() ثابت { أرجع m_selfRelative; }
    فراغ selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() ثابت { أرجع m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() ثابت { أرجع scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() ثابت { أرجع scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() ثابت { أرجع scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() ثابت { أرجع scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : عام Ast_ڳنڍ_اظهار {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عام:
    AstStable(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_Sen_وڻ* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { أرجع "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class Ast_ڍير_Trace_F final : عام Ast_ڳنڍ_اظهار {
    // $stacktrace used as function
عام:
    explicit Ast_ڍير_Trace_F(File_سٽ* fl)
        : ASTGEN_SUPER_ڍير_Trace_F(fl) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_Ast_ڍير_Trace_F;
    string verilogKwd() ثابت override { أرجع "$stacktrace"; }
    string emitVerilog() override { أرجع verilogKwd(); }
    string emitC() override { أرجع "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstSteady final : عام Ast_ڳنڍ_اظهار {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
عام:
    AstSteady(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { أرجع "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstStructSel final : عام Ast_ڳنڍ_اظهار {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
عام:
    AstStructSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, ثابت string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override {
        // Not a union
        أرجع VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstStructSel* ثابت sp = VN_DBG_AS(samep, StructSel);
        أرجع m_name == sp->m_name;
    }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
};
class AstSysIgnore final : عام Ast_ڳنڍ_اظهار {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
عام:
    AstSysIgnore(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        جوڙيو_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() ثابت override { أرجع "$ignored"; }
    bool isGateOptimizable() ثابت override { أرجع false; }  // Though deleted before opt
    bool isPredictOptimizable() ثابت override { أرجع false; }  // Though deleted before opt
    bool is_خالص() override { أرجع false; }  // Though deleted before opt
    bool isOutputter() override { أرجع true; }  // Though deleted before opt
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : عام Ast_ڳنڍ_اظهار {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstSystemF(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtype_سيٽ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() ثابت override { أرجع "$system"; }
    string emitVerilog() override { أرجع verilogKwd(); }
    string emitC() override { أرجع "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class Ast_Tagged_اظهار final : عام Ast_ڳنڍ_اظهار {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
عام:
    Ast_Tagged_اظهار(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_Tagged_اظهار(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_اظهار;
    string emitVerilog() override { أرجع "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع m_name == VN_DBG_AS(samep, Tagged_اظهار)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class Ast_Tagged_نمونو final : عام Ast_ڳنڍ_اظهار {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
عام:
    Ast_Tagged_نمونو(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* patternp)
        : ASTGEN_SUPER_Tagged_نمونو(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_نمونو;
    string emitVerilog() override { أرجع "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع m_name == VN_DBG_AS(samep, Tagged_نمونو)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class Ast_Test_Plus_دليلون final : عام Ast_ڳنڍ_اظهار {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
عام:
    Ast_Test_Plus_دليلون(File_سٽ* fl, Ast_ڳنڍ_اظهار* searchp)
        : ASTGEN_SUPER_Test_Plus_دليلون(fl) {
        this->searchp(searchp);
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_Test_Plus_دليلون;
    string verilogKwd() ثابت override { أرجع "$test$plusargs"; }
    string emitVerilog() override { أرجع verilogKwd(); }
    string emitC() override { أرجع "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    // but isPure() true
    bool isSystemFunc() ثابت override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstThisRef final : عام Ast_ڳنڍ_اظهار {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
عام:
    AstThisRef(File_سٽ* fl, VFlag_ٻار_DType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        ٻار_DTypep(dtypep);
    }
    AstThisRef(File_سٽ* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { أرجع "this"; }
    string emitVerilog() override { أرجع "this"; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    Ast_ڳنڍ_DType* get_ٻار_DTypep() ثابت override { أرجع ٻار_DTypep(); }
};
class Ast_وقت_Precision final : عام Ast_ڳنڍ_اظهار {
    // Verilog $timeprecision
عام:
    explicit Ast_وقت_Precision(File_سٽ* fl)
        : ASTGEN_SUPER_وقت_Precision(fl) {
        dtype_سيٽ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_وقت_Precision;
    string emitVerilog() override { أرجع "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_وقت_Unit final : عام Ast_ڳنڍ_اظهار {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
عام:
    explicit Ast_وقت_Unit(File_سٽ* fl)
        : ASTGEN_SUPER_وقت_Unit(fl) {
        dtype_سيٽ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_وقت_Unit;
    string emitVerilog() override { أرجع "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : عام Ast_ڳنڍ_اظهار {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    ثابت std::ويڪٽر<uint32_t> m_indices;  // Indices of the triggers tested
عام:
    AstTriggered(File_سٽ* fl, AstVarScope* trigVscp, ثابت std::ويڪٽر<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    ثابت std::ويڪٽر<uint32_t>& indices() ثابت { أرجع m_indices; }
};
class AstUnbounded final : عام Ast_ڳنڍ_اظهار {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
عام:
    explicit AstUnbounded(File_سٽ* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtype_سيٽ_Int();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { أرجع "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
};
class AstUnlinkedRef final : عام Ast_ڳنڍ_اظهار {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
عام:
    AstUnlinkedRef(File_سٽ* fl, Ast_ڳنڍ* refp, ثابت string& name, Ast_ڳنڍ* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : عام Ast_ڳنڍ_اظهار {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    ثابت bool m_strong;  // 's_' preffix
    ثابت bool m_overlapping;  // '_with` suffix
عام:
    AstUntil(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() ثابت override {
        أرجع (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
    bool isStrong() ثابت { أرجع m_strong; }
    bool isOverlapping() ثابت { أرجع m_overlapping; }
};
class Ast_قدر_Plus_دليلون final : عام Ast_ڳنڍ_اظهار {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
عام:
    Ast_قدر_Plus_دليلون(File_سٽ* fl, Ast_ڳنڍ_اظهار* searchp, Ast_ڳنڍ_اظهار* outp)
        : ASTGEN_SUPER_قدر_Plus_دليلون(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_قدر_Plus_دليلون;
    string verilogKwd() ثابت override { أرجع "$value$plusargs"; }
    string emitVerilog() override { أرجع "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع !outp(); }
    bool isSystemFunc() ثابت override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstWithParse final : عام Ast_ڳنڍ_اظهار {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
خاص:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

عام:
    AstWithParse(File_سٽ* fl, Ast_ڳنڍ_اظهار* funcrefp, Ast_ڳنڍ_اظهار* exprsp,
                 Ast_ڳنڍ* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        جوڙيو_Exprsp(exprsp);
        جوڙيو_Constraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool restricted() ثابت { أرجع m_restricted; }
    فراغ restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : عام Ast_ڳنڍ_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
عام:
    AstBufIf1(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { أرجع "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstCastDynamic final : عام Ast_ڳنڍ_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
عام:
    AstCastDynamic(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Unsized(32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { أرجع "%f$cast(%r, %l)"; }
    string emitC() override { أرجع "VL_DYNAMIC_CAST(%r, %l)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 20; }
    bool is_خالص() override { أرجع false; }
};
class AstCompareNN final : عام Ast_ڳنڍ_Biop {
    // Verilog str.compare() and str.icompare()
    ثابت bool m_ignoreCase;  // True for str.icompare()
عام:
    AstCompareNN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtype_سيٽ_Int();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        أرجع m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        أرجع m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool ignoreCase() ثابت { أرجع m_ignoreCase; }
};
class AstConcat final : عام Ast_ڳنڍ_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
عام:
    AstConcat(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        إذا (lhsp->dtypep() && rhsp->dtypep()) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Sized(lhsp->dtypep()->ويڪر() + rhsp->dtypep()->ويڪر(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { أرجع "%f{%l, %k%r}"; }
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opConcat(lhs, rhs);
    }
    string emitC() override { أرجع "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(concat %l %r)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 2; }
};
class AstConcatN final : عام Ast_ڳنڍ_Biop {
    // String concatenate
عام:
    AstConcatN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { أرجع "%f{%l, %k%r}"; }
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opConcatN(lhs, rhs);
    }
    string emitC() override { أرجع "VL_CONCATN_NNN(%li, %ri)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstDiv final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstDiv(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opDiv(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f/ %r)"; }
    string emitC() override { أرجع "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvudiv %l %r)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_DIV; }
};
class AstDivD final : عام Ast_ڳنڍ_Biop {
عام:
    AstDivD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_AstDivD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opDivD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "/"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL_DIV; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstDivS final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstDivS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opDivS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f/ %r)"; }
    string emitC() override { أرجع "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvsdiv %l %r)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_DIV; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstEqWild final : عام Ast_ڳنڍ_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
عام:
    AstEqWild(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    ساكن Ast_ڳنڍ_Biop* newTyped(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp);
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f==? %r)"; }
    string emitC() override {
        إذا (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } وإلا {
            أرجع "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() ثابت override { أرجع "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        إذا (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } وإلا {
            أرجع "==";
        }
    }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstFGetS final : عام Ast_ڳنڍ_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
عام:
    AstFGetS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtype_سيٽ_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$fgets(%l,%r)"; }
    string emitC() override {
        أرجع strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 64; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : عام Ast_ڳنڍ_Biop {
عام:
    AstFUngetC(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        أرجع "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 64; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ڳنڍ* filep() ثابت { أرجع lhsp(); }
    Ast_ڳنڍ* charp() ثابت { أرجع rhsp(); }
};
class AstGetcN final : عام Ast_ڳنڍ_Biop {
    // Verilog string.getc()
عام:
    AstGetcN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGetcN(lhs, rhs);
    }
    string name() ثابت override VL_MT_STABLE { أرجع "getc"; }
    string emitVerilog() override { أرجع "%k(%l.getc(%r))"; }
    string emitC() override { أرجع "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstGetcRefN final : عام Ast_ڳنڍ_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
عام:
    AstGetcRefN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstGt final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstGt(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGt(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f> %r)"; }
    string emitC() override { أرجع "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { أرجع ">"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstGtD final : عام Ast_ڳنڍ_Biop {
عام:
    AstGtD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstGtD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGtD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ">"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstGtN final : عام Ast_ڳنڍ_Biop {
عام:
    AstGtN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstGtN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGtN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ">"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstGtS final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstGtS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGtS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f> %r)"; }
    string emitC() override { أرجع "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstGte final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstGte(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGte(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>= %r)"; }
    string emitC() override { أرجع "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { أرجع ">="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstGteD final : عام Ast_ڳنڍ_Biop {
عام:
    AstGteD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstGteD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGteD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ">="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstGteN final : عام Ast_ڳنڍ_Biop {
عام:
    AstGteN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstGteN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGteN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ">="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstGteS final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstGteS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opGteS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>= %r)"; }
    string emitC() override { أرجع "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstLogAnd final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstLogAnd(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f&& %r)"; }
    string emitC() override { أرجع "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvand %l %r)"; }
    string emitSimpleOperator() override { أرجع "&&"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() + INSTR_ڳڻپ_BRANCH; }
};
class AstLogIf final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstLogIf(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f-> %r)"; }
    string emitC() override { أرجع "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { أرجع "->"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() + INSTR_ڳڻپ_BRANCH; }
};
class AstLogOr final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstLogOr(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f|| %r)"; }
    string emitC() override { أرجع "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvor %l %r)"; }
    string emitSimpleOperator() override { أرجع "||"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() + INSTR_ڳڻپ_BRANCH; }
};
class AstLt final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstLt(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLt(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f< %r)"; }
    string emitC() override { أرجع "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { أرجع "<"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstLtD final : عام Ast_ڳنڍ_Biop {
عام:
    AstLtD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstLtD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLtD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "<"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstLtN final : عام Ast_ڳنڍ_Biop {
عام:
    AstLtN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstLtN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLtN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "<"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstLtS final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstLtS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLtS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f< %r)"; }
    string emitC() override { أرجع "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstLte final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstLte(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLte(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<= %r)"; }
    string emitC() override { أرجع "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { أرجع "<="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstLteD final : عام Ast_ڳنڍ_Biop {
عام:
    AstLteD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstLteD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLteD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "<="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstLteN final : عام Ast_ڳنڍ_Biop {
عام:
    AstLteN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstLteN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLteN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "<="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstLteS final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstLteS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLteS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<= %r)"; }
    string emitC() override { أرجع "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstModDiv final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstModDiv(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f%% %r)"; }
    string emitC() override { أرجع "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvurem %l %r)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_DIV; }
};
class AstModDivS final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstModDivS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f%% %r)"; }
    string emitC() override { أرجع "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvsmod %l %r)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_DIV; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstNeqWild final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstNeqWild(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!=? %r)"; }
    string emitC() override {
        إذا (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } وإلا {
            أرجع "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        إذا (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } وإلا {
            أرجع "!=";
        }
    }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstPow final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstPow(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opPow(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_MUL * 10; }
};
class AstPowD final : عام Ast_ڳنڍ_Biop {
عام:
    AstPowD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_AstPowD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opPowD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "pow(%li,%ri)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL_DIV * 5; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstPowSS final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstPowSS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_MUL * 10; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstPowSU final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstPowSU(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_MUL * 10; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstPowUS final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstPowUS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_MUL * 10; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstReplicate final : عام Ast_ڳنڍ_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
عام:
    AstReplicate(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        إذا (lhsp) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(rhsp, Const)) {
                إذا (constp->انگ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->انگ().isNegative())) {
                    dtype_سيٽ_ٻٽو_انگ_Or_Logic_Sized(lhsp->ويڪر(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } وإلا {
                    dtype_سيٽ_ٻٽو_انگ_Or_Logic_Sized(lhsp->ويڪر() * constp->منزل_SInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, uint32_t rep_ڳڻپ)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_ڳڻپ}} {}
    ASTGEN_MEMBERS_AstReplicate;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opRepl(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%f{%r{%k%l}}"; }
    string emitC() override { أرجع "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override {
        أرجع "((_ repeat " + cvt_منزل_Str(ويڪر() / lhsp()->ويڪر()) + ") %l)";
    }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 2; }
};
class AstReplicateN final : عام Ast_ڳنڍ_Biop {
    // String replicate
عام:
    AstReplicateN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opReplN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%f{%r{%k%l}}"; }
    string emitC() override { أرجع "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 2; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstSAnd final : عام Ast_ڳنڍ_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    ثابت bool m_propertyControl;  // Parser-generated property if/case branch conjunction
عام:
    AstSAnd(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstSAnd;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() + INSTR_ڳڻپ_BRANCH; }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {  // LCOV_EXCL_LINE
        أرجع m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() ثابت { أرجع m_propertyControl; }
};
class AstSIntersect final : عام Ast_ڳنڍ_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
عام:
    AstSIntersect(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() + INSTR_ڳڻپ_BRANCH; }
    // LCOV_EXCL_STOP
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstSOr final : عام Ast_ڳنڍ_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
عام:
    AstSOr(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstSOr;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() + INSTR_ڳڻپ_BRANCH; }
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstSThroughout final : عام Ast_ڳنڍ_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
عام:
    AstSThroughout(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    // LCOV_EXCL_STOP
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstSWithin final : عام Ast_ڳنڍ_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
عام:
    AstSWithin(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    // LCOV_EXCL_STOP
    bool is_Multi_چڪر_Sva() ثابت override { أرجع true; }
};
class AstSel final : عام Ast_ڳنڍ_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    صحيح m_decl_El_ويڪر;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    صحيح m_ويڪر_Const;

عام:
    AstSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* lsbp, صحيح bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_decl_El_ويڪر{1}
        , m_ويڪر_Const{bitwidth} {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic_Sized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, صحيح lsb, صحيح bitwidth, صحيح bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_decl_El_ويڪر{1}
        , m_ويڪر_Const{bitwidth} {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& from, ثابت V3_عدد& ٻٽو_انگ_2) override {
        ٻاهريون.opSel(from, ٻٽو_انگ_2.منزل_UInt() + ويڪر_Const() - 1, ٻٽو_انگ_2.منزل_UInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        أرجع ويڪر_Const() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() ثابت override { أرجع "((_ extract %t %r) %l)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* otherp) ثابت override {
        أرجع ويڪر_Const() == VN_DBG_AS(otherp, Sel)->ويڪر_Const();
    }
    صحيح instr_ڳڻپ() ثابت override {
        أرجع ويڪر_Instrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    صحيح ويڪر_Const() ثابت { أرجع m_ويڪر_Const; }
    فراغ ويڪر_Const(صحيح قدر) { m_ويڪر_Const = قدر; }
    صحيح lsbConst() ثابت { أرجع VN_AS(lsbp(), Const)->منزل_SInt(); }
    صحيح msbConst() ثابت { أرجع lsbConst() + ويڪر_Const() - 1; }
    VNumRange& declRange() VL_MT_STABLE { أرجع m_declRange; }
    ثابت VNumRange& declRange() ثابت VL_MT_STABLE { أرجع m_declRange; }
    فراغ declRange(ثابت VNumRange& flag) { m_declRange = flag; }
    صحيح decl_El_ويڪر() ثابت { أرجع m_decl_El_ويڪر; }
    فراغ decl_El_ويڪر(صحيح flag) { m_decl_El_ويڪر = flag; }
};
class Ast_سرڪايو_L final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    Ast_سرڪايو_L(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_سرڪايو_L(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا إذا (ثابت Ast_ڳنڍ_DType* ثابت dtypep = lhsp->dtypep()) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(dtypep->ويڪر(), dtypep->ويڪر_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرڪايو_L;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_سرڪايو_L(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<< %r)"; }
    string emitC() override { أرجع "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        أرجع (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class Ast_سرڪايو_LOvr final : عام Ast_ڳنڍ_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
عام:
    Ast_سرڪايو_LOvr(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_سرڪايو_LOvr(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا {
            ثابت Ast_ڳنڍ_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(dtypep->ويڪر(), dtypep->ويڪر_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرڪايو_LOvr;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_سرڪايو_L(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<< %r)"; }
    string emitC() override { أرجع "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class Ast_سرڪايو_R final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    Ast_سرڪايو_R(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_سرڪايو_R(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا إذا (ثابت Ast_ڳنڍ_DType* ثابت dtypep = lhsp->dtypep()) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(dtypep->ويڪر(), dtypep->ويڪر_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرڪايو_R;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_سرڪايو_R(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>> %r)"; }
    string emitC() override { أرجع "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        أرجع (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    // LHS size might be > output size, so don't want to force size
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class Ast_سرڪايو_ROvr final : عام Ast_ڳنڍ_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
عام:
    Ast_سرڪايو_ROvr(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_سرڪايو_ROvr(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا {
            ثابت Ast_ڳنڍ_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(dtypep->ويڪر(), dtypep->ويڪر_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرڪايو_ROvr;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_سرڪايو_R(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>> %r)"; }
    string emitC() override { أرجع "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    // LHS size might be > output size, so don't want to force size
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class Ast_سرڪايو_RS final : عام Ast_ڳنڍ_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
عام:
    Ast_سرڪايو_RS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_سرڪايو_RS(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا إذا (ثابت Ast_ڳنڍ_DType* ثابت dtypep = lhsp->dtypep()) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(dtypep->ويڪر(), dtypep->ويڪر_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرڪايو_RS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_سرڪايو_RS(lhs, rhs, lhsp()->ويڪر_Min_V());
    }
    string emitVerilog() override { أرجع "%k(%l %f>>> %r)"; }
    string emitSMT() ثابت override { أرجع "(bvashr %l %r)"; }
    string emitC() override { أرجع "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class Ast_سرڪايو_RSOvr final : عام Ast_ڳنڍ_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
عام:
    Ast_سرڪايو_RSOvr(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_سرڪايو_RSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        إذا (setwidth) {
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا {
            ثابت Ast_ڳنڍ_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(dtypep->ويڪر(), dtypep->ويڪر_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_سرڪايو_RSOvr;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_سرڪايو_RS(lhs, rhs, lhsp()->ويڪر_Min_V());
    }
    string emitVerilog() override { أرجع "%k(%l %f>>> %r)"; }
    string emitC() override { أرجع "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstSub final : عام Ast_ڳنڍ_Biop {
    // @astgen makeDfgVertex
عام:
    AstSub(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opSub(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f- %r)"; }
    string emitC() override { أرجع "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvsub %l %r)"; }
    string emitSimpleOperator() override { أرجع "-"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
};
class AstSubD final : عام Ast_ڳنڍ_Biop {
عام:
    AstSubD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_AstSubD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opSubD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "-"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstURandomRange final : عام Ast_ڳنڍ_Biop {
    // $urandom_range
عام:
    AstURandomRange(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtype_سيٽ_UInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$urandom_range(%l, %r)"; }
    string emitC() override { أرجع "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : عام Ast_ڳنڍ_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstEq(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    ساكن Ast_ڳنڍ_Biop* newTyped(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp);
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opEq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f== %r)"; }
    string emitC() override { أرجع "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { أرجع "=="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstEqCase final : عام Ast_ڳنڍ_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstEqCase(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstEqCase;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f=== %r)"; }
    string emitC() override { أرجع "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع "=="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstEqD final : عام Ast_ڳنڍ_Bi_Com {
عام:
    AstEqD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstEqD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opEqD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "=="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstEqN final : عام Ast_ڳنڍ_Bi_Com {
عام:
    AstEqN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstEqN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opEqN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "=="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstEqT final : عام Ast_ڳنڍ_Bi_Com {
    // Equal (==) for data types
عام:
    AstEqT(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstEqT;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "=="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
};
class AstLogEq final : عام Ast_ڳنڍ_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstLogEq(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<-> %r)"; }
    string emitC() override { أرجع "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { أرجع "<->"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() + INSTR_ڳڻپ_BRANCH; }
};
class AstNeq final : عام Ast_ڳنڍ_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstNeq(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    ساكن Ast_ڳنڍ_Biop* newTyped(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp);
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opNeq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!= %r)"; }
    string emitC() override { أرجع "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع "!="; }
    string emitSMT() ثابت override { أرجع "(__Vbv (not (= %l %r)))"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstNeqCase final : عام Ast_ڳنڍ_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstNeqCase(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!== %r)"; }
    string emitC() override { أرجع "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع "!="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
};
class AstNeqD final : عام Ast_ڳنڍ_Bi_Com {
عام:
    AstNeqD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstNeqD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "!="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstNeqN final : عام Ast_ڳنڍ_Bi_Com {
عام:
    AstNeqN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstNeqN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "!="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstNeqT final : عام Ast_ڳنڍ_Bi_Com {
    // Not-equal (!=) for data types
عام:
    AstNeqT(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstNeqT;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "!="; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_STR; }
};

// === AstNodeBiComAsv ===
class Ast_جوڙيو final : عام Ast_ڳنڍ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    Ast_جوڙيو(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_جوڙيو(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_جوڙيو;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_جوڙيو(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f+ %r)"; }
    string emitC() override { أرجع "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvadd %l %r)"; }
    string emitSimpleOperator() override { أرجع "+"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
};
class Ast_جوڙيو_D final : عام Ast_ڳنڍ_Bi_Com_Asv {
عام:
    Ast_جوڙيو_D(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_جوڙيو_D(fl, lhsp, rhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_Ast_جوڙيو_D;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_جوڙيو_D(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "+"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstAnd final : عام Ast_ڳنڍ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstAnd(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f& %r)"; }
    string emitC() override { أرجع "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvand %l %r)"; }
    string emitSimpleOperator() override { أرجع "&"; }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    ثابت حرف* ويڪر_Mismatch() ثابت override VL_MT_STABLE;
};
class AstMul final : عام Ast_ڳنڍ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstMul(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opMul(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f* %r)"; }
    string emitC() override { أرجع "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvmul %l %r)"; }
    string emitSimpleOperator() override { أرجع "*"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_MUL; }
};
class AstMulD final : عام Ast_ڳنڍ_Bi_Com_Asv {
عام:
    AstMulD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_AstMulD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opMulD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "*"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstMulS final : عام Ast_ڳنڍ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstMulS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opMulS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f* %r)"; }
    string emitC() override { أرجع "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvmul %l %r)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * INSTR_ڳڻپ_INT_MUL; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstOr final : عام Ast_ڳنڍ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstOr(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opOr(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f| %r)"; }
    string emitC() override { أرجع "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvor %l %r)"; }
    string emitSimpleOperator() override { أرجع "|"; }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    ثابت حرف* ويڪر_Mismatch() ثابت override VL_MT_STABLE;
};
class AstXor final : عام Ast_ڳنڍ_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstXor(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.opXor(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f^ %r)"; }
    string emitC() override { أرجع "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvxor %l %r)"; }
    string emitSimpleOperator() override { أرجع "^"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }  // Lclean && Rclean
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    ثابت حرف* ويڪر_Mismatch() ثابت override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class Ast_Dist_Chi_چورس final : عام Ast_ڳنڍ_Dist_Biop {
عام:
    Ast_Dist_Chi_چورس(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Dist_Chi_چورس(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_Chi_چورس;
    string emitVerilog() override { أرجع "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { أرجع "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class Ast_Dist_قوت_نمائي final : عام Ast_ڳنڍ_Dist_Biop {
عام:
    Ast_Dist_قوت_نمائي(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Dist_قوت_نمائي(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_قوت_نمائي;
    string emitVerilog() override { أرجع "%f$dist_exponential(%l, %r)"; }
    string emitC() override { أرجع "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class Ast_Dist_پواسون final : عام Ast_ڳنڍ_Dist_Biop {
عام:
    Ast_Dist_پواسون(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Dist_پواسون(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_پواسون;
    string emitVerilog() override { أرجع "%f$dist_poisson(%l, %r)"; }
    string emitC() override { أرجع "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : عام Ast_ڳنڍ_Dist_Biop {
عام:
    AstDistT(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { أرجع "%f$dist_t(%l, %r)"; }
    string emitC() override { أرجع "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : عام Ast_ڳنڍ_Sel {
    // @astgen makeDfgVertex
    فراغ init(ثابت Ast_ڳنڍ* fromp) {
        إذا (fromp && VN_IS(fromp->dtypep()->skipRefp(), ڳنڍ_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), ڳنڍ_Array_DType)->subDTypep());
        }
    }

عام:
    AstArraySel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, صحيح ٻٽو_انگ_2)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, ٻٽو_انگ_2)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() ثابت override { أرجع "(select %l %r)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
};
class AstAssocSel final : عام Ast_ڳنڍ_Sel {
    فراغ init(ثابت Ast_ڳنڍ* fromp) {
        إذا (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

عام:
    AstAssocSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l%f[%r])"; }
    string emitC() override { أرجع "%li%k[%ri]"; }
    string emitSMT() ثابت override { أرجع "(select %l %r)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override {
        أرجع !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع !isLValue(); }  // AssocSel creates on miss
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
};
class AstWildcardSel final : عام Ast_ڳنڍ_Sel {
    فراغ init(ثابت Ast_ڳنڍ* fromp) {
        إذا (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

عام:
    AstWildcardSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l%f[%r])"; }
    string emitC() override { أرجع "%li%k[%ri]"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
};
class Ast_لفظ_Sel final : عام Ast_ڳنڍ_Sel {
    // Select a single word from a multi-word wide value
عام:
    Ast_لفظ_Sel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* bitp)
        : ASTGEN_SUPER_لفظ_Sel(fl, fromp, bitp) {
        dtype_سيٽ_UInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_Ast_لفظ_Sel;
    فراغ عدد_هلايو(V3_عدد&, ثابت V3_عدد&, ثابت V3_عدد&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};

// === AstNodeStream ===
class Ast_وهڪرو_L final : عام Ast_ڳنڍ_وهڪرو {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
عام:
    Ast_وهڪرو_L(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_وهڪرو_L(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_وهڪرو_L;
    string emitVerilog() override { أرجع "%f{ << %r %k{%l} }"; }
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_وهڪرو_L(lhs, rhs);
    }
    string emitC() override { أرجع "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 2; }
};
class Ast_وهڪرو_R final : عام Ast_ڳنڍ_وهڪرو {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
عام:
    Ast_وهڪرو_R(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_وهڪرو_R(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_وهڪرو_R;
    string emitVerilog() override { أرجع "%f{ >> %r %k{%l} }"; }
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.op_سونپيو(lhs);
    }
    string emitC() override { أرجع isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : عام Ast_ڳنڍ_System_Biop_D {
عام:
    AstAtan2D(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.سيٽ_Double(std::atan2(lhs.منزل_Double(), rhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$atan2(%l,%r)"; }
    string emitC() override { أرجع "atan2(%li,%ri)"; }
};
class AstHypotD final : عام Ast_ڳنڍ_System_Biop_D {
عام:
    AstHypotD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) override {
        ٻاهريون.سيٽ_Double(std::hypot(lhs.منزل_Double(), rhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$hypot(%l,%r)"; }
    string emitC() override { أرجع "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : عام Ast_ڳنڍ_CCall {
    // C++ function call
    VSelf_اشارو_متن m_self_اشارو
        = VSelf_اشارو_متن{VSelf_اشارو_متن::خالي_جايون_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
عام:
    AstCCall(File_سٽ* fl, AstCFunc* funcp, Ast_ڳنڍ_اظهار* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    ثابت VSelf_اشارو_متن& self_اشارو() ثابت { أرجع m_self_اشارو; }
    فراغ self_اشارو(ثابت VSelf_اشارو_متن& self_اشارو) { m_self_اشارو = self_اشارو; }
    string self_اشارو_Protect(bool useSelfForThis) ثابت {
        أرجع self_اشارو().protect(useSelfForThis, protect());
    }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
};
class AstCMethodCall final : عام Ast_ڳنڍ_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
عام:
    AstCMethodCall(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, AstCFunc* funcp, Ast_ڳنڍ_اظهار* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : عام Ast_ڳنڍ_CCall {
    // C++ new() call
عام:
    AstCNew(File_سٽ* fl, AstCFunc* funcp, Ast_ڳنڍ_اظهار* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : عام Ast_ڳنڍ_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, Ast_Cons_Array_شي*> m_itemps;  // The item at each index given

عام:
    AstConsArray(File_سٽ* fl, Ast_ڳنڍ_DType* newDTypep, Ast_ڳنڍ_اظهار* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    ثابت حرف* broken() ثابت override;
    فراغ cloneRelink() override;
    مضمن Ast_Cons_Array_شي* itemsp() ثابت VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_ڳنڍ_اظهار* valuep(uint64_t اشاريو) ثابت;
    // Add the value of the element at the index, which must not be given yet
    فراغ valuep(uint64_t اشاريو, Ast_ڳنڍ_اظهار* newp);
    // Add the elements of the array 'arrayp', starting at the index
    فراغ splice_قدرون(uint64_t اشاريو, Ast_ڳنڍ_اظهار* arrayp);
};
class AstConsAssoc final : عام Ast_ڳنڍ_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
عام:
    explicit AstConsAssoc(File_سٽ* fl, Ast_ڳنڍ_اظهار* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        أرجع nullptr;
    }
};
class AstConsDynArray final : عام Ast_ڳنڍ_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
عام:
    explicit AstConsDynArray(File_سٽ* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    ثابت حرف* broken() ثابت override;
};
class Ast_Cons_قطار final : عام Ast_ڳنڍ_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
عام:
    explicit Ast_Cons_قطار(File_سٽ* fl)
        : ASTGEN_SUPER_Cons_قطار(fl) {}
    ASTGEN_MEMBERS_Ast_Cons_قطار;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), قطار_DType));
        أرجع nullptr;
    }
};
class AstConsStruct final : عام Ast_ڳنڍ_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
عام:
    AstConsStruct(File_سٽ* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    ثابت حرف* broken() ثابت override;
    // The value of the member called 'name'
    Ast_ڳنڍ_اظهار* memberValuep(ثابت string& name) ثابت;
};
class AstConsWildcard final : عام Ast_ڳنڍ_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
عام:
    explicit AstConsWildcard(File_سٽ* fl, Ast_ڳنڍ_اظهار* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        أرجع nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : عام Ast_ڳنڍ_FTask_Ref {
    // A reference to a function
عام:
    مضمن AstFuncRef(File_سٽ* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_سٽ* fl, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class Ast_طريقو_Call final : عام Ast_ڳنڍ_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
عام:
    Ast_طريقو_Call(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, VFlag_ٻار_DType, ثابت string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_طريقو_Call(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_طريقو_Call(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_طريقو_Call(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_Ast_طريقو_Call;
};
class AstNew final : عام Ast_ڳنڍ_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
عام:
    AstNew(File_سٽ* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool isImplicit() ثابت { أرجع m_isImplicit; }
    فراغ isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() ثابت { أرجع m_isScoped; }
    فراغ isScoped(bool flag) { m_isScoped = flag; }
    bool is_خالص() override { أرجع false; }
};
class AstTaskRef final : عام Ast_ڳنڍ_FTask_Ref {
    // A reference to a task
عام:
    مضمن AstTaskRef(File_سٽ* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_سٽ* fl, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtype_سيٽ_Void();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class Ast_Sel_ٻٽو_انگ final : عام Ast_ڳنڍ_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
خاص:
    VAccess m_access;  // Left hand side assignment
عام:
    Ast_Sel_ٻٽو_انگ(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* bitp)
        : ASTGEN_SUPER_Sel_ٻٽو_انگ(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_Ast_Sel_ٻٽو_انگ;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    VAccess access() ثابت { أرجع m_access; }
    فراغ access(ثابت VAccess& flag) { m_access = flag; }
};
class Ast_Sel_ڪڍو final : عام Ast_ڳنڍ_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
عام:
    Ast_Sel_ڪڍو(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* msbp, Ast_ڳنڍ_اظهار* lsbp)
        : ASTGEN_SUPER_Sel_ڪڍو(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_Ast_Sel_ڪڍو;
};
class AstSelMinus final : عام Ast_ڳنڍ_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
عام:
    AstSelMinus(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* bitp, Ast_ڳنڍ_اظهار* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : عام Ast_ڳنڍ_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
عام:
    AstSelPlus(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_اظهار* bitp, Ast_ڳنڍ_اظهار* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_ڳڻپ_بٽ final : عام Ast_ڳنڍ_Quadop {
    // Number of bits set in vector
عام:
    Ast_ڳڻپ_بٽ(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* ctrl1p)
        : ASTGEN_SUPER_ڳڻپ_بٽ(fl, exprp, ctrl1p, ctrl1p->clone_وڻ_خالص(false),
                                 ctrl1p->clone_وڻ_خالص(false)) {
        dtype_سيٽ_Int();
    }
    Ast_ڳڻپ_بٽ(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* ctrl1p, Ast_ڳنڍ_اظهار* ctrl2p)
        : ASTGEN_SUPER_ڳڻپ_بٽ(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_وڻ_خالص(false)) {
        dtype_سيٽ_Int();
    }
    Ast_ڳڻپ_بٽ(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* ctrl1p, Ast_ڳنڍ_اظهار* ctrl2p,
                 Ast_ڳنڍ_اظهار* ctrl3p)
        : ASTGEN_SUPER_ڳڻپ_بٽ(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtype_سيٽ_Int();
    }
    ASTGEN_MEMBERS_Ast_ڳڻپ_بٽ;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& اظهار, ثابت V3_عدد& ctrl1,
                       ثابت V3_عدد& ctrl2, ثابت V3_عدد& ctrl3) override {
        ٻاهريون.op_ڳڻپ_بٽ(اظهار, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { أرجع "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool cleanThs() ثابت override { أرجع true; }
    bool cleanFhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Ths() ثابت override { أرجع false; }
    bool سائيز_Matters_Fhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : عام Ast_ڳنڍ_Termop {
عام:
    explicit AstInferredDisable(File_سٽ* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { أرجع "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_وقت final : عام Ast_ڳنڍ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
عام:
    Ast_وقت(File_سٽ* fl, ثابت VTimescale& timeunit)
        : ASTGEN_SUPER_وقت(fl)
        , m_timeunit{timeunit} {
        dtype_سيٽ_UInt64();
    }
    ASTGEN_MEMBERS_Ast_وقت;
    string emitVerilog() override { أرجع "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_وقت; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class Ast_وقت_D final : عام Ast_ڳنڍ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
عام:
    Ast_وقت_D(File_سٽ* fl, ثابت VTimescale& timeunit)
        : ASTGEN_SUPER_وقت_D(fl)
        , m_timeunit{timeunit} {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_Ast_وقت_D;
    string emitVerilog() override { أرجع "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_وقت; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : عام Ast_ڳنڍ_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
عام:
    AstCond(File_سٽ* fl, Ast_ڳنڍ_اظهار* condp, Ast_ڳنڍ_اظهار* thenp, Ast_ڳنڍ_اظهار* elsep);
    ASTGEN_MEMBERS_AstCond;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,
                       ثابت V3_عدد& ths) override {
        ٻاهريون.op_سونپيو(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { أرجع "%k(%l %f? %r %k: %t)"; }
    string emitC() override { أرجع "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() ثابت override { أرجع "(ite (__Vbool %l) %r %t)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }  // clean if e1 & e2 clean
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool cleanThs() ثابت override { أرجع false; }  // Propagates up
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Ths() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_BRANCH; }
};
class AstPutcN final : عام Ast_ڳنڍ_Triop {
    // Verilog string.putc()
عام:
    AstPutcN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, Ast_ڳنڍ_اظهار* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_AstPutcN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,
                       ثابت V3_عدد& ths) override {
        ٻاهريون.opPutcN(lhs, rhs, ths);
    }
    string name() ثابت override VL_MT_STABLE { أرجع "putc"; }
    string emitVerilog() override { أرجع "%k(%l.putc(%r,%t))"; }
    string emitC() override { أرجع "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool cleanThs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Ths() ثابت override { أرجع false; }
};
class Ast_ٽڪرو_Sel final : عام Ast_ڳنڍ_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
عام:
    Ast_ٽڪرو_Sel(File_سٽ* fl, Ast_ڳنڍ_اظهار* fromp, ثابت VNumRange& declRange)
        : ASTGEN_SUPER_ٽڪرو_Sel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_Ast_ٽڪرو_Sel;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& from, ثابت V3_عدد& lo,
                       ثابت V3_عدد& ويڪر) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool cleanThs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Ths() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { أرجع m_declRange; }
    ثابت VNumRange& declRange() ثابت VL_MT_STABLE { أرجع m_declRange; }
};
class AstSubstrN final : عام Ast_ڳنڍ_Triop {
    // Verilog string.substr()
عام:
    AstSubstrN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, Ast_ڳنڍ_اظهار* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,
                       ثابت V3_عدد& ths) override {
        ٻاهريون.opSubstrN(lhs, rhs, ths);
    }
    string name() ثابت override VL_MT_STABLE { أرجع "substr"; }
    string emitVerilog() override { أرجع "%k(%l.substr(%r,%t))"; }
    string emitC() override { أرجع "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool cleanThs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Rhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Ths() ثابت override { أرجع false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : عام Ast_ڳنڍ_Dist_Triop {
عام:
    AstDistErlang(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, Ast_ڳنڍ_اظهار* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { أرجع "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { أرجع "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class Ast_Dist_عام final : عام Ast_ڳنڍ_Dist_Triop {
عام:
    Ast_Dist_عام(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, Ast_ڳنڍ_اظهار* thsp)
        : ASTGEN_SUPER_Dist_عام(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_عام;
    string emitVerilog() override { أرجع "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { أرجع "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class Ast_Dist_هڪجهڙو final : عام Ast_ڳنڍ_Dist_Triop {
عام:
    Ast_Dist_هڪجهڙو(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp, Ast_ڳنڍ_اظهار* thsp)
        : ASTGEN_SUPER_Dist_هڪجهڙو(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_هڪجهڙو;
    string emitVerilog() override { أرجع "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { أرجع "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : عام Ast_ڳنڍ_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
عام:
    تعداد FmtType : صحيح { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

خاص:
    // dist-ast-dump-suppress  // Part of name()
    ثابت FmtType m_fmtType;  // Operation type
عام:
    AstAtoN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtype_سيٽ_Double() : dtype_سيٽ_Integer();
    }
    ASTGEN_MEMBERS_AstAtoN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opAtoN(lhs, m_fmtType); }
    string name() ثابت override VL_MT_STABLE {
        بدل (m_fmtType) {
        حالة ATOI: أرجع "atoi";
        حالة ATOHEX: أرجع "atohex";
        حالة ATOOCT: أرجع "atooct";
        حالة ATOBIN: أرجع "atobin";
        حالة ATOREAL: أرجع "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { أرجع "%l." + name() + "()"; }
    string emitC() override {
        بدل (m_fmtType) {
        حالة ATOI: أرجع "VL_ATOI_N(%li, 10)";
        حالة ATOHEX: أرجع "VL_ATOI_N(%li, 16)";
        حالة ATOOCT: أرجع "VL_ATOI_N(%li, 8)";
        حالة ATOBIN: أرجع "VL_ATOI_N(%li, 2)";
        حالة ATOREAL: أرجع "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    FmtType fmtType() ثابت { أرجع m_fmtType; }
};
class Ast_بٽ_منزل_Real_D final : عام Ast_ڳنڍ_Uniop {
عام:
    Ast_بٽ_منزل_Real_D(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_بٽ_منزل_Real_D(fl, lhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_Ast_بٽ_منزل_Real_D;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_بٽ_منزل_Real_D(lhs); }
    string emitVerilog() override { أرجع "%f$bitstoreal(%l)"; }
    string emitC() override { أرجع "VL_CVT_D_Q(%li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }  // Eliminated before matters
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstCCast final : عام Ast_ڳنڍ_Uniop {
    // Cast to C-based data type
    صحيح m_سائيز;

عام:
    AstCCast(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, صحيح setwidth, صحيح minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_سائيز = setwidth;
        إذا (setwidth) {
            إذا (minwidth == -1) minwidth = setwidth;
            dtype_سيٽ_ٻٽو_انگ_Unsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_سائيز = ويڪر();
    }
    ASTGEN_MEMBERS_AstCCast;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_سونپيو(lhs); }
    string emitVerilog() override { أرجع "%f$_CAST(%l)"; }
    string emitC() override { أرجع "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }  // Special cased in V3Cast
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع سائيز() == VN_DBG_AS(samep, CCast)->سائيز();
    }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    //
    صحيح سائيز() ثابت { أرجع m_سائيز; }
};
class AstCLog2 final : عام Ast_ڳنڍ_Uniop {
عام:
    AstCLog2(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtype_سيٽ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opCLog2(lhs); }
    string emitVerilog() override { أرجع "%f$clog2(%l)"; }
    string emitC() override { أرجع "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 16; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstCastWrap final : عام Ast_ڳنڍ_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
عام:
    AstCastWrap(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_سونپيو(lhs); }
    string emitVerilog() override { أرجع "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
};
class Ast_ڳڻپ_Ones final : عام Ast_ڳنڍ_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
عام:
    Ast_ڳڻپ_Ones(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_ڳڻپ_Ones(fl, lhsp) {
        dtype_سيٽ_Int();
    }
    ASTGEN_MEMBERS_Ast_ڳڻپ_Ones;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_ڳڻپ_Ones(lhs); }
    string emitVerilog() override { أرجع "%f$countones(%l)"; }
    string emitC() override { أرجع "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 16; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstCvtPackString final : عام Ast_ڳنڍ_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
عام:
    AstCvtPackString(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstExtend final : عام Ast_ڳنڍ_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
عام:
    AstExtend(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, صحيح ويڪر, صحيح ويڪر_Min = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(ويڪر, ويڪر_Min ? ويڪر_Min : ويڪر, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_سونپيو(lhs); }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { أرجع "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override {
        أرجع "((_ zero_extend " + cvt_منزل_Str(ويڪر() - lhsp()->ويڪر()) + ") %l)";
    }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override {
        أرجع false;  // Because the EXTEND operator self-casts
    }
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
};
class AstExtendS final : عام Ast_ڳنڍ_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
عام:
    AstExtendS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, صحيح ويڪر)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic_Sized(ويڪر, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.opExtendS(lhs, lhsp()->ويڪر_Min_V());
    }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { أرجع "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override {
        أرجع "((_ sign_extend " + cvt_منزل_Str(ويڪر() - lhsp()->ويڪر()) + ") %l)";
    }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override {
        أرجع false;  // Because the EXTEND operator self-casts
    }
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstFEof final : عام Ast_ڳنڍ_Uniop {
عام:
    AstFEof(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$feof(%l)"; }
    string emitC() override { أرجع "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 16; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ثابت override { أرجع true; }
    Ast_ڳنڍ* filep() ثابت { أرجع lhsp(); }
};
class AstFGetC final : عام Ast_ڳنڍ_Uniop {
عام:
    AstFGetC(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtype_سيٽ_Logic2_State_Unsized(
            32, v3Global.ويڪر_Min_Usage() == VWidthMinUsage::MATCHES_ويڪر ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { أرجع "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 64; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ڳنڍ* filep() ثابت { أرجع lhsp(); }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstISToRD final : عام Ast_ڳنڍ_Uniop {
    // $itor where lhs is signed
عام:
    AstISToRD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_AstISToRD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opISToRD(lhs); }
    string emitVerilog() override { أرجع "%f$itor($signed(%l))"; }
    string emitC() override { أرجع "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_لفظ() override { أرجع true; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstIToRD final : عام Ast_ڳنڍ_Uniop {
    // $itor where lhs is unsigned
عام:
    AstIToRD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_AstIToRD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opIToRD(lhs); }
    string emitVerilog() override { أرجع "%f$itor(%l)"; }
    string emitC() override { أرجع "VL_ITOR_D_%lq(%lw, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstIsUnbounded final : عام Ast_ڳنڍ_Uniop {
    // True if is unbounded ($)
عام:
    AstIsUnbounded(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد&) override {
        // Any constant isn't unbounded
        ٻاهريون.سيٽ_Zero();
    }
    string emitVerilog() override { أرجع "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstIsUnknown final : عام Ast_ڳنڍ_Uniop {
    // True if any unknown bits
عام:
    AstIsUnknown(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opIsUnknown(lhs); }
    string emitVerilog() override { أرجع "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstLenN final : عام Ast_ڳنڍ_Uniop {
    // Length of a string
عام:
    AstLenN(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtype_سيٽ_Int();
    }
    ASTGEN_MEMBERS_AstLenN;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opLenN(lhs); }
    string emitVerilog() override { أرجع "%f(%l)"; }
    string emitC() override { أرجع "VL_LEN_IN(%li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
};
class AstLogNot final : عام Ast_ڳنڍ_Uniop {
    // @astgen makeDfgVertex
خاص:
    ثابت bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
عام:
    AstLogNot(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opLogNot(lhs); }
    string emitVerilog() override { أرجع "%f(! %l)"; }
    string emitC() override { أرجع "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { أرجع "!"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool fromProperty() ثابت { أرجع m_fromProperty; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class Ast_Most_سيٽ_ٻٽو_انگ_P1 final : عام Ast_ڳنڍ_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
عام:
    Ast_Most_سيٽ_ٻٽو_انگ_P1(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_Most_سيٽ_ٻٽو_انگ_P1(fl, lhsp) {
        dtype_سيٽ_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_Most_سيٽ_ٻٽو_انگ_P1;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_Most_سيٽ_ٻٽو_انگ_P1(lhs); }
    string emitVerilog() override { أرجع "%f$mostsetbitp1(%l)"; }
    string emitC() override { أرجع "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 16; }
};
class AstNToI final : عام Ast_ڳنڍ_Uniop {
    // String to any-size integral
عام:
    AstNToI(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opNToI(lhs); }
    string emitVerilog() override { أرجع "'(%l)"; }
    string emitC() override { أرجع "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
};
class AstNegate final : عام Ast_ڳنڍ_Uniop {
    // @astgen makeDfgVertex
عام:
    AstNegate(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opNegate(lhs); }
    string emitVerilog() override { أرجع "%f(- %l)"; }
    string emitC() override { أرجع "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() ثابت override { أرجع "(bvneg %l)"; }
    string emitSimpleOperator() override { أرجع "-"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
};
class AstNegateD final : عام Ast_ڳنڍ_Uniop {
عام:
    AstNegateD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtype_سيٽ_Double();
    }
    ASTGEN_MEMBERS_AstNegateD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opNegateD(lhs); }
    string emitVerilog() override { أرجع "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "-"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstNot final : عام Ast_ڳنڍ_Uniop {
    // @astgen makeDfgVertex
عام:
    AstNot(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opNot(lhs); }
    string emitVerilog() override { أرجع "%f(~ %l)"; }
    string emitC() override { أرجع "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() ثابت override { أرجع "(bvnot %l)"; }
    string emitSimpleOperator() override { أرجع "~"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }
    ثابت حرف* ويڪر_Mismatch() ثابت override VL_MT_STABLE;
};
class AstNullCheck final : عام Ast_ڳنڍ_Uniop {
    // Return LHS after checking that LHS is non-null
عام:
    AstNullCheck(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { V3ERROR_NA; }
    صحيح instr_ڳڻپ() ثابت override { أرجع 1; }  // Rarely executes
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override { أرجع fileline() == samep->fileline(); }
};
class AstOneHot final : عام Ast_ڳنڍ_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
عام:
    AstOneHot(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstOneHot;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opOneHot(lhs); }
    string emitVerilog() override { أرجع "%f$onehot(%l)"; }
    string emitC() override { أرجع "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 4; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstOneHot0 final : عام Ast_ڳنڍ_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
عام:
    AstOneHot0(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opOneHot0(lhs); }
    string emitVerilog() override { أرجع "%f$onehot0(%l)"; }
    string emitC() override { أرجع "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs() * 3; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstPostDec final : عام Ast_ڳنڍ_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
عام:
    AstPostDec(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { أرجع "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool سائيز_Matters_Lhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : عام Ast_ڳنڍ_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
عام:
    AstPostInc(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { أرجع "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool سائيز_Matters_Lhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : عام Ast_ڳنڍ_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
عام:
    AstPreDec(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { أرجع "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool سائيز_Matters_Lhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : عام Ast_ڳنڍ_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
عام:
    AstPreInc(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { أرجع "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool سائيز_Matters_Lhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : عام Ast_ڳنڍ_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
عام:
    AstRToIRoundS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtype_سيٽ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opRToIRoundS(lhs); }
    string emitVerilog() override { أرجع "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        أرجع isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstRToIS final : عام Ast_ڳنڍ_Uniop {
    // $rtoi(lhs)
عام:
    AstRToIS(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtype_سيٽ_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opRToIS(lhs); }
    string emitVerilog() override { أرجع "%f$rtoi(%l)"; }
    string emitC() override { أرجع "VL_RTOI_I_D(%li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }  // Eliminated before matters
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class Ast_Real_منزل_بٽ final : عام Ast_ڳنڍ_Uniop {
عام:
    Ast_Real_منزل_بٽ(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_Real_منزل_بٽ(fl, lhsp) {
        dtype_سيٽ_UInt64();
    }
    ASTGEN_MEMBERS_Ast_Real_منزل_بٽ;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_Real_منزل_بٽ(lhs); }
    string emitVerilog() override { أرجع "%f$realtobits(%l)"; }
    string emitC() override { أرجع "VL_CVT_Q_D(%li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }  // Eliminated before matters
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstRedAnd final : عام Ast_ڳنڍ_Uniop {
    // @astgen makeDfgVertex
عام:
    AstRedAnd(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opRedAnd(lhs); }
    string emitVerilog() override { أرجع "%f(& %l)"; }
    string emitC() override { أرجع "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
};
class AstRedOr final : عام Ast_ڳنڍ_Uniop {
    // @astgen makeDfgVertex
عام:
    AstRedOr(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opRedOr(lhs); }
    string emitVerilog() override { أرجع "%f(| %l)"; }
    string emitC() override { أرجع "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
};
class AstRedXor final : عام Ast_ڳنڍ_Uniop {
    // @astgen makeDfgVertex
عام:
    AstRedXor(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtype_سيٽ_ٻٽو_انگ_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.opRedXor(lhs); }
    string emitVerilog() override { أرجع "%f(^ %l)"; }
    string emitC() override { أرجع "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override {
        ثابت صحيح w = lhsp()->ويڪر();
        أرجع (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع 1 + V3_عدد::log2b(ويڪر()); }
};
class AstResizeLValue final : عام Ast_ڳنڍ_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
عام:
    AstResizeLValue(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
};
class AstSigned final : عام Ast_ڳنڍ_Uniop {
    // $signed(lhs)
عام:
    AstSigned(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.op_سونپيو(lhs);
        ٻاهريون.isSigned(false);
    }
    string emitVerilog() override { أرجع "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }  // Eliminated before matters
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class Ast_وقت_Import final : عام Ast_ڳنڍ_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
عام:
    Ast_وقت_Import(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_وقت_Import(fl, lhsp) {}
    ASTGEN_MEMBERS_Ast_وقت_Import;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class Ast_منزل_هيٺاهون_N final : عام Ast_ڳنڍ_Uniop {
    // string.tolower()
عام:
    Ast_منزل_هيٺاهون_N(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_منزل_هيٺاهون_N(fl, lhsp) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_Ast_منزل_هيٺاهون_N;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_منزل_هيٺاهون_N(lhs); }
    string emitVerilog() override { أرجع "%l.tolower()"; }
    string emitC() override { أرجع "VL_TOLOWER_NN(%li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
};
class Ast_منزل_String_N final : عام Ast_ڳنڍ_Uniop {
عام:
    Ast_منزل_String_N(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_منزل_String_N(fl, lhsp) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_Ast_منزل_String_N;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        أرجع isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
};
class Ast_منزل_مٿانهون_N final : عام Ast_ڳنڍ_Uniop {
    // string.toupper()
عام:
    Ast_منزل_مٿانهون_N(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_منزل_مٿانهون_N(fl, lhsp) {
        dtype_سيٽ_String();
    }
    ASTGEN_MEMBERS_Ast_منزل_مٿانهون_N;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override { ٻاهريون.op_منزل_مٿانهون_N(lhs); }
    string emitVerilog() override { أرجع "%l.toupper()"; }
    string emitC() override { أرجع "VL_TOUPPER_NN(%li)"; }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool سائيز_Matters_Lhs() ثابت override { أرجع false; }
};
class AstUnsigned final : عام Ast_ڳنڍ_Uniop {
    // $unsigned(lhs)
عام:
    AstUnsigned(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.op_سونپيو(lhs);
        ٻاهريون.isSigned(false);
    }
    string emitVerilog() override { أرجع "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool سائيز_Matters_Lhs() ثابت override { أرجع true; }  // Eliminated before matters
    صحيح instr_ڳڻپ() ثابت override { أرجع 0; }
    bool isSystemFunc() ثابت override { أرجع true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstAcosD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::acos(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$acos(%l)"; }
    string emitC() override { أرجع "acos(%li)"; }
};
class AstAcoshD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstAcoshD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::acosh(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$acosh(%l)"; }
    string emitC() override { أرجع "acosh(%li)"; }
};
class AstAsinD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstAsinD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::asin(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$asin(%l)"; }
    string emitC() override { أرجع "asin(%li)"; }
};
class AstAsinhD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstAsinhD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::asinh(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$asinh(%l)"; }
    string emitC() override { أرجع "asinh(%li)"; }
};
class AstAtanD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstAtanD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::atan(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$atan(%l)"; }
    string emitC() override { أرجع "atan(%li)"; }
};
class AstAtanhD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstAtanhD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::atanh(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$atanh(%l)"; }
    string emitC() override { أرجع "atanh(%li)"; }
};
class AstCeilD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstCeilD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::ceil(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$ceil(%l)"; }
    string emitC() override { أرجع "ceil(%li)"; }
};
class AstCosD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstCosD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::جيب_تمام(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$cos(%l)"; }
    string emitC() override { أرجع "cos(%li)"; }
};
class AstCoshD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstCoshD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::cosh(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$cosh(%l)"; }
    string emitC() override { أرجع "cosh(%li)"; }
};
class AstExpD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstExpD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::قوت_نما(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$exp(%l)"; }
    string emitC() override { أرجع "exp(%li)"; }
};
class AstFloorD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstFloorD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::floor(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$floor(%l)"; }
    string emitC() override { أرجع "floor(%li)"; }
};
class AstLog10D final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstLog10D(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::log10(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$log10(%l)"; }
    string emitC() override { أرجع "log10(%li)"; }
};
class AstLogD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstLogD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::لاگرٿم(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$ln(%l)"; }
    string emitC() override { أرجع "log(%li)"; }
};
class AstSinD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstSinD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::جيب(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$sin(%l)"; }
    string emitC() override { أرجع "sin(%li)"; }
};
class AstSinhD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstSinhD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::sinh(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$sinh(%l)"; }
    string emitC() override { أرجع "sinh(%li)"; }
};
class AstSqrtD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstSqrtD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::جذر(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$sqrt(%l)"; }
    string emitC() override { أرجع "sqrt(%li)"; }
};
class AstTanD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstTanD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::tan(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$tan(%l)"; }
    string emitC() override { أرجع "tan(%li)"; }
};
class AstTanhD final : عام Ast_ڳنڍ_System_Uniop_D {
عام:
    AstTanhD(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    فراغ عدد_هلايو(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs) override {
        ٻاهريون.سيٽ_Double(std::tanh(lhs.منزل_Double()));
    }
    string emitVerilog() override { أرجع "%f$tanh(%l)"; }
    string emitC() override { أرجع "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : عام Ast_ڳنڍ_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
عام:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    مضمن AstVarRef(File_سٽ* fl, AstVar* varp, ثابت VAccess& access);
    مضمن AstVarRef(File_سٽ* fl, Ast_ڳنڍ_Module* classOrPackagep, AstVar* varp,
                     ثابت VAccess& access);
    // This form only allowed post-link (see above)
    مضمن AstVarRef(File_سٽ* fl, AstVarScope* varscp, ثابت VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() ثابت { أرجع m_name; }
    مضمن string name() ثابت override;  // * = Var name
    فراغ name(ثابت string& name) override { m_name = name; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override;
    مضمن bool same_ڳنڍ(ثابت AstVarRef* samep) ثابت;
    مضمن bool sameNoLvalue(ثابت AstVarRef* samep) ثابت;
    صحيح instr_ڳڻپ() ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
};
class AstVarXRef final : عام Ast_ڳنڍ_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_شامل_آهي_Gen_بلاڪ = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
عام:
    AstVarXRef(File_سٽ* fl, ثابت string& name, ثابت string& dotted, ثابت VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    مضمن AstVarXRef(File_سٽ* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ name(ثابت std::string& name) override { m_name = name; }  // * = Var name
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string dotted() ثابت { أرجع m_dotted; }
    فراغ dotted(ثابت string& dotted) { m_dotted = dotted; }
    string inlinedDots() ثابت { أرجع m_inlinedDots; }
    فراغ inlinedDots(ثابت string& flag) { m_inlinedDots = flag; }
    bool شامل_آهي_Gen_بلاڪ() ثابت { أرجع m_شامل_آهي_Gen_بلاڪ; }
    فراغ شامل_آهي_Gen_بلاڪ(ثابت bool flag) { m_شامل_آهي_Gen_بلاڪ = flag; }
    bool readOnlyModport() ثابت { أرجع m_readOnlyModport; }
    فراغ readOnlyModport(ثابت bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_ٻاهريون() ثابت override { أرجع true; }
    صحيح instr_ڳڻپ() ثابت override { أرجع ويڪر_Instrs(); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        أرجع (self_اشارو() == asamep->self_اشارو() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#جيڪڏهن_پورو  // Guard
