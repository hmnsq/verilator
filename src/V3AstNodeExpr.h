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
#அந்தர்பாவ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_முனை_கோவை VL_NOT_FINAL : பொது Ast_முனை {
    // An expression tree node
பாதுகாக்கப்பட்ட:
    Ast_முனை_கோவை(VNType t, File_வரி* fl)
        : Ast_முனை{t, fl} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_கோவை;
    // METHODS
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    // Every expression must have a data type after V3Width
    bool hasDType() மாறிலி override final VL_MT_SAFE { திரும்பு true; }
    மெய்நிகர் string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    மெய்நிகர் string emitC() = 0;
    மெய்நிகர் string emitSMT() மாறிலி { திரும்பு ""; };
    மெய்நிகர் string emitSimpleOperator() { திரும்பு ""; }  // "" means not ok to use
    மெய்நிகர் bool emit_Check_Max_சொற்கள்() { திரும்பு false; }  // Check VL_MULS_MAX_WORDS
    மெய்நிகர் bool cleanOut() மாறிலி = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    மெய்நிகர் bool isMultiCycleSva() மாறிலி { திரும்பு false; }
    மாறிலி Ast_முனை_கோவை* getVAccessTargetRecurse() மாறிலி;
    Ast_முனை_கோவை* getVAccessTargetRecurse() {
        திரும்பு const_cast<Ast_முனை_கோவை*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<மாறிலி Ast_முனை_கோவை*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() மாறிலி;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() மாறிலி { திரும்பு getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_மரம்() மாறிலி;
    // Return base var (or const) nodep dereferences
    Ast_முனை* baseFromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    உள்வரி Ast_Stmt_கோவை* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    மெய்நிகர் மாறிலி எழுத்து* widthMismatch() மாறிலி VL_MT_STABLE { திரும்பு nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string patternString(uint32_t depth = 0) மாறிலி;
    bool isFourstateOrNoDtype() மாறிலி {
        எனில் (!dtypep()) திரும்பு true;
        திரும்பு dtypep()->skipRefp()->isFourstate();
    }
    நிலையான bool isFourstateOrNoDtype(மாறிலி Ast_முனை_கோவை* மாறிலி expr1p) {
        திரும்பு expr1p->isFourstateOrNoDtype();
    }
    நிலையான bool isFourstateOrNoDtype(மாறிலி Ast_முனை_கோவை* மாறிலி expr1p,
                                     மாறிலி Ast_முனை_கோவை* மாறிலி expr2p) {
        திரும்பு expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    நிலையான bool isFourstateOrNoDtype(மாறிலி Ast_முனை_கோவை* மாறிலி expr1p,
                                     மாறிலி Ast_முனை_கோவை* மாறிலி expr2p,
                                     மாறிலி Ast_முனை_கோவை* மாறிலி expr3p) {
        திரும்பு isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    நிலையான bool isFourstateOrNoDtype(மாறிலி Ast_முனை_கோவை* மாறிலி expr1p,
                                     மாறிலி Ast_முனை_கோவை* மாறிலி expr2p,
                                     மாறிலி Ast_முனை_கோவை* மாறிலி expr3p,
                                     மாறிலி Ast_முனை_கோவை* மாறிலி expr4p) {
        திரும்பு isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_முனை_Biop VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

பாதுகாக்கப்பட்ட:
    Ast_முனை_Biop(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : Ast_முனை_கோவை{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    மெய்நிகர் வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) = 0;
    மெய்நிகர் bool cleanLhs() மாறிலி = 0;  // True if LHS must have extra upper bits zero
    மெய்நிகர் bool cleanRhs() மாறிலி = 0;  // True if RHS must have extra upper bits zero
    மெய்நிகர் bool sizeMattersLhs() மாறிலி = 0;  // True if output result depends on lhs size
    மெய்நிகர் bool sizeMattersRhs() மாறிலி = 0;  // True if output result depends on rhs size
    மெய்நிகர் bool doubleFlavor() மாறிலி { திரும்பு false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    மெய்நிகர் bool signedFlavor() மாறிலி { திரும்பு false; }
    மெய்நிகர் bool stringFlavor() மாறிலி { திரும்பு false; }  // N flavor of nodes with both flavors?
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    bool isPure() override;
    மாறிலி எழுத்து* broken() மாறிலி override;

தனிப்பட்ட:
    bool getPurityRecurse() மாறிலி { திரும்பு lhsp()->isPure() && rhsp()->isPure(); }
};
class Ast_முனை_Bi_Com VL_NOT_FINAL : பொது Ast_முனை_Biop {
    // Binary expr with commutative properties
பாதுகாக்கப்பட்ட:
    Ast_முனை_Bi_Com(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhs, Ast_முனை_கோவை* rhs)
        : Ast_முனை_Biop{t, fl, lhs, rhs} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Bi_Com;
};
class Ast_முனை_Bi_Com_Asv VL_NOT_FINAL : பொது Ast_முனை_Bi_Com {
    // Binary expr with commutative & associative properties
பாதுகாக்கப்பட்ட:
    Ast_முனை_Bi_Com_Asv(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhs, Ast_முனை_கோவை* rhs)
        : Ast_முனை_Bi_Com{t, fl, lhs, rhs} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Bi_Com_Asv;
};
class Ast_முனை_Dist_Biop VL_NOT_FINAL : பொது Ast_முனை_Biop {
பொது:
    Ast_முனை_Dist_Biop(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : Ast_முனை_Biop{t, fl, lhsp, rhsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_முனை_Dist_Biop;
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL_TRIG; }
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
};
class Ast_முனை_Sel VL_NOT_FINAL : பொது Ast_முனை_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
பாதுகாக்கப்பட்ட:
    Ast_முனை_Sel(VNType t, File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* bitp)
        : Ast_முனை_Biop{t, fl, fromp, bitp} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Sel;
    முழு bitConst() மாறிலி;
};
class Ast_முனை_Stream VL_NOT_FINAL : பொது Ast_முனை_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
பாதுகாக்கப்பட்ட:
    Ast_முனை_Stream(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : Ast_முனை_Biop{t, fl, lhsp, rhsp} {
        எனில் (lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(lhsp->width(), lhsp->widthMin(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Stream;
};
class Ast_முனை_System_Biop_D VL_NOT_FINAL : பொது Ast_முனை_Biop {
பொது:
    Ast_முனை_System_Biop_D(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : Ast_முனை_Biop{t, fl, lhsp, rhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_முனை_System_Biop_D;
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL_TRIG; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class Ast_முனை_CCall VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

பாதுகாக்கப்பட்ட:
    Ast_முனை_CCall(VNType t, File_வரி* fl, AstCFunc* funcp, Ast_முனை_கோவை* argsp = nullptr)
        : Ast_முனை_கோவை{t, fl}
        , m_funcp{funcp} {
        addArgsp(argsp);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_CCall;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_CALL; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி Ast_முனை_CCall* மாறிலி asamep = VN_DBG_AS(samep, முனை_CCall);
        திரும்பு (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override;
    bool isOutputter() override { திரும்பு !isPure(); }
    AstCFunc* funcp() மாறிலி { திரும்பு m_funcp; }
    வெறுமை funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() மாறிலி { திரும்பு m_argTypes; }
    வெறுமை argTypes(மாறிலி string& str) { m_argTypes = str; }
    bool isUnlikely() மாறிலி override { திரும்பு m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி final override { திரும்பு true; }
    bool superReference() மாறிலி { திரும்பு m_superReference; }
    வெறுமை superReference(bool flag) { m_superReference = flag; }
};
class Ast_முனை_Cons VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // Construct a value of an aggregate type
    friend class Ast_முனை_கோவை;  // For m_knownConstantTree
    mutable bool m_known_Constant_மரம் = false;  // Known to be 'this->isConstantTree() == true'

பாதுகாக்கப்பட்ட:
    Ast_முனை_Cons(VNType t, File_வரி* fl)
        : Ast_முனை_கோவை{t, fl} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Cons;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி final override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி final override { திரும்பு widthInstrs(); }
};
class Ast_முனை_FTask_Ref VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_முனை_FTask_Ref;  // Gen pointers before other members for performance
தனிப்பட்ட:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

பாதுகாக்கப்பட்ட:
    Ast_முனை_FTask_Ref(VNType t, File_வரி* fl, மாறிலி string& name, AstArg* argsp)
        : Ast_முனை_கோவை{t, fl}
        , m_name{name} {
        addArgsp(argsp);
    }

பொது:
    மாறிலி எழுத்து* broken() மாறிலி override;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    bool isGateOptimizable() மாறிலி override;
    string dotted() மாறிலி { திரும்பு m_dotted; }  // * = Scope name or ""
    string inlinedDots() மாறிலி { திரும்பு m_inlinedDots; }
    வெறுமை inlinedDots(மாறிலி string& flag) { m_inlinedDots = flag; }
    Ast_முனை_FTask* taskp() மாறிலி { திரும்பு m_taskp; }  // [After Link] Pointer to variable
    வெறுமை taskp(Ast_முனை_FTask* taskp) { m_taskp = taskp; }
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    வெறுமை dotted(மாறிலி string& name) { m_dotted = name; }
    Ast_முனை_Module* classOrPackagep() மாறிலி { திரும்பு m_classOrPackagep; }
    வெறுமை classOrPackagep(Ast_முனை_Module* nodep) { m_classOrPackagep = nodep; }
    bool containsGenBlock() மாறிலி { திரும்பு m_containsGenBlock; }
    வெறுமை containsGenBlock(மாறிலி bool flag) { m_containsGenBlock = flag; }
    bool pli() மாறிலி { திரும்பு m_pli; }
    வெறுமை pli(bool flag) { m_pli = flag; }
    bool superReference() மாறிலி { திரும்பு m_superReference; }
    வெறுமை superReference(bool flag) { m_superReference = flag; }
    bool isPure() override;
    bool isUnlikely() மாறிலி override { திரும்பு m_taskp && m_taskp->isUnlikely(); }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி Ast_முனை_FTask_Ref* மாறிலி asamep = VN_DBG_AS(samep, முனை_FTask_Ref);
        திரும்பு taskp() == asamep->taskp()  //
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
    bool cleanOut() மாறிலி final override { V3ERROR_NA_RETURN(true); }

தனிப்பட்ட:
    bool getPurityRecurse() மாறிலி;
};
class Ast_முனை_Pre_Sel VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

பாதுகாக்கப்பட்ட:
    Ast_முனை_Pre_Sel(VNType t, File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* rhsp, Ast_முனை_கோவை* thsp)
        : Ast_முனை_கோவை{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Pre_Sel;
    // METHODS
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி final override { V3ERROR_NA_RETURN(true); }
    bool isPure() override;
    மாறிலி எழுத்து* broken() மாறிலி override;

தனிப்பட்ட:
    bool getPurityRecurse() மாறிலி {
        திரும்பு fromp()->isPure() && rhsp()->isPure() && (!thsp() || thsp()->isPure());
    }
};
class Ast_முனை_Quadop VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

பாதுகாக்கப்பட்ட:
    Ast_முனை_Quadop(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, Ast_முனை_கோவை* thsp,
                  Ast_முனை_கோவை* fhsp)
        : Ast_முனை_கோவை{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    மெய்நிகர் வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs,
                               மாறிலி V3_எண்& ths, மாறிலி V3_எண்& fhs)
        = 0;
    மெய்நிகர் bool cleanLhs() மாறிலி = 0;  // True if LHS must have extra upper bits zero
    மெய்நிகர் bool cleanRhs() மாறிலி = 0;  // True if RHS must have extra upper bits zero
    மெய்நிகர் bool cleanThs() மாறிலி = 0;  // True if THS must have extra upper bits zero
    மெய்நிகர் bool cleanFhs() மாறிலி = 0;  // True if THS must have extra upper bits zero
    மெய்நிகர் bool sizeMattersLhs() மாறிலி = 0;  // True if output result depends on lhs size
    மெய்நிகர் bool sizeMattersRhs() மாறிலி = 0;  // True if output result depends on rhs size
    மெய்நிகர் bool sizeMattersThs() மாறிலி = 0;  // True if output result depends on ths size
    மெய்நிகர் bool sizeMattersFhs() மாறிலி = 0;  // True if output result depends on ths size
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    bool isPure() override;
    மாறிலி எழுத்து* broken() மாறிலி override;

தனிப்பட்ட:
    bool getPurityRecurse() மாறிலி {
        திரும்பு lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure() && fhsp()->isPure();
    }
};
class Ast_முனை_Termop VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // Terminal operator -- an operator with no "inputs"
பாதுகாக்கப்பட்ட:
    Ast_முனை_Termop(VNType t, File_வரி* fl)
        : Ast_முனை_கோவை{t, fl} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Termop;
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class Ast_முனை_Triop VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

பாதுகாக்கப்பட்ட:
    Ast_முனை_Triop(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, Ast_முனை_கோவை* thsp)
        : Ast_முனை_கோவை{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Triop;
    // METHODS
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    // Set out to evaluation of a AstConst'ed
    மெய்நிகர் வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs,
                               மாறிலி V3_எண்& ths)
        = 0;
    மெய்நிகர் bool cleanLhs() மாறிலி = 0;  // True if LHS must have extra upper bits zero
    மெய்நிகர் bool cleanRhs() மாறிலி = 0;  // True if RHS must have extra upper bits zero
    மெய்நிகர் bool cleanThs() மாறிலி = 0;  // True if THS must have extra upper bits zero
    மெய்நிகர் bool sizeMattersLhs() மாறிலி = 0;  // True if output result depends on lhs size
    மெய்நிகர் bool sizeMattersRhs() மாறிலி = 0;  // True if output result depends on rhs size
    மெய்நிகர் bool sizeMattersThs() மாறிலி = 0;  // True if output result depends on ths size
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    bool isPure() override;
    மாறிலி எழுத்து* broken() மாறிலி override;

தனிப்பட்ட:
    bool getPurityRecurse() மாறிலி {
        திரும்பு lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure();
    }
};
class Ast_முனை_Dist_Triop VL_NOT_FINAL : பொது Ast_முனை_Triop {
பொது:
    Ast_முனை_Dist_Triop(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp,
                     Ast_முனை_கோவை* thsp)
        : Ast_முனை_Triop{t, fl, lhsp, rhsp, thsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_முனை_Dist_Triop;
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool cleanThs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersThs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL_TRIG; }
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs,
                       மாறிலி V3_எண்& ths) override {
        V3ERROR_NA;
    }
};
class Ast_முனை_Uniop VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

பாதுகாக்கப்பட்ட:
    Ast_முனை_Uniop(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : Ast_முனை_கோவை{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Uniop;
    // METHODS
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    // Set out to evaluation of a AstConst'ed lhs
    மெய்நிகர் வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) = 0;
    மெய்நிகர் bool cleanLhs() மாறிலி = 0;
    மெய்நிகர் bool sizeMattersLhs() மாறிலி = 0;  // True if output result depends on lhs size
    மெய்நிகர் bool doubleFlavor() மாறிலி { திரும்பு false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    மெய்நிகர் bool signedFlavor() மாறிலி { திரும்பு false; }
    மெய்நிகர் bool stringFlavor() மாறிலி { திரும்பு false; }  // N flavor of nodes with both flavors?
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    bool isPure() override;
    மாறிலி எழுத்து* broken() மாறிலி override;
};
class Ast_முனை_System_Uniop_D VL_NOT_FINAL : பொது Ast_முனை_Uniop {
பொது:
    Ast_முனை_System_Uniop_D(VNType t, File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : Ast_முனை_Uniop{t, fl, lhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_முனை_System_Uniop_D;
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL_TRIG; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class Ast_முனை_Var_Ref VL_NOT_FINAL : பொது Ast_முனை_கோவை {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_முனை_Var_Ref;  // Gen pointers before other members for performance
தனிப்பட்ட:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
பாதுகாக்கப்பட்ட:
    Ast_முனை_Var_Ref(VNType t, File_வரி* fl, AstVar* varp, மாறிலி VAccess& access)
        : Ast_முனை_கோவை{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

பொது:
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    VAccess access() மாறிலி { திரும்பு m_access; }
    வெறுமை access(மாறிலி VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() மாறிலி VL_MT_STABLE { திரும்பு m_varp; }  // [After Link] Pointer to variable
    வெறுமை varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_முனை*>(varp));
    }
    AstVarScope* varScopep() மாறிலி { திரும்பு m_varScopep; }
    வெறுமை varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    மாறிலி VSelfPointerText& selfPointer() மாறிலி { திரும்பு m_selfPointer; }
    வெறுமை selfPointer(மாறிலி VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) மாறிலி {
        திரும்பு selfPointer().protect(useSelfForThis, protect());
    }
    Ast_முனை_Module* classOrPackagep() மாறிலி { திரும்பு m_classOrPackagep; }
    வெறுமை classOrPackagep(Ast_முனை_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : பொது Ast_முனை {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
பொது:
    AstArg(File_வரி* fl, மாறிலி std::string& name, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி std::string& name) override { m_name = name; }
    bool emptyConnectNoNext() மாறிலி { திரும்பு !exprp() && name() == "" && !nextp(); }
};
class AstWith final : பொது Ast_முனை {
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
தனிப்பட்ட:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::set<std::string> m_restrictedNames;

பொது:
    AstWith(File_வரி* fl, AstLambdaArgRef* indexArgRefp, AstLambdaArgRef* மதிப்பு_Arg_Refp,
            Ast_முனை* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->indexArgRefp(indexArgRefp);
        this->மதிப்பு_Arg_Refp(மதிப்பு_Arg_Refp);
        addExprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!indexArgRefp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!மதிப்பு_Arg_Refp());  // varp needed to know lambda's arg dtype
        திரும்பு nullptr;
    }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    வெறுமை restricted(bool flag) { m_restricted = flag; }
    bool restricted() மாறிலி { திரும்பு m_restricted; }
    bool validated() மாறிலி { திரும்பு m_validated; }
    வெறுமை validated(bool flag) { m_validated = flag; }
    வெறுமை addRestrictedName(மாறிலி std::string& name) { m_restrictedNames.செருகு(name); }
    மாறிலி std::set<std::string>& restrictedNames() மாறிலி { திரும்பு m_restrictedNames; }
    // True if 'name' binds into the randomize() target class.
    bool nameResolvesToTarget(மாறிலி std::string& name) மாறிலி {
        திரும்பு !m_restricted || m_restrictedNames.மொத்தம்(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : பொது Ast_முனை_கோவை {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbortKind m_kind{VAbortKind::ACCEPT_ON};

பொது:
    AstAbortOn(File_வரி* fl, VAbortKind kind, Ast_முனை_கோவை* condp, Ast_முனை_கோவை* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_kind{kind} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    VAbortKind kind() மாறிலி { திரும்பு m_kind; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstAddrOfCFunc final : பொது Ast_முனை_கோவை {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

பொது:
    AstAddrOfCFunc(File_வரி* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_AstAddrOfCFunc;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    AstCFunc* funcp() மாறிலி { திரும்பு m_funcp; }
};
class AstAttrOf final : பொது Ast_முனை_கோவை {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
பொது:
    AstAttrOf(File_வரி* fl, VAttrType attrtype, Ast_முனை* fromp = nullptr,
              Ast_முனை_கோவை* dimp = nullptr)
        : ASTGEN_SUPER_AttrOf(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_AstAttrOf;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    VAttrType attrType() மாறிலி { திரும்பு m_attrType; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : பொது Ast_முனை_கோவை {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    மாறிலி bool m_pure;  // Pure optimizable

    வெறுமை init(மாறிலி string& text, முழு setwidth) {
        எனில் (!text.empty()) add(text);
        எனில் (setwidth) {
            dtypeSetBitSized(setwidth, VSigning::UNSIGNED);
        } இல்லையெனில் {
            dtypeSetVoid();  // Caller to override if necessary
        }
    }

பொது:
    class Pure {};
    explicit AstCExpr(File_வரி* fl, மாறிலி string& text = "", முழு setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{false} {
        init(text, setwidth);
    }
    AstCExpr(File_வரி* fl, Pure, மாறிலி string& text = "", முழு setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    bool cleanOut() மாறிலி override { திரும்பு true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு m_pure; }
    bool isOutputter() override { திரும்பு true; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு m_pure; }
    bool isPure() override { திரும்பு m_pure; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    // Add some text, or a node to this expression
    வெறுமை add(மாறிலி std::string& text) { addNodesp(new AstText{fileline(), text}); }
    வெறுமை add(Ast_முனை* nodep) { addNodesp(nodep); }
};
class AstCExprUser final : பொது Ast_முனை_கோவை {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    மாறிலி bool m_pure;  // Whether the function is pure
பொது:
    class Pure {};
    explicit AstCExprUser(File_வரி* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{false} {}
    AstCExprUser(File_வரி* fl, Pure)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    bool cleanOut() மாறிலி override { திரும்பு false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு m_pure; }
    bool isOutputter() override { திரும்பு true; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு m_pure; }
    bool isPure() override { திரும்பு m_pure; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    // Add some text, or a node to this expression
    வெறுமை add(மாறிலி std::string& text) { addNodesp(new AstText{fileline(), text}); }
    வெறுமை add(Ast_முனை* nodep) { addNodesp(nodep); }
};
class AstCFuncHard final : பொது Ast_முனை_கோவை {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
பொது:
    AstCFuncHard(File_வரி* fl, VCFunction function, Ast_முனை_கோவை* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு function().ascii(); }
    bool isPure() override {
        எனில் (!m_purity.isCached()) m_purity.set(getPurity());
        திரும்பு m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    VCFunction function() மாறிலி VL_MT_STABLE { திரும்பு m_function; }

தனிப்பட்ட:
    bool getPurity();
};
class AstCMethodHard final : பொது Ast_முனை_கோவை {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_method;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
பொது:
    AstCMethodHard(File_வரி* fl, Ast_முனை_கோவை* fromp, VCMethod method, Ast_முனை_கோவை* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_method{method} {
        this->fromp(fromp);
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு method().ascii(); }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstCMethodHard* மாறிலி asamep = VN_DBG_AS(samep, CMethodHard);
        திரும்பு (m_method == asamep->m_method);
    }
    bool isPure() override {
        எனில் (!m_purity.isCached()) m_purity.set(getPurity());
        திரும்பு m_purity.get();
    }
    முழு instr_மொத்தம்() மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு m_method.isClean(); }
    bool usePtr() மாறிலி { திரும்பு m_usePtr; }
    வெறுமை usePtr(bool flag) { m_usePtr = flag; }
    VCMethod method() மாறிலி VL_MT_STABLE { திரும்பு m_method; }
    வெறுமை method(VCMethod மதிப்பு) { m_method = மதிப்பு; }

தனிப்பட்ட:
    bool getPurity();
};
class AstCReset final : பொது Ast_முனை_கோவை {
    // Reset variable at startup
    மாறிலி bool m_constructing;  // Previously cleared by constructor
பொது:
    AstCReset(File_வரி* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isPure() override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!VN_IS(backp(), முனை_Assign));  // V3Emit* assumption
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() மாறிலி { திரும்பு m_constructing; }
};
class AstCast final : பொது Ast_முனை_கோவை {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
பொது:
    AstCast(File_வரி* fl, Ast_முனை_கோவை* fromp, VFlagChildDType, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { திரும்பு "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி VL_MT_STABLE { திரும்பு dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : பொது Ast_முனை_கோவை {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
பொது:
    AstCastParse(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { திரும்பு "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstCastSize final : பொது Ast_முனை_கோவை {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
பொது:
    AstCastSize(File_வரி* fl, Ast_முனை_கோவை* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_CastSize(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCastSize;
    string emitVerilog() override { திரும்பு "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : பொது Ast_முனை_கோவை {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
பொது:
    AstCellArrayRef(File_வரி* fl, மாறிலி string& name, Ast_முனை_கோவை* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        addSelp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : பொது Ast_முனை_கோவை {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
பொது:
    AstCellRef(File_வரி* fl, மாறிலி string& name, Ast_முனை* cellp, Ast_முனை* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : பொது Ast_முனை_கோவை {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
பொது:
    AstClassOrPackageRef(File_வரி* fl, மாறிலி string& name, Ast_முனை* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_முனை_Module* classOrPackageSkipp(மாறிலி bool doRefs = true) மாறிலி;
    Ast_முனை* classOrPackageNodep() மாறிலி { திரும்பு m_classOrPackageNodep; }
    வெறுமை classOrPackageNodep(Ast_முனை* nodep) { m_classOrPackageNodep = nodep; }
    வெறுமை classOrPackagep(Ast_முனை_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_முனை*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : பொது Ast_முனை_கோவை {
    // A constant
    V3_எண் m_இலக்கம்;  // Constant value
    வெறுமை init_With_எண்() {
        எனில் (m_இலக்கம்.isDouble()) {
            dtypeSetDouble();
        } இல்லையெனில் எனில் (m_இலக்கம்.isString()) {
            dtypeSetString();
        } இல்லையெனில் {
            dtypeSetBitOrLogicUnsized(m_இலக்கம்.width(), (m_இலக்கம்.sized() ? 0 : m_இலக்கம்.widthToFit()),
                                      VSigning::fromBool(m_இலக்கம்.isSigned()), m_இலக்கம்.isAnyXZ());
        }
        m_இலக்கம்.nodep(this);
    }

பொது:
    ~AstConst() override;
    AstConst(File_வரி* fl, மாறிலி V3_எண்& இலக்கம்)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{இலக்கம்} {
        init_With_எண்();
    }
    class Widthed_மதிப்பு {};  // for creator type-overload selection
    AstConst(File_வரி* fl, Widthed_மதிப்பு, முழு width, uint32_t மதிப்பு)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, width, மதிப்பு) {  // Need () constructor
        init_With_எண்();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_வரி* fl, DTyped, மாறிலி Ast_முனை_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{this, nodedtypep} {
        init_With_எண்();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(File_வரி* fl, StringToParse, மாறிலி எழுத்து* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{this, V3_எண்::Verilog_எண்_Literal{}, sourcep} {
        init_With_எண்();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_வரி* fl, VerilogStringLiteral, மாறிலி string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{this, V3_எண்::VerilogStringLiteral{}, str} {
        init_With_எண்();
    }
    AstConst(File_வரி* fl, uint32_t இலக்கம்)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, 32, இலக்கம்) {  // Need () constructor
        dtypeSetBitUnsized(m_இலக்கம்.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_வரி* fl, Unsized32, uint32_t இலக்கம்)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, 32, இலக்கம்) {  // Need () constructor
        m_இலக்கம்.width(32, false);
        dtypeSetBitUnsized(32, m_இலக்கம்.widthToFit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_வரி* fl, Signed32, int32_t இலக்கம்)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, 32, இலக்கம்) {  // Need () constructor
        m_இலக்கம்.width(32, true);
        dtypeSetBitUnsized(32, m_இலக்கம்.widthToFit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_வரி* fl, Unsized64, uint64_t இலக்கம்)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, 64, 0) {  // Need () constructor
        m_இலக்கம்.setQuad(இலக்கம்);
        dtypeSetBitSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_வரி* fl, SizedEData, uint64_t இலக்கம்)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_இலக்கம்.setQuad(இலக்கம்);
        dtypeSetBitSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_வரி* fl, RealDouble, இரட்டை இலக்கம்)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, 64) {  // Need () constructor
        m_இலக்கம்.setDouble(இலக்கம்);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_வரி* fl, String, மாறிலி string& இலக்கம்)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{this, V3_எண்::String{}, இலக்கம்} {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(File_வரி* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_வரி* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
        File_வரி* மாறிலி newfl = new File_வரி{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_வரி* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்(this, 1, on) {  // Need () constructor
        dtypeSetBit();
    }
    class All0 {};
    AstConst(File_வரி* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{this, V3_எண்::Verilog_எண்_Literal{}, "'0"} {
        init_With_எண்();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_வரி* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{this, V3_எண்::Verilog_எண்_Literal{}, "'1"} {
        init_With_எண்();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_வரி* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{this, V3_எண்::Null{}} {
        dtypep(findNullDType());
        init_With_எண்();
    }
    class OneStep {};
    AstConst(File_வரி* fl, OneStep)
        : ASTGEN_SUPER_Const(fl)
        , m_இலக்கம்{this, V3_எண்::OneStep{}} {
        dtypeSetBitSized(64, VSigning::UNSIGNED);
        init_With_எண்();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு இலக்கம்().ascii(); }  // * = Value
    மாறிலி V3_எண்& இலக்கம்() மாறிலி VL_MT_SAFE { திரும்பு m_இலக்கம்; }  // * = Value
    V3_எண்& இலக்கம்() { திரும்பு m_இலக்கம்; }  // * = Value
    string origParamName() மாறிலி;
    வெறுமை origParamName(மாறிலி string& name);
    uint32_t toUInt() மாறிலி { திரும்பு இலக்கம்().toUInt(); }
    int32_t toSInt() மாறிலி VL_MT_SAFE { திரும்பு இலக்கம்().toSInt(); }
    uint64_t toUQuad() மாறிலி { திரும்பு இலக்கம்().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstConst* மாறிலி sp = VN_DBG_AS(samep, Const);
        திரும்பு இலக்கம்().isCaseEq(sp->இலக்கம்());
    }
    வெறுமை cloneRelink() override;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(m_இலக்கம்.nodep() && m_இலக்கம்.nodep() != this);
        திரும்பு nullptr;
    }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool isEqAllOnes() மாறிலி { திரும்பு இலக்கம்().isEqAllOnes(width()); }
    bool isEqAllOnesV() மாறிலி { திரும்பு இலக்கம்().isEqAllOnes(widthMinV()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_மதிப்பு_Type(மாறிலி AstConst* samep) மாறிலி;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    நிலையான AstConst* parseParamLiteral(File_வரி* fl, மாறிலி string& literal);
};
class AstConstraintRef final : பொது Ast_முனை_கோவை {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

பொது:
    AstConstraintRef(File_வரி* fl, Ast_முனை_கோவை* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() மாறிலி override VL_MT_STABLE;
    AstConstraint* constrp() மாறிலி VL_MT_STABLE { திரும்பு m_constrp; }
    வெறுமை constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_முனை_Module* classOrPackagep() மாறிலி VL_MT_STABLE { திரும்பு m_classOrPackagep; }
    // TODO: this setter is unused
    வெறுமை classOrPackagep(Ast_முனை_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி final override { V3ERROR_NA_RETURN(true); }
};
class AstCvtArrayToArray final : பொது Ast_முனை_கோவை {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
தனிப்பட்ட:
    மாறிலி bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    மாறிலி முழு m_blockSize;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    மாறிலி முழு m_dstElementBits;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    மாறிலி முழு m_srcElementBits;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

பொது:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToArray(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_DType* dtp, bool reverse,
                       முழு blockSize, முழு dstElementBits, முழு srcElementBits)
        : ASTGEN_SUPER_CvtArrayToArray(fl)
        , m_reverse{reverse}
        , m_blockSize{blockSize}
        , m_dstElementBits{dstElementBits}
        , m_srcElementBits{srcElementBits} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToArray;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool reverse() மாறிலி { திரும்பு m_reverse; }
    முழு blockSize() மாறிலி { திரும்பு m_blockSize; }
    முழு dstElementBits() மாறிலி { திரும்பு m_dstElementBits; }
    முழு srcElementBits() மாறிலி { திரும்பு m_srcElementBits; }
};
class AstCvtArrayToPacked final : பொது Ast_முனை_கோவை {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
பொது:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToPacked(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_CvtArrayToPacked(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToPacked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
};
class AstCvtPackedToArray final : பொது Ast_முனை_கோவை {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
பொது:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtPackedToArray(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_CvtPackedToArray(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtPackedToArray;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
};
class AstCvtUnpackToQueue final : பொது Ast_முனை_கோவை {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
பொது:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtUnpackToQueue(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToQueue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToQueue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { திரும்பு "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
};
class AstCvtUnpackToUnpack final : பொது Ast_முனை_கோவை {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
பொது:
    AstCvtUnpackToUnpack(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToUnpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToUnpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
};
class AstDist final : பொது Ast_முனை_கோவை {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
பொது:
    AstDist(File_வரி* fl, Ast_முனை_கோவை* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { திரும்பு "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }  // NA
};
class AstDistItem final : பொது Ast_முனை_கோவை {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_isWhole = false;  // True for weight ':/', false for ':='
பொது:
    AstDistItem(File_வரி* fl, Ast_முனை_கோவை* rangep, Ast_முனை_கோவை* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { திரும்பு "%l "s + (m_isWhole ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }  // NA
    வெறுமை isWhole(bool flag) { m_isWhole = flag; }
    bool isWhole() மாறிலி { திரும்பு m_isWhole; }
};
class AstDot final : பொது Ast_முனை_கோவை {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    மாறிலி bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
பொது:
    AstDot(File_வரி* fl, bool colon, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    நிலையான Ast_முனை_கோவை* newIfPkg(File_வரி* fl, Ast_முனை_கோவை* packageOrClassp, Ast_முனை_கோவை* rhsp) {
        எனில் (!packageOrClassp) திரும்பு rhsp;
        திரும்பு new AstDot{fl, true, packageOrClassp, rhsp};
    }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool colon() மாறிலி { திரும்பு m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstEmptyQueue final : பொது Ast_முனை_கோவை {
பொது:
    explicit AstEmptyQueue(File_வரி* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_AstEmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { திரும்பு "{}"; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
};
class AstEnumItemRef final : பொது Ast_முனை_கோவை {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_containsGenBlock = false;  // Contains gen block reference
பொது:
    AstEnumItemRef(File_வரி* fl, AstEnumItem* itemp, Ast_முனை_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    மாறிலி எழுத்து* broken() மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு itemp() ? itemp()->name() : m_name; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstEnumItemRef* மாறிலி sp = VN_DBG_AS(samep, EnumItemRef);
        திரும்பு itemp() == sp->itemp() && dotted() == sp->dotted()
               && containsGenBlock() == sp->containsGenBlock();
    }
    AstEnumItem* itemp() மாறிலி VL_MT_STABLE { திரும்பு m_itemp; }
    வெறுமை itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() மாறிலி { திரும்பு m_dotted; }
    வெறுமை dotted(மாறிலி string& dotted) { m_dotted = dotted; }
    bool containsGenBlock() மாறிலி { திரும்பு m_containsGenBlock; }
    வெறுமை containsGenBlock(மாறிலி bool flag) { m_containsGenBlock = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    Ast_முனை_Module* classOrPackagep() மாறிலி { திரும்பு m_classOrPackagep; }
    வெறுமை classOrPackagep(Ast_முனை_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_கோவை_Stmt final : பொது Ast_முனை_கோவை {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
தனிப்பட்ட:
    bool m_hasResult = true;  // Returns result via resultp()

பொது:
    Ast_கோவை_Stmt(File_வரி* fl, Ast_முனை* stmtsp, Ast_முனை_கோவை* resultp)
        : ASTGEN_SUPER_கோவை_Stmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_கோவை_Stmt;
    // METHODS
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool isPure() override {
        எனில் (Ast_முனை::afterCommentp(stmtsp())) திரும்பு false;
        திரும்பு resultp()->isPure();
    }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    bool hasResult() மாறிலி { திரும்பு m_hasResult; }
    வெறுமை hasResult(bool flag) { m_hasResult = flag; }
};
class AstFError final : பொது Ast_முனை_கோவை {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
பொது:
    AstFError(File_வரி* fl, Ast_முனை_கோவை* filep, Ast_முனை_கோவை* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { திரும்பு "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 64; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstFOpen final : பொது Ast_முனை_கோவை {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
பொது:
    AstFOpen(File_வரி* fl, Ast_முனை_கோவை* filenamep, Ast_முனை_கோவை* modep)
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
    bool cleanOut() மாறிலி override { திரும்பு true; }
    string verilogKwd() மாறிலி override { திரும்பு "$fopen"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstFOpenMcd final : பொது Ast_முனை_கோவை {
    // @astgen op2 := filenamep : AstNodeExpr
பொது:
    AstFOpenMcd(File_வரி* fl, Ast_முனை_கோவை* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    string verilogKwd() மாறிலி override { திரும்பு "$fopen"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstFRead final : பொது Ast_முனை_கோவை {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
பொது:
    AstFRead(File_வரி* fl, Ast_முனை_கோவை* memp, Ast_முனை_கோவை* filep, Ast_முனை_கோவை* startp,
             Ast_முனை_கோவை* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() மாறிலி override { திரும்பு "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: makes output
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstFRewind final : பொது Ast_முனை_கோவை {
    // @astgen op1 := filep : Optional[AstNodeExpr]
பொது:
    AstFRewind(File_வரி* fl, Ast_முனை_கோவை* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() மாறிலி override { திரும்பு "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstFScanF final : பொது Ast_முனை_கோவை {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

பொது:
    AstFScanF(File_வரி* fl, மாறிலி string& text, Ast_முனை_கோவை* filep, Ast_முனை_கோவை* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_text; }
    string verilogKwd() மாறிலி override { திரும்பு "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: makes output
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() மாறிலி { திரும்பு m_text; }  // * = Text to display
    வெறுமை text(மாறிலி string& text) { m_text = text; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstFSeek final : பொது Ast_முனை_கோவை {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
பொது:
    AstFSeek(File_வரி* fl, Ast_முனை_கோவை* filep, Ast_முனை_கோவை* offset, Ast_முனை_கோவை* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() மாறிலி override { திரும்பு "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: makes output
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstFTell final : பொது Ast_முனை_கோவை {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
பொது:
    AstFTell(File_வரி* fl, Ast_முனை_கோவை* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() மாறிலி override { திரும்பு "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstFalling final : பொது Ast_முனை_கோவை {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
பொது:
    AstFalling(File_வரி* fl, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { திரும்பு "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstFell final : பொது Ast_முனை_கோவை {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
பொது:
    AstFell(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_Sen_மரம்* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { திரும்பு "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstFuture final : பொது Ast_முனை_கோவை {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
பொது:
    AstFuture(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_Sen_மரம்* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { திரும்பு "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstGatePin final : பொது Ast_முனை_கோவை {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
பொது:
    AstGatePin(File_வரி* fl, Ast_முனை_கோவை* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { திரும்பு "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
};
class AstGetInitialRandomSeed final : பொது Ast_முனை_கோவை {
    // Verilog $get_initial_random_seed()
பொது:
    explicit AstGetInitialRandomSeed(File_வரி* fl)
        : ASTGEN_SUPER_GetInitialRandomSeed(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstGetInitialRandomSeed;
    string emitVerilog() override { திரும்பு "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு true; }
    bool isPure() override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstImplication final : பொது Ast_முனை_கோவை {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

தனிப்பட்ட:
    மாறிலி bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    மாறிலி bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

பொது:
    AstImplication(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool isMultiCycleSva() மாறிலி override { திரும்பு m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() மாறிலி { திரும்பு m_isOverlapped; }
    bool isFollowedBy() மாறிலி { திரும்பு m_isFollowedBy; }
};
class AstInside final : பொது Ast_முனை_கோவை {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
பொது:
    AstInside(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { திரும்பு "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }  // NA
};
class AstInsideRange final : பொது Ast_முனை_கோவை {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
பொது:
    AstInsideRange(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { திரும்பு "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_முனை_கோவை* newAndFromInside(Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp);
};
class AstLambdaArgRef final : பொது Ast_முனை_கோவை {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_index;  // Index, not value

பொது:
    AstLambdaArgRef(File_வரி* fl, மாறிலி string& name, bool index)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_index{index} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    string emitVerilog() override { திரும்பு name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    bool index() மாறிலி { திரும்பு m_index; }
    bool is_கோவை_Coverage_Eligible() மாறிலி override { திரும்பு false; }
};
class AstMatchMasked final : பொது Ast_முனை_கோவை {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
பொது:
    உள்வரி AstMatchMasked(File_வரி* fl, Ast_முனை_கோவை* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_AstMatchMasked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { திரும்பு "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    நிலையான uint32_t fold(மாறிலி V3_எண்& lhs, AstVar* matchVarp);
};
class AstMatches final : பொது Ast_முனை_கோவை {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
பொது:
    AstMatches(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { திரும்பு "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstMemberSel final : பொது Ast_முனை_கோவை {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
பொது:
    AstMemberSel(File_வரி* fl, Ast_முனை_கோவை* fromp, VFlagChildDType, மாறிலி string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_வரி* fl, Ast_முனை_கோவை* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    VAccess access() மாறிலி { திரும்பு m_access; }
    வெறுமை access(மாறிலி VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    AstVar* varp() மாறிலி { திரும்பு m_varp; }
    வெறுமை varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : பொது Ast_முனை_கோவை {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
பொது:
    AstNewCopy(File_வரி* fl, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { திரும்பு "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
};
class AstNewDynamic final : பொது Ast_முனை_கோவை {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
பொது:
    AstNewDynamic(File_வரி* fl, Ast_முனை_கோவை* sizep, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { திரும்பு "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
};
class AstPExpr final : பொது Ast_முனை_கோவை {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
பொது:
    explicit AstPExpr(File_வரி* fl, AstBegin* bodyp, Ast_முனை_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_வரி* fl, AstBegin* bodyp, Ast_முனை_Stmt* finalp, Ast_முனை_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
};
class AstParseHolder final : பொது Ast_முனை_கோவை {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
பொது:
    explicit AstParseHolder(File_வரி* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : பொது Ast_முனை_கோவை {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

பொது:
    AstParseRef(File_வரி* fl, மாறிலி string& name, Ast_முனை_கோவை* lhsp = nullptr,
                Ast_முனை_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstParseRef* மாறிலி asamep = VN_DBG_AS(samep, ParseRef);
        திரும்பு m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : பொது Ast_முனை_கோவை {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

பொது:
    AstPast(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* ticksp = nullptr,
            Ast_Sen_மரம்* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() மாறிலி { திரும்பு m_propertyTiming; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstPatMember final : பொது Ast_முனை_கோவை {
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

பொது:
    AstPatMember(File_வரி* fl, Ast_முனை_கோவை* lhssp, Ast_முனை* keyp, Ast_முனை_கோவை* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { திரும்பு lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 2; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    bool isConcat() மாறிலி { திரும்பு m_isConcat; }
    வெறுமை isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() மாறிலி { திரும்பு m_isDefault; }
    வெறுமை isDefault(bool flag) { m_isDefault = flag; }
    Ast_முனை_கோவை* சாவி_Exprp() மாறிலி { திரும்பு varrefp() ? varrefp() : VN_CAST(keyp(), முனை_கோவை); }
};
class AstPattern final : பொது Ast_முனை_கோவை {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
பொது:
    AstPattern(File_வரி* fl, Ast_முனை* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstPattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி VL_MT_STABLE { திரும்பு dtypep() ? dtypep() : childDTypep(); }
};
class AstPatternStar final : பொது Ast_முனை_கோவை {
    // Pattern wildcard: ".*"
பொது:
    explicit AstPatternStar(File_வரி* fl)
        : ASTGEN_SUPER_PatternStar(fl) {}
    ASTGEN_MEMBERS_AstPatternStar;
    string emitVerilog() override { திரும்பு ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstPatternVar final : பொது Ast_முனை_கோவை {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
பொது:
    AstPatternVar(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_PatternVar(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPatternVar;
    string emitVerilog() override { திரும்பு ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_name == VN_DBG_AS(samep, PatternVar)->m_name;
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
};
class AstPropAlways final : பொது Ast_முனை_கோவை {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    மாறிலி bool m_isStrong = false;  // s_always
பொது:
    AstPropAlways(File_வரி* fl, Ast_முனை_கோவை* propp, Ast_முனை_கோவை* loBoundp, Ast_முனை_கோவை* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { V3ERROR_NA_RETURN(0); }
    bool isStrong() மாறிலி { திரும்பு m_isStrong; }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstRand final : பொது Ast_முனை_கோவை {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    மாறிலி bool m_urandom;  // $urandom vs $random
பொது:
    class Reset {};
    AstRand(File_வரி* fl, Ast_முனை_கோவை* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_வரி* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtypeSetUInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override {
        திரும்பு seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        எனில் (seedp()) {
            எனில் (urandom()) திரும்பு "VL_URANDOM_SEEDED_%nq%lq(%li)";
            திரும்பு "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        எனில் (isWide()) திரும்பு "VL_RANDOM_%nq(%nw, %P)";
        திரும்பு "VL_RANDOM_%nq()";
    }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு !seedp(); }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool combinable(மாறிலி AstRand* samep) மாறிலி {
        திரும்பு !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() மாறிலி { திரும்பு m_urandom; }
};
class AstRandRNG final : பொது Ast_முனை_கோவை {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
பொது:
    AstRandRNG(File_வரி* fl, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { திரும்பு "%f$rngrandom()"; }
    string emitC() override {
        திரும்பு isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstRising final : பொது Ast_முனை_கோவை {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
பொது:
    AstRising(File_வரி* fl, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { திரும்பு "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstRose final : பொது Ast_முனை_கோவை {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
பொது:
    AstRose(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_Sen_மரம்* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { திரும்பு "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstSClocked final : பொது Ast_முனை_கோவை {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
பொது:
    AstSClocked(File_வரி* fl, AstSenItem* sensesp, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : பொது Ast_முனை_கோவை {
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
    மாறிலி bool m_unbounded = false;  // True when the upper bound is $
பொது:
    // Exact [*N]
    AstSConsRep(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* countp, Ast_முனை_கோவை* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { V3ERROR_NA_RETURN(0); }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {  // LCOV_EXCL_LINE
        திரும்பு m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() மாறிலி { திரும்பு m_unbounded; }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstSEventually final : பொது Ast_முனை_கோவை {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
பொது:
    explicit AstSEventually(File_வரி* fl, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : பொது Ast_முனை_கோவை {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
பொது:
    explicit AstSExpr(File_வரி* fl, Ast_முனை_கோவை* preExprp, Ast_முனை_Stmt* delayp, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_வரி* fl, Ast_முனை_Stmt* delayp, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstSFormatArg final : பொது Ast_முனை_கோவை {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

பொது:
    AstSFormatArg(File_வரி* fl, VFormatAttr formatAttr, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() மாறிலி override { திரும்பு "$sformatarg"; }
    string emitVerilog() override { திரும்பு "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        திரும்பு nullptr;
    }
    VFormatAttr formatAttr() மாறிலி { திரும்பு m_formatAttr; }
    வெறுமை formatAttr(மாறிலி VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    நிலையான VFormatAttr formatAttrDefauled(மாறிலி AstSFormatArg* nodep, மாறிலி Ast_முனை_DType* dtypep);
};
class AstSFormatF final : பொது Ast_முனை_கோவை {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_கோவை_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    மாறிலி எழுத்து m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
பொது:
    class கோவை_Format {};
    AstSFormatF(File_வரி* fl, மாறிலி string& text, bool hidden, Ast_முனை_கோவை* exprsp,
                எழுத்து missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    AstSFormatF(File_வரி* fl, கோவை_Format, Ast_முனை_கோவை* exprsp, எழுத்து missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_கோவை_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_text; }
    வெறுமை name(மாறிலி string& name) override { m_text = name; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() மாறிலி override { திரும்பு "$sformatf"; }
    string text() மாறிலி { திரும்பு m_text; }  // * = Text to display
    வெறுமை text(மாறிலி string& text) { m_text = text; }
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(text() != "" && கோவை_Format_2());  // Expr format means no literal format
        திரும்பு nullptr;
    }
    bool formatScopeTracking() மாறிலி {  // Track scopeNamep();  Ok if false positive
        எனில் (கோவை_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            திரும்பு true;
        க்கு (மாறிலி Ast_முனை* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            எனில் (மாறிலி AstConst* மாறிலி fmtp = VN_CAST(exprp, Const)) {
                எனில் (fmtp->இலக்கம்().isFromString()) {
                    மாறிலி string str = fmtp->இலக்கம்().toString();
                    எனில் (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        திரும்பு true;
                }
            }
        }
        திரும்பு false;
    }
    bool hidden() மாறிலி { திரும்பு m_hidden; }
    வெறுமை hidden(bool flag) { m_hidden = flag; }
    bool கோவை_Format_2() மாறிலி { திரும்பு m_கோவை_Format; }
    வெறுமை கோவை_Format_2(bool flag) { m_கோவை_Format = flag; }
    bool optionalFormat() மாறிலி { திரும்பு m_optionalFormat; }
    வெறுமை optionalFormat(bool flag) { m_optionalFormat = flag; }
    எழுத்து missingArgChar() மாறிலி { திரும்பு m_missingArgChar; }
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstSGotoRep final : பொது Ast_முனை_கோவை {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
பொது:
    // Exact [->N]
    AstSGotoRep(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* countp, Ast_முனை_கோவை* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstSNonConsRep final : பொது Ast_முனை_கோவை {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
பொது:
    // Exact [=N]
    AstSNonConsRep(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* countp, Ast_முனை_கோவை* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstSScanF final : பொது Ast_முனை_கோவை {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

பொது:
    AstSScanF(File_வரி* fl, மாறிலி string& text, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->fromp(fromp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_text; }
    string verilogKwd() மாறிலி override { திரும்பு "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: makes output
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstSScanF* மாறிலி asamep = VN_DBG_AS(samep, SScanF);
        திரும்பு text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() மாறிலி { திரும்பு m_text; }  // * = Text to display
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstSampled final : பொது Ast_முனை_கோவை {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_internal : 1;  // Internally created, not from a source $sampled
பொது:
    AstSampled(File_வரி* fl, Ast_முனை* exprp, Ast_முனை_DType* dtypep, bool internal = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_internal{internal} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { திரும்பு "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_internal == VN_DBG_AS(samep, Sampled)->m_internal;
    }
    bool internal() மாறிலி { திரும்பு m_internal; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstScopeName final : பொது Ast_முனை_கோவை {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    மாறிலி bool m_forFormat;  // Is for a format %m
    நிலையான std::string scopeNameFormatter(மாறிலி std::string& text);
    நிலையான std::string scopePrettyNameFormatter(மாறிலி std::string& text);

பொது:
    class ForFormat {};
    AstScopeName(File_வரி* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstScopeName* மாறிலி sp = VN_DBG_AS(samep, ScopeName);
        திரும்பு m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    // ACCESSORS
    மாறிலி std::string& scopeAttr() மாறிலி { திரும்பு m_scopeAttr; }
    வெறுமை scopeAttr(மாறிலி std::string& val) { m_scopeAttr = val; }
    மாறிலி std::string& scopeEntr() மாறிலி { திரும்பு m_scopeEntr; }
    வெறுமை scopeEntr(மாறிலி std::string& val) { m_scopeEntr = val; }
    bool dpiExport() மாறிலி { திரும்பு m_dpiExport; }
    வெறுமை dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() மாறிலி { திரும்பு m_selfRelative; }
    வெறுமை selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() மாறிலி { திரும்பு m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() மாறிலி { திரும்பு scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() மாறிலி { திரும்பு scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() மாறிலி { திரும்பு scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() மாறிலி { திரும்பு scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : பொது Ast_முனை_கோவை {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
பொது:
    AstStable(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_Sen_மரம்* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { திரும்பு "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class Ast_அடுக்கு_Trace_F final : பொது Ast_முனை_கோவை {
    // $stacktrace used as function
பொது:
    explicit Ast_அடுக்கு_Trace_F(File_வரி* fl)
        : ASTGEN_SUPER_அடுக்கு_Trace_F(fl) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_Ast_அடுக்கு_Trace_F;
    string verilogKwd() மாறிலி override { திரும்பு "$stacktrace"; }
    string emitVerilog() override { திரும்பு verilogKwd(); }
    string emitC() override { திரும்பு "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstSteady final : பொது Ast_முனை_கோவை {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
பொது:
    AstSteady(File_வரி* fl, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { திரும்பு "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstStructSel final : பொது Ast_முனை_கோவை {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
பொது:
    AstStructSel(File_வரி* fl, Ast_முனை_கோவை* fromp, மாறிலி string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override {
        // Not a union
        திரும்பு VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstStructSel* மாறிலி sp = VN_DBG_AS(samep, StructSel);
        திரும்பு m_name == sp->m_name;
    }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
};
class AstSysIgnore final : பொது Ast_முனை_கோவை {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
பொது:
    AstSysIgnore(File_வரி* fl, Ast_முனை_கோவை* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() மாறிலி override { திரும்பு "$ignored"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }  // Though deleted before opt
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }  // Though deleted before opt
    bool isPure() override { திரும்பு false; }  // Though deleted before opt
    bool isOutputter() override { திரும்பு true; }  // Though deleted before opt
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : பொது Ast_முனை_கோவை {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
பொது:
    AstSystemF(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() மாறிலி override { திரும்பு "$system"; }
    string emitVerilog() override { திரும்பு verilogKwd(); }
    string emitC() override { திரும்பு "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class Ast_Tagged_கோவை final : பொது Ast_முனை_கோவை {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
பொது:
    Ast_Tagged_கோவை(File_வரி* fl, மாறிலி string& name, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_Tagged_கோவை(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_கோவை;
    string emitVerilog() override { திரும்பு "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_name == VN_DBG_AS(samep, Tagged_கோவை)->m_name;
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
};
class AstTaggedPattern final : பொது Ast_முனை_கோவை {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
பொது:
    AstTaggedPattern(File_வரி* fl, மாறிலி string& name, Ast_முனை* patternp)
        : ASTGEN_SUPER_TaggedPattern(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstTaggedPattern;
    string emitVerilog() override { திரும்பு "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_name == VN_DBG_AS(samep, TaggedPattern)->m_name;
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
};
class AstTestPlusArgs final : பொது Ast_முனை_கோவை {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
பொது:
    AstTestPlusArgs(File_வரி* fl, Ast_முனை_கோவை* searchp)
        : ASTGEN_SUPER_TestPlusArgs(fl) {
        this->searchp(searchp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstTestPlusArgs;
    string verilogKwd() மாறிலி override { திரும்பு "$test$plusargs"; }
    string emitVerilog() override { திரும்பு verilogKwd(); }
    string emitC() override { திரும்பு "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    // but isPure() true
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstThisRef final : பொது Ast_முனை_கோவை {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
பொது:
    AstThisRef(File_வரி* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_வரி* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { திரும்பு "this"; }
    string emitVerilog() override { திரும்பு "this"; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
};
class AstTimePrecision final : பொது Ast_முனை_கோவை {
    // Verilog $timeprecision
பொது:
    explicit AstTimePrecision(File_வரி* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { திரும்பு "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstTimeUnit final : பொது Ast_முனை_கோவை {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
பொது:
    explicit AstTimeUnit(File_வரி* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { திரும்பு "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : பொது Ast_முனை_கோவை {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    மாறிலி std::vector<uint32_t> m_indices;  // Indices of the triggers tested
பொது:
    AstTriggered(File_வரி* fl, AstVarScope* trigVscp, மாறிலி std::vector<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    மாறிலி std::vector<uint32_t>& indices() மாறிலி { திரும்பு m_indices; }
};
class AstUnbounded final : பொது Ast_முனை_கோவை {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
பொது:
    explicit AstUnbounded(File_வரி* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { திரும்பு "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
};
class AstUnlinkedRef final : பொது Ast_முனை_கோவை {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
பொது:
    AstUnlinkedRef(File_வரி* fl, Ast_முனை* refp, மாறிலி string& name, Ast_முனை* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : பொது Ast_முனை_கோவை {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    மாறிலி bool m_strong;  // 's_' preffix
    மாறிலி bool m_overlapping;  // '_with` suffix
பொது:
    AstUntil(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() மாறிலி override {
        திரும்பு (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
    bool isStrong() மாறிலி { திரும்பு m_strong; }
    bool isOverlapping() மாறிலி { திரும்பு m_overlapping; }
};
class Ast_மதிப்பு_Plus_Args final : பொது Ast_முனை_கோவை {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
பொது:
    Ast_மதிப்பு_Plus_Args(File_வரி* fl, Ast_முனை_கோவை* searchp, Ast_முனை_கோவை* outp)
        : ASTGEN_SUPER_மதிப்பு_Plus_Args(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_மதிப்பு_Plus_Args;
    string verilogKwd() மாறிலி override { திரும்பு "$value$plusargs"; }
    string emitVerilog() override { திரும்பு "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு !outp(); }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstWithParse final : பொது Ast_முனை_கோவை {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
தனிப்பட்ட:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

பொது:
    AstWithParse(File_வரி* fl, Ast_முனை_கோவை* funcrefp, Ast_முனை_கோவை* exprsp,
                 Ast_முனை* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        addExprsp(exprsp);
        addConstraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool restricted() மாறிலி { திரும்பு m_restricted; }
    வெறுமை restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : பொது Ast_முனை_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
பொது:
    AstBufIf1(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstCastDynamic final : பொது Ast_முனை_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
பொது:
    AstCastDynamic(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtypeSetBitUnsized(32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { திரும்பு "%f$cast(%r, %l)"; }
    string emitC() override { திரும்பு "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 20; }
    bool isPure() override { திரும்பு false; }
};
class AstCompareNN final : பொது Ast_முனை_Biop {
    // Verilog str.compare() and str.icompare()
    மாறிலி bool m_ignoreCase;  // True for str.icompare()
பொது:
    AstCompareNN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        திரும்பு m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        திரும்பு m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool ignoreCase() மாறிலி { திரும்பு m_ignoreCase; }
};
class AstConcat final : பொது Ast_முனை_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
பொது:
    AstConcat(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        எனில் (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetBitOrLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { திரும்பு "%f{%l, %k%r}"; }
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { திரும்பு "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(concat %l %r)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 2; }
};
class AstConcatN final : பொது Ast_முனை_Biop {
    // String concatenate
பொது:
    AstConcatN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { திரும்பு "%f{%l, %k%r}"; }
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { திரும்பு "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
    bool stringFlavor() மாறிலி override { திரும்பு true; }
};
class AstDiv final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstDiv(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f/ %r)"; }
    string emitC() override { திரும்பு "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvudiv %l %r)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_DIV; }
};
class AstDivD final : பொது Ast_முனை_Biop {
பொது:
    AstDivD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstDivD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "/"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL_DIV; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstDivS final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstDivS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f/ %r)"; }
    string emitC() override { திரும்பு "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvsdiv %l %r)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_DIV; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstEqWild final : பொது Ast_முனை_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
பொது:
    AstEqWild(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    நிலையான Ast_முனை_Biop* newTyped(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp);
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f==? %r)"; }
    string emitC() override {
        எனில் (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } இல்லையெனில் {
            திரும்பு "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        எனில் (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } இல்லையெனில் {
            திரும்பு "==";
        }
    }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstFGetS final : பொது Ast_முனை_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
பொது:
    AstFGetS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%f$fgets(%l,%r)"; }
    string emitC() override {
        திரும்பு strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 64; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : பொது Ast_முனை_Biop {
பொது:
    AstFUngetC(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        திரும்பு "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 64; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    Ast_முனை* filep() மாறிலி { திரும்பு lhsp(); }
    Ast_முனை* charp() மாறிலி { திரும்பு rhsp(); }
};
class AstGetcN final : பொது Ast_முனை_Biop {
    // Verilog string.getc()
பொது:
    AstGetcN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு "getc"; }
    string emitVerilog() override { திரும்பு "%k(%l.getc(%r))"; }
    string emitC() override { திரும்பு "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstGetcRefN final : பொது Ast_முனை_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
பொது:
    AstGetcRefN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstGt final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstGt(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f> %r)"; }
    string emitC() override { திரும்பு "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { திரும்பு ">"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstGtD final : பொது Ast_முனை_Biop {
பொது:
    AstGtD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு ">"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstGtN final : பொது Ast_முனை_Biop {
பொது:
    AstGtN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு ">"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
    bool stringFlavor() மாறிலி override { திரும்பு true; }
};
class AstGtS final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstGtS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f> %r)"; }
    string emitC() override { திரும்பு "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstGte final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstGte(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f>= %r)"; }
    string emitC() override { திரும்பு "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { திரும்பு ">="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstGteD final : பொது Ast_முனை_Biop {
பொது:
    AstGteD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு ">="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstGteN final : பொது Ast_முனை_Biop {
பொது:
    AstGteN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு ">="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
    bool stringFlavor() மாறிலி override { திரும்பு true; }
};
class AstGteS final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstGteS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f>= %r)"; }
    string emitC() override { திரும்பு "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstLogAnd final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstLogAnd(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f&& %r)"; }
    string emitC() override { திரும்பு "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvand %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "&&"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() + INSTR_மொத்தம்_BRANCH; }
};
class AstLogIf final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstLogIf(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f-> %r)"; }
    string emitC() override { திரும்பு "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { திரும்பு "->"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() + INSTR_மொத்தம்_BRANCH; }
};
class AstLogOr final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstLogOr(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f|| %r)"; }
    string emitC() override { திரும்பு "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvor %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "||"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() + INSTR_மொத்தம்_BRANCH; }
};
class AstLt final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstLt(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f< %r)"; }
    string emitC() override { திரும்பு "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { திரும்பு "<"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstLtD final : பொது Ast_முனை_Biop {
பொது:
    AstLtD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "<"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstLtN final : பொது Ast_முனை_Biop {
பொது:
    AstLtN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "<"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
    bool stringFlavor() மாறிலி override { திரும்பு true; }
};
class AstLtS final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstLtS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f< %r)"; }
    string emitC() override { திரும்பு "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstLte final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstLte(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f<= %r)"; }
    string emitC() override { திரும்பு "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { திரும்பு "<="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstLteD final : பொது Ast_முனை_Biop {
பொது:
    AstLteD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "<="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstLteN final : பொது Ast_முனை_Biop {
பொது:
    AstLteN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "<="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
    bool stringFlavor() மாறிலி override { திரும்பு true; }
};
class AstLteS final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstLteS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f<= %r)"; }
    string emitC() override { திரும்பு "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstModDiv final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstModDiv(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f%% %r)"; }
    string emitC() override { திரும்பு "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvurem %l %r)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_DIV; }
};
class AstModDivS final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstModDivS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f%% %r)"; }
    string emitC() override { திரும்பு "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvsmod %l %r)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_DIV; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstNeqWild final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstNeqWild(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f!=? %r)"; }
    string emitC() override {
        எனில் (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } இல்லையெனில் {
            திரும்பு "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        எனில் (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } இல்லையெனில் {
            திரும்பு "!=";
        }
    }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstPow final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstPow(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f** %r)"; }
    string emitC() override { திரும்பு "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_MUL * 10; }
};
class AstPowD final : பொது Ast_முனை_Biop {
பொது:
    AstPowD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstPowD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f** %r)"; }
    string emitC() override { திரும்பு "pow(%li,%ri)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL_DIV * 5; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstPowSS final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstPowSS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f** %r)"; }
    string emitC() override { திரும்பு "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_MUL * 10; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstPowSU final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstPowSU(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f** %r)"; }
    string emitC() override { திரும்பு "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_MUL * 10; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstPowUS final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstPowUS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f** %r)"; }
    string emitC() override { திரும்பு "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_MUL * 10; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstReplicate final : பொது Ast_முனை_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
பொது:
    AstReplicate(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        எனில் (lhsp) {
            எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(rhsp, Const)) {
                எனில் (constp->இலக்கம்().isFourState()
                    || (constp->dtypep()->isSigned() && constp->இலக்கம்().isNegative())) {
                    dtypeSetBitOrLogicSized(lhsp->width(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } இல்லையெனில் {
                    dtypeSetBitOrLogicSized(lhsp->width() * constp->toSInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_வரி* fl, Ast_முனை_கோவை* lhsp, uint32_t rep_மொத்தம்)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_மொத்தம்}} {}
    ASTGEN_MEMBERS_AstReplicate;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%f{%r{%k%l}}"; }
    string emitC() override { திரும்பு "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override {
        திரும்பு "((_ repeat " + cvtToStr(width() / lhsp()->width()) + ") %l)";
    }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 2; }
};
class AstReplicateN final : பொது Ast_முனை_Biop {
    // String replicate
பொது:
    AstReplicateN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%f{%r{%k%l}}"; }
    string emitC() override { திரும்பு "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 2; }
    bool stringFlavor() மாறிலி override { திரும்பு true; }
};
class AstSAnd final : பொது Ast_முனை_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    மாறிலி bool m_propertyControl;  // Parser-generated property if/case branch conjunction
பொது:
    AstSAnd(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() + INSTR_மொத்தம்_BRANCH; }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {  // LCOV_EXCL_LINE
        திரும்பு m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() மாறிலி { திரும்பு m_propertyControl; }
};
class AstSIntersect final : பொது Ast_முனை_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
பொது:
    AstSIntersect(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() + INSTR_மொத்தம்_BRANCH; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstSOr final : பொது Ast_முனை_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
பொது:
    AstSOr(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSOr;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() + INSTR_மொத்தம்_BRANCH; }
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstSThroughout final : பொது Ast_முனை_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
பொது:
    AstSThroughout(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstSWithin final : பொது Ast_முனை_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
பொது:
    AstSWithin(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() மாறிலி override { திரும்பு true; }
};
class AstSel final : பொது Ast_முனை_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    முழு m_declElWidth;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    முழு m_widthConst;

பொது:
    AstSel(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* lsbp, முழு bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicSized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_வரி* fl, Ast_முனை_கோவை* fromp, முழு lsb, முழு bitwidth, முழு bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicUnsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& from, மாறிலி V3_எண்& bit) override {
        out.opSel(from, bit.toUInt() + widthConst() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        திரும்பு widthConst() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() மாறிலி override { திரும்பு "((_ extract %t %r) %l)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* otherp) மாறிலி override {
        திரும்பு widthConst() == VN_DBG_AS(otherp, Sel)->widthConst();
    }
    முழு instr_மொத்தம்() மாறிலி override {
        திரும்பு widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    முழு widthConst() மாறிலி { திரும்பு m_widthConst; }
    வெறுமை widthConst(முழு மதிப்பு) { m_widthConst = மதிப்பு; }
    முழு lsbConst() மாறிலி { திரும்பு VN_AS(lsbp(), Const)->toSInt(); }
    முழு msbConst() மாறிலி { திரும்பு lsbConst() + widthConst() - 1; }
    VNumRange& declRange() VL_MT_STABLE { திரும்பு m_declRange; }
    மாறிலி VNumRange& declRange() மாறிலி VL_MT_STABLE { திரும்பு m_declRange; }
    வெறுமை declRange(மாறிலி VNumRange& flag) { m_declRange = flag; }
    முழு declElWidth() மாறிலி { திரும்பு m_declElWidth; }
    வெறுமை declElWidth(முழு flag) { m_declElWidth = flag; }
};
class AstShiftL final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstShiftL(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, முழு setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        எனில் (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_DType* மாறிலி dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftL;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f<< %r)"; }
    string emitC() override { திரும்பு "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        திரும்பு (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstShiftLOvr final : பொது Ast_முனை_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
பொது:
    AstShiftLOvr(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, முழு setwidth = 0)
        : ASTGEN_SUPER_ShiftLOvr(fl, lhsp, rhsp) {
        எனில் (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } இல்லையெனில் {
            மாறிலி Ast_முனை_DType* மாறிலி dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftLOvr;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f<< %r)"; }
    string emitC() override { திரும்பு "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstShiftR final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstShiftR(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, முழு setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        எனில் (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_DType* மாறிலி dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftR;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f>> %r)"; }
    string emitC() override { திரும்பு "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        திரும்பு (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstShiftROvr final : பொது Ast_முனை_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
பொது:
    AstShiftROvr(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, முழு setwidth = 0)
        : ASTGEN_SUPER_ShiftROvr(fl, lhsp, rhsp) {
        எனில் (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } இல்லையெனில் {
            மாறிலி Ast_முனை_DType* மாறிலி dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftROvr;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f>> %r)"; }
    string emitC() override { திரும்பு "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstShiftRS final : பொது Ast_முனை_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
பொது:
    AstShiftRS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, முழு setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        எனில் (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_DType* மாறிலி dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { திரும்பு "%k(%l %f>>> %r)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvashr %l %r)"; }
    string emitC() override { திரும்பு "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstShiftRSOvr final : பொது Ast_முனை_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
பொது:
    AstShiftRSOvr(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, முழு setwidth = 0)
        : ASTGEN_SUPER_ShiftRSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        எனில் (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } இல்லையெனில் {
            மாறிலி Ast_முனை_DType* மாறிலி dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRSOvr;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { திரும்பு "%k(%l %f>>> %r)"; }
    string emitC() override { திரும்பு "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstSub final : பொது Ast_முனை_Biop {
    // @astgen makeDfgVertex
பொது:
    AstSub(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f- %r)"; }
    string emitC() override { திரும்பு "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvsub %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "-"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
};
class AstSubD final : பொது Ast_முனை_Biop {
பொது:
    AstSubD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstSubD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "-"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstURandomRange final : பொது Ast_முனை_Biop {
    // $urandom_range
பொது:
    AstURandomRange(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%f$urandom_range(%l, %r)"; }
    string emitC() override { திரும்பு "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : பொது Ast_முனை_Bi_Com {
    // @astgen makeDfgVertex
பொது:
    AstEq(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    நிலையான Ast_முனை_Biop* newTyped(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp);
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f== %r)"; }
    string emitC() override { திரும்பு "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { திரும்பு "=="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstEqCase final : பொது Ast_முனை_Bi_Com {
    // @astgen makeDfgVertex
பொது:
    AstEqCase(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f=== %r)"; }
    string emitC() override { திரும்பு "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { திரும்பு "=="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstEqD final : பொது Ast_முனை_Bi_Com {
பொது:
    AstEqD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "=="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstEqN final : பொது Ast_முனை_Bi_Com {
பொது:
    AstEqN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "=="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
    bool stringFlavor() மாறிலி override { திரும்பு true; }
};
class AstEqT final : பொது Ast_முனை_Bi_Com {
    // Equal (==) for data types
பொது:
    AstEqT(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqT;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "=="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
};
class AstLogEq final : பொது Ast_முனை_Bi_Com {
    // @astgen makeDfgVertex
பொது:
    AstLogEq(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f<-> %r)"; }
    string emitC() override { திரும்பு "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "<->"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() + INSTR_மொத்தம்_BRANCH; }
};
class AstNeq final : பொது Ast_முனை_Bi_Com {
    // @astgen makeDfgVertex
பொது:
    AstNeq(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    நிலையான Ast_முனை_Biop* newTyped(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp);
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f!= %r)"; }
    string emitC() override { திரும்பு "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { திரும்பு "!="; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (not (= %l %r)))"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstNeqCase final : பொது Ast_முனை_Bi_Com {
    // @astgen makeDfgVertex
பொது:
    AstNeqCase(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f!== %r)"; }
    string emitC() override { திரும்பு "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { திரும்பு "!="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
};
class AstNeqD final : பொது Ast_முனை_Bi_Com {
பொது:
    AstNeqD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "!="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstNeqN final : பொது Ast_முனை_Bi_Com {
பொது:
    AstNeqN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "!="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
    bool stringFlavor() மாறிலி override { திரும்பு true; }
};
class AstNeqT final : பொது Ast_முனை_Bi_Com {
    // Not-equal (!=) for data types
பொது:
    AstNeqT(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "!="; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_STR; }
};

// === AstNodeBiComAsv ===
class AstAdd final : பொது Ast_முனை_Bi_Com_Asv {
    // @astgen makeDfgVertex
பொது:
    AstAdd(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAdd;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f+ %r)"; }
    string emitC() override { திரும்பு "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvadd %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "+"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
};
class AstAddD final : பொது Ast_முனை_Bi_Com_Asv {
பொது:
    AstAddD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstAddD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "+"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstAnd final : பொது Ast_முனை_Bi_Com_Asv {
    // @astgen makeDfgVertex
பொது:
    AstAnd(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f& %r)"; }
    string emitC() override { திரும்பு "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvand %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "&"; }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    மாறிலி எழுத்து* widthMismatch() மாறிலி override VL_MT_STABLE;
};
class AstMul final : பொது Ast_முனை_Bi_Com_Asv {
    // @astgen makeDfgVertex
பொது:
    AstMul(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f* %r)"; }
    string emitC() override { திரும்பு "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvmul %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "*"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_MUL; }
};
class AstMulD final : பொது Ast_முனை_Bi_Com_Asv {
பொது:
    AstMulD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstMulD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "*"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstMulS final : பொது Ast_முனை_Bi_Com_Asv {
    // @astgen makeDfgVertex
பொது:
    AstMulS(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f* %r)"; }
    string emitC() override { திரும்பு "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvmul %l %r)"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * INSTR_மொத்தம்_INT_MUL; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstOr final : பொது Ast_முனை_Bi_Com_Asv {
    // @astgen makeDfgVertex
பொது:
    AstOr(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f| %r)"; }
    string emitC() override { திரும்பு "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvor %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "|"; }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    மாறிலி எழுத்து* widthMismatch() மாறிலி override VL_MT_STABLE;
};
class AstXor final : பொது Ast_முனை_Bi_Com_Asv {
    // @astgen makeDfgVertex
பொது:
    AstXor(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f^ %r)"; }
    string emitC() override { திரும்பு "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvxor %l %r)"; }
    string emitSimpleOperator() override { திரும்பு "^"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }  // Lclean && Rclean
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    மாறிலி எழுத்து* widthMismatch() மாறிலி override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class AstDistChiSquare final : பொது Ast_முனை_Dist_Biop {
பொது:
    AstDistChiSquare(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_DistChiSquare(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistChiSquare;
    string emitVerilog() override { திரும்பு "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { திரும்பு "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class AstDistExponential final : பொது Ast_முனை_Dist_Biop {
பொது:
    AstDistExponential(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_DistExponential(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistExponential;
    string emitVerilog() override { திரும்பு "%f$dist_exponential(%l, %r)"; }
    string emitC() override { திரும்பு "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class AstDistPoisson final : பொது Ast_முனை_Dist_Biop {
பொது:
    AstDistPoisson(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_DistPoisson(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistPoisson;
    string emitVerilog() override { திரும்பு "%f$dist_poisson(%l, %r)"; }
    string emitC() override { திரும்பு "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : பொது Ast_முனை_Dist_Biop {
பொது:
    AstDistT(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { திரும்பு "%f$dist_t(%l, %r)"; }
    string emitC() override { திரும்பு "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : பொது Ast_முனை_Sel {
    // @astgen makeDfgVertex
    வெறுமை init(மாறிலி Ast_முனை* fromp) {
        எனில் (fromp && VN_IS(fromp->dtypep()->skipRefp(), முனை_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), முனை_Array_DType)->subDTypep());
        }
    }

பொது:
    AstArraySel(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_வரி* fl, Ast_முனை_கோவை* fromp, முழு bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() மாறிலி override { திரும்பு "(select %l %r)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isGateOptimizable() மாறிலி override { திரும்பு true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
};
class AstAssocSel final : பொது Ast_முனை_Sel {
    வெறுமை init(மாறிலி Ast_முனை* fromp) {
        எனில் (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

பொது:
    AstAssocSel(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%k(%l%f[%r])"; }
    string emitC() override { திரும்பு "%li%k[%ri]"; }
    string emitSMT() மாறிலி override { திரும்பு "(select %l %r)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isGateOptimizable() மாறிலி override {
        திரும்பு !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு !isLValue(); }  // AssocSel creates on miss
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
};
class AstWildcardSel final : பொது Ast_முனை_Sel {
    வெறுமை init(மாறிலி Ast_முனை* fromp) {
        எனில் (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

பொது:
    AstWildcardSel(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%k(%l%f[%r])"; }
    string emitC() override { திரும்பு "%li%k[%ri]"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool isGateOptimizable() மாறிலி override { திரும்பு true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
};
class AstWordSel final : பொது Ast_முனை_Sel {
    // Select a single word from a multi-word wide value
பொது:
    AstWordSel(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    வெறுமை எண்_Operate(V3_எண்&, மாறிலி V3_எண்&, மாறிலி V3_எண்&) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};

// === AstNodeStream ===
class AstStreamL final : பொது Ast_முனை_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
பொது:
    AstStreamL(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamL;
    string emitVerilog() override { திரும்பு "%f{ << %r %k{%l} }"; }
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { திரும்பு "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 2; }
};
class AstStreamR final : பொது Ast_முனை_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
பொது:
    AstStreamR(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamR;
    string emitVerilog() override { திரும்பு "%f{ >> %r %k{%l} }"; }
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { திரும்பு isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : பொது Ast_முனை_System_Biop_D {
பொது:
    AstAtan2D(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$atan2(%l,%r)"; }
    string emitC() override { திரும்பு "atan2(%li,%ri)"; }
};
class AstHypotD final : பொது Ast_முனை_System_Biop_D {
பொது:
    AstHypotD(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$hypot(%l,%r)"; }
    string emitC() override { திரும்பு "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : பொது Ast_முனை_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
பொது:
    AstCCall(File_வரி* fl, AstCFunc* funcp, Ast_முனை_கோவை* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    மாறிலி VSelfPointerText& selfPointer() மாறிலி { திரும்பு m_selfPointer; }
    வெறுமை selfPointer(மாறிலி VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) மாறிலி {
        திரும்பு selfPointer().protect(useSelfForThis, protect());
    }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
};
class AstCMethodCall final : பொது Ast_முனை_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
பொது:
    AstCMethodCall(File_வரி* fl, Ast_முனை_கோவை* fromp, AstCFunc* funcp, Ast_முனை_கோவை* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : பொது Ast_முனை_CCall {
    // C++ new() call
பொது:
    AstCNew(File_வரி* fl, AstCFunc* funcp, Ast_முனை_கோவை* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : பொது Ast_முனை_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

பொது:
    AstConsArray(File_வரி* fl, Ast_முனை_DType* newDTypep, Ast_முனை_கோவை* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    மாறிலி எழுத்து* broken() மாறிலி override;
    வெறுமை cloneRelink() override;
    உள்வரி AstConsArrayItem* itemsp() மாறிலி VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_முனை_கோவை* valuep(uint64_t index) மாறிலி;
    // Add the value of the element at the index, which must not be given yet
    வெறுமை valuep(uint64_t index, Ast_முனை_கோவை* newp);
    // Add the elements of the array 'arrayp', starting at the index
    வெறுமை splice_மதிப்புகள்(uint64_t index, Ast_முனை_கோவை* arrayp);
};
class AstConsAssoc final : பொது Ast_முனை_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
பொது:
    explicit AstConsAssoc(File_வரி* fl, Ast_முனை_கோவை* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        திரும்பு nullptr;
    }
};
class AstConsDynArray final : பொது Ast_முனை_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
பொது:
    explicit AstConsDynArray(File_வரி* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    மாறிலி எழுத்து* broken() மாறிலி override;
};
class AstConsQueue final : பொது Ast_முனை_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
பொது:
    explicit AstConsQueue(File_வரி* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        திரும்பு nullptr;
    }
};
class AstConsStruct final : பொது Ast_முனை_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
பொது:
    AstConsStruct(File_வரி* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    மாறிலி எழுத்து* broken() மாறிலி override;
    // The value of the member called 'name'
    Ast_முனை_கோவை* memberValuep(மாறிலி string& name) மாறிலி;
};
class AstConsWildcard final : பொது Ast_முனை_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
பொது:
    explicit AstConsWildcard(File_வரி* fl, Ast_முனை_கோவை* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        திரும்பு nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : பொது Ast_முனை_FTask_Ref {
    // A reference to a function
பொது:
    உள்வரி AstFuncRef(File_வரி* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_வரி* fl, மாறிலி string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class AstMethodCall final : பொது Ast_முனை_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
பொது:
    AstMethodCall(File_வரி* fl, Ast_முனை_கோவை* fromp, VFlagChildDType, மாறிலி string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMethodCall(File_வரி* fl, Ast_முனை_கோவை* fromp, மாறிலி string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_AstMethodCall;
};
class AstNew final : பொது Ast_முனை_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
பொது:
    AstNew(File_வரி* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool isImplicit() மாறிலி { திரும்பு m_isImplicit; }
    வெறுமை isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() மாறிலி { திரும்பு m_isScoped; }
    வெறுமை isScoped(bool flag) { m_isScoped = flag; }
    bool isPure() override { திரும்பு false; }
};
class AstTaskRef final : பொது Ast_முனை_FTask_Ref {
    // A reference to a task
பொது:
    உள்வரி AstTaskRef(File_வரி* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_வரி* fl, மாறிலி string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtypeSetVoid();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : பொது Ast_முனை_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
தனிப்பட்ட:
    VAccess m_access;  // Left hand side assignment
பொது:
    AstSelBit(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    VAccess access() மாறிலி { திரும்பு m_access; }
    வெறுமை access(மாறிலி VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : பொது Ast_முனை_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
பொது:
    AstSelExtract(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* msbp, Ast_முனை_கோவை* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : பொது Ast_முனை_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
பொது:
    AstSelMinus(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* bitp, Ast_முனை_கோவை* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : பொது Ast_முனை_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
பொது:
    AstSelPlus(File_வரி* fl, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* bitp, Ast_முனை_கோவை* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_மொத்தம்_Bits final : பொது Ast_முனை_Quadop {
    // Number of bits set in vector
பொது:
    Ast_மொத்தம்_Bits(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* ctrl1p)
        : ASTGEN_SUPER_மொத்தம்_Bits(fl, exprp, ctrl1p, ctrl1p->clone_மரம்_Pure(false),
                                 ctrl1p->clone_மரம்_Pure(false)) {
        dtypeSetInt();
    }
    Ast_மொத்தம்_Bits(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* ctrl1p, Ast_முனை_கோவை* ctrl2p)
        : ASTGEN_SUPER_மொத்தம்_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_மரம்_Pure(false)) {
        dtypeSetInt();
    }
    Ast_மொத்தம்_Bits(File_வரி* fl, Ast_முனை_கோவை* exprp, Ast_முனை_கோவை* ctrl1p, Ast_முனை_கோவை* ctrl2p,
                 Ast_முனை_கோவை* ctrl3p)
        : ASTGEN_SUPER_மொத்தம்_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_மொத்தம்_Bits;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& கோவை, மாறிலி V3_எண்& ctrl1,
                       மாறிலி V3_எண்& ctrl2, மாறிலி V3_எண்& ctrl3) override {
        out.op_மொத்தம்_Bits(கோவை, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { திரும்பு "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool cleanThs() மாறிலி override { திரும்பு true; }
    bool cleanFhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersThs() மாறிலி override { திரும்பு false; }
    bool sizeMattersFhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : பொது Ast_முனை_Termop {
பொது:
    explicit AstInferredDisable(File_வரி* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { திரும்பு "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstTime final : பொது Ast_முனை_Termop {
    VTimescale m_timeunit;  // Parent module time unit
பொது:
    AstTime(File_வரி* fl, மாறிலி VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { திரும்பு "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_TIME; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : பொது Ast_முனை_Termop {
    VTimescale m_timeunit;  // Parent module time unit
பொது:
    AstTimeD(File_வரி* fl, மாறிலி VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { திரும்பு "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_TIME; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : பொது Ast_முனை_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
பொது:
    AstCond(File_வரி* fl, Ast_முனை_கோவை* condp, Ast_முனை_கோவை* thenp, Ast_முனை_கோவை* elsep);
    ASTGEN_MEMBERS_AstCond;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs,
                       மாறிலி V3_எண்& ths) override {
        out.opAssign(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { திரும்பு "%k(%l %f? %r %k: %t)"; }
    string emitC() override { திரும்பு "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() மாறிலி override { திரும்பு "(ite (__Vbool %l) %r %t)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }  // clean if e1 & e2 clean
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு false; }
    bool cleanThs() மாறிலி override { திரும்பு false; }  // Propagates up
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersThs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_BRANCH; }
};
class AstPutcN final : பொது Ast_முனை_Triop {
    // Verilog string.putc()
பொது:
    AstPutcN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, Ast_முனை_கோவை* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstPutcN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs,
                       மாறிலி V3_எண்& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு "putc"; }
    string emitVerilog() override { திரும்பு "%k(%l.putc(%r,%t))"; }
    string emitC() override { திரும்பு "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool cleanThs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersThs() மாறிலி override { திரும்பு false; }
};
class AstSliceSel final : பொது Ast_முனை_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
பொது:
    AstSliceSel(File_வரி* fl, Ast_முனை_கோவை* fromp, மாறிலி VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_AstSliceSel;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& from, மாறிலி V3_எண்& lo,
                       மாறிலி V3_எண்& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool cleanThs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersThs() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { திரும்பு m_declRange; }
    மாறிலி VNumRange& declRange() மாறிலி VL_MT_STABLE { திரும்பு m_declRange; }
};
class AstSubstrN final : பொது Ast_முனை_Triop {
    // Verilog string.substr()
பொது:
    AstSubstrN(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, Ast_முனை_கோவை* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs,
                       மாறிலி V3_எண்& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு "substr"; }
    string emitVerilog() override { திரும்பு "%k(%l.substr(%r,%t))"; }
    string emitC() override { திரும்பு "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { திரும்பு ""; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool cleanRhs() மாறிலி override { திரும்பு true; }
    bool cleanThs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersRhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersThs() மாறிலி override { திரும்பு false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : பொது Ast_முனை_Dist_Triop {
பொது:
    AstDistErlang(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, Ast_முனை_கோவை* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { திரும்பு "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { திரும்பு "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class AstDistNormal final : பொது Ast_முனை_Dist_Triop {
பொது:
    AstDistNormal(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, Ast_முனை_கோவை* thsp)
        : ASTGEN_SUPER_DistNormal(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistNormal;
    string emitVerilog() override { திரும்பு "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { திரும்பு "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class AstDistUniform final : பொது Ast_முனை_Dist_Triop {
பொது:
    AstDistUniform(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp, Ast_முனை_கோவை* thsp)
        : ASTGEN_SUPER_DistUniform(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistUniform;
    string emitVerilog() override { திரும்பு "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { திரும்பு "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : பொது Ast_முனை_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
பொது:
    எண்ணிக்கை FmtType : முழு { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

தனிப்பட்ட:
    // dist-ast-dump-suppress  // Part of name()
    மாறிலி FmtType m_fmtType;  // Operation type
பொது:
    AstAtoN(File_வரி* fl, Ast_முனை_கோவை* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtypeSetDouble() : dtypeSetInteger();
    }
    ASTGEN_MEMBERS_AstAtoN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opAtoN(lhs, m_fmtType); }
    string name() மாறிலி override VL_MT_STABLE {
        தேர்வு (m_fmtType) {
        நிலை ATOI: திரும்பு "atoi";
        நிலை ATOHEX: திரும்பு "atohex";
        நிலை ATOOCT: திரும்பு "atooct";
        நிலை ATOBIN: திரும்பு "atobin";
        நிலை ATOREAL: திரும்பு "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { திரும்பு "%l." + name() + "()"; }
    string emitC() override {
        தேர்வு (m_fmtType) {
        நிலை ATOI: திரும்பு "VL_ATOI_N(%li, 10)";
        நிலை ATOHEX: திரும்பு "VL_ATOI_N(%li, 16)";
        நிலை ATOOCT: திரும்பு "VL_ATOI_N(%li, 8)";
        நிலை ATOBIN: திரும்பு "VL_ATOI_N(%li, 2)";
        நிலை ATOREAL: திரும்பு "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    FmtType fmtType() மாறிலி { திரும்பு m_fmtType; }
};
class AstBitsToRealD final : பொது Ast_முனை_Uniop {
பொது:
    AstBitsToRealD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstBitsToRealD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { திரும்பு "%f$bitstoreal(%l)"; }
    string emitC() override { திரும்பு "VL_CVT_D_Q(%li)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }  // Eliminated before matters
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }  // Eliminated before matters
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstCCast final : பொது Ast_முனை_Uniop {
    // Cast to C-based data type
    முழு m_size;

பொது:
    AstCCast(File_வரி* fl, Ast_முனை_கோவை* lhsp, முழு setwidth, முழு minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_size = setwidth;
        எனில் (setwidth) {
            எனில் (minwidth == -1) minwidth = setwidth;
            dtypeSetBitUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_size = width();
    }
    ASTGEN_MEMBERS_AstCCast;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { திரும்பு "%f$_CAST(%l)"; }
    string emitC() override { திரும்பு "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }  // Special cased in V3Cast
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு size() == VN_DBG_AS(samep, CCast)->size();
    }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    //
    முழு size() மாறிலி { திரும்பு m_size; }
};
class AstCLog2 final : பொது Ast_முனை_Uniop {
பொது:
    AstCLog2(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { திரும்பு "%f$clog2(%l)"; }
    string emitC() override { திரும்பு "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 16; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstCastWrap final : பொது Ast_முனை_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
பொது:
    AstCastWrap(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { திரும்பு "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
};
class Ast_மொத்தம்_Ones final : பொது Ast_முனை_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
பொது:
    Ast_மொத்தம்_Ones(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_மொத்தம்_Ones(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_மொத்தம்_Ones;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.op_மொத்தம்_Ones(lhs); }
    string emitVerilog() override { திரும்பு "%f$countones(%l)"; }
    string emitC() override { திரும்பு "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 16; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstCvtPackString final : பொது Ast_முனை_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
பொது:
    AstCvtPackString(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstExtend final : பொது Ast_முனை_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
பொது:
    AstExtend(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_வரி* fl, Ast_முனை_கோவை* lhsp, முழு width, முழு widthMin = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetBitOrLogicUnsized(width, widthMin ? widthMin : width, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { திரும்பு "%l"; }
    string emitC() override { திரும்பு "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() மாறிலி override {
        திரும்பு "((_ zero_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override {
        திரும்பு false;  // Because the EXTEND operator self-casts
    }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
};
class AstExtendS final : பொது Ast_முனை_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
பொது:
    AstExtendS(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_வரி* fl, Ast_முனை_கோவை* lhsp, முழு width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { திரும்பு "%l"; }
    string emitC() override { திரும்பு "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() மாறிலி override {
        திரும்பு "((_ sign_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override {
        திரும்பு false;  // Because the EXTEND operator self-casts
    }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool signedFlavor() மாறிலி override { திரும்பு true; }
};
class AstFEof final : பொது Ast_முனை_Uniop {
பொது:
    AstFEof(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%f$feof(%l)"; }
    string emitC() override { திரும்பு "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 16; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
    Ast_முனை* filep() மாறிலி { திரும்பு lhsp(); }
};
class AstFGetC final : பொது Ast_முனை_Uniop {
பொது:
    AstFGetC(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { திரும்பு "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 64; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    Ast_முனை* filep() மாறிலி { திரும்பு lhsp(); }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstISToRD final : பொது Ast_முனை_Uniop {
    // $itor where lhs is signed
பொது:
    AstISToRD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstISToRD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { திரும்பு "%f$itor($signed(%l))"; }
    string emitC() override { திரும்பு "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_சொற்கள்() override { திரும்பு true; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstIToRD final : பொது Ast_முனை_Uniop {
    // $itor where lhs is unsigned
பொது:
    AstIToRD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstIToRD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { திரும்பு "%f$itor(%l)"; }
    string emitC() override { திரும்பு "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstIsUnbounded final : பொது Ast_முனை_Uniop {
    // True if is unbounded ($)
பொது:
    AstIsUnbounded(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { திரும்பு "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstIsUnknown final : பொது Ast_முனை_Uniop {
    // True if any unknown bits
பொது:
    AstIsUnknown(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { திரும்பு "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstLenN final : பொது Ast_முனை_Uniop {
    // Length of a string
பொது:
    AstLenN(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstLenN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { திரும்பு "%f(%l)"; }
    string emitC() override { திரும்பு "VL_LEN_IN(%li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
};
class AstLogNot final : பொது Ast_முனை_Uniop {
    // @astgen makeDfgVertex
தனிப்பட்ட:
    மாறிலி bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
பொது:
    AstLogNot(File_வரி* fl, Ast_முனை_கோவை* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { திரும்பு "%f(! %l)"; }
    string emitC() override { திரும்பு "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() மாறிலி override { திரும்பு "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { திரும்பு "!"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool fromProperty() மாறிலி { திரும்பு m_fromProperty; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstMostSetBitP1 final : பொது Ast_முனை_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
பொது:
    AstMostSetBitP1(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_MostSetBitP1(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstMostSetBitP1;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opMostSetBitP1(lhs); }
    string emitVerilog() override { திரும்பு "%f$mostsetbitp1(%l)"; }
    string emitC() override { திரும்பு "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 16; }
};
class AstNToI final : பொது Ast_முனை_Uniop {
    // String to any-size integral
பொது:
    AstNToI(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opNToI(lhs); }
    string emitVerilog() override { திரும்பு "'(%l)"; }
    string emitC() override { திரும்பு "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
};
class AstNegate final : பொது Ast_முனை_Uniop {
    // @astgen makeDfgVertex
பொது:
    AstNegate(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { திரும்பு "%f(- %l)"; }
    string emitC() override { திரும்பு "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvneg %l)"; }
    string emitSimpleOperator() override { திரும்பு "-"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
};
class AstNegateD final : பொது Ast_முனை_Uniop {
பொது:
    AstNegateD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstNegateD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { திரும்பு "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { திரும்பு "-"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool doubleFlavor() மாறிலி override { திரும்பு true; }
};
class AstNot final : பொது Ast_முனை_Uniop {
    // @astgen makeDfgVertex
பொது:
    AstNot(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { திரும்பு "%f(~ %l)"; }
    string emitC() override { திரும்பு "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() மாறிலி override { திரும்பு "(bvnot %l)"; }
    string emitSimpleOperator() override { திரும்பு "~"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }
    மாறிலி எழுத்து* widthMismatch() மாறிலி override VL_MT_STABLE;
};
class AstNullCheck final : பொது Ast_முனை_Uniop {
    // Return LHS after checking that LHS is non-null
பொது:
    AstNullCheck(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { V3ERROR_NA; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 1; }  // Rarely executes
    string emitVerilog() override { திரும்பு "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override { திரும்பு fileline() == samep->fileline(); }
};
class AstOneHot final : பொது Ast_முனை_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
பொது:
    AstOneHot(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { திரும்பு "%f$onehot(%l)"; }
    string emitC() override { திரும்பு "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 4; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstOneHot0 final : பொது Ast_முனை_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
பொது:
    AstOneHot0(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { திரும்பு "%f$onehot0(%l)"; }
    string emitC() override { திரும்பு "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs() * 3; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstPostDec final : பொது Ast_முனை_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
பொது:
    AstPostDec(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { திரும்பு "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : பொது Ast_முனை_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
பொது:
    AstPostInc(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { திரும்பு "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : பொது Ast_முனை_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
பொது:
    AstPreDec(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { திரும்பு "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : பொது Ast_முனை_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
பொது:
    AstPreInc(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { திரும்பு "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() மாறிலி override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() மாறிலி override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : பொது Ast_முனை_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
பொது:
    AstRToIRoundS(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { திரும்பு "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        திரும்பு isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstRToIS final : பொது Ast_முனை_Uniop {
    // $rtoi(lhs)
பொது:
    AstRToIS(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { திரும்பு "%f$rtoi(%l)"; }
    string emitC() override { திரும்பு "VL_RTOI_I_D(%li)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }  // Eliminated before matters
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }  // Eliminated before matters
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstRealToBits final : பொது Ast_முனை_Uniop {
பொது:
    AstRealToBits(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstRealToBits;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { திரும்பு "%f$realtobits(%l)"; }
    string emitC() override { திரும்பு "VL_CVT_Q_D(%li)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }  // Eliminated before matters
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }  // Eliminated before matters
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_DBL; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstRedAnd final : பொது Ast_முனை_Uniop {
    // @astgen makeDfgVertex
பொது:
    AstRedAnd(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { திரும்பு "%f(& %l)"; }
    string emitC() override { திரும்பு "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
};
class AstRedOr final : பொது Ast_முனை_Uniop {
    // @astgen makeDfgVertex
பொது:
    AstRedOr(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { திரும்பு "%f(| %l)"; }
    string emitC() override { திரும்பு "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
};
class AstRedXor final : பொது Ast_முனை_Uniop {
    // @astgen makeDfgVertex
பொது:
    AstRedXor(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { திரும்பு "%f(^ %l)"; }
    string emitC() override { திரும்பு "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override {
        மாறிலி முழு w = lhsp()->width();
        திரும்பு (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 1 + V3_எண்::log2b(width()); }
};
class AstResizeLValue final : பொது Ast_முனை_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
பொது:
    AstResizeLValue(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
};
class AstSigned final : பொது Ast_முனை_Uniop {
    // $signed(lhs)
பொது:
    AstSigned(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { திரும்பு "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }  // Eliminated before matters
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }  // Eliminated before matters
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};
class AstTimeImport final : பொது Ast_முனை_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
பொது:
    AstTimeImport(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }
};
class AstToLowerN final : பொது Ast_முனை_Uniop {
    // string.tolower()
பொது:
    AstToLowerN(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToLowerN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { திரும்பு "%l.tolower()"; }
    string emitC() override { திரும்பு "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
};
class AstToStringN final : பொது Ast_முனை_Uniop {
பொது:
    AstToStringN(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_ToStringN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToStringN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { திரும்பு "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        திரும்பு isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
};
class AstToUpperN final : பொது Ast_முனை_Uniop {
    // string.toupper()
பொது:
    AstToUpperN(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToUpperN;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { திரும்பு "%l.toupper()"; }
    string emitC() override { திரும்பு "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    bool cleanLhs() மாறிலி override { திரும்பு true; }
    bool sizeMattersLhs() மாறிலி override { திரும்பு false; }
};
class AstUnsigned final : பொது Ast_முனை_Uniop {
    // $unsigned(lhs)
பொது:
    AstUnsigned(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { திரும்பு "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு false; }
    bool cleanLhs() மாறிலி override { திரும்பு false; }  // Eliminated before matters
    bool sizeMattersLhs() மாறிலி override { திரும்பு true; }  // Eliminated before matters
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 0; }
    bool isSystemFunc() மாறிலி override { திரும்பு true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstAcosD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$acos(%l)"; }
    string emitC() override { திரும்பு "acos(%li)"; }
};
class AstAcoshD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstAcoshD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$acosh(%l)"; }
    string emitC() override { திரும்பு "acosh(%li)"; }
};
class AstAsinD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstAsinD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$asin(%l)"; }
    string emitC() override { திரும்பு "asin(%li)"; }
};
class AstAsinhD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstAsinhD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$asinh(%l)"; }
    string emitC() override { திரும்பு "asinh(%li)"; }
};
class AstAtanD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstAtanD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$atan(%l)"; }
    string emitC() override { திரும்பு "atan(%li)"; }
};
class AstAtanhD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstAtanhD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$atanh(%l)"; }
    string emitC() override { திரும்பு "atanh(%li)"; }
};
class AstCeilD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstCeilD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::ceil(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$ceil(%l)"; }
    string emitC() override { திரும்பு "ceil(%li)"; }
};
class AstCosD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstCosD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$cos(%l)"; }
    string emitC() override { திரும்பு "cos(%li)"; }
};
class AstCoshD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstCoshD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$cosh(%l)"; }
    string emitC() override { திரும்பு "cosh(%li)"; }
};
class AstExpD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstExpD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$exp(%l)"; }
    string emitC() override { திரும்பு "exp(%li)"; }
};
class AstFloorD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstFloorD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::floor(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$floor(%l)"; }
    string emitC() override { திரும்பு "floor(%li)"; }
};
class AstLog10D final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstLog10D(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$log10(%l)"; }
    string emitC() override { திரும்பு "log10(%li)"; }
};
class AstLogD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstLogD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$ln(%l)"; }
    string emitC() override { திரும்பு "log(%li)"; }
};
class AstSinD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstSinD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$sin(%l)"; }
    string emitC() override { திரும்பு "sin(%li)"; }
};
class AstSinhD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstSinhD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$sinh(%l)"; }
    string emitC() override { திரும்பு "sinh(%li)"; }
};
class AstSqrtD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstSqrtD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::sqrt(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$sqrt(%l)"; }
    string emitC() override { திரும்பு "sqrt(%li)"; }
};
class AstTanD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstTanD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$tan(%l)"; }
    string emitC() override { திரும்பு "tan(%li)"; }
};
class AstTanhD final : பொது Ast_முனை_System_Uniop_D {
பொது:
    AstTanhD(File_வரி* fl, Ast_முனை_கோவை* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    வெறுமை எண்_Operate(V3_எண்& out, மாறிலி V3_எண்& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { திரும்பு "%f$tanh(%l)"; }
    string emitC() override { திரும்பு "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : பொது Ast_முனை_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
பொது:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    உள்வரி AstVarRef(File_வரி* fl, AstVar* varp, மாறிலி VAccess& access);
    உள்வரி AstVarRef(File_வரி* fl, Ast_முனை_Module* classOrPackagep, AstVar* varp,
                     மாறிலி VAccess& access);
    // This form only allowed post-link (see above)
    உள்வரி AstVarRef(File_வரி* fl, AstVarScope* varscp, மாறிலி VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() மாறிலி { திரும்பு m_name; }
    உள்வரி string name() மாறிலி override;  // * = Var name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    மாறிலி எழுத்து* broken() மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    உள்வரி bool same_முனை(மாறிலி AstVarRef* samep) மாறிலி;
    உள்வரி bool sameNoLvalue(மாறிலி AstVarRef* samep) மாறிலி;
    முழு instr_மொத்தம்() மாறிலி override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
};
class AstVarXRef final : பொது Ast_முனை_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
பொது:
    AstVarXRef(File_வரி* fl, மாறிலி string& name, மாறிலி string& dotted, மாறிலி VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    உள்வரி AstVarXRef(File_வரி* fl, AstVar* varp, மாறிலி string& dotted, மாறிலி VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    வெறுமை name(மாறிலி std::string& name) override { m_name = name; }  // * = Var name
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string dotted() மாறிலி { திரும்பு m_dotted; }
    வெறுமை dotted(மாறிலி string& dotted) { m_dotted = dotted; }
    string inlinedDots() மாறிலி { திரும்பு m_inlinedDots; }
    வெறுமை inlinedDots(மாறிலி string& flag) { m_inlinedDots = flag; }
    bool containsGenBlock() மாறிலி { திரும்பு m_containsGenBlock; }
    வெறுமை containsGenBlock(மாறிலி bool flag) { m_containsGenBlock = flag; }
    bool readOnlyModport() மாறிலி { திரும்பு m_readOnlyModport; }
    வெறுமை readOnlyModport(மாறிலி bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() மாறிலி override { திரும்பு true; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு widthInstrs(); }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        திரும்பு (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#endif  // Guard
