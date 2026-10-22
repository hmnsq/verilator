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
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_عقدة_تعبير VL_NOT_FINAL : عام Ast_عقدة {
    // An expression tree node
محمي:
    Ast_عقدة_تعبير(VNType t, File_سطر* fl)
        : Ast_عقدة{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_تعبير;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // Every expression must have a data type after V3Width
    bool hasDType() ثابت override final VL_MT_SAFE { أرجع true; }
    virtual string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    virtual string emitC() = 0;
    virtual string emitSMT() ثابت { أرجع ""; };
    virtual string emitSimpleOperator() { أرجع ""; }  // "" means not ok to use
    virtual bool emit_Check_Max_كلمات() { أرجع false; }  // Check VL_MULS_MAX_WORDS
    virtual bool cleanOut() ثابت = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    virtual bool isMultiCycleSva() ثابت { أرجع false; }
    ثابت Ast_عقدة_تعبير* getVAccessTargetRecurse() ثابت;
    Ast_عقدة_تعبير* getVAccessTargetRecurse() {
        أرجع const_cast<Ast_عقدة_تعبير*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<ثابت Ast_عقدة_تعبير*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() ثابت;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() ثابت { أرجع getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_شجرة() ثابت;
    // Return base var (or const) nodep dereferences
    Ast_عقدة* baseFromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    مضمن Ast_Stmt_تعبير* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    virtual ثابت حرف* widthMismatch() ثابت VL_MT_STABLE { أرجع nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string patternString(uint32_t depth = 0) ثابت;
    bool isFourstateOrNoDtype() ثابت {
        إذا (!dtypep()) أرجع true;
        أرجع dtypep()->skipRefp()->isFourstate();
    }
    ساكن bool isFourstateOrNoDtype(ثابت Ast_عقدة_تعبير* ثابت expr1p) {
        أرجع expr1p->isFourstateOrNoDtype();
    }
    ساكن bool isFourstateOrNoDtype(ثابت Ast_عقدة_تعبير* ثابت expr1p,
                                     ثابت Ast_عقدة_تعبير* ثابت expr2p) {
        أرجع expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    ساكن bool isFourstateOrNoDtype(ثابت Ast_عقدة_تعبير* ثابت expr1p,
                                     ثابت Ast_عقدة_تعبير* ثابت expr2p,
                                     ثابت Ast_عقدة_تعبير* ثابت expr3p) {
        أرجع isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    ساكن bool isFourstateOrNoDtype(ثابت Ast_عقدة_تعبير* ثابت expr1p,
                                     ثابت Ast_عقدة_تعبير* ثابت expr2p,
                                     ثابت Ast_عقدة_تعبير* ثابت expr3p,
                                     ثابت Ast_عقدة_تعبير* ثابت expr4p) {
        أرجع isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_عقدة_Biop VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محمي:
    Ast_عقدة_Biop(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : Ast_عقدة_تعبير{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    virtual فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) = 0;
    virtual bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    virtual bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    virtual bool sizeMattersLhs() ثابت = 0;  // True if output result depends on lhs size
    virtual bool sizeMattersRhs() ثابت = 0;  // True if output result depends on rhs size
    virtual bool doubleFlavor() ثابت { أرجع false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    virtual bool signedFlavor() ثابت { أرجع false; }
    virtual bool stringFlavor() ثابت { أرجع false; }  // N flavor of nodes with both flavors?
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    bool isPure() override;
    ثابت حرف* broken() ثابت override;

خاص:
    bool getPurityRecurse() ثابت { أرجع lhsp()->isPure() && rhsp()->isPure(); }
};
class Ast_عقدة_Bi_Com VL_NOT_FINAL : عام Ast_عقدة_Biop {
    // Binary expr with commutative properties
محمي:
    Ast_عقدة_Bi_Com(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhs, Ast_عقدة_تعبير* rhs)
        : Ast_عقدة_Biop{t, fl, lhs, rhs} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Bi_Com;
};
class Ast_عقدة_Bi_Com_Asv VL_NOT_FINAL : عام Ast_عقدة_Bi_Com {
    // Binary expr with commutative & associative properties
محمي:
    Ast_عقدة_Bi_Com_Asv(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhs, Ast_عقدة_تعبير* rhs)
        : Ast_عقدة_Bi_Com{t, fl, lhs, rhs} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Bi_Com_Asv;
};
class Ast_عقدة_Dist_Biop VL_NOT_FINAL : عام Ast_عقدة_Biop {
عام:
    Ast_عقدة_Dist_Biop(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : Ast_عقدة_Biop{t, fl, lhsp, rhsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_عقدة_Dist_Biop;
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL_TRIG; }
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
};
class Ast_عقدة_Sel VL_NOT_FINAL : عام Ast_عقدة_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
محمي:
    Ast_عقدة_Sel(VNType t, File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* bitp)
        : Ast_عقدة_Biop{t, fl, fromp, bitp} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Sel;
    صحيح bitConst() ثابت;
};
class Ast_عقدة_Stream VL_NOT_FINAL : عام Ast_عقدة_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
محمي:
    Ast_عقدة_Stream(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : Ast_عقدة_Biop{t, fl, lhsp, rhsp} {
        إذا (lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(lhsp->width(), lhsp->widthMin(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Stream;
};
class Ast_عقدة_System_Biop_D VL_NOT_FINAL : عام Ast_عقدة_Biop {
عام:
    Ast_عقدة_System_Biop_D(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : Ast_عقدة_Biop{t, fl, lhsp, rhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_عقدة_System_Biop_D;
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL_TRIG; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class Ast_عقدة_CCall VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

محمي:
    Ast_عقدة_CCall(VNType t, File_سطر* fl, AstCFunc* funcp, Ast_عقدة_تعبير* argsp = nullptr)
        : Ast_عقدة_تعبير{t, fl}
        , m_funcp{funcp} {
        addArgsp(argsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_CCall;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_CALL; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت Ast_عقدة_CCall* ثابت asamep = VN_DBG_AS(samep, عقدة_CCall);
        أرجع (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override;
    bool isOutputter() override { أرجع !isPure(); }
    AstCFunc* funcp() ثابت { أرجع m_funcp; }
    فراغ funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() ثابت { أرجع m_argTypes; }
    فراغ argTypes(ثابت string& str) { m_argTypes = str; }
    bool isUnlikely() ثابت override { أرجع m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت final override { أرجع true; }
    bool superReference() ثابت { أرجع m_superReference; }
    فراغ superReference(bool flag) { m_superReference = flag; }
};
class Ast_عقدة_Cons VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // Construct a value of an aggregate type
    friend class Ast_عقدة_تعبير;  // For m_knownConstantTree
    mutable bool m_known_Constant_شجرة = false;  // Known to be 'this->isConstantTree() == true'

محمي:
    Ast_عقدة_Cons(VNType t, File_سطر* fl)
        : Ast_عقدة_تعبير{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Cons;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت final override { أرجع true; }
    صحيح instr_عدد() ثابت final override { أرجع widthInstrs(); }
};
class Ast_عقدة_FTask_Ref VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_عقدة_FTask_Ref;  // Gen pointers before other members for performance
خاص:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

محمي:
    Ast_عقدة_FTask_Ref(VNType t, File_سطر* fl, ثابت string& name, AstArg* argsp)
        : Ast_عقدة_تعبير{t, fl}
        , m_name{name} {
        addArgsp(argsp);
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
    Ast_عقدة_FTask* taskp() ثابت { أرجع m_taskp; }  // [After Link] Pointer to variable
    فراغ taskp(Ast_عقدة_FTask* taskp) { m_taskp = taskp; }
    فراغ name(ثابت string& name) override { m_name = name; }
    فراغ dotted(ثابت string& name) { m_dotted = name; }
    Ast_عقدة_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_عقدة_Module* nodep) { m_classOrPackagep = nodep; }
    bool containsGenBlock() ثابت { أرجع m_containsGenBlock; }
    فراغ containsGenBlock(ثابت bool flag) { m_containsGenBlock = flag; }
    bool pli() ثابت { أرجع m_pli; }
    فراغ pli(bool flag) { m_pli = flag; }
    bool superReference() ثابت { أرجع m_superReference; }
    فراغ superReference(bool flag) { m_superReference = flag; }
    bool isPure() override;
    bool isUnlikely() ثابت override { أرجع m_taskp && m_taskp->isUnlikely(); }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت Ast_عقدة_FTask_Ref* ثابت asamep = VN_DBG_AS(samep, عقدة_FTask_Ref);
        أرجع taskp() == asamep->taskp()  //
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
    bool cleanOut() ثابت final override { V3ERROR_NA_RETURN(true); }

خاص:
    bool getPurityRecurse() ثابت;
};
class Ast_عقدة_Pre_Sel VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

محمي:
    Ast_عقدة_Pre_Sel(VNType t, File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* rhsp, Ast_عقدة_تعبير* thsp)
        : Ast_عقدة_تعبير{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Pre_Sel;
    // METHODS
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت final override { V3ERROR_NA_RETURN(true); }
    bool isPure() override;
    ثابت حرف* broken() ثابت override;

خاص:
    bool getPurityRecurse() ثابت {
        أرجع fromp()->isPure() && rhsp()->isPure() && (!thsp() || thsp()->isPure());
    }
};
class Ast_عقدة_Quadop VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محمي:
    Ast_عقدة_Quadop(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, Ast_عقدة_تعبير* thsp,
                  Ast_عقدة_تعبير* fhsp)
        : Ast_عقدة_تعبير{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    virtual فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs,
                               ثابت V3_رقم& ths, ثابت V3_رقم& fhs)
        = 0;
    virtual bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    virtual bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    virtual bool cleanThs() ثابت = 0;  // True if THS must have extra upper bits zero
    virtual bool cleanFhs() ثابت = 0;  // True if THS must have extra upper bits zero
    virtual bool sizeMattersLhs() ثابت = 0;  // True if output result depends on lhs size
    virtual bool sizeMattersRhs() ثابت = 0;  // True if output result depends on rhs size
    virtual bool sizeMattersThs() ثابت = 0;  // True if output result depends on ths size
    virtual bool sizeMattersFhs() ثابت = 0;  // True if output result depends on ths size
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    bool isPure() override;
    ثابت حرف* broken() ثابت override;

خاص:
    bool getPurityRecurse() ثابت {
        أرجع lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure() && fhsp()->isPure();
    }
};
class Ast_عقدة_Termop VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // Terminal operator -- an operator with no "inputs"
محمي:
    Ast_عقدة_Termop(VNType t, File_سطر* fl)
        : Ast_عقدة_تعبير{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Termop;
    bool isSystemFunc() ثابت override { أرجع true; }
};
class Ast_عقدة_Triop VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محمي:
    Ast_عقدة_Triop(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, Ast_عقدة_تعبير* thsp)
        : Ast_عقدة_تعبير{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Triop;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // Set out to evaluation of a AstConst'ed
    virtual فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs,
                               ثابت V3_رقم& ths)
        = 0;
    virtual bool cleanLhs() ثابت = 0;  // True if LHS must have extra upper bits zero
    virtual bool cleanRhs() ثابت = 0;  // True if RHS must have extra upper bits zero
    virtual bool cleanThs() ثابت = 0;  // True if THS must have extra upper bits zero
    virtual bool sizeMattersLhs() ثابت = 0;  // True if output result depends on lhs size
    virtual bool sizeMattersRhs() ثابت = 0;  // True if output result depends on rhs size
    virtual bool sizeMattersThs() ثابت = 0;  // True if output result depends on ths size
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    bool isPure() override;
    ثابت حرف* broken() ثابت override;

خاص:
    bool getPurityRecurse() ثابت {
        أرجع lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure();
    }
};
class Ast_عقدة_Dist_Triop VL_NOT_FINAL : عام Ast_عقدة_Triop {
عام:
    Ast_عقدة_Dist_Triop(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp,
                     Ast_عقدة_تعبير* thsp)
        : Ast_عقدة_Triop{t, fl, lhsp, rhsp, thsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_عقدة_Dist_Triop;
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool cleanThs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool sizeMattersThs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL_TRIG; }
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs,
                       ثابت V3_رقم& ths) override {
        V3ERROR_NA;
    }
};
class Ast_عقدة_Uniop VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

محمي:
    Ast_عقدة_Uniop(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : Ast_عقدة_تعبير{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Uniop;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // Set out to evaluation of a AstConst'ed lhs
    virtual فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) = 0;
    virtual bool cleanLhs() ثابت = 0;
    virtual bool sizeMattersLhs() ثابت = 0;  // True if output result depends on lhs size
    virtual bool doubleFlavor() ثابت { أرجع false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    virtual bool signedFlavor() ثابت { أرجع false; }
    virtual bool stringFlavor() ثابت { أرجع false; }  // N flavor of nodes with both flavors?
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    bool isPure() override;
    ثابت حرف* broken() ثابت override;
};
class Ast_عقدة_System_Uniop_D VL_NOT_FINAL : عام Ast_عقدة_Uniop {
عام:
    Ast_عقدة_System_Uniop_D(VNType t, File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : Ast_عقدة_Uniop{t, fl, lhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_عقدة_System_Uniop_D;
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL_TRIG; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class Ast_عقدة_Var_Ref VL_NOT_FINAL : عام Ast_عقدة_تعبير {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_عقدة_Var_Ref;  // Gen pointers before other members for performance
خاص:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
محمي:
    Ast_عقدة_Var_Ref(VNType t, File_سطر* fl, AstVar* varp, ثابت VAccess& access)
        : Ast_عقدة_تعبير{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

عام:
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    VAccess access() ثابت { أرجع m_access; }
    فراغ access(ثابت VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() ثابت VL_MT_STABLE { أرجع m_varp; }  // [After Link] Pointer to variable
    فراغ varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_عقدة*>(varp));
    }
    AstVarScope* varScopep() ثابت { أرجع m_varScopep; }
    فراغ varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    ثابت VSelfPointerText& selfPointer() ثابت { أرجع m_selfPointer; }
    فراغ selfPointer(ثابت VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) ثابت {
        أرجع selfPointer().protect(useSelfForThis, protect());
    }
    Ast_عقدة_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_عقدة_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : عام Ast_عقدة {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
عام:
    AstArg(File_سطر* fl, ثابت std::string& name, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت std::string& name) override { m_name = name; }
    bool emptyConnectNoNext() ثابت { أرجع !exprp() && name() == "" && !nextp(); }
};
class AstWith final : عام Ast_عقدة {
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
    std::set<std::string> m_restrictedNames;

عام:
    AstWith(File_سطر* fl, AstLambdaArgRef* indexArgRefp, AstLambdaArgRef* قيمة_Arg_Refp,
            Ast_عقدة* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->indexArgRefp(indexArgRefp);
        this->قيمة_Arg_Refp(قيمة_Arg_Refp);
        addExprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!indexArgRefp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!قيمة_Arg_Refp());  // varp needed to know lambda's arg dtype
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ restricted(bool flag) { m_restricted = flag; }
    bool restricted() ثابت { أرجع m_restricted; }
    bool validated() ثابت { أرجع m_validated; }
    فراغ validated(bool flag) { m_validated = flag; }
    فراغ addRestrictedName(ثابت std::string& name) { m_restrictedNames.أدرج(name); }
    ثابت std::set<std::string>& restrictedNames() ثابت { أرجع m_restrictedNames; }
    // True if 'name' binds into the randomize() target class.
    bool nameResolvesToTarget(ثابت std::string& name) ثابت {
        أرجع !m_restricted || m_restrictedNames.عدد(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : عام Ast_عقدة_تعبير {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbortKind m_kind{VAbortKind::ACCEPT_ON};

عام:
    AstAbortOn(File_سطر* fl, VAbortKind kind, Ast_عقدة_تعبير* condp, Ast_عقدة_تعبير* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_kind{kind} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VAbortKind kind() ثابت { أرجع m_kind; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstAddrOfCFunc final : عام Ast_عقدة_تعبير {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

عام:
    AstAddrOfCFunc(File_سطر* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_AstAddrOfCFunc;
    فراغ dump(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    AstCFunc* funcp() ثابت { أرجع m_funcp; }
};
class AstAttrOf final : عام Ast_عقدة_تعبير {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
عام:
    AstAttrOf(File_سطر* fl, VAttrType attrtype, Ast_عقدة* fromp = nullptr,
              Ast_عقدة_تعبير* dimp = nullptr)
        : ASTGEN_SUPER_AttrOf(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_AstAttrOf;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ name(ثابت string& name) override { m_name = name; }
    VAttrType attrType() ثابت { أرجع m_attrType; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : عام Ast_عقدة_تعبير {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ثابت bool m_pure;  // Pure optimizable

    فراغ init(ثابت string& text, صحيح setwidth) {
        إذا (!text.empty()) add(text);
        إذا (setwidth) {
            dtypeSetBitSized(setwidth, VSigning::UNSIGNED);
        } وإلا {
            dtypeSetVoid();  // Caller to override if necessary
        }
    }

عام:
    class Pure {};
    explicit AstCExpr(File_سطر* fl, ثابت string& text = "", صحيح setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{false} {
        init(text, setwidth);
    }
    AstCExpr(File_سطر* fl, Pure, ثابت string& text = "", صحيح setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool cleanOut() ثابت override { أرجع true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع m_pure; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع m_pure; }
    bool isPure() override { أرجع m_pure; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    // Add some text, or a node to this expression
    فراغ add(ثابت std::string& text) { addNodesp(new AstText{fileline(), text}); }
    فراغ add(Ast_عقدة* nodep) { addNodesp(nodep); }
};
class AstCExprUser final : عام Ast_عقدة_تعبير {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ثابت bool m_pure;  // Whether the function is pure
عام:
    class Pure {};
    explicit AstCExprUser(File_سطر* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{false} {}
    AstCExprUser(File_سطر* fl, Pure)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool cleanOut() ثابت override { أرجع false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع m_pure; }
    bool isOutputter() override { أرجع true; }
    bool isPredictOptimizable() ثابت override { أرجع m_pure; }
    bool isPure() override { أرجع m_pure; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    // Add some text, or a node to this expression
    فراغ add(ثابت std::string& text) { addNodesp(new AstText{fileline(), text}); }
    فراغ add(Ast_عقدة* nodep) { addNodesp(nodep); }
};
class AstCFuncHard final : عام Ast_عقدة_تعبير {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
عام:
    AstCFuncHard(File_سطر* fl, VCFunction function, Ast_عقدة_تعبير* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع function().ascii(); }
    bool isPure() override {
        إذا (!m_purity.isCached()) m_purity.set(getPurity());
        أرجع m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    VCFunction function() ثابت VL_MT_STABLE { أرجع m_function; }

خاص:
    bool getPurity();
};
class AstCMethodHard final : عام Ast_عقدة_تعبير {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_method;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
عام:
    AstCMethodHard(File_سطر* fl, Ast_عقدة_تعبير* fromp, VCMethod method, Ast_عقدة_تعبير* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_method{method} {
        this->fromp(fromp);
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع method().ascii(); }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstCMethodHard* ثابت asamep = VN_DBG_AS(samep, CMethodHard);
        أرجع (m_method == asamep->m_method);
    }
    bool isPure() override {
        إذا (!m_purity.isCached()) m_purity.set(getPurity());
        أرجع m_purity.get();
    }
    صحيح instr_عدد() ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع m_method.isClean(); }
    bool usePtr() ثابت { أرجع m_usePtr; }
    فراغ usePtr(bool flag) { m_usePtr = flag; }
    VCMethod method() ثابت VL_MT_STABLE { أرجع m_method; }
    فراغ method(VCMethod قيمة) { m_method = قيمة; }

خاص:
    bool getPurity();
};
class AstCReset final : عام Ast_عقدة_تعبير {
    // Reset variable at startup
    ثابت bool m_constructing;  // Previously cleared by constructor
عام:
    AstCReset(File_سطر* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isPure() override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(backp(), عقدة_Assign));  // V3Emit* assumption
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() ثابت { أرجع m_constructing; }
};
class AstCast final : عام Ast_عقدة_تعبير {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
عام:
    AstCast(File_سطر* fl, Ast_عقدة_تعبير* fromp, VFlagChildDType, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { أرجع "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : عام Ast_عقدة_تعبير {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
عام:
    AstCastParse(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { أرجع "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCastSize final : عام Ast_عقدة_تعبير {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
عام:
    AstCastSize(File_سطر* fl, Ast_عقدة_تعبير* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_CastSize(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCastSize;
    string emitVerilog() override { أرجع "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : عام Ast_عقدة_تعبير {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
عام:
    AstCellArrayRef(File_سطر* fl, ثابت string& name, Ast_عقدة_تعبير* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        addSelp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : عام Ast_عقدة_تعبير {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
عام:
    AstCellRef(File_سطر* fl, ثابت string& name, Ast_عقدة* cellp, Ast_عقدة* exprp)
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
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : عام Ast_عقدة_تعبير {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
عام:
    AstClassOrPackageRef(File_سطر* fl, ثابت string& name, Ast_عقدة* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_عقدة_Module* classOrPackageSkipp(ثابت bool doRefs = true) ثابت;
    Ast_عقدة* classOrPackageNodep() ثابت { أرجع m_classOrPackageNodep; }
    فراغ classOrPackageNodep(Ast_عقدة* nodep) { m_classOrPackageNodep = nodep; }
    فراغ classOrPackagep(Ast_عقدة_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_عقدة*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : عام Ast_عقدة_تعبير {
    // A constant
    V3_رقم m_رقم;  // Constant value
    فراغ init_With_رقم() {
        إذا (m_رقم.isDouble()) {
            dtypeSetDouble();
        } وإلا إذا (m_رقم.isString()) {
            dtypeSetString();
        } وإلا {
            dtypeSetBitOrLogicUnsized(m_رقم.width(), (m_رقم.sized() ? 0 : m_رقم.widthToFit()),
                                      VSigning::fromBool(m_رقم.isSigned()), m_رقم.isAnyXZ());
        }
        m_رقم.nodep(this);
    }

عام:
    ~AstConst() override;
    AstConst(File_سطر* fl, ثابت V3_رقم& رقم)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{رقم} {
        init_With_رقم();
    }
    class Widthed_قيمة {};  // for creator type-overload selection
    AstConst(File_سطر* fl, Widthed_قيمة, صحيح width, uint32_t قيمة)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, width, قيمة) {  // Need () constructor
        init_With_رقم();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_سطر* fl, DTyped, ثابت Ast_عقدة_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{this, nodedtypep} {
        init_With_رقم();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(File_سطر* fl, StringToParse, ثابت حرف* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{this, V3_رقم::Verilog_رقم_Literal{}, sourcep} {
        init_With_رقم();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_سطر* fl, VerilogStringLiteral, ثابت string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{this, V3_رقم::VerilogStringLiteral{}, str} {
        init_With_رقم();
    }
    AstConst(File_سطر* fl, uint32_t رقم)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, 32, رقم) {  // Need () constructor
        dtypeSetBitUnsized(m_رقم.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_سطر* fl, Unsized32, uint32_t رقم)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, 32, رقم) {  // Need () constructor
        m_رقم.width(32, false);
        dtypeSetBitUnsized(32, m_رقم.widthToFit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_سطر* fl, Signed32, int32_t رقم)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, 32, رقم) {  // Need () constructor
        m_رقم.width(32, true);
        dtypeSetBitUnsized(32, m_رقم.widthToFit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_سطر* fl, Unsized64, uint64_t رقم)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, 64, 0) {  // Need () constructor
        m_رقم.setQuad(رقم);
        dtypeSetBitSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_سطر* fl, SizedEData, uint64_t رقم)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_رقم.setQuad(رقم);
        dtypeSetBitSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_سطر* fl, RealDouble, مزدوج رقم)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, 64) {  // Need () constructor
        m_رقم.setDouble(رقم);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_سطر* fl, String, ثابت string& رقم)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{this, V3_رقم::String{}, رقم} {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(File_سطر* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_سطر* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
        File_سطر* ثابت newfl = new File_سطر{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_سطر* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم(this, 1, on) {  // Need () constructor
        dtypeSetBit();
    }
    class All0 {};
    AstConst(File_سطر* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{this, V3_رقم::Verilog_رقم_Literal{}, "'0"} {
        init_With_رقم();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_سطر* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{this, V3_رقم::Verilog_رقم_Literal{}, "'1"} {
        init_With_رقم();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_سطر* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{this, V3_رقم::Null{}} {
        dtypep(findNullDType());
        init_With_رقم();
    }
    class OneStep {};
    AstConst(File_سطر* fl, OneStep)
        : ASTGEN_SUPER_Const(fl)
        , m_رقم{this, V3_رقم::OneStep{}} {
        dtypeSetBitSized(64, VSigning::UNSIGNED);
        init_With_رقم();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() ثابت override VL_MT_STABLE { أرجع رقم().ascii(); }  // * = Value
    ثابت V3_رقم& رقم() ثابت VL_MT_SAFE { أرجع m_رقم; }  // * = Value
    V3_رقم& رقم() { أرجع m_رقم; }  // * = Value
    string origParamName() ثابت;
    فراغ origParamName(ثابت string& name);
    uint32_t toUInt() ثابت { أرجع رقم().toUInt(); }
    int32_t toSInt() ثابت VL_MT_SAFE { أرجع رقم().toSInt(); }
    uint64_t toUQuad() ثابت { أرجع رقم().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstConst* ثابت sp = VN_DBG_AS(samep, Const);
        أرجع رقم().isCaseEq(sp->رقم());
    }
    فراغ cloneRelink() override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(m_رقم.nodep() && m_رقم.nodep() != this);
        أرجع nullptr;
    }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool isEqAllOnes() ثابت { أرجع رقم().isEqAllOnes(width()); }
    bool isEqAllOnesV() ثابت { أرجع رقم().isEqAllOnes(widthMinV()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_قيمة_Type(ثابت AstConst* samep) ثابت;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    ساكن AstConst* parseParamLiteral(File_سطر* fl, ثابت string& literal);
};
class AstConstraintRef final : عام Ast_عقدة_تعبير {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

عام:
    AstConstraintRef(File_سطر* fl, Ast_عقدة_تعبير* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() ثابت override VL_MT_STABLE;
    AstConstraint* constrp() ثابت VL_MT_STABLE { أرجع m_constrp; }
    فراغ constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_عقدة_Module* classOrPackagep() ثابت VL_MT_STABLE { أرجع m_classOrPackagep; }
    // TODO: this setter is unused
    فراغ classOrPackagep(Ast_عقدة_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت final override { V3ERROR_NA_RETURN(true); }
};
class AstCvtArrayToArray final : عام Ast_عقدة_تعبير {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
خاص:
    ثابت bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    ثابت صحيح m_blockSize;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    ثابت صحيح m_dstElementBits;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    ثابت صحيح m_srcElementBits;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

عام:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToArray(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_DType* dtp, bool reverse,
                       صحيح blockSize, صحيح dstElementBits, صحيح srcElementBits)
        : ASTGEN_SUPER_CvtArrayToArray(fl)
        , m_reverse{reverse}
        , m_blockSize{blockSize}
        , m_dstElementBits{dstElementBits}
        , m_srcElementBits{srcElementBits} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToArray;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool reverse() ثابت { أرجع m_reverse; }
    صحيح blockSize() ثابت { أرجع m_blockSize; }
    صحيح dstElementBits() ثابت { أرجع m_dstElementBits; }
    صحيح srcElementBits() ثابت { أرجع m_srcElementBits; }
};
class AstCvtArrayToPacked final : عام Ast_عقدة_تعبير {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
عام:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToPacked(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_CvtArrayToPacked(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToPacked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
};
class AstCvtPackedToArray final : عام Ast_عقدة_تعبير {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
عام:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtPackedToArray(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_CvtPackedToArray(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtPackedToArray;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
};
class AstCvtUnpackToQueue final : عام Ast_عقدة_تعبير {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
عام:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtUnpackToQueue(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToQueue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToQueue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { أرجع "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool cleanOut() ثابت override { أرجع true; }
};
class AstCvtUnpackToUnpack final : عام Ast_عقدة_تعبير {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
عام:
    AstCvtUnpackToUnpack(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToUnpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToUnpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
};
class AstDist final : عام Ast_عقدة_تعبير {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
عام:
    AstDist(File_سطر* fl, Ast_عقدة_تعبير* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { أرجع "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }  // NA
};
class AstDistItem final : عام Ast_عقدة_تعبير {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_isWhole = false;  // True for weight ':/', false for ':='
عام:
    AstDistItem(File_سطر* fl, Ast_عقدة_تعبير* rangep, Ast_عقدة_تعبير* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { أرجع "%l "s + (m_isWhole ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }  // NA
    فراغ isWhole(bool flag) { m_isWhole = flag; }
    bool isWhole() ثابت { أرجع m_isWhole; }
};
class AstDot final : عام Ast_عقدة_تعبير {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    ثابت bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
عام:
    AstDot(File_سطر* fl, bool colon, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    ساكن Ast_عقدة_تعبير* newIfPkg(File_سطر* fl, Ast_عقدة_تعبير* packageOrClassp, Ast_عقدة_تعبير* rhsp) {
        إذا (!packageOrClassp) أرجع rhsp;
        أرجع new AstDot{fl, true, packageOrClassp, rhsp};
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool colon() ثابت { أرجع m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstEmptyQueue final : عام Ast_عقدة_تعبير {
عام:
    explicit AstEmptyQueue(File_سطر* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_AstEmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { أرجع "{}"; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool cleanOut() ثابت override { أرجع true; }
};
class AstEnumItemRef final : عام Ast_عقدة_تعبير {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_containsGenBlock = false;  // Contains gen block reference
عام:
    AstEnumItemRef(File_سطر* fl, AstEnumItem* itemp, Ast_عقدة_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع itemp() ? itemp()->name() : m_name; }
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstEnumItemRef* ثابت sp = VN_DBG_AS(samep, EnumItemRef);
        أرجع itemp() == sp->itemp() && dotted() == sp->dotted()
               && containsGenBlock() == sp->containsGenBlock();
    }
    AstEnumItem* itemp() ثابت VL_MT_STABLE { أرجع m_itemp; }
    فراغ itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() ثابت { أرجع m_dotted; }
    فراغ dotted(ثابت string& dotted) { m_dotted = dotted; }
    bool containsGenBlock() ثابت { أرجع m_containsGenBlock; }
    فراغ containsGenBlock(ثابت bool flag) { m_containsGenBlock = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    Ast_عقدة_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_عقدة_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_تعبير_Stmt final : عام Ast_عقدة_تعبير {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
خاص:
    bool m_hasResult = true;  // Returns result via resultp()

عام:
    Ast_تعبير_Stmt(File_سطر* fl, Ast_عقدة* stmtsp, Ast_عقدة_تعبير* resultp)
        : ASTGEN_SUPER_تعبير_Stmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_تعبير_Stmt;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool isPure() override {
        إذا (Ast_عقدة::afterCommentp(stmtsp())) أرجع false;
        أرجع resultp()->isPure();
    }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    bool hasResult() ثابت { أرجع m_hasResult; }
    فراغ hasResult(bool flag) { m_hasResult = flag; }
};
class AstFError final : عام Ast_عقدة_تعبير {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
عام:
    AstFError(File_سطر* fl, Ast_عقدة_تعبير* filep, Ast_عقدة_تعبير* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { أرجع "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 64; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstFOpen final : عام Ast_عقدة_تعبير {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
عام:
    AstFOpen(File_سطر* fl, Ast_عقدة_تعبير* filenamep, Ast_عقدة_تعبير* modep)
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
    bool cleanOut() ثابت override { أرجع true; }
    string verilogKwd() ثابت override { أرجع "$fopen"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFOpenMcd final : عام Ast_عقدة_تعبير {
    // @astgen op2 := filenamep : AstNodeExpr
عام:
    AstFOpenMcd(File_سطر* fl, Ast_عقدة_تعبير* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    string verilogKwd() ثابت override { أرجع "$fopen"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstFRead final : عام Ast_عقدة_تعبير {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
عام:
    AstFRead(File_سطر* fl, Ast_عقدة_تعبير* memp, Ast_عقدة_تعبير* filep, Ast_عقدة_تعبير* startp,
             Ast_عقدة_تعبير* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() ثابت override { أرجع "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: makes output
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFRewind final : عام Ast_عقدة_تعبير {
    // @astgen op1 := filep : Optional[AstNodeExpr]
عام:
    AstFRewind(File_سطر* fl, Ast_عقدة_تعبير* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() ثابت override { أرجع "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstFScanF final : عام Ast_عقدة_تعبير {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

عام:
    AstFScanF(File_سطر* fl, ثابت string& text, Ast_عقدة_تعبير* filep, Ast_عقدة_تعبير* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() ثابت override VL_MT_STABLE { أرجع m_text; }
    string verilogKwd() ثابت override { أرجع "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: makes output
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() ثابت { أرجع m_text; }  // * = Text to display
    فراغ text(ثابت string& text) { m_text = text; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFSeek final : عام Ast_عقدة_تعبير {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
عام:
    AstFSeek(File_سطر* fl, Ast_عقدة_تعبير* filep, Ast_عقدة_تعبير* offset, Ast_عقدة_تعبير* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() ثابت override { أرجع "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: makes output
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFTell final : عام Ast_عقدة_تعبير {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
عام:
    AstFTell(File_سطر* fl, Ast_عقدة_تعبير* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() ثابت override { أرجع "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFalling final : عام Ast_عقدة_تعبير {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
عام:
    AstFalling(File_سطر* fl, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { أرجع "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFell final : عام Ast_عقدة_تعبير {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عام:
    AstFell(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_Sen_شجرة* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { أرجع "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstFuture final : عام Ast_عقدة_تعبير {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عام:
    AstFuture(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_Sen_شجرة* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { أرجع "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstGatePin final : عام Ast_عقدة_تعبير {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
عام:
    AstGatePin(File_سطر* fl, Ast_عقدة_تعبير* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
};
class AstGetInitialRandomSeed final : عام Ast_عقدة_تعبير {
    // Verilog $get_initial_random_seed()
عام:
    explicit AstGetInitialRandomSeed(File_سطر* fl)
        : ASTGEN_SUPER_GetInitialRandomSeed(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstGetInitialRandomSeed;
    string emitVerilog() override { أرجع "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع true; }
    bool isPure() override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstImplication final : عام Ast_عقدة_تعبير {
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
    AstImplication(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, bool isOverlapped,
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
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool isMultiCycleSva() ثابت override { أرجع m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() ثابت { أرجع m_isOverlapped; }
    bool isFollowedBy() ثابت { أرجع m_isFollowedBy; }
};
class AstInside final : عام Ast_عقدة_تعبير {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
عام:
    AstInside(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { أرجع "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }  // NA
};
class AstInsideRange final : عام Ast_عقدة_تعبير {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
عام:
    AstInsideRange(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { أرجع "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_عقدة_تعبير* newAndFromInside(Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp);
};
class AstLambdaArgRef final : عام Ast_عقدة_تعبير {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_index;  // Index, not value

عام:
    AstLambdaArgRef(File_سطر* fl, ثابت string& name, bool index)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_index{index} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    string emitVerilog() override { أرجع name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ name(ثابت string& name) override { m_name = name; }
    bool index() ثابت { أرجع m_index; }
    bool is_تعبير_Coverage_Eligible() ثابت override { أرجع false; }
};
class AstMatchMasked final : عام Ast_عقدة_تعبير {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
عام:
    مضمن AstMatchMasked(File_سطر* fl, Ast_عقدة_تعبير* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_AstMatchMasked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { أرجع "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool cleanOut() ثابت override { أرجع true; }
    ساكن uint32_t fold(ثابت V3_رقم& lhs, AstVar* matchVarp);
};
class AstMatches final : عام Ast_عقدة_تعبير {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
عام:
    AstMatches(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { أرجع "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstMemberSel final : عام Ast_عقدة_تعبير {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
عام:
    AstMemberSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, VFlagChildDType, ثابت string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    VAccess access() ثابت { أرجع m_access; }
    فراغ access(ثابت VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    AstVar* varp() ثابت { أرجع m_varp; }
    فراغ varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : عام Ast_عقدة_تعبير {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
عام:
    AstNewCopy(File_سطر* fl, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { أرجع "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
};
class AstNewDynamic final : عام Ast_عقدة_تعبير {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
عام:
    AstNewDynamic(File_سطر* fl, Ast_عقدة_تعبير* sizep, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { أرجع "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
};
class AstPExpr final : عام Ast_عقدة_تعبير {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
عام:
    explicit AstPExpr(File_سطر* fl, AstBegin* bodyp, Ast_عقدة_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_سطر* fl, AstBegin* bodyp, Ast_عقدة_Stmt* finalp, Ast_عقدة_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
};
class AstParseHolder final : عام Ast_عقدة_تعبير {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
عام:
    explicit AstParseHolder(File_سطر* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : عام Ast_عقدة_تعبير {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

عام:
    AstParseRef(File_سطر* fl, ثابت string& name, Ast_عقدة_تعبير* lhsp = nullptr,
                Ast_عقدة_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ name(ثابت string& name) override { m_name = name; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstParseRef* ثابت asamep = VN_DBG_AS(samep, ParseRef);
        أرجع m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : عام Ast_عقدة_تعبير {
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
    AstPast(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* ticksp = nullptr,
            Ast_Sen_شجرة* sentreep = nullptr, bool propertyTiming = false)
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
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() ثابت { أرجع m_propertyTiming; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstPatMember final : عام Ast_عقدة_تعبير {
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
    AstPatMember(File_سطر* fl, Ast_عقدة_تعبير* lhssp, Ast_عقدة* keyp, Ast_عقدة_تعبير* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { أرجع lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 2; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool isConcat() ثابت { أرجع m_isConcat; }
    فراغ isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() ثابت { أرجع m_isDefault; }
    فراغ isDefault(bool flag) { m_isDefault = flag; }
    Ast_عقدة_تعبير* مفتاح_Exprp() ثابت { أرجع varrefp() ? varrefp() : VN_CAST(keyp(), عقدة_تعبير); }
};
class AstPattern final : عام Ast_عقدة_تعبير {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
عام:
    AstPattern(File_سطر* fl, Ast_عقدة* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstPattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت VL_MT_STABLE { أرجع dtypep() ? dtypep() : childDTypep(); }
};
class AstPatternStar final : عام Ast_عقدة_تعبير {
    // Pattern wildcard: ".*"
عام:
    explicit AstPatternStar(File_سطر* fl)
        : ASTGEN_SUPER_PatternStar(fl) {}
    ASTGEN_MEMBERS_AstPatternStar;
    string emitVerilog() override { أرجع ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstPatternVar final : عام Ast_عقدة_تعبير {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
عام:
    AstPatternVar(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_PatternVar(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPatternVar;
    string emitVerilog() override { أرجع ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع m_name == VN_DBG_AS(samep, PatternVar)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstPropAlways final : عام Ast_عقدة_تعبير {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    ثابت bool m_isStrong = false;  // s_always
عام:
    AstPropAlways(File_سطر* fl, Ast_عقدة_تعبير* propp, Ast_عقدة_تعبير* loBoundp, Ast_عقدة_تعبير* hiBoundp,
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
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { V3ERROR_NA_RETURN(0); }
    bool isStrong() ثابت { أرجع m_isStrong; }
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstRand final : عام Ast_عقدة_تعبير {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    ثابت bool m_urandom;  // $urandom vs $random
عام:
    class Reset {};
    AstRand(File_سطر* fl, Ast_عقدة_تعبير* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_سطر* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtypeSetUInt32();
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
    bool cleanOut() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع !seedp(); }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool combinable(ثابت AstRand* samep) ثابت {
        أرجع !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() ثابت { أرجع m_urandom; }
};
class AstRandRNG final : عام Ast_عقدة_تعبير {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
عام:
    AstRandRNG(File_سطر* fl, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { أرجع "%f$rngrandom()"; }
    string emitC() override {
        أرجع isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool cleanOut() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstRising final : عام Ast_عقدة_تعبير {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
عام:
    AstRising(File_سطر* fl, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { أرجع "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstRose final : عام Ast_عقدة_تعبير {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عام:
    AstRose(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_Sen_شجرة* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { أرجع "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstSClocked final : عام Ast_عقدة_تعبير {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
عام:
    AstSClocked(File_سطر* fl, AstSenItem* sensesp, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : عام Ast_عقدة_تعبير {
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
    AstSConsRep(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* countp, Ast_عقدة_تعبير* maxCountp,
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
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { V3ERROR_NA_RETURN(0); }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {  // LCOV_EXCL_LINE
        أرجع m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() ثابت { أرجع m_unbounded; }
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstSEventually final : عام Ast_عقدة_تعبير {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
عام:
    explicit AstSEventually(File_سطر* fl, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : عام Ast_عقدة_تعبير {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
عام:
    explicit AstSExpr(File_سطر* fl, Ast_عقدة_تعبير* preExprp, Ast_عقدة_Stmt* delayp, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_سطر* fl, Ast_عقدة_Stmt* delayp, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstSFormatArg final : عام Ast_عقدة_تعبير {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

عام:
    AstSFormatArg(File_سطر* fl, VFormatAttr formatAttr, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() ثابت override { أرجع "$sformatarg"; }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        أرجع nullptr;
    }
    VFormatAttr formatAttr() ثابت { أرجع m_formatAttr; }
    فراغ formatAttr(ثابت VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    ساكن VFormatAttr formatAttrDefauled(ثابت AstSFormatArg* nodep, ثابت Ast_عقدة_DType* dtypep);
};
class AstSFormatF final : عام Ast_عقدة_تعبير {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_تعبير_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    ثابت حرف m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
عام:
    class تعبير_Format {};
    AstSFormatF(File_سطر* fl, ثابت string& text, bool hidden, Ast_عقدة_تعبير* exprsp,
                حرف missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    AstSFormatF(File_سطر* fl, تعبير_Format, Ast_عقدة_تعبير* exprsp, حرف missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_تعبير_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_text; }
    فراغ name(ثابت string& name) override { m_text = name; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() ثابت override { أرجع "$sformatf"; }
    string text() ثابت { أرجع m_text; }  // * = Text to display
    فراغ text(ثابت string& text) { m_text = text; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(text() != "" && تعبير_Format_2());  // Expr format means no literal format
        أرجع nullptr;
    }
    bool formatScopeTracking() ثابت {  // Track scopeNamep();  Ok if false positive
        إذا (تعبير_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            أرجع true;
        لكل (ثابت Ast_عقدة* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            إذا (ثابت AstConst* ثابت fmtp = VN_CAST(exprp, Const)) {
                إذا (fmtp->رقم().isFromString()) {
                    ثابت string str = fmtp->رقم().toString();
                    إذا (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        أرجع true;
                }
            }
        }
        أرجع false;
    }
    bool hidden() ثابت { أرجع m_hidden; }
    فراغ hidden(bool flag) { m_hidden = flag; }
    bool تعبير_Format_2() ثابت { أرجع m_تعبير_Format; }
    فراغ تعبير_Format_2(bool flag) { m_تعبير_Format = flag; }
    bool optionalFormat() ثابت { أرجع m_optionalFormat; }
    فراغ optionalFormat(bool flag) { m_optionalFormat = flag; }
    حرف missingArgChar() ثابت { أرجع m_missingArgChar; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstSGotoRep final : عام Ast_عقدة_تعبير {
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
    AstSGotoRep(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* countp, Ast_عقدة_تعبير* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstSNonConsRep final : عام Ast_عقدة_تعبير {
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
    AstSNonConsRep(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* countp, Ast_عقدة_تعبير* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstSScanF final : عام Ast_عقدة_تعبير {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

عام:
    AstSScanF(File_سطر* fl, ثابت string& text, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->fromp(fromp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() ثابت override VL_MT_STABLE { أرجع m_text; }
    string verilogKwd() ثابت override { أرجع "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: makes output
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstSScanF* ثابت asamep = VN_DBG_AS(samep, SScanF);
        أرجع text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() ثابت { أرجع m_text; }  // * = Text to display
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstSampled final : عام Ast_عقدة_تعبير {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_internal : 1;  // Internally created, not from a source $sampled
عام:
    AstSampled(File_سطر* fl, Ast_عقدة* exprp, Ast_عقدة_DType* dtypep, bool internal = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_internal{internal} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { أرجع "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع m_internal == VN_DBG_AS(samep, Sampled)->m_internal;
    }
    bool internal() ثابت { أرجع m_internal; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstScopeName final : عام Ast_عقدة_تعبير {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    ثابت bool m_forFormat;  // Is for a format %m
    ساكن std::string scopeNameFormatter(ثابت std::string& text);
    ساكن std::string scopePrettyNameFormatter(ثابت std::string& text);

عام:
    class ForFormat {};
    AstScopeName(File_سطر* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstScopeName* ثابت sp = VN_DBG_AS(samep, ScopeName);
        أرجع m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
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
class AstStable final : عام Ast_عقدة_تعبير {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
عام:
    AstStable(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_Sen_شجرة* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { أرجع "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class Ast_مكدس_Trace_F final : عام Ast_عقدة_تعبير {
    // $stacktrace used as function
عام:
    explicit Ast_مكدس_Trace_F(File_سطر* fl)
        : ASTGEN_SUPER_مكدس_Trace_F(fl) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_Ast_مكدس_Trace_F;
    string verilogKwd() ثابت override { أرجع "$stacktrace"; }
    string emitVerilog() override { أرجع verilogKwd(); }
    string emitC() override { أرجع "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool cleanOut() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstSteady final : عام Ast_عقدة_تعبير {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
عام:
    AstSteady(File_سطر* fl, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { أرجع "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstStructSel final : عام Ast_عقدة_تعبير {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
عام:
    AstStructSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, ثابت string& name)
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
    bool cleanOut() ثابت override {
        // Not a union
        أرجع VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstStructSel* ثابت sp = VN_DBG_AS(samep, StructSel);
        أرجع m_name == sp->m_name;
    }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
};
class AstSysIgnore final : عام Ast_عقدة_تعبير {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
عام:
    AstSysIgnore(File_سطر* fl, Ast_عقدة_تعبير* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() ثابت override { أرجع "$ignored"; }
    bool isGateOptimizable() ثابت override { أرجع false; }  // Though deleted before opt
    bool isPredictOptimizable() ثابت override { أرجع false; }  // Though deleted before opt
    bool isPure() override { أرجع false; }  // Though deleted before opt
    bool isOutputter() override { أرجع true; }  // Though deleted before opt
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : عام Ast_عقدة_تعبير {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
عام:
    AstSystemF(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() ثابت override { أرجع "$system"; }
    string emitVerilog() override { أرجع verilogKwd(); }
    string emitC() override { أرجع "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
    bool isUnlikely() ثابت override { أرجع true; }
    bool cleanOut() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class Ast_Tagged_تعبير final : عام Ast_عقدة_تعبير {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
عام:
    Ast_Tagged_تعبير(File_سطر* fl, ثابت string& name, Ast_عقدة_تعبير* exprp)
        : ASTGEN_SUPER_Tagged_تعبير(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_تعبير;
    string emitVerilog() override { أرجع "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع m_name == VN_DBG_AS(samep, Tagged_تعبير)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstTaggedPattern final : عام Ast_عقدة_تعبير {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
عام:
    AstTaggedPattern(File_سطر* fl, ثابت string& name, Ast_عقدة* patternp)
        : ASTGEN_SUPER_TaggedPattern(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstTaggedPattern;
    string emitVerilog() override { أرجع "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع m_name == VN_DBG_AS(samep, TaggedPattern)->m_name;
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class Ast_Test_Plus_وسائط final : عام Ast_عقدة_تعبير {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
عام:
    Ast_Test_Plus_وسائط(File_سطر* fl, Ast_عقدة_تعبير* searchp)
        : ASTGEN_SUPER_Test_Plus_وسائط(fl) {
        this->searchp(searchp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_Test_Plus_وسائط;
    string verilogKwd() ثابت override { أرجع "$test$plusargs"; }
    string emitVerilog() override { أرجع verilogKwd(); }
    string emitC() override { أرجع "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    // but isPure() true
    bool isSystemFunc() ثابت override { أرجع true; }
    bool cleanOut() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstThisRef final : عام Ast_عقدة_تعبير {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
عام:
    AstThisRef(File_سطر* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_سطر* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { أرجع "this"; }
    string emitVerilog() override { أرجع "this"; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool cleanOut() ثابت override { أرجع true; }
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
};
class AstTimePrecision final : عام Ast_عقدة_تعبير {
    // Verilog $timeprecision
عام:
    explicit AstTimePrecision(File_سطر* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { أرجع "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstTimeUnit final : عام Ast_عقدة_تعبير {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
عام:
    explicit AstTimeUnit(File_سطر* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { أرجع "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : عام Ast_عقدة_تعبير {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    ثابت std::vector<uint32_t> m_indices;  // Indices of the triggers tested
عام:
    AstTriggered(File_سطر* fl, AstVarScope* trigVscp, ثابت std::vector<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    ثابت std::vector<uint32_t>& indices() ثابت { أرجع m_indices; }
};
class AstUnbounded final : عام Ast_عقدة_تعبير {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
عام:
    explicit AstUnbounded(File_سطر* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { أرجع "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
};
class AstUnlinkedRef final : عام Ast_عقدة_تعبير {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
عام:
    AstUnlinkedRef(File_سطر* fl, Ast_عقدة* refp, ثابت string& name, Ast_عقدة* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : عام Ast_عقدة_تعبير {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    ثابت bool m_strong;  // 's_' preffix
    ثابت bool m_overlapping;  // '_with` suffix
عام:
    AstUntil(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, bool strong, bool overlapping)
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
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool isMultiCycleSva() ثابت override { أرجع true; }
    bool isStrong() ثابت { أرجع m_strong; }
    bool isOverlapping() ثابت { أرجع m_overlapping; }
};
class Ast_قيمة_Plus_وسائط final : عام Ast_عقدة_تعبير {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
عام:
    Ast_قيمة_Plus_وسائط(File_سطر* fl, Ast_عقدة_تعبير* searchp, Ast_عقدة_تعبير* outp)
        : ASTGEN_SUPER_قيمة_Plus_وسائط(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_قيمة_Plus_وسائط;
    string verilogKwd() ثابت override { أرجع "$value$plusargs"; }
    string emitVerilog() override { أرجع "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع !outp(); }
    bool isSystemFunc() ثابت override { أرجع true; }
    bool cleanOut() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstWithParse final : عام Ast_عقدة_تعبير {
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
    AstWithParse(File_سطر* fl, Ast_عقدة_تعبير* funcrefp, Ast_عقدة_تعبير* exprsp,
                 Ast_عقدة* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        addExprsp(exprsp);
        addConstraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    bool restricted() ثابت { أرجع m_restricted; }
    فراغ restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : عام Ast_عقدة_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
عام:
    AstBufIf1(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { أرجع "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstCastDynamic final : عام Ast_عقدة_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
عام:
    AstCastDynamic(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtypeSetBitUnsized(32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { أرجع "%f$cast(%r, %l)"; }
    string emitC() override { أرجع "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 20; }
    bool isPure() override { أرجع false; }
};
class AstCompareNN final : عام Ast_عقدة_Biop {
    // Verilog str.compare() and str.icompare()
    ثابت bool m_ignoreCase;  // True for str.icompare()
عام:
    AstCompareNN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        أرجع m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        أرجع m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool ignoreCase() ثابت { أرجع m_ignoreCase; }
};
class AstConcat final : عام Ast_عقدة_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
عام:
    AstConcat(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        إذا (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetBitOrLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { أرجع "%f{%l, %k%r}"; }
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { أرجع "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(concat %l %r)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 2; }
};
class AstConcatN final : عام Ast_عقدة_Biop {
    // String concatenate
عام:
    AstConcatN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { أرجع "%f{%l, %k%r}"; }
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { أرجع "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstDiv final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstDiv(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f/ %r)"; }
    string emitC() override { أرجع "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvudiv %l %r)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_DIV; }
};
class AstDivD final : عام Ast_عقدة_Biop {
عام:
    AstDivD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstDivD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "/"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL_DIV; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstDivS final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstDivS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f/ %r)"; }
    string emitC() override { أرجع "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvsdiv %l %r)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_DIV; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstEqWild final : عام Ast_عقدة_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
عام:
    AstEqWild(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    ساكن Ast_عقدة_Biop* newTyped(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp);
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opWildEq(lhs, rhs);
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
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstFGetS final : عام Ast_عقدة_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
عام:
    AstFGetS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$fgets(%l,%r)"; }
    string emitC() override {
        أرجع strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 64; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : عام Ast_عقدة_Biop {
عام:
    AstFUngetC(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        أرجع "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 64; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    Ast_عقدة* filep() ثابت { أرجع lhsp(); }
    Ast_عقدة* charp() ثابت { أرجع rhsp(); }
};
class AstGetcN final : عام Ast_عقدة_Biop {
    // Verilog string.getc()
عام:
    AstGetcN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() ثابت override VL_MT_STABLE { أرجع "getc"; }
    string emitVerilog() override { أرجع "%k(%l.getc(%r))"; }
    string emitC() override { أرجع "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstGetcRefN final : عام Ast_عقدة_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
عام:
    AstGetcRefN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstGt final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstGt(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f> %r)"; }
    string emitC() override { أرجع "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { أرجع ">"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstGtD final : عام Ast_عقدة_Biop {
عام:
    AstGtD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ">"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstGtN final : عام Ast_عقدة_Biop {
عام:
    AstGtN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ">"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstGtS final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstGtS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f> %r)"; }
    string emitC() override { أرجع "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstGte final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstGte(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>= %r)"; }
    string emitC() override { أرجع "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { أرجع ">="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstGteD final : عام Ast_عقدة_Biop {
عام:
    AstGteD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ">="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstGteN final : عام Ast_عقدة_Biop {
عام:
    AstGteN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع ">="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstGteS final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstGteS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>= %r)"; }
    string emitC() override { أرجع "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstLogAnd final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstLogAnd(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f&& %r)"; }
    string emitC() override { أرجع "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvand %l %r)"; }
    string emitSimpleOperator() override { أرجع "&&"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() + INSTR_عدد_BRANCH; }
};
class AstLogIf final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstLogIf(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f-> %r)"; }
    string emitC() override { أرجع "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { أرجع "->"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() + INSTR_عدد_BRANCH; }
};
class AstLogOr final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstLogOr(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f|| %r)"; }
    string emitC() override { أرجع "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvor %l %r)"; }
    string emitSimpleOperator() override { أرجع "||"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() + INSTR_عدد_BRANCH; }
};
class AstLt final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstLt(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f< %r)"; }
    string emitC() override { أرجع "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { أرجع "<"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstLtD final : عام Ast_عقدة_Biop {
عام:
    AstLtD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "<"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstLtN final : عام Ast_عقدة_Biop {
عام:
    AstLtN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "<"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstLtS final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstLtS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f< %r)"; }
    string emitC() override { أرجع "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstLte final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstLte(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<= %r)"; }
    string emitC() override { أرجع "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { أرجع "<="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstLteD final : عام Ast_عقدة_Biop {
عام:
    AstLteD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "<="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstLteN final : عام Ast_عقدة_Biop {
عام:
    AstLteN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "<="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstLteS final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstLteS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<= %r)"; }
    string emitC() override { أرجع "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstModDiv final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstModDiv(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f%% %r)"; }
    string emitC() override { أرجع "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvurem %l %r)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_DIV; }
};
class AstModDivS final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstModDivS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f%% %r)"; }
    string emitC() override { أرجع "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvsmod %l %r)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_DIV; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstNeqWild final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstNeqWild(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opWildNeq(lhs, rhs);
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
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstPow final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstPow(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_MUL * 10; }
};
class AstPowD final : عام Ast_عقدة_Biop {
عام:
    AstPowD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstPowD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "pow(%li,%ri)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL_DIV * 5; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstPowSS final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstPowSS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_MUL * 10; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstPowSU final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstPowSU(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_MUL * 10; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstPowUS final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstPowUS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f** %r)"; }
    string emitC() override { أرجع "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_MUL * 10; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstReplicate final : عام Ast_عقدة_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
عام:
    AstReplicate(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        إذا (lhsp) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(rhsp, Const)) {
                إذا (constp->رقم().isFourState()
                    || (constp->dtypep()->isSigned() && constp->رقم().isNegative())) {
                    dtypeSetBitOrLogicSized(lhsp->width(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } وإلا {
                    dtypeSetBitOrLogicSized(lhsp->width() * constp->toSInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_سطر* fl, Ast_عقدة_تعبير* lhsp, uint32_t rep_عدد)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_عدد}} {}
    ASTGEN_MEMBERS_AstReplicate;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%f{%r{%k%l}}"; }
    string emitC() override { أرجع "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override {
        أرجع "((_ repeat " + cvtToStr(width() / lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 2; }
};
class AstReplicateN final : عام Ast_عقدة_Biop {
    // String replicate
عام:
    AstReplicateN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%f{%r{%k%l}}"; }
    string emitC() override { أرجع "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 2; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstSAnd final : عام Ast_عقدة_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    ثابت bool m_propertyControl;  // Parser-generated property if/case branch conjunction
عام:
    AstSAnd(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() + INSTR_عدد_BRANCH; }
    bool isMultiCycleSva() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {  // LCOV_EXCL_LINE
        أرجع m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() ثابت { أرجع m_propertyControl; }
};
class AstSIntersect final : عام Ast_عقدة_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
عام:
    AstSIntersect(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() + INSTR_عدد_BRANCH; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstSOr final : عام Ast_عقدة_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
عام:
    AstSOr(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSOr;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() + INSTR_عدد_BRANCH; }
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstSThroughout final : عام Ast_عقدة_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
عام:
    AstSThroughout(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstSWithin final : عام Ast_عقدة_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
عام:
    AstSWithin(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() ثابت override { أرجع true; }
};
class AstSel final : عام Ast_عقدة_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    صحيح m_declElWidth;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    صحيح m_widthConst;

عام:
    AstSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* lsbp, صحيح bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicSized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, صحيح lsb, صحيح bitwidth, صحيح bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicUnsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& from, ثابت V3_رقم& bit) override {
        out.opSel(from, bit.toUInt() + widthConst() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        أرجع widthConst() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() ثابت override { أرجع "((_ extract %t %r) %l)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* otherp) ثابت override {
        أرجع widthConst() == VN_DBG_AS(otherp, Sel)->widthConst();
    }
    صحيح instr_عدد() ثابت override {
        أرجع widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    صحيح widthConst() ثابت { أرجع m_widthConst; }
    فراغ widthConst(صحيح قيمة) { m_widthConst = قيمة; }
    صحيح lsbConst() ثابت { أرجع VN_AS(lsbp(), Const)->toSInt(); }
    صحيح msbConst() ثابت { أرجع lsbConst() + widthConst() - 1; }
    VNumRange& declRange() VL_MT_STABLE { أرجع m_declRange; }
    ثابت VNumRange& declRange() ثابت VL_MT_STABLE { أرجع m_declRange; }
    فراغ declRange(ثابت VNumRange& flag) { m_declRange = flag; }
    صحيح declElWidth() ثابت { أرجع m_declElWidth; }
    فراغ declElWidth(صحيح flag) { m_declElWidth = flag; }
};
class AstShiftL final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstShiftL(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا إذا (ثابت Ast_عقدة_DType* ثابت dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftL;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<< %r)"; }
    string emitC() override { أرجع "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        أرجع (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstShiftLOvr final : عام Ast_عقدة_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
عام:
    AstShiftLOvr(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_ShiftLOvr(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا {
            ثابت Ast_عقدة_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftLOvr;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<< %r)"; }
    string emitC() override { أرجع "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstShiftR final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstShiftR(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا إذا (ثابت Ast_عقدة_DType* ثابت dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftR;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>> %r)"; }
    string emitC() override { أرجع "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        أرجع (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstShiftROvr final : عام Ast_عقدة_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
عام:
    AstShiftROvr(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_ShiftROvr(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا {
            ثابت Ast_عقدة_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftROvr;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f>> %r)"; }
    string emitC() override { أرجع "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstShiftRS final : عام Ast_عقدة_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
عام:
    AstShiftRS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        إذا (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا إذا (ثابت Ast_عقدة_DType* ثابت dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { أرجع "%k(%l %f>>> %r)"; }
    string emitSMT() ثابت override { أرجع "(bvashr %l %r)"; }
    string emitC() override { أرجع "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstShiftRSOvr final : عام Ast_عقدة_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
عام:
    AstShiftRSOvr(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, صحيح setwidth = 0)
        : ASTGEN_SUPER_ShiftRSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        إذا (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } وإلا {
            ثابت Ast_عقدة_DType* ثابت dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRSOvr;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { أرجع "%k(%l %f>>> %r)"; }
    string emitC() override { أرجع "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstSub final : عام Ast_عقدة_Biop {
    // @astgen makeDfgVertex
عام:
    AstSub(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f- %r)"; }
    string emitC() override { أرجع "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvsub %l %r)"; }
    string emitSimpleOperator() override { أرجع "-"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
};
class AstSubD final : عام Ast_عقدة_Biop {
عام:
    AstSubD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstSubD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "-"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstURandomRange final : عام Ast_عقدة_Biop {
    // $urandom_range
عام:
    AstURandomRange(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$urandom_range(%l, %r)"; }
    string emitC() override { أرجع "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : عام Ast_عقدة_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstEq(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    ساكن Ast_عقدة_Biop* newTyped(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp);
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f== %r)"; }
    string emitC() override { أرجع "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { أرجع "=="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstEqCase final : عام Ast_عقدة_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstEqCase(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f=== %r)"; }
    string emitC() override { أرجع "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع "=="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstEqD final : عام Ast_عقدة_Bi_Com {
عام:
    AstEqD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "=="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstEqN final : عام Ast_عقدة_Bi_Com {
عام:
    AstEqN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "=="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstEqT final : عام Ast_عقدة_Bi_Com {
    // Equal (==) for data types
عام:
    AstEqT(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqT;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "=="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
};
class AstLogEq final : عام Ast_عقدة_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstLogEq(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f<-> %r)"; }
    string emitC() override { أرجع "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { أرجع "<->"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() + INSTR_عدد_BRANCH; }
};
class AstNeq final : عام Ast_عقدة_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstNeq(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    ساكن Ast_عقدة_Biop* newTyped(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp);
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!= %r)"; }
    string emitC() override { أرجع "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع "!="; }
    string emitSMT() ثابت override { أرجع "(__Vbv (not (= %l %r)))"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstNeqCase final : عام Ast_عقدة_Bi_Com {
    // @astgen makeDfgVertex
عام:
    AstNeqCase(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!== %r)"; }
    string emitC() override { أرجع "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { أرجع "!="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
};
class AstNeqD final : عام Ast_عقدة_Bi_Com {
عام:
    AstNeqD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "!="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstNeqN final : عام Ast_عقدة_Bi_Com {
عام:
    AstNeqN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "!="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
    bool stringFlavor() ثابت override { أرجع true; }
};
class AstNeqT final : عام Ast_عقدة_Bi_Com {
    // Not-equal (!=) for data types
عام:
    AstNeqT(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "!="; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_STR; }
};

// === AstNodeBiComAsv ===
class AstAdd final : عام Ast_عقدة_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstAdd(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAdd;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f+ %r)"; }
    string emitC() override { أرجع "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvadd %l %r)"; }
    string emitSimpleOperator() override { أرجع "+"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
};
class AstAddD final : عام Ast_عقدة_Bi_Com_Asv {
عام:
    AstAddD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstAddD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "+"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstAnd final : عام Ast_عقدة_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstAnd(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f& %r)"; }
    string emitC() override { أرجع "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvand %l %r)"; }
    string emitSimpleOperator() override { أرجع "&"; }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    ثابت حرف* widthMismatch() ثابت override VL_MT_STABLE;
};
class AstMul final : عام Ast_عقدة_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstMul(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f* %r)"; }
    string emitC() override { أرجع "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvmul %l %r)"; }
    string emitSimpleOperator() override { أرجع "*"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_MUL; }
};
class AstMulD final : عام Ast_عقدة_Bi_Com_Asv {
عام:
    AstMulD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstMulD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "*"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstMulS final : عام Ast_عقدة_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstMulS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f* %r)"; }
    string emitC() override { أرجع "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvmul %l %r)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    bool sizeMattersRhs() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * INSTR_عدد_INT_MUL; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstOr final : عام Ast_عقدة_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstOr(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f| %r)"; }
    string emitC() override { أرجع "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvor %l %r)"; }
    string emitSimpleOperator() override { أرجع "|"; }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    ثابت حرف* widthMismatch() ثابت override VL_MT_STABLE;
};
class AstXor final : عام Ast_عقدة_Bi_Com_Asv {
    // @astgen makeDfgVertex
عام:
    AstXor(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { أرجع "%k(%l %f^ %r)"; }
    string emitC() override { أرجع "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ثابت override { أرجع "(bvxor %l %r)"; }
    string emitSimpleOperator() override { أرجع "^"; }
    bool cleanOut() ثابت override { أرجع false; }  // Lclean && Rclean
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    ثابت حرف* widthMismatch() ثابت override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class AstDistChiSquare final : عام Ast_عقدة_Dist_Biop {
عام:
    AstDistChiSquare(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_DistChiSquare(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistChiSquare;
    string emitVerilog() override { أرجع "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { أرجع "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class AstDistExponential final : عام Ast_عقدة_Dist_Biop {
عام:
    AstDistExponential(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_DistExponential(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistExponential;
    string emitVerilog() override { أرجع "%f$dist_exponential(%l, %r)"; }
    string emitC() override { أرجع "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class AstDistPoisson final : عام Ast_عقدة_Dist_Biop {
عام:
    AstDistPoisson(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_DistPoisson(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistPoisson;
    string emitVerilog() override { أرجع "%f$dist_poisson(%l, %r)"; }
    string emitC() override { أرجع "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : عام Ast_عقدة_Dist_Biop {
عام:
    AstDistT(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { أرجع "%f$dist_t(%l, %r)"; }
    string emitC() override { أرجع "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : عام Ast_عقدة_Sel {
    // @astgen makeDfgVertex
    فراغ init(ثابت Ast_عقدة* fromp) {
        إذا (fromp && VN_IS(fromp->dtypep()->skipRefp(), عقدة_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), عقدة_Array_DType)->subDTypep());
        }
    }

عام:
    AstArraySel(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_سطر* fl, Ast_عقدة_تعبير* fromp, صحيح bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() ثابت override { أرجع "(select %l %r)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
};
class AstAssocSel final : عام Ast_عقدة_Sel {
    فراغ init(ثابت Ast_عقدة* fromp) {
        إذا (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

عام:
    AstAssocSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l%f[%r])"; }
    string emitC() override { أرجع "%li%k[%ri]"; }
    string emitSMT() ثابت override { أرجع "(select %l %r)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override {
        أرجع !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع !isLValue(); }  // AssocSel creates on miss
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
};
class AstWildcardSel final : عام Ast_عقدة_Sel {
    فراغ init(ثابت Ast_عقدة* fromp) {
        إذا (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

عام:
    AstWildcardSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l%f[%r])"; }
    string emitC() override { أرجع "%li%k[%ri]"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool isGateOptimizable() ثابت override { أرجع true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
};
class AstWordSel final : عام Ast_عقدة_Sel {
    // Select a single word from a multi-word wide value
عام:
    AstWordSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    فراغ رقم_Operate(V3_رقم&, ثابت V3_رقم&, ثابت V3_رقم&) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};

// === AstNodeStream ===
class AstStreamL final : عام Ast_عقدة_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
عام:
    AstStreamL(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamL;
    string emitVerilog() override { أرجع "%f{ << %r %k{%l} }"; }
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { أرجع "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 2; }
};
class AstStreamR final : عام Ast_عقدة_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
عام:
    AstStreamR(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamR;
    string emitVerilog() override { أرجع "%f{ >> %r %k{%l} }"; }
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { أرجع isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : عام Ast_عقدة_System_Biop_D {
عام:
    AstAtan2D(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$atan2(%l,%r)"; }
    string emitC() override { أرجع "atan2(%li,%ri)"; }
};
class AstHypotD final : عام Ast_عقدة_System_Biop_D {
عام:
    AstHypotD(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$hypot(%l,%r)"; }
    string emitC() override { أرجع "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : عام Ast_عقدة_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
عام:
    AstCCall(File_سطر* fl, AstCFunc* funcp, Ast_عقدة_تعبير* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    ثابت VSelfPointerText& selfPointer() ثابت { أرجع m_selfPointer; }
    فراغ selfPointer(ثابت VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) ثابت {
        أرجع selfPointer().protect(useSelfForThis, protect());
    }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
};
class AstCMethodCall final : عام Ast_عقدة_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
عام:
    AstCMethodCall(File_سطر* fl, Ast_عقدة_تعبير* fromp, AstCFunc* funcp, Ast_عقدة_تعبير* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : عام Ast_عقدة_CCall {
    // C++ new() call
عام:
    AstCNew(File_سطر* fl, AstCFunc* funcp, Ast_عقدة_تعبير* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : عام Ast_عقدة_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

عام:
    AstConsArray(File_سطر* fl, Ast_عقدة_DType* newDTypep, Ast_عقدة_تعبير* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    ثابت حرف* broken() ثابت override;
    فراغ cloneRelink() override;
    مضمن AstConsArrayItem* itemsp() ثابت VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_عقدة_تعبير* valuep(uint64_t index) ثابت;
    // Add the value of the element at the index, which must not be given yet
    فراغ valuep(uint64_t index, Ast_عقدة_تعبير* newp);
    // Add the elements of the array 'arrayp', starting at the index
    فراغ splice_قيم(uint64_t index, Ast_عقدة_تعبير* arrayp);
};
class AstConsAssoc final : عام Ast_عقدة_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
عام:
    explicit AstConsAssoc(File_سطر* fl, Ast_عقدة_تعبير* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        أرجع nullptr;
    }
};
class AstConsDynArray final : عام Ast_عقدة_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
عام:
    explicit AstConsDynArray(File_سطر* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    ثابت حرف* broken() ثابت override;
};
class AstConsQueue final : عام Ast_عقدة_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
عام:
    explicit AstConsQueue(File_سطر* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        أرجع nullptr;
    }
};
class AstConsStruct final : عام Ast_عقدة_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
عام:
    AstConsStruct(File_سطر* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    ثابت حرف* broken() ثابت override;
    // The value of the member called 'name'
    Ast_عقدة_تعبير* memberValuep(ثابت string& name) ثابت;
};
class AstConsWildcard final : عام Ast_عقدة_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
عام:
    explicit AstConsWildcard(File_سطر* fl, Ast_عقدة_تعبير* defaultp = nullptr)
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
class AstFuncRef final : عام Ast_عقدة_FTask_Ref {
    // A reference to a function
عام:
    مضمن AstFuncRef(File_سطر* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_سطر* fl, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class AstMethodCall final : عام Ast_عقدة_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
عام:
    AstMethodCall(File_سطر* fl, Ast_عقدة_تعبير* fromp, VFlagChildDType, ثابت string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMethodCall(File_سطر* fl, Ast_عقدة_تعبير* fromp, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_AstMethodCall;
};
class AstNew final : عام Ast_عقدة_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
عام:
    AstNew(File_سطر* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool isImplicit() ثابت { أرجع m_isImplicit; }
    فراغ isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() ثابت { أرجع m_isScoped; }
    فراغ isScoped(bool flag) { m_isScoped = flag; }
    bool isPure() override { أرجع false; }
};
class AstTaskRef final : عام Ast_عقدة_FTask_Ref {
    // A reference to a task
عام:
    مضمن AstTaskRef(File_سطر* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_سطر* fl, ثابت string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtypeSetVoid();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : عام Ast_عقدة_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
خاص:
    VAccess m_access;  // Left hand side assignment
عام:
    AstSelBit(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    VAccess access() ثابت { أرجع m_access; }
    فراغ access(ثابت VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : عام Ast_عقدة_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
عام:
    AstSelExtract(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* msbp, Ast_عقدة_تعبير* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : عام Ast_عقدة_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
عام:
    AstSelMinus(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* bitp, Ast_عقدة_تعبير* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : عام Ast_عقدة_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
عام:
    AstSelPlus(File_سطر* fl, Ast_عقدة_تعبير* fromp, Ast_عقدة_تعبير* bitp, Ast_عقدة_تعبير* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_عدد_Bits final : عام Ast_عقدة_Quadop {
    // Number of bits set in vector
عام:
    Ast_عدد_Bits(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* ctrl1p)
        : ASTGEN_SUPER_عدد_Bits(fl, exprp, ctrl1p, ctrl1p->clone_شجرة_Pure(false),
                                 ctrl1p->clone_شجرة_Pure(false)) {
        dtypeSetInt();
    }
    Ast_عدد_Bits(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* ctrl1p, Ast_عقدة_تعبير* ctrl2p)
        : ASTGEN_SUPER_عدد_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_شجرة_Pure(false)) {
        dtypeSetInt();
    }
    Ast_عدد_Bits(File_سطر* fl, Ast_عقدة_تعبير* exprp, Ast_عقدة_تعبير* ctrl1p, Ast_عقدة_تعبير* ctrl2p,
                 Ast_عقدة_تعبير* ctrl3p)
        : ASTGEN_SUPER_عدد_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_عدد_Bits;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& تعبير, ثابت V3_رقم& ctrl1,
                       ثابت V3_رقم& ctrl2, ثابت V3_رقم& ctrl3) override {
        out.op_عدد_Bits(تعبير, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { أرجع "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool cleanThs() ثابت override { أرجع true; }
    bool cleanFhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool sizeMattersThs() ثابت override { أرجع false; }
    bool sizeMattersFhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : عام Ast_عقدة_Termop {
عام:
    explicit AstInferredDisable(File_سطر* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { أرجع "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool isSystemFunc() ثابت override { أرجع true; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstTime final : عام Ast_عقدة_Termop {
    VTimescale m_timeunit;  // Parent module time unit
عام:
    AstTime(File_سطر* fl, ثابت VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { أرجع "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_TIME; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : عام Ast_عقدة_Termop {
    VTimescale m_timeunit;  // Parent module time unit
عام:
    AstTimeD(File_سطر* fl, ثابت VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { أرجع "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_TIME; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : عام Ast_عقدة_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
عام:
    AstCond(File_سطر* fl, Ast_عقدة_تعبير* condp, Ast_عقدة_تعبير* thenp, Ast_عقدة_تعبير* elsep);
    ASTGEN_MEMBERS_AstCond;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs,
                       ثابت V3_رقم& ths) override {
        out.opAssign(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { أرجع "%k(%l %f? %r %k: %t)"; }
    string emitC() override { أرجع "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() ثابت override { أرجع "(ite (__Vbool %l) %r %t)"; }
    bool cleanOut() ثابت override { أرجع false; }  // clean if e1 & e2 clean
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع false; }
    bool cleanThs() ثابت override { أرجع false; }  // Propagates up
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool sizeMattersThs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_BRANCH; }
};
class AstPutcN final : عام Ast_عقدة_Triop {
    // Verilog string.putc()
عام:
    AstPutcN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, Ast_عقدة_تعبير* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstPutcN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs,
                       ثابت V3_رقم& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() ثابت override VL_MT_STABLE { أرجع "putc"; }
    string emitVerilog() override { أرجع "%k(%l.putc(%r,%t))"; }
    string emitC() override { أرجع "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool cleanThs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool sizeMattersThs() ثابت override { أرجع false; }
};
class AstSliceSel final : عام Ast_عقدة_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
عام:
    AstSliceSel(File_سطر* fl, Ast_عقدة_تعبير* fromp, ثابت VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_AstSliceSel;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& from, ثابت V3_رقم& lo,
                       ثابت V3_رقم& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool cleanThs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool sizeMattersThs() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة*) ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { أرجع m_declRange; }
    ثابت VNumRange& declRange() ثابت VL_MT_STABLE { أرجع m_declRange; }
};
class AstSubstrN final : عام Ast_عقدة_Triop {
    // Verilog string.substr()
عام:
    AstSubstrN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, Ast_عقدة_تعبير* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs,
                       ثابت V3_رقم& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() ثابت override VL_MT_STABLE { أرجع "substr"; }
    string emitVerilog() override { أرجع "%k(%l.substr(%r,%t))"; }
    string emitC() override { أرجع "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { أرجع ""; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool cleanRhs() ثابت override { أرجع true; }
    bool cleanThs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool sizeMattersRhs() ثابت override { أرجع false; }
    bool sizeMattersThs() ثابت override { أرجع false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : عام Ast_عقدة_Dist_Triop {
عام:
    AstDistErlang(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, Ast_عقدة_تعبير* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { أرجع "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { أرجع "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class AstDistNormal final : عام Ast_عقدة_Dist_Triop {
عام:
    AstDistNormal(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, Ast_عقدة_تعبير* thsp)
        : ASTGEN_SUPER_DistNormal(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistNormal;
    string emitVerilog() override { أرجع "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { أرجع "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class AstDistUniform final : عام Ast_عقدة_Dist_Triop {
عام:
    AstDistUniform(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_تعبير* rhsp, Ast_عقدة_تعبير* thsp)
        : ASTGEN_SUPER_DistUniform(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistUniform;
    string emitVerilog() override { أرجع "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { أرجع "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : عام Ast_عقدة_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
عام:
    تعداد FmtType : صحيح { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

خاص:
    // dist-ast-dump-suppress  // Part of name()
    ثابت FmtType m_fmtType;  // Operation type
عام:
    AstAtoN(File_سطر* fl, Ast_عقدة_تعبير* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtypeSetDouble() : dtypeSetInteger();
    }
    ASTGEN_MEMBERS_AstAtoN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opAtoN(lhs, m_fmtType); }
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
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    FmtType fmtType() ثابت { أرجع m_fmtType; }
};
class AstBitsToRealD final : عام Ast_عقدة_Uniop {
عام:
    AstBitsToRealD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstBitsToRealD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { أرجع "%f$bitstoreal(%l)"; }
    string emitC() override { أرجع "VL_CVT_D_Q(%li)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool sizeMattersLhs() ثابت override { أرجع false; }  // Eliminated before matters
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstCCast final : عام Ast_عقدة_Uniop {
    // Cast to C-based data type
    صحيح m_size;

عام:
    AstCCast(File_سطر* fl, Ast_عقدة_تعبير* lhsp, صحيح setwidth, صحيح minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_size = setwidth;
        إذا (setwidth) {
            إذا (minwidth == -1) minwidth = setwidth;
            dtypeSetBitUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_size = width();
    }
    ASTGEN_MEMBERS_AstCCast;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { أرجع "%f$_CAST(%l)"; }
    string emitC() override { أرجع "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }  // Special cased in V3Cast
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        أرجع size() == VN_DBG_AS(samep, CCast)->size();
    }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    //
    صحيح size() ثابت { أرجع m_size; }
};
class AstCLog2 final : عام Ast_عقدة_Uniop {
عام:
    AstCLog2(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { أرجع "%f$clog2(%l)"; }
    string emitC() override { أرجع "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 16; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstCastWrap final : عام Ast_عقدة_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
عام:
    AstCastWrap(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { أرجع "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع 0; }
};
class Ast_عدد_Ones final : عام Ast_عقدة_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
عام:
    Ast_عدد_Ones(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_عدد_Ones(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_عدد_Ones;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.op_عدد_Ones(lhs); }
    string emitVerilog() override { أرجع "%f$countones(%l)"; }
    string emitC() override { أرجع "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 16; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstCvtPackString final : عام Ast_عقدة_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
عام:
    AstCvtPackString(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* /*samep*/) ثابت override { أرجع true; }
};
class AstExtend final : عام Ast_عقدة_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
عام:
    AstExtend(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_سطر* fl, Ast_عقدة_تعبير* lhsp, صحيح width, صحيح widthMin = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetBitOrLogicUnsized(width, widthMin ? widthMin : width, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { أرجع "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override {
        أرجع "((_ zero_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override {
        أرجع false;  // Because the EXTEND operator self-casts
    }
    صحيح instr_عدد() ثابت override { أرجع 0; }
};
class AstExtendS final : عام Ast_عقدة_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
عام:
    AstExtendS(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_سطر* fl, Ast_عقدة_تعبير* lhsp, صحيح width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { أرجع "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override {
        أرجع "((_ sign_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override {
        أرجع false;  // Because the EXTEND operator self-casts
    }
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool signedFlavor() ثابت override { أرجع true; }
};
class AstFEof final : عام Ast_عقدة_Uniop {
عام:
    AstFEof(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$feof(%l)"; }
    string emitC() override { أرجع "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 16; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ثابت override { أرجع true; }
    Ast_عقدة* filep() ثابت { أرجع lhsp(); }
};
class AstFGetC final : عام Ast_عقدة_Uniop {
عام:
    AstFGetC(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { أرجع "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 64; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool isPure() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    Ast_عقدة* filep() ثابت { أرجع lhsp(); }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstISToRD final : عام Ast_عقدة_Uniop {
    // $itor where lhs is signed
عام:
    AstISToRD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstISToRD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { أرجع "%f$itor($signed(%l))"; }
    string emitC() override { أرجع "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_كلمات() override { أرجع true; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstIToRD final : عام Ast_عقدة_Uniop {
    // $itor where lhs is unsigned
عام:
    AstIToRD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstIToRD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { أرجع "%f$itor(%l)"; }
    string emitC() override { أرجع "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstIsUnbounded final : عام Ast_عقدة_Uniop {
    // True if is unbounded ($)
عام:
    AstIsUnbounded(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { أرجع "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstIsUnknown final : عام Ast_عقدة_Uniop {
    // True if any unknown bits
عام:
    AstIsUnknown(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { أرجع "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstLenN final : عام Ast_عقدة_Uniop {
    // Length of a string
عام:
    AstLenN(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstLenN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { أرجع "%f(%l)"; }
    string emitC() override { أرجع "VL_LEN_IN(%li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
};
class AstLogNot final : عام Ast_عقدة_Uniop {
    // @astgen makeDfgVertex
خاص:
    ثابت bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
عام:
    AstLogNot(File_سطر* fl, Ast_عقدة_تعبير* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { أرجع "%f(! %l)"; }
    string emitC() override { أرجع "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ثابت override { أرجع "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { أرجع "!"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool fromProperty() ثابت { أرجع m_fromProperty; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstMostSetBitP1 final : عام Ast_عقدة_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
عام:
    AstMostSetBitP1(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_MostSetBitP1(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstMostSetBitP1;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opMostSetBitP1(lhs); }
    string emitVerilog() override { أرجع "%f$mostsetbitp1(%l)"; }
    string emitC() override { أرجع "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 16; }
};
class AstNToI final : عام Ast_عقدة_Uniop {
    // String to any-size integral
عام:
    AstNToI(File_سطر* fl, Ast_عقدة_تعبير* lhsp, Ast_عقدة_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opNToI(lhs); }
    string emitVerilog() override { أرجع "'(%l)"; }
    string emitC() override { أرجع "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
};
class AstNegate final : عام Ast_عقدة_Uniop {
    // @astgen makeDfgVertex
عام:
    AstNegate(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { أرجع "%f(- %l)"; }
    string emitC() override { أرجع "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() ثابت override { أرجع "(bvneg %l)"; }
    string emitSimpleOperator() override { أرجع "-"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
};
class AstNegateD final : عام Ast_عقدة_Uniop {
عام:
    AstNegateD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstNegateD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { أرجع "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { أرجع "-"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool doubleFlavor() ثابت override { أرجع true; }
};
class AstNot final : عام Ast_عقدة_Uniop {
    // @astgen makeDfgVertex
عام:
    AstNot(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { أرجع "%f(~ %l)"; }
    string emitC() override { أرجع "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() ثابت override { أرجع "(bvnot %l)"; }
    string emitSimpleOperator() override { أرجع "~"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع true; }
    ثابت حرف* widthMismatch() ثابت override VL_MT_STABLE;
};
class AstNullCheck final : عام Ast_عقدة_Uniop {
    // Return LHS after checking that LHS is non-null
عام:
    AstNullCheck(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { V3ERROR_NA; }
    صحيح instr_عدد() ثابت override { أرجع 1; }  // Rarely executes
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override { أرجع fileline() == samep->fileline(); }
};
class AstOneHot final : عام Ast_عقدة_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
عام:
    AstOneHot(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { أرجع "%f$onehot(%l)"; }
    string emitC() override { أرجع "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 4; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstOneHot0 final : عام Ast_عقدة_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
عام:
    AstOneHot0(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { أرجع "%f$onehot0(%l)"; }
    string emitC() override { أرجع "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs() * 3; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstPostDec final : عام Ast_عقدة_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
عام:
    AstPostDec(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { أرجع "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : عام Ast_عقدة_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
عام:
    AstPostInc(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { أرجع "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : عام Ast_عقدة_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
عام:
    AstPreDec(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { أرجع "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : عام Ast_عقدة_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
عام:
    AstPreInc(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { أرجع "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ثابت override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() ثابت override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : عام Ast_عقدة_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
عام:
    AstRToIRoundS(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { أرجع "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        أرجع isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstRToIS final : عام Ast_عقدة_Uniop {
    // $rtoi(lhs)
عام:
    AstRToIS(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { أرجع "%f$rtoi(%l)"; }
    string emitC() override { أرجع "VL_RTOI_I_D(%li)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool sizeMattersLhs() ثابت override { أرجع false; }  // Eliminated before matters
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstRealToBits final : عام Ast_عقدة_Uniop {
عام:
    AstRealToBits(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstRealToBits;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { أرجع "%f$realtobits(%l)"; }
    string emitC() override { أرجع "VL_CVT_Q_D(%li)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool sizeMattersLhs() ثابت override { أرجع false; }  // Eliminated before matters
    صحيح instr_عدد() ثابت override { أرجع INSTR_عدد_DBL; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstRedAnd final : عام Ast_عقدة_Uniop {
    // @astgen makeDfgVertex
عام:
    AstRedAnd(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { أرجع "%f(& %l)"; }
    string emitC() override { أرجع "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
};
class AstRedOr final : عام Ast_عقدة_Uniop {
    // @astgen makeDfgVertex
عام:
    AstRedOr(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { أرجع "%f(| %l)"; }
    string emitC() override { أرجع "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
};
class AstRedXor final : عام Ast_عقدة_Uniop {
    // @astgen makeDfgVertex
عام:
    AstRedXor(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { أرجع "%f(^ %l)"; }
    string emitC() override { أرجع "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override {
        ثابت صحيح w = lhsp()->width();
        أرجع (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع 1 + V3_رقم::log2b(width()); }
};
class AstResizeLValue final : عام Ast_عقدة_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
عام:
    AstResizeLValue(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    صحيح instr_عدد() ثابت override { أرجع 0; }
};
class AstSigned final : عام Ast_عقدة_Uniop {
    // $signed(lhs)
عام:
    AstSigned(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { أرجع "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool sizeMattersLhs() ثابت override { أرجع true; }  // Eliminated before matters
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool isSystemFunc() ثابت override { أرجع true; }
};
class AstTimeImport final : عام Ast_عقدة_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
عام:
    AstTimeImport(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
};
class AstToLowerN final : عام Ast_عقدة_Uniop {
    // string.tolower()
عام:
    AstToLowerN(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToLowerN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { أرجع "%l.tolower()"; }
    string emitC() override { أرجع "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
};
class AstToStringN final : عام Ast_عقدة_Uniop {
عام:
    AstToStringN(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_ToStringN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToStringN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { أرجع "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        أرجع isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
};
class AstToUpperN final : عام Ast_عقدة_Uniop {
    // string.toupper()
عام:
    AstToUpperN(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToUpperN;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { أرجع "%l.toupper()"; }
    string emitC() override { أرجع "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() ثابت override { أرجع true; }
    bool cleanLhs() ثابت override { أرجع true; }
    bool sizeMattersLhs() ثابت override { أرجع false; }
};
class AstUnsigned final : عام Ast_عقدة_Uniop {
    // $unsigned(lhs)
عام:
    AstUnsigned(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { أرجع "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع false; }
    bool cleanLhs() ثابت override { أرجع false; }  // Eliminated before matters
    bool sizeMattersLhs() ثابت override { أرجع true; }  // Eliminated before matters
    صحيح instr_عدد() ثابت override { أرجع 0; }
    bool isSystemFunc() ثابت override { أرجع true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstAcosD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$acos(%l)"; }
    string emitC() override { أرجع "acos(%li)"; }
};
class AstAcoshD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstAcoshD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$acosh(%l)"; }
    string emitC() override { أرجع "acosh(%li)"; }
};
class AstAsinD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstAsinD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$asin(%l)"; }
    string emitC() override { أرجع "asin(%li)"; }
};
class AstAsinhD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstAsinhD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$asinh(%l)"; }
    string emitC() override { أرجع "asinh(%li)"; }
};
class AstAtanD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstAtanD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$atan(%l)"; }
    string emitC() override { أرجع "atan(%li)"; }
};
class AstAtanhD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstAtanhD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$atanh(%l)"; }
    string emitC() override { أرجع "atanh(%li)"; }
};
class AstCeilD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstCeilD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::ceil(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$ceil(%l)"; }
    string emitC() override { أرجع "ceil(%li)"; }
};
class AstCosD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstCosD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$cos(%l)"; }
    string emitC() override { أرجع "cos(%li)"; }
};
class AstCoshD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstCoshD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$cosh(%l)"; }
    string emitC() override { أرجع "cosh(%li)"; }
};
class AstExpD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstExpD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$exp(%l)"; }
    string emitC() override { أرجع "exp(%li)"; }
};
class AstFloorD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstFloorD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::floor(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$floor(%l)"; }
    string emitC() override { أرجع "floor(%li)"; }
};
class AstLog10D final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstLog10D(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$log10(%l)"; }
    string emitC() override { أرجع "log10(%li)"; }
};
class AstLogD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstLogD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$ln(%l)"; }
    string emitC() override { أرجع "log(%li)"; }
};
class AstSinD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstSinD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$sin(%l)"; }
    string emitC() override { أرجع "sin(%li)"; }
};
class AstSinhD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstSinhD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$sinh(%l)"; }
    string emitC() override { أرجع "sinh(%li)"; }
};
class AstSqrtD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstSqrtD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::sqrt(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$sqrt(%l)"; }
    string emitC() override { أرجع "sqrt(%li)"; }
};
class AstTanD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstTanD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$tan(%l)"; }
    string emitC() override { أرجع "tan(%li)"; }
};
class AstTanhD final : عام Ast_عقدة_System_Uniop_D {
عام:
    AstTanhD(File_سطر* fl, Ast_عقدة_تعبير* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    فراغ رقم_Operate(V3_رقم& out, ثابت V3_رقم& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { أرجع "%f$tanh(%l)"; }
    string emitC() override { أرجع "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : عام Ast_عقدة_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
عام:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    مضمن AstVarRef(File_سطر* fl, AstVar* varp, ثابت VAccess& access);
    مضمن AstVarRef(File_سطر* fl, Ast_عقدة_Module* classOrPackagep, AstVar* varp,
                     ثابت VAccess& access);
    // This form only allowed post-link (see above)
    مضمن AstVarRef(File_سطر* fl, AstVarScope* varscp, ثابت VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() ثابت { أرجع m_name; }
    مضمن string name() ثابت override;  // * = Var name
    فراغ name(ثابت string& name) override { m_name = name; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override;
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override;
    مضمن bool same_عقدة(ثابت AstVarRef* samep) ثابت;
    مضمن bool sameNoLvalue(ثابت AstVarRef* samep) ثابت;
    صحيح instr_عدد() ثابت override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
};
class AstVarXRef final : عام Ast_عقدة_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
عام:
    AstVarXRef(File_سطر* fl, ثابت string& name, ثابت string& dotted, ثابت VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    مضمن AstVarXRef(File_سطر* fl, AstVar* varp, ثابت string& dotted, ثابت VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    فراغ name(ثابت std::string& name) override { m_name = name; }  // * = Var name
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string dotted() ثابت { أرجع m_dotted; }
    فراغ dotted(ثابت string& dotted) { m_dotted = dotted; }
    string inlinedDots() ثابت { أرجع m_inlinedDots; }
    فراغ inlinedDots(ثابت string& flag) { m_inlinedDots = flag; }
    bool containsGenBlock() ثابت { أرجع m_containsGenBlock; }
    فراغ containsGenBlock(ثابت bool flag) { m_containsGenBlock = flag; }
    bool readOnlyModport() ثابت { أرجع m_readOnlyModport; }
    فراغ readOnlyModport(ثابت bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() ثابت override { أرجع true; }
    صحيح instr_عدد() ثابت override { أرجع widthInstrs(); }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        أرجع (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#endif  // Guard
