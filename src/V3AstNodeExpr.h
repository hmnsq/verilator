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
#अन्तर्भाव "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_गाँठो_अभिव्यक्ति VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // An expression tree node
रक्षित:
    Ast_गाँठो_अभिव्यक्ति(VNType t, File_पङ्क्ति* fl)
        : Ast_गाँठो{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_अभिव्यक्ति;
    // METHODS
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    // Every expression must have a data type after V3Width
    bool hasDType() अचर override final VL_MT_SAFE { फर्काऊ true; }
    आभास string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    आभास string emitC() = 0;
    आभास string emitSMT() अचर { फर्काऊ ""; };
    आभास string emitSimpleOperator() { फर्काऊ ""; }  // "" means not ok to use
    आभास bool emit_Check_Max_शब्दहरू() { फर्काऊ false; }  // Check VL_MULS_MAX_WORDS
    आभास bool cleanOut() अचर = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    आभास bool isMultiCycleSva() अचर { फर्काऊ false; }
    अचर Ast_गाँठो_अभिव्यक्ति* getVAccessTargetRecurse() अचर;
    Ast_गाँठो_अभिव्यक्ति* getVAccessTargetRecurse() {
        फर्काऊ const_cast<Ast_गाँठो_अभिव्यक्ति*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<अचर Ast_गाँठो_अभिव्यक्ति*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() अचर;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() अचर { फर्काऊ getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_रूख() अचर;
    // Return base var (or const) nodep dereferences
    Ast_गाँठो* baseFromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    अन्तःस्थ Ast_Stmt_अभिव्यक्ति* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    आभास अचर वर्ण* widthMismatch() अचर VL_MT_STABLE { फर्काऊ nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string patternString(uint32_t depth = 0) अचर;
    bool isFourstateOrNoDtype() अचर {
        यदि (!dtypep()) फर्काऊ true;
        फर्काऊ dtypep()->skipRefp()->isFourstate();
    }
    स्थिर bool isFourstateOrNoDtype(अचर Ast_गाँठो_अभिव्यक्ति* अचर expr1p) {
        फर्काऊ expr1p->isFourstateOrNoDtype();
    }
    स्थिर bool isFourstateOrNoDtype(अचर Ast_गाँठो_अभिव्यक्ति* अचर expr1p,
                                     अचर Ast_गाँठो_अभिव्यक्ति* अचर expr2p) {
        फर्काऊ expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    स्थिर bool isFourstateOrNoDtype(अचर Ast_गाँठो_अभिव्यक्ति* अचर expr1p,
                                     अचर Ast_गाँठो_अभिव्यक्ति* अचर expr2p,
                                     अचर Ast_गाँठो_अभिव्यक्ति* अचर expr3p) {
        फर्काऊ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    स्थिर bool isFourstateOrNoDtype(अचर Ast_गाँठो_अभिव्यक्ति* अचर expr1p,
                                     अचर Ast_गाँठो_अभिव्यक्ति* अचर expr2p,
                                     अचर Ast_गाँठो_अभिव्यक्ति* अचर expr3p,
                                     अचर Ast_गाँठो_अभिव्यक्ति* अचर expr4p) {
        फर्काऊ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_गाँठो_Biop VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठो_Biop(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : Ast_गाँठो_अभिव्यक्ति{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    आभास शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) = 0;
    आभास bool cleanLhs() अचर = 0;  // True if LHS must have extra upper bits zero
    आभास bool cleanRhs() अचर = 0;  // True if RHS must have extra upper bits zero
    आभास bool sizeMattersLhs() अचर = 0;  // True if output result depends on lhs size
    आभास bool sizeMattersRhs() अचर = 0;  // True if output result depends on rhs size
    आभास bool doubleFlavor() अचर { फर्काऊ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    आभास bool signedFlavor() अचर { फर्काऊ false; }
    आभास bool stringFlavor() अचर { फर्काऊ false; }  // N flavor of nodes with both flavors?
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ true; }
    bool isPure() override;
    अचर वर्ण* broken() अचर override;

गुप्त:
    bool getPurityRecurse() अचर { फर्काऊ lhsp()->isPure() && rhsp()->isPure(); }
};
class Ast_गाँठो_Bi_Com VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_Biop {
    // Binary expr with commutative properties
रक्षित:
    Ast_गाँठो_Bi_Com(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhs, Ast_गाँठो_अभिव्यक्ति* rhs)
        : Ast_गाँठो_Biop{t, fl, lhs, rhs} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Bi_Com;
};
class Ast_गाँठो_Bi_Com_Asv VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_Bi_Com {
    // Binary expr with commutative & associative properties
रक्षित:
    Ast_गाँठो_Bi_Com_Asv(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhs, Ast_गाँठो_अभिव्यक्ति* rhs)
        : Ast_गाँठो_Bi_Com{t, fl, lhs, rhs} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Bi_Com_Asv;
};
class Ast_गाँठो_Dist_Biop VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    Ast_गाँठो_Dist_Biop(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : Ast_गाँठो_Biop{t, fl, lhsp, rhsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_गाँठो_Dist_Biop;
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL_TRIG; }
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
};
class Ast_गाँठो_Sel VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
रक्षित:
    Ast_गाँठो_Sel(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* bitp)
        : Ast_गाँठो_Biop{t, fl, fromp, bitp} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Sel;
    पूर्णाङ्क bitConst() अचर;
};
class Ast_गाँठो_Stream VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
रक्षित:
    Ast_गाँठो_Stream(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : Ast_गाँठो_Biop{t, fl, lhsp, rhsp} {
        यदि (lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(lhsp->width(), lhsp->widthMin(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Stream;
};
class Ast_गाँठो_System_Biop_D VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    Ast_गाँठो_System_Biop_D(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : Ast_गाँठो_Biop{t, fl, lhsp, rhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_गाँठो_System_Biop_D;
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL_TRIG; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class Ast_गाँठो_CCall VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

रक्षित:
    Ast_गाँठो_CCall(VNType t, File_पङ्क्ति* fl, AstCFunc* funcp, Ast_गाँठो_अभिव्यक्ति* argsp = nullptr)
        : Ast_गाँठो_अभिव्यक्ति{t, fl}
        , m_funcp{funcp} {
        addArgsp(argsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_CCall;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_CALL; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर Ast_गाँठो_CCall* अचर asamep = VN_DBG_AS(samep, गाँठो_CCall);
        फर्काऊ (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override;
    bool isOutputter() override { फर्काऊ !isPure(); }
    AstCFunc* funcp() अचर { फर्काऊ m_funcp; }
    शून्य funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() अचर { फर्काऊ m_argTypes; }
    शून्य argTypes(अचर string& str) { m_argTypes = str; }
    bool isUnlikely() अचर override { फर्काऊ m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर final override { फर्काऊ true; }
    bool superReference() अचर { फर्काऊ m_superReference; }
    शून्य superReference(bool flag) { m_superReference = flag; }
};
class Ast_गाँठो_Cons VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Construct a value of an aggregate type
    मित्र class Ast_गाँठो_अभिव्यक्ति;  // For m_knownConstantTree
    mutable bool m_known_Constant_रूख = false;  // Known to be 'this->isConstantTree() == true'

रक्षित:
    Ast_गाँठो_Cons(VNType t, File_पङ्क्ति* fl)
        : Ast_गाँठो_अभिव्यक्ति{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Cons;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर final override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर final override { फर्काऊ widthInstrs(); }
};
class Ast_गाँठो_FTask_Ref VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_गाँठो_FTask_Ref;  // Gen pointers before other members for performance
गुप्त:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठो_FTask_Ref(VNType t, File_पङ्क्ति* fl, अचर string& name, AstArg* argsp)
        : Ast_गाँठो_अभिव्यक्ति{t, fl}
        , m_name{name} {
        addArgsp(argsp);
    }

सार्वजनिक:
    अचर वर्ण* broken() अचर override;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    bool isGateOptimizable() अचर override;
    string dotted() अचर { फर्काऊ m_dotted; }  // * = Scope name or ""
    string inlinedDots() अचर { फर्काऊ m_inlinedDots; }
    शून्य inlinedDots(अचर string& flag) { m_inlinedDots = flag; }
    Ast_गाँठो_FTask* taskp() अचर { फर्काऊ m_taskp; }  // [After Link] Pointer to variable
    शून्य taskp(Ast_गाँठो_FTask* taskp) { m_taskp = taskp; }
    शून्य name(अचर string& name) override { m_name = name; }
    शून्य dotted(अचर string& name) { m_dotted = name; }
    Ast_गाँठो_Module* classOrPackagep() अचर { फर्काऊ m_classOrPackagep; }
    शून्य classOrPackagep(Ast_गाँठो_Module* nodep) { m_classOrPackagep = nodep; }
    bool containsGenBlock() अचर { फर्काऊ m_containsGenBlock; }
    शून्य containsGenBlock(अचर bool flag) { m_containsGenBlock = flag; }
    bool pli() अचर { फर्काऊ m_pli; }
    शून्य pli(bool flag) { m_pli = flag; }
    bool superReference() अचर { फर्काऊ m_superReference; }
    शून्य superReference(bool flag) { m_superReference = flag; }
    bool isPure() override;
    bool isUnlikely() अचर override { फर्काऊ m_taskp && m_taskp->isUnlikely(); }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर Ast_गाँठो_FTask_Ref* अचर asamep = VN_DBG_AS(samep, गाँठो_FTask_Ref);
        फर्काऊ taskp() == asamep->taskp()  //
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
    bool cleanOut() अचर final override { V3ERROR_NA_RETURN(true); }

गुप्त:
    bool getPurityRecurse() अचर;
};
class Ast_गाँठो_Pre_Sel VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठो_Pre_Sel(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* rhsp, Ast_गाँठो_अभिव्यक्ति* thsp)
        : Ast_गाँठो_अभिव्यक्ति{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Pre_Sel;
    // METHODS
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर final override { V3ERROR_NA_RETURN(true); }
    bool isPure() override;
    अचर वर्ण* broken() अचर override;

गुप्त:
    bool getPurityRecurse() अचर {
        फर्काऊ fromp()->isPure() && rhsp()->isPure() && (!thsp() || thsp()->isPure());
    }
};
class Ast_गाँठो_Quadop VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठो_Quadop(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, Ast_गाँठो_अभिव्यक्ति* thsp,
                  Ast_गाँठो_अभिव्यक्ति* fhsp)
        : Ast_गाँठो_अभिव्यक्ति{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    आभास शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs,
                               अचर V3_सङ्ख्या& ths, अचर V3_सङ्ख्या& fhs)
        = 0;
    आभास bool cleanLhs() अचर = 0;  // True if LHS must have extra upper bits zero
    आभास bool cleanRhs() अचर = 0;  // True if RHS must have extra upper bits zero
    आभास bool cleanThs() अचर = 0;  // True if THS must have extra upper bits zero
    आभास bool cleanFhs() अचर = 0;  // True if THS must have extra upper bits zero
    आभास bool sizeMattersLhs() अचर = 0;  // True if output result depends on lhs size
    आभास bool sizeMattersRhs() अचर = 0;  // True if output result depends on rhs size
    आभास bool sizeMattersThs() अचर = 0;  // True if output result depends on ths size
    आभास bool sizeMattersFhs() अचर = 0;  // True if output result depends on ths size
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ true; }
    bool isPure() override;
    अचर वर्ण* broken() अचर override;

गुप्त:
    bool getPurityRecurse() अचर {
        फर्काऊ lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure() && fhsp()->isPure();
    }
};
class Ast_गाँठो_Termop VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Terminal operator -- an operator with no "inputs"
रक्षित:
    Ast_गाँठो_Termop(VNType t, File_पङ्क्ति* fl)
        : Ast_गाँठो_अभिव्यक्ति{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Termop;
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class Ast_गाँठो_Triop VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठो_Triop(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, Ast_गाँठो_अभिव्यक्ति* thsp)
        : Ast_गाँठो_अभिव्यक्ति{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Triop;
    // METHODS
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    // Set out to evaluation of a AstConst'ed
    आभास शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs,
                               अचर V3_सङ्ख्या& ths)
        = 0;
    आभास bool cleanLhs() अचर = 0;  // True if LHS must have extra upper bits zero
    आभास bool cleanRhs() अचर = 0;  // True if RHS must have extra upper bits zero
    आभास bool cleanThs() अचर = 0;  // True if THS must have extra upper bits zero
    आभास bool sizeMattersLhs() अचर = 0;  // True if output result depends on lhs size
    आभास bool sizeMattersRhs() अचर = 0;  // True if output result depends on rhs size
    आभास bool sizeMattersThs() अचर = 0;  // True if output result depends on ths size
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ true; }
    bool isPure() override;
    अचर वर्ण* broken() अचर override;

गुप्त:
    bool getPurityRecurse() अचर {
        फर्काऊ lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure();
    }
};
class Ast_गाँठो_Dist_Triop VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_Triop {
सार्वजनिक:
    Ast_गाँठो_Dist_Triop(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp,
                     Ast_गाँठो_अभिव्यक्ति* thsp)
        : Ast_गाँठो_Triop{t, fl, lhsp, rhsp, thsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_गाँठो_Dist_Triop;
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool cleanThs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool sizeMattersThs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL_TRIG; }
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs,
                       अचर V3_सङ्ख्या& ths) override {
        V3ERROR_NA;
    }
};
class Ast_गाँठो_Uniop VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठो_Uniop(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : Ast_गाँठो_अभिव्यक्ति{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Uniop;
    // METHODS
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    // Set out to evaluation of a AstConst'ed lhs
    आभास शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) = 0;
    आभास bool cleanLhs() अचर = 0;
    आभास bool sizeMattersLhs() अचर = 0;  // True if output result depends on lhs size
    आभास bool doubleFlavor() अचर { फर्काऊ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    आभास bool signedFlavor() अचर { फर्काऊ false; }
    आभास bool stringFlavor() अचर { फर्काऊ false; }  // N flavor of nodes with both flavors?
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ true; }
    bool isPure() override;
    अचर वर्ण* broken() अचर override;
};
class Ast_गाँठो_System_Uniop_D VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_Uniop {
सार्वजनिक:
    Ast_गाँठो_System_Uniop_D(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : Ast_गाँठो_Uniop{t, fl, lhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_गाँठो_System_Uniop_D;
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL_TRIG; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class Ast_गाँठो_Var_Ref VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_गाँठो_Var_Ref;  // Gen pointers before other members for performance
गुप्त:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
रक्षित:
    Ast_गाँठो_Var_Ref(VNType t, File_पङ्क्ति* fl, AstVar* varp, अचर VAccess& access)
        : Ast_गाँठो_अभिव्यक्ति{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

सार्वजनिक:
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    VAccess access() अचर { फर्काऊ m_access; }
    शून्य access(अचर VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() अचर VL_MT_STABLE { फर्काऊ m_varp; }  // [After Link] Pointer to variable
    शून्य varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_गाँठो*>(varp));
    }
    AstVarScope* varScopep() अचर { फर्काऊ m_varScopep; }
    शून्य varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    अचर VSelfPointerText& selfPointer() अचर { फर्काऊ m_selfPointer; }
    शून्य selfPointer(अचर VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) अचर {
        फर्काऊ selfPointer().protect(useSelfForThis, protect());
    }
    Ast_गाँठो_Module* classOrPackagep() अचर { फर्काऊ m_classOrPackagep; }
    शून्य classOrPackagep(Ast_गाँठो_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : सार्वजनिक Ast_गाँठो {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
सार्वजनिक:
    AstArg(File_पङ्क्ति* fl, अचर std::string& name, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर std::string& name) override { m_name = name; }
    bool emptyConnectNoNext() अचर { फर्काऊ !exprp() && name() == "" && !nextp(); }
};
class AstWith final : सार्वजनिक Ast_गाँठो {
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
गुप्त:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::set<std::string> m_restrictedNames;

सार्वजनिक:
    AstWith(File_पङ्क्ति* fl, AstLambdaArgRef* indexArgRefp, AstLambdaArgRef* मान_Arg_Refp,
            Ast_गाँठो* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->indexArgRefp(indexArgRefp);
        this->मान_Arg_Refp(मान_Arg_Refp);
        addExprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!indexArgRefp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!मान_Arg_Refp());  // varp needed to know lambda's arg dtype
        फर्काऊ nullptr;
    }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    शून्य restricted(bool flag) { m_restricted = flag; }
    bool restricted() अचर { फर्काऊ m_restricted; }
    bool validated() अचर { फर्काऊ m_validated; }
    शून्य validated(bool flag) { m_validated = flag; }
    शून्य addRestrictedName(अचर std::string& name) { m_restrictedNames.घुसाउ(name); }
    अचर std::set<std::string>& restrictedNames() अचर { फर्काऊ m_restrictedNames; }
    // True if 'name' binds into the randomize() target class.
    bool nameResolvesToTarget(अचर std::string& name) अचर {
        फर्काऊ !m_restricted || m_restrictedNames.गन्ती(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbortKind m_kind{VAbortKind::ACCEPT_ON};

सार्वजनिक:
    AstAbortOn(File_पङ्क्ति* fl, VAbortKind kind, Ast_गाँठो_अभिव्यक्ति* condp, Ast_गाँठो_अभिव्यक्ति* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_kind{kind} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    VAbortKind kind() अचर { फर्काऊ m_kind; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstAddrOfCFunc final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

सार्वजनिक:
    AstAddrOfCFunc(File_पङ्क्ति* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_AstAddrOfCFunc;
    शून्य dump(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    AstCFunc* funcp() अचर { फर्काऊ m_funcp; }
};
class AstAttrOf final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
सार्वजनिक:
    AstAttrOf(File_पङ्क्ति* fl, VAttrType attrtype, Ast_गाँठो* fromp = nullptr,
              Ast_गाँठो_अभिव्यक्ति* dimp = nullptr)
        : ASTGEN_SUPER_AttrOf(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_AstAttrOf;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    शून्य name(अचर string& name) override { m_name = name; }
    VAttrType attrType() अचर { फर्काऊ m_attrType; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    अचर bool m_pure;  // Pure optimizable

    शून्य init(अचर string& text, पूर्णाङ्क setwidth) {
        यदि (!text.empty()) add(text);
        यदि (setwidth) {
            dtypeSetBitSized(setwidth, VSigning::UNSIGNED);
        } अन्यथा {
            dtypeSetVoid();  // Caller to override if necessary
        }
    }

सार्वजनिक:
    class Pure {};
    explicit AstCExpr(File_पङ्क्ति* fl, अचर string& text = "", पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{false} {
        init(text, setwidth);
    }
    AstCExpr(File_पङ्क्ति* fl, Pure, अचर string& text = "", पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    bool cleanOut() अचर override { फर्काऊ true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ m_pure; }
    bool isOutputter() override { फर्काऊ true; }
    bool isPredictOptimizable() अचर override { फर्काऊ m_pure; }
    bool isPure() override { फर्काऊ m_pure; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    // Add some text, or a node to this expression
    शून्य add(अचर std::string& text) { addNodesp(new AstText{fileline(), text}); }
    शून्य add(Ast_गाँठो* nodep) { addNodesp(nodep); }
};
class AstCExprUser final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    अचर bool m_pure;  // Whether the function is pure
सार्वजनिक:
    class Pure {};
    explicit AstCExprUser(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{false} {}
    AstCExprUser(File_पङ्क्ति* fl, Pure)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    bool cleanOut() अचर override { फर्काऊ false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ m_pure; }
    bool isOutputter() override { फर्काऊ true; }
    bool isPredictOptimizable() अचर override { फर्काऊ m_pure; }
    bool isPure() override { फर्काऊ m_pure; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    // Add some text, or a node to this expression
    शून्य add(अचर std::string& text) { addNodesp(new AstText{fileline(), text}); }
    शून्य add(Ast_गाँठो* nodep) { addNodesp(nodep); }
};
class AstCFuncHard final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
सार्वजनिक:
    AstCFuncHard(File_पङ्क्ति* fl, VCFunction function, Ast_गाँठो_अभिव्यक्ति* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ function().ascii(); }
    bool isPure() override {
        यदि (!m_purity.isCached()) m_purity.set(getPurity());
        फर्काऊ m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    VCFunction function() अचर VL_MT_STABLE { फर्काऊ m_function; }

गुप्त:
    bool getPurity();
};
class AstCMethodHard final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_method;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
सार्वजनिक:
    AstCMethodHard(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, VCMethod method, Ast_गाँठो_अभिव्यक्ति* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_method{method} {
        this->fromp(fromp);
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ method().ascii(); }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstCMethodHard* अचर asamep = VN_DBG_AS(samep, CMethodHard);
        फर्काऊ (m_method == asamep->m_method);
    }
    bool isPure() override {
        यदि (!m_purity.isCached()) m_purity.set(getPurity());
        फर्काऊ m_purity.get();
    }
    पूर्णाङ्क instr_गन्ती() अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ m_method.isClean(); }
    bool usePtr() अचर { फर्काऊ m_usePtr; }
    शून्य usePtr(bool flag) { m_usePtr = flag; }
    VCMethod method() अचर VL_MT_STABLE { फर्काऊ m_method; }
    शून्य method(VCMethod मान) { m_method = मान; }

गुप्त:
    bool getPurity();
};
class AstCReset final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Reset variable at startup
    अचर bool m_constructing;  // Previously cleared by constructor
सार्वजनिक:
    AstCReset(File_पङ्क्ति* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool isPure() override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!VN_IS(backp(), गाँठो_Assign));  // V3Emit* assumption
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() अचर { फर्काऊ m_constructing; }
};
class AstCast final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
सार्वजनिक:
    AstCast(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, VFlagChildDType, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { फर्काऊ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर VL_MT_STABLE { फर्काऊ dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
सार्वजनिक:
    AstCastParse(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { फर्काऊ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstCastSize final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
सार्वजनिक:
    AstCastSize(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_CastSize(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCastSize;
    string emitVerilog() override { फर्काऊ "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
सार्वजनिक:
    AstCellArrayRef(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_अभिव्यक्ति* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        addSelp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
सार्वजनिक:
    AstCellRef(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* cellp, Ast_गाँठो* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
सार्वजनिक:
    AstClassOrPackageRef(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_गाँठो_Module* classOrPackageSkipp(अचर bool doRefs = true) अचर;
    Ast_गाँठो* classOrPackageNodep() अचर { फर्काऊ m_classOrPackageNodep; }
    शून्य classOrPackageNodep(Ast_गाँठो* nodep) { m_classOrPackageNodep = nodep; }
    शून्य classOrPackagep(Ast_गाँठो_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_गाँठो*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A constant
    V3_सङ्ख्या m_अङ्क;  // Constant value
    शून्य init_With_सङ्ख्या() {
        यदि (m_अङ्क.isDouble()) {
            dtypeSetDouble();
        } अन्यथा यदि (m_अङ्क.isString()) {
            dtypeSetString();
        } अन्यथा {
            dtypeSetBitOrLogicUnsized(m_अङ्क.width(), (m_अङ्क.sized() ? 0 : m_अङ्क.widthToFit()),
                                      VSigning::fromBool(m_अङ्क.isSigned()), m_अङ्क.isAnyXZ());
        }
        m_अङ्क.nodep(this);
    }

सार्वजनिक:
    ~AstConst() override;
    AstConst(File_पङ्क्ति* fl, अचर V3_सङ्ख्या& अङ्क)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{अङ्क} {
        init_With_सङ्ख्या();
    }
    class Widthed_मान {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, Widthed_मान, पूर्णाङ्क width, uint32_t मान)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, width, मान) {  // Need () constructor
        init_With_सङ्ख्या();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_पङ्क्ति* fl, DTyped, अचर Ast_गाँठो_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{this, nodedtypep} {
        init_With_सङ्ख्या();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, StringToParse, अचर वर्ण* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{this, V3_सङ्ख्या::Verilog_सङ्ख्या_Literal{}, sourcep} {
        init_With_सङ्ख्या();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, VerilogStringLiteral, अचर string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{this, V3_सङ्ख्या::VerilogStringLiteral{}, str} {
        init_With_सङ्ख्या();
    }
    AstConst(File_पङ्क्ति* fl, uint32_t अङ्क)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, 32, अङ्क) {  // Need () constructor
        dtypeSetBitUnsized(m_अङ्क.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, Unsized32, uint32_t अङ्क)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, 32, अङ्क) {  // Need () constructor
        m_अङ्क.width(32, false);
        dtypeSetBitUnsized(32, m_अङ्क.widthToFit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, Signed32, int32_t अङ्क)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, 32, अङ्क) {  // Need () constructor
        m_अङ्क.width(32, true);
        dtypeSetBitUnsized(32, m_अङ्क.widthToFit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, Unsized64, uint64_t अङ्क)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, 64, 0) {  // Need () constructor
        m_अङ्क.setQuad(अङ्क);
        dtypeSetBitSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, SizedEData, uint64_t अङ्क)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_अङ्क.setQuad(अङ्क);
        dtypeSetBitSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, RealDouble, द्विगुण अङ्क)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, 64) {  // Need () constructor
        m_अङ्क.setDouble(अङ्क);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_पङ्क्ति* fl, String, अचर string& अङ्क)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{this, V3_सङ्ख्या::String{}, अङ्क} {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(File_पङ्क्ति* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_पङ्क्ति* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
        File_पङ्क्ति* अचर newfl = new File_पङ्क्ति{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_पङ्क्ति* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क(this, 1, on) {  // Need () constructor
        dtypeSetBit();
    }
    class All0 {};
    AstConst(File_पङ्क्ति* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{this, V3_सङ्ख्या::Verilog_सङ्ख्या_Literal{}, "'0"} {
        init_With_सङ्ख्या();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_पङ्क्ति* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{this, V3_सङ्ख्या::Verilog_सङ्ख्या_Literal{}, "'1"} {
        init_With_सङ्ख्या();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_पङ्क्ति* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{this, V3_सङ्ख्या::Null{}} {
        dtypep(findNullDType());
        init_With_सङ्ख्या();
    }
    class OneStep {};
    AstConst(File_पङ्क्ति* fl, OneStep)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्क{this, V3_सङ्ख्या::OneStep{}} {
        dtypeSetBitSized(64, VSigning::UNSIGNED);
        init_With_सङ्ख्या();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() अचर override VL_MT_STABLE { फर्काऊ अङ्क().ascii(); }  // * = Value
    अचर V3_सङ्ख्या& अङ्क() अचर VL_MT_SAFE { फर्काऊ m_अङ्क; }  // * = Value
    V3_सङ्ख्या& अङ्क() { फर्काऊ m_अङ्क; }  // * = Value
    string origParamName() अचर;
    शून्य origParamName(अचर string& name);
    uint32_t toUInt() अचर { फर्काऊ अङ्क().toUInt(); }
    int32_t toSInt() अचर VL_MT_SAFE { फर्काऊ अङ्क().toSInt(); }
    uint64_t toUQuad() अचर { फर्काऊ अङ्क().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstConst* अचर sp = VN_DBG_AS(samep, Const);
        फर्काऊ अङ्क().isCaseEq(sp->अङ्क());
    }
    शून्य cloneRelink() override;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(m_अङ्क.nodep() && m_अङ्क.nodep() != this);
        फर्काऊ nullptr;
    }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool isEqAllOnes() अचर { फर्काऊ अङ्क().isEqAllOnes(width()); }
    bool isEqAllOnesV() अचर { फर्काऊ अङ्क().isEqAllOnes(widthMinV()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_मान_Type(अचर AstConst* samep) अचर;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    स्थिर AstConst* parseParamLiteral(File_पङ्क्ति* fl, अचर string& literal);
};
class AstConstraintRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

सार्वजनिक:
    AstConstraintRef(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() अचर override VL_MT_STABLE;
    AstConstraint* constrp() अचर VL_MT_STABLE { फर्काऊ m_constrp; }
    शून्य constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_गाँठो_Module* classOrPackagep() अचर VL_MT_STABLE { फर्काऊ m_classOrPackagep; }
    // TODO: this setter is unused
    शून्य classOrPackagep(Ast_गाँठो_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर final override { V3ERROR_NA_RETURN(true); }
};
class AstCvtArrayToArray final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
गुप्त:
    अचर bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    अचर पूर्णाङ्क m_blockSize;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    अचर पूर्णाङ्क m_dstElementBits;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    अचर पूर्णाङ्क m_srcElementBits;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

सार्वजनिक:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToArray(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_DType* dtp, bool reverse,
                       पूर्णाङ्क blockSize, पूर्णाङ्क dstElementBits, पूर्णाङ्क srcElementBits)
        : ASTGEN_SUPER_CvtArrayToArray(fl)
        , m_reverse{reverse}
        , m_blockSize{blockSize}
        , m_dstElementBits{dstElementBits}
        , m_srcElementBits{srcElementBits} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToArray;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool reverse() अचर { फर्काऊ m_reverse; }
    पूर्णाङ्क blockSize() अचर { फर्काऊ m_blockSize; }
    पूर्णाङ्क dstElementBits() अचर { फर्काऊ m_dstElementBits; }
    पूर्णाङ्क srcElementBits() अचर { फर्काऊ m_srcElementBits; }
};
class AstCvtArrayToPacked final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToPacked(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_CvtArrayToPacked(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToPacked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
};
class AstCvtPackedToArray final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtPackedToArray(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_CvtPackedToArray(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtPackedToArray;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
};
class AstCvtUnpackToQueue final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtUnpackToQueue(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToQueue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToQueue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { फर्काऊ "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
};
class AstCvtUnpackToUnpack final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    AstCvtUnpackToUnpack(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToUnpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToUnpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
};
class AstDist final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
सार्वजनिक:
    AstDist(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { फर्काऊ "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }  // NA
};
class AstDistItem final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_isWhole = false;  // True for weight ':/', false for ':='
सार्वजनिक:
    AstDistItem(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* rangep, Ast_गाँठो_अभिव्यक्ति* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { फर्काऊ "%l "s + (m_isWhole ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }  // NA
    शून्य isWhole(bool flag) { m_isWhole = flag; }
    bool isWhole() अचर { फर्काऊ m_isWhole; }
};
class AstDot final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    अचर bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
सार्वजनिक:
    AstDot(File_पङ्क्ति* fl, bool colon, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    स्थिर Ast_गाँठो_अभिव्यक्ति* newIfPkg(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* packageOrClassp, Ast_गाँठो_अभिव्यक्ति* rhsp) {
        यदि (!packageOrClassp) फर्काऊ rhsp;
        फर्काऊ new AstDot{fl, true, packageOrClassp, rhsp};
    }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool colon() अचर { फर्काऊ m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstEmptyQueue final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
सार्वजनिक:
    explicit AstEmptyQueue(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_AstEmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { फर्काऊ "{}"; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ true; }
};
class AstEnumItemRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_containsGenBlock = false;  // Contains gen block reference
सार्वजनिक:
    AstEnumItemRef(File_पङ्क्ति* fl, AstEnumItem* itemp, Ast_गाँठो_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    अचर वर्ण* broken() अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ itemp() ? itemp()->name() : m_name; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstEnumItemRef* अचर sp = VN_DBG_AS(samep, EnumItemRef);
        फर्काऊ itemp() == sp->itemp() && dotted() == sp->dotted()
               && containsGenBlock() == sp->containsGenBlock();
    }
    AstEnumItem* itemp() अचर VL_MT_STABLE { फर्काऊ m_itemp; }
    शून्य itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() अचर { फर्काऊ m_dotted; }
    शून्य dotted(अचर string& dotted) { m_dotted = dotted; }
    bool containsGenBlock() अचर { फर्काऊ m_containsGenBlock; }
    शून्य containsGenBlock(अचर bool flag) { m_containsGenBlock = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    Ast_गाँठो_Module* classOrPackagep() अचर { फर्काऊ m_classOrPackagep; }
    शून्य classOrPackagep(Ast_गाँठो_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_अभिव्यक्ति_Stmt final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
गुप्त:
    bool m_hasResult = true;  // Returns result via resultp()

सार्वजनिक:
    Ast_अभिव्यक्ति_Stmt(File_पङ्क्ति* fl, Ast_गाँठो* stmtsp, Ast_गाँठो_अभिव्यक्ति* resultp)
        : ASTGEN_SUPER_अभिव्यक्ति_Stmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_अभिव्यक्ति_Stmt;
    // METHODS
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool isPure() override {
        यदि (Ast_गाँठो::afterCommentp(stmtsp())) फर्काऊ false;
        फर्काऊ resultp()->isPure();
    }
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ true; }
    bool hasResult() अचर { फर्काऊ m_hasResult; }
    शून्य hasResult(bool flag) { m_hasResult = flag; }
};
class AstFError final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
सार्वजनिक:
    AstFError(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* filep, Ast_गाँठो_अभिव्यक्ति* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { फर्काऊ "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 64; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstFOpen final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
सार्वजनिक:
    AstFOpen(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* filenamep, Ast_गाँठो_अभिव्यक्ति* modep)
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
    bool cleanOut() अचर override { फर्काऊ true; }
    string verilogKwd() अचर override { फर्काऊ "$fopen"; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }
    bool isOutputter() override { फर्काऊ true; }
    bool isUnlikely() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstFOpenMcd final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op2 := filenamep : AstNodeExpr
सार्वजनिक:
    AstFOpenMcd(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    string verilogKwd() अचर override { फर्काऊ "$fopen"; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }
    bool isOutputter() override { फर्काऊ true; }
    bool isUnlikely() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstFRead final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
सार्वजनिक:
    AstFRead(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* memp, Ast_गाँठो_अभिव्यक्ति* filep, Ast_गाँठो_अभिव्यक्ति* startp,
             Ast_गाँठो_अभिव्यक्ति* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() अचर override { फर्काऊ "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { फर्काऊ true; }  // SPECIAL: makes output
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstFRewind final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := filep : Optional[AstNodeExpr]
सार्वजनिक:
    AstFRewind(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() अचर override { फर्काऊ "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }
    bool isOutputter() override { फर्काऊ true; }
    bool isUnlikely() अचर override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstFScanF final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

सार्वजनिक:
    AstFScanF(File_पङ्क्ति* fl, अचर string& text, Ast_गाँठो_अभिव्यक्ति* filep, Ast_गाँठो_अभिव्यक्ति* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_text; }
    string verilogKwd() अचर override { फर्काऊ "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { फर्काऊ true; }  // SPECIAL: makes output
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() अचर { फर्काऊ m_text; }  // * = Text to display
    शून्य text(अचर string& text) { m_text = text; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstFSeek final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
सार्वजनिक:
    AstFSeek(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* filep, Ast_गाँठो_अभिव्यक्ति* offset, Ast_गाँठो_अभिव्यक्ति* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() अचर override { फर्काऊ "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { फर्काऊ true; }  // SPECIAL: makes output
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstFTell final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
सार्वजनिक:
    AstFTell(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() अचर override { फर्काऊ "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }
    bool isOutputter() override { फर्काऊ true; }
    bool isUnlikely() अचर override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstFalling final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    AstFalling(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { फर्काऊ "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstFell final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
सार्वजनिक:
    AstFell(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_Sen_रूख* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { फर्काऊ "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstFuture final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
सार्वजनिक:
    AstFuture(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_Sen_रूख* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { फर्काऊ "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstGatePin final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
सार्वजनिक:
    AstGatePin(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { फर्काऊ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
};
class AstGetInitialRandomSeed final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $get_initial_random_seed()
सार्वजनिक:
    explicit AstGetInitialRandomSeed(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_GetInitialRandomSeed(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstGetInitialRandomSeed;
    string emitVerilog() override { फर्काऊ "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ true; }
    bool isPure() override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_PLI; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstImplication final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

गुप्त:
    अचर bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    अचर bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

सार्वजनिक:
    AstImplication(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool isMultiCycleSva() अचर override { फर्काऊ m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() अचर { फर्काऊ m_isOverlapped; }
    bool isFollowedBy() अचर { फर्काऊ m_isFollowedBy; }
};
class AstInside final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
सार्वजनिक:
    AstInside(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { फर्काऊ "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }  // NA
};
class AstInsideRange final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
सार्वजनिक:
    AstInsideRange(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { फर्काऊ "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_गाँठो_अभिव्यक्ति* newAndFromInside(Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp);
};
class AstLambdaArgRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_index;  // Index, not value

सार्वजनिक:
    AstLambdaArgRef(File_पङ्क्ति* fl, अचर string& name, bool index)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_index{index} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    string emitVerilog() override { फर्काऊ name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    शून्य name(अचर string& name) override { m_name = name; }
    bool index() अचर { फर्काऊ m_index; }
    bool is_अभिव्यक्ति_Coverage_Eligible() अचर override { फर्काऊ false; }
};
class AstMatchMasked final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
सार्वजनिक:
    अन्तःस्थ AstMatchMasked(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_AstMatchMasked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { फर्काऊ "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    स्थिर uint32_t fold(अचर V3_सङ्ख्या& lhs, AstVar* matchVarp);
};
class AstMatches final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
सार्वजनिक:
    AstMatches(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { फर्काऊ "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstMemberSel final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
सार्वजनिक:
    AstMemberSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, VFlagChildDType, अचर string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& name) override { m_name = name; }
    VAccess access() अचर { फर्काऊ m_access; }
    शून्य access(अचर VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    AstVar* varp() अचर { फर्काऊ m_varp; }
    शून्य varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
सार्वजनिक:
    AstNewCopy(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { फर्काऊ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
};
class AstNewDynamic final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
सार्वजनिक:
    AstNewDynamic(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* sizep, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { फर्काऊ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
};
class AstPExpr final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
सार्वजनिक:
    explicit AstPExpr(File_पङ्क्ति* fl, AstBegin* bodyp, Ast_गाँठो_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_पङ्क्ति* fl, AstBegin* bodyp, Ast_गाँठो_Stmt* finalp, Ast_गाँठो_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
};
class AstParseHolder final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
सार्वजनिक:
    explicit AstParseHolder(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

सार्वजनिक:
    AstParseRef(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_अभिव्यक्ति* lhsp = nullptr,
                Ast_गाँठो_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    शून्य name(अचर string& name) override { m_name = name; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstParseRef* अचर asamep = VN_DBG_AS(samep, ParseRef);
        फर्काऊ m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

सार्वजनिक:
    AstPast(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* ticksp = nullptr,
            Ast_Sen_रूख* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() अचर { फर्काऊ m_propertyTiming; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstPatMember final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
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

सार्वजनिक:
    AstPatMember(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhssp, Ast_गाँठो* keyp, Ast_गाँठो_अभिव्यक्ति* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { फर्काऊ lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 2; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    bool isConcat() अचर { फर्काऊ m_isConcat; }
    शून्य isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() अचर { फर्काऊ m_isDefault; }
    शून्य isDefault(bool flag) { m_isDefault = flag; }
    Ast_गाँठो_अभिव्यक्ति* साँचो_Exprp() अचर { फर्काऊ varrefp() ? varrefp() : VN_CAST(keyp(), गाँठो_अभिव्यक्ति); }
};
class AstPattern final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
सार्वजनिक:
    AstPattern(File_पङ्क्ति* fl, Ast_गाँठो* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstPattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर VL_MT_STABLE { फर्काऊ dtypep() ? dtypep() : childDTypep(); }
};
class AstPatternStar final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Pattern wildcard: ".*"
सार्वजनिक:
    explicit AstPatternStar(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_PatternStar(fl) {}
    ASTGEN_MEMBERS_AstPatternStar;
    string emitVerilog() override { फर्काऊ ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstPatternVar final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
सार्वजनिक:
    AstPatternVar(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_PatternVar(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPatternVar;
    string emitVerilog() override { फर्काऊ ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ m_name == VN_DBG_AS(samep, PatternVar)->m_name;
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
};
class AstPropAlways final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    अचर bool m_isStrong = false;  // s_always
सार्वजनिक:
    AstPropAlways(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* propp, Ast_गाँठो_अभिव्यक्ति* loBoundp, Ast_गाँठो_अभिव्यक्ति* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { V3ERROR_NA_RETURN(0); }
    bool isStrong() अचर { फर्काऊ m_isStrong; }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstRand final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    अचर bool m_urandom;  // $urandom vs $random
सार्वजनिक:
    class Reset {};
    AstRand(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_पङ्क्ति* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtypeSetUInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override {
        फर्काऊ seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        यदि (seedp()) {
            यदि (urandom()) फर्काऊ "VL_URANDOM_SEEDED_%nq%lq(%li)";
            फर्काऊ "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        यदि (isWide()) फर्काऊ "VL_RANDOM_%nq(%nw, %P)";
        फर्काऊ "VL_RANDOM_%nq()";
    }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ !seedp(); }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_PLI; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool combinable(अचर AstRand* samep) अचर {
        फर्काऊ !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() अचर { फर्काऊ m_urandom; }
};
class AstRandRNG final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
सार्वजनिक:
    AstRandRNG(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { फर्काऊ "%f$rngrandom()"; }
    string emitC() override {
        फर्काऊ isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_PLI; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstRising final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    AstRising(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { फर्काऊ "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstRose final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
सार्वजनिक:
    AstRose(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_Sen_रूख* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { फर्काऊ "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstSClocked final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
सार्वजनिक:
    AstSClocked(File_पङ्क्ति* fl, AstSenItem* sensesp, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
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
    अचर bool m_unbounded = false;  // True when the upper bound is $
सार्वजनिक:
    // Exact [*N]
    AstSConsRep(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* countp, Ast_गाँठो_अभिव्यक्ति* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { V3ERROR_NA_RETURN(0); }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {  // LCOV_EXCL_LINE
        फर्काऊ m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() अचर { फर्काऊ m_unbounded; }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstSEventually final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    explicit AstSEventually(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
सार्वजनिक:
    explicit AstSExpr(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* preExprp, Ast_गाँठो_Stmt* delayp, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_पङ्क्ति* fl, Ast_गाँठो_Stmt* delayp, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstSFormatArg final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

सार्वजनिक:
    AstSFormatArg(File_पङ्क्ति* fl, VFormatAttr formatAttr, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() अचर override { फर्काऊ "$sformatarg"; }
    string emitVerilog() override { फर्काऊ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        फर्काऊ nullptr;
    }
    VFormatAttr formatAttr() अचर { फर्काऊ m_formatAttr; }
    शून्य formatAttr(अचर VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    स्थिर VFormatAttr formatAttrDefauled(अचर AstSFormatArg* nodep, अचर Ast_गाँठो_DType* dtypep);
};
class AstSFormatF final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_अभिव्यक्ति_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    अचर वर्ण m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    class अभिव्यक्ति_Format {};
    AstSFormatF(File_पङ्क्ति* fl, अचर string& text, bool hidden, Ast_गाँठो_अभिव्यक्ति* exprsp,
                वर्ण missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    AstSFormatF(File_पङ्क्ति* fl, अभिव्यक्ति_Format, Ast_गाँठो_अभिव्यक्ति* exprsp, वर्ण missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_अभिव्यक्ति_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_text; }
    शून्य name(अचर string& name) override { m_text = name; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_PLI; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() अचर override { फर्काऊ "$sformatf"; }
    string text() अचर { फर्काऊ m_text; }  // * = Text to display
    शून्य text(अचर string& text) { m_text = text; }
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(text() != "" && अभिव्यक्ति_Format_2());  // Expr format means no literal format
        फर्काऊ nullptr;
    }
    bool formatScopeTracking() अचर {  // Track scopeNamep();  Ok if false positive
        यदि (अभिव्यक्ति_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            फर्काऊ true;
        लागि (अचर Ast_गाँठो* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            यदि (अचर AstConst* अचर fmtp = VN_CAST(exprp, Const)) {
                यदि (fmtp->अङ्क().isFromString()) {
                    अचर string str = fmtp->अङ्क().toString();
                    यदि (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        फर्काऊ true;
                }
            }
        }
        फर्काऊ false;
    }
    bool hidden() अचर { फर्काऊ m_hidden; }
    शून्य hidden(bool flag) { m_hidden = flag; }
    bool अभिव्यक्ति_Format_2() अचर { फर्काऊ m_अभिव्यक्ति_Format; }
    शून्य अभिव्यक्ति_Format_2(bool flag) { m_अभिव्यक्ति_Format = flag; }
    bool optionalFormat() अचर { फर्काऊ m_optionalFormat; }
    शून्य optionalFormat(bool flag) { m_optionalFormat = flag; }
    वर्ण missingArgChar() अचर { फर्काऊ m_missingArgChar; }
    VTimescale timeunit() अचर { फर्काऊ m_timeunit; }
    शून्य timeunit(अचर VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstSGotoRep final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
सार्वजनिक:
    // Exact [->N]
    AstSGotoRep(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* countp, Ast_गाँठो_अभिव्यक्ति* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstSNonConsRep final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
सार्वजनिक:
    // Exact [=N]
    AstSNonConsRep(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* countp, Ast_गाँठो_अभिव्यक्ति* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstSScanF final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

सार्वजनिक:
    AstSScanF(File_पङ्क्ति* fl, अचर string& text, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->fromp(fromp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_text; }
    string verilogKwd() अचर override { फर्काऊ "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { फर्काऊ true; }  // SPECIAL: makes output
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstSScanF* अचर asamep = VN_DBG_AS(samep, SScanF);
        फर्काऊ text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() अचर { फर्काऊ m_text; }  // * = Text to display
    VTimescale timeunit() अचर { फर्काऊ m_timeunit; }
    शून्य timeunit(अचर VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstSampled final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_internal : 1;  // Internally created, not from a source $sampled
सार्वजनिक:
    AstSampled(File_पङ्क्ति* fl, Ast_गाँठो* exprp, Ast_गाँठो_DType* dtypep, bool internal = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_internal{internal} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { फर्काऊ "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ m_internal == VN_DBG_AS(samep, Sampled)->m_internal;
    }
    bool internal() अचर { फर्काऊ m_internal; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstScopeName final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    अचर bool m_forFormat;  // Is for a format %m
    स्थिर std::string scopeNameFormatter(अचर std::string& text);
    स्थिर std::string scopePrettyNameFormatter(अचर std::string& text);

सार्वजनिक:
    class ForFormat {};
    AstScopeName(File_पङ्क्ति* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstScopeName* अचर sp = VN_DBG_AS(samep, ScopeName);
        फर्काऊ m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    // ACCESSORS
    अचर std::string& scopeAttr() अचर { फर्काऊ m_scopeAttr; }
    शून्य scopeAttr(अचर std::string& val) { m_scopeAttr = val; }
    अचर std::string& scopeEntr() अचर { फर्काऊ m_scopeEntr; }
    शून्य scopeEntr(अचर std::string& val) { m_scopeEntr = val; }
    bool dpiExport() अचर { फर्काऊ m_dpiExport; }
    शून्य dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() अचर { फर्काऊ m_selfRelative; }
    शून्य selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() अचर { फर्काऊ m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() अचर { फर्काऊ scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() अचर { फर्काऊ scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() अचर { फर्काऊ scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() अचर { फर्काऊ scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
सार्वजनिक:
    AstStable(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_Sen_रूख* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { फर्काऊ "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class Ast_थाक_Trace_F final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // $stacktrace used as function
सार्वजनिक:
    explicit Ast_थाक_Trace_F(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_थाक_Trace_F(fl) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_Ast_थाक_Trace_F;
    string verilogKwd() अचर override { फर्काऊ "$stacktrace"; }
    string emitVerilog() override { फर्काऊ verilogKwd(); }
    string emitC() override { फर्काऊ "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }
    bool isOutputter() override { फर्काऊ true; }
    bool isUnlikely() अचर override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstSteady final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    AstSteady(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { फर्काऊ "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstStructSel final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
सार्वजनिक:
    AstStructSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, अचर string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override {
        // Not a union
        फर्काऊ VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstStructSel* अचर sp = VN_DBG_AS(samep, StructSel);
        फर्काऊ m_name == sp->m_name;
    }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
};
class AstSysIgnore final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
सार्वजनिक:
    AstSysIgnore(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() अचर override { फर्काऊ "$ignored"; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }  // Though deleted before opt
    bool isPredictOptimizable() अचर override { फर्काऊ false; }  // Though deleted before opt
    bool isPure() override { फर्काऊ false; }  // Though deleted before opt
    bool isOutputter() override { फर्काऊ true; }  // Though deleted before opt
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
सार्वजनिक:
    AstSystemF(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() अचर override { फर्काऊ "$system"; }
    string emitVerilog() override { फर्काऊ verilogKwd(); }
    string emitC() override { फर्काऊ "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }
    bool isOutputter() override { फर्काऊ true; }
    bool isUnlikely() अचर override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class Ast_Tagged_अभिव्यक्ति final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
सार्वजनिक:
    Ast_Tagged_अभिव्यक्ति(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_Tagged_अभिव्यक्ति(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_अभिव्यक्ति;
    string emitVerilog() override { फर्काऊ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ m_name == VN_DBG_AS(samep, Tagged_अभिव्यक्ति)->m_name;
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
};
class AstTaggedPattern final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
सार्वजनिक:
    AstTaggedPattern(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* patternp)
        : ASTGEN_SUPER_TaggedPattern(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstTaggedPattern;
    string emitVerilog() override { फर्काऊ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ m_name == VN_DBG_AS(samep, TaggedPattern)->m_name;
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
};
class AstTestPlusArgs final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
सार्वजनिक:
    AstTestPlusArgs(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* searchp)
        : ASTGEN_SUPER_TestPlusArgs(fl) {
        this->searchp(searchp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstTestPlusArgs;
    string verilogKwd() अचर override { फर्काऊ "$test$plusargs"; }
    string emitVerilog() override { फर्काऊ verilogKwd(); }
    string emitC() override { फर्काऊ "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    // but isPure() true
    bool isSystemFunc() अचर override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstThisRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
सार्वजनिक:
    AstThisRef(File_पङ्क्ति* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_पङ्क्ति* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { फर्काऊ "this"; }
    string emitVerilog() override { फर्काऊ "this"; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ true; }
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
};
class AstTimePrecision final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Verilog $timeprecision
सार्वजनिक:
    explicit AstTimePrecision(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { फर्काऊ "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstTimeUnit final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
सार्वजनिक:
    explicit AstTimeUnit(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { फर्काऊ "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    VTimescale timeunit() अचर { फर्काऊ m_timeunit; }
    शून्य timeunit(अचर VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    अचर std::vector<uint32_t> m_indices;  // Indices of the triggers tested
सार्वजनिक:
    AstTriggered(File_पङ्क्ति* fl, AstVarScope* trigVscp, अचर std::vector<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    अचर std::vector<uint32_t>& indices() अचर { फर्काऊ m_indices; }
};
class AstUnbounded final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
सार्वजनिक:
    explicit AstUnbounded(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { फर्काऊ "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
};
class AstUnlinkedRef final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
सार्वजनिक:
    AstUnlinkedRef(File_पङ्क्ति* fl, Ast_गाँठो* refp, अचर string& name, Ast_गाँठो* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    अचर bool m_strong;  // 's_' preffix
    अचर bool m_overlapping;  // '_with` suffix
सार्वजनिक:
    AstUntil(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() अचर override {
        फर्काऊ (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
    bool isStrong() अचर { फर्काऊ m_strong; }
    bool isOverlapping() अचर { फर्काऊ m_overlapping; }
};
class Ast_मान_Plus_Args final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
सार्वजनिक:
    Ast_मान_Plus_Args(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* searchp, Ast_गाँठो_अभिव्यक्ति* outp)
        : ASTGEN_SUPER_मान_Plus_Args(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_मान_Plus_Args;
    string verilogKwd() अचर override { फर्काऊ "$value$plusargs"; }
    string emitVerilog() override { फर्काऊ "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ !outp(); }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstWithParse final : सार्वजनिक Ast_गाँठो_अभिव्यक्ति {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
गुप्त:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

सार्वजनिक:
    AstWithParse(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* funcrefp, Ast_गाँठो_अभिव्यक्ति* exprsp,
                 Ast_गाँठो* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        addExprsp(exprsp);
        addConstraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool restricted() अचर { फर्काऊ m_restricted; }
    शून्य restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : सार्वजनिक Ast_गाँठो_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
सार्वजनिक:
    AstBufIf1(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstCastDynamic final : सार्वजनिक Ast_गाँठो_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
सार्वजनिक:
    AstCastDynamic(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtypeSetBitUnsized(32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { फर्काऊ "%f$cast(%r, %l)"; }
    string emitC() override { फर्काऊ "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 20; }
    bool isPure() override { फर्काऊ false; }
};
class AstCompareNN final : सार्वजनिक Ast_गाँठो_Biop {
    // Verilog str.compare() and str.icompare()
    अचर bool m_ignoreCase;  // True for str.icompare()
सार्वजनिक:
    AstCompareNN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        फर्काऊ m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        फर्काऊ m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool ignoreCase() अचर { फर्काऊ m_ignoreCase; }
};
class AstConcat final : सार्वजनिक Ast_गाँठो_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
सार्वजनिक:
    AstConcat(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        यदि (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetBitOrLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { फर्काऊ "%f{%l, %k%r}"; }
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { फर्काऊ "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(concat %l %r)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 2; }
};
class AstConcatN final : सार्वजनिक Ast_गाँठो_Biop {
    // String concatenate
सार्वजनिक:
    AstConcatN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { फर्काऊ "%f{%l, %k%r}"; }
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { फर्काऊ "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
    bool stringFlavor() अचर override { फर्काऊ true; }
};
class AstDiv final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstDiv(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f/ %r)"; }
    string emitC() override { फर्काऊ "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvudiv %l %r)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_DIV; }
};
class AstDivD final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstDivD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstDivD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "/"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL_DIV; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstDivS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstDivS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f/ %r)"; }
    string emitC() override { फर्काऊ "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvsdiv %l %r)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_DIV; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstEqWild final : सार्वजनिक Ast_गाँठो_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
सार्वजनिक:
    AstEqWild(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    स्थिर Ast_गाँठो_Biop* newTyped(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp);
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f==? %r)"; }
    string emitC() override {
        यदि (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            फर्काऊ "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        यदि (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            फर्काऊ "==";
        }
    }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstFGetS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
सार्वजनिक:
    AstFGetS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%f$fgets(%l,%r)"; }
    string emitC() override {
        फर्काऊ strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 64; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstFUngetC(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        फर्काऊ "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 64; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_गाँठो* filep() अचर { फर्काऊ lhsp(); }
    Ast_गाँठो* charp() अचर { फर्काऊ rhsp(); }
};
class AstGetcN final : सार्वजनिक Ast_गाँठो_Biop {
    // Verilog string.getc()
सार्वजनिक:
    AstGetcN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ "getc"; }
    string emitVerilog() override { फर्काऊ "%k(%l.getc(%r))"; }
    string emitC() override { फर्काऊ "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstGetcRefN final : सार्वजनिक Ast_गाँठो_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
सार्वजनिक:
    AstGetcRefN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstGt final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstGt(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f> %r)"; }
    string emitC() override { फर्काऊ "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ ">"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstGtD final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstGtD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ ">"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstGtN final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstGtN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ ">"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
    bool stringFlavor() अचर override { फर्काऊ true; }
};
class AstGtS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstGtS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f> %r)"; }
    string emitC() override { फर्काऊ "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstGte final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstGte(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f>= %r)"; }
    string emitC() override { फर्काऊ "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ ">="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstGteD final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstGteD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ ">="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstGteN final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstGteN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ ">="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
    bool stringFlavor() अचर override { फर्काऊ true; }
};
class AstGteS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstGteS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f>= %r)"; }
    string emitC() override { फर्काऊ "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstLogAnd final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLogAnd(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f&& %r)"; }
    string emitC() override { फर्काऊ "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvand %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "&&"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() + INSTR_गन्ती_BRANCH; }
};
class AstLogIf final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLogIf(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f-> %r)"; }
    string emitC() override { फर्काऊ "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { फर्काऊ "->"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() + INSTR_गन्ती_BRANCH; }
};
class AstLogOr final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLogOr(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f|| %r)"; }
    string emitC() override { फर्काऊ "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvor %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "||"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() + INSTR_गन्ती_BRANCH; }
};
class AstLt final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLt(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f< %r)"; }
    string emitC() override { फर्काऊ "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ "<"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstLtD final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstLtD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "<"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstLtN final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstLtN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "<"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
    bool stringFlavor() अचर override { फर्काऊ true; }
};
class AstLtS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLtS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f< %r)"; }
    string emitC() override { फर्काऊ "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstLte final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLte(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f<= %r)"; }
    string emitC() override { फर्काऊ "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ "<="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstLteD final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstLteD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "<="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstLteN final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstLteN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "<="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
    bool stringFlavor() अचर override { फर्काऊ true; }
};
class AstLteS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLteS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f<= %r)"; }
    string emitC() override { फर्काऊ "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstModDiv final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstModDiv(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f%% %r)"; }
    string emitC() override { फर्काऊ "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvurem %l %r)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_DIV; }
};
class AstModDivS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstModDivS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f%% %r)"; }
    string emitC() override { फर्काऊ "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvsmod %l %r)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_DIV; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstNeqWild final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNeqWild(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f!=? %r)"; }
    string emitC() override {
        यदि (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            फर्काऊ "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        यदि (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            फर्काऊ "!=";
        }
    }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstPow final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstPow(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f** %r)"; }
    string emitC() override { फर्काऊ "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_MUL * 10; }
};
class AstPowD final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstPowD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstPowD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f** %r)"; }
    string emitC() override { फर्काऊ "pow(%li,%ri)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL_DIV * 5; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstPowSS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstPowSS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f** %r)"; }
    string emitC() override { फर्काऊ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_MUL * 10; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstPowSU final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstPowSU(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f** %r)"; }
    string emitC() override { फर्काऊ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_MUL * 10; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstPowUS final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstPowUS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f** %r)"; }
    string emitC() override { फर्काऊ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_MUL * 10; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstReplicate final : सार्वजनिक Ast_गाँठो_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
सार्वजनिक:
    AstReplicate(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        यदि (lhsp) {
            यदि (अचर AstConst* अचर constp = VN_CAST(rhsp, Const)) {
                यदि (constp->अङ्क().isFourState()
                    || (constp->dtypep()->isSigned() && constp->अङ्क().isNegative())) {
                    dtypeSetBitOrLogicSized(lhsp->width(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } अन्यथा {
                    dtypeSetBitOrLogicSized(lhsp->width() * constp->toSInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, uint32_t rep_गन्ती)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_गन्ती}} {}
    ASTGEN_MEMBERS_AstReplicate;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%f{%r{%k%l}}"; }
    string emitC() override { फर्काऊ "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override {
        फर्काऊ "((_ repeat " + cvtToStr(width() / lhsp()->width()) + ") %l)";
    }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 2; }
};
class AstReplicateN final : सार्वजनिक Ast_गाँठो_Biop {
    // String replicate
सार्वजनिक:
    AstReplicateN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%f{%r{%k%l}}"; }
    string emitC() override { फर्काऊ "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 2; }
    bool stringFlavor() अचर override { फर्काऊ true; }
};
class AstSAnd final : सार्वजनिक Ast_गाँठो_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    अचर bool m_propertyControl;  // Parser-generated property if/case branch conjunction
सार्वजनिक:
    AstSAnd(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() + INSTR_गन्ती_BRANCH; }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {  // LCOV_EXCL_LINE
        फर्काऊ m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() अचर { फर्काऊ m_propertyControl; }
};
class AstSIntersect final : सार्वजनिक Ast_गाँठो_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
सार्वजनिक:
    AstSIntersect(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() + INSTR_गन्ती_BRANCH; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstSOr final : सार्वजनिक Ast_गाँठो_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
सार्वजनिक:
    AstSOr(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSOr;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() + INSTR_गन्ती_BRANCH; }
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstSThroughout final : सार्वजनिक Ast_गाँठो_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
सार्वजनिक:
    AstSThroughout(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstSWithin final : सार्वजनिक Ast_गाँठो_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
सार्वजनिक:
    AstSWithin(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() अचर override { फर्काऊ true; }
};
class AstSel final : सार्वजनिक Ast_गाँठो_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    पूर्णाङ्क m_declElWidth;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    पूर्णाङ्क m_widthConst;

सार्वजनिक:
    AstSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* lsbp, पूर्णाङ्क bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicSized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, पूर्णाङ्क lsb, पूर्णाङ्क bitwidth, पूर्णाङ्क bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicUnsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& from, अचर V3_सङ्ख्या& bit) override {
        out.opSel(from, bit.toUInt() + widthConst() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        फर्काऊ widthConst() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() अचर override { फर्काऊ "((_ extract %t %r) %l)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* otherp) अचर override {
        फर्काऊ widthConst() == VN_DBG_AS(otherp, Sel)->widthConst();
    }
    पूर्णाङ्क instr_गन्ती() अचर override {
        फर्काऊ widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    पूर्णाङ्क widthConst() अचर { फर्काऊ m_widthConst; }
    शून्य widthConst(पूर्णाङ्क मान) { m_widthConst = मान; }
    पूर्णाङ्क lsbConst() अचर { फर्काऊ VN_AS(lsbp(), Const)->toSInt(); }
    पूर्णाङ्क msbConst() अचर { फर्काऊ lsbConst() + widthConst() - 1; }
    VNumRange& declRange() VL_MT_STABLE { फर्काऊ m_declRange; }
    अचर VNumRange& declRange() अचर VL_MT_STABLE { फर्काऊ m_declRange; }
    शून्य declRange(अचर VNumRange& flag) { m_declRange = flag; }
    पूर्णाङ्क declElWidth() अचर { फर्काऊ m_declElWidth; }
    शून्य declElWidth(पूर्णाङ्क flag) { m_declElWidth = flag; }
};
class AstShiftL final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstShiftL(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा यदि (अचर Ast_गाँठो_DType* अचर dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftL;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f<< %r)"; }
    string emitC() override { फर्काऊ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        फर्काऊ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstShiftLOvr final : सार्वजनिक Ast_गाँठो_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
सार्वजनिक:
    AstShiftLOvr(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_ShiftLOvr(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            अचर Ast_गाँठो_DType* अचर dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftLOvr;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f<< %r)"; }
    string emitC() override { फर्काऊ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstShiftR final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstShiftR(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा यदि (अचर Ast_गाँठो_DType* अचर dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftR;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f>> %r)"; }
    string emitC() override { फर्काऊ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        फर्काऊ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstShiftROvr final : सार्वजनिक Ast_गाँठो_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
सार्वजनिक:
    AstShiftROvr(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_ShiftROvr(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            अचर Ast_गाँठो_DType* अचर dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftROvr;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f>> %r)"; }
    string emitC() override { फर्काऊ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstShiftRS final : सार्वजनिक Ast_गाँठो_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
सार्वजनिक:
    AstShiftRS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा यदि (अचर Ast_गाँठो_DType* अचर dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f>>> %r)"; }
    string emitSMT() अचर override { फर्काऊ "(bvashr %l %r)"; }
    string emitC() override { फर्काऊ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstShiftRSOvr final : सार्वजनिक Ast_गाँठो_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
सार्वजनिक:
    AstShiftRSOvr(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_ShiftRSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        यदि (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            अचर Ast_गाँठो_DType* अचर dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRSOvr;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f>>> %r)"; }
    string emitC() override { फर्काऊ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstSub final : सार्वजनिक Ast_गाँठो_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstSub(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f- %r)"; }
    string emitC() override { फर्काऊ "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvsub %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "-"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
};
class AstSubD final : सार्वजनिक Ast_गाँठो_Biop {
सार्वजनिक:
    AstSubD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstSubD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "-"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstURandomRange final : सार्वजनिक Ast_गाँठो_Biop {
    // $urandom_range
सार्वजनिक:
    AstURandomRange(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%f$urandom_range(%l, %r)"; }
    string emitC() override { फर्काऊ "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : सार्वजनिक Ast_गाँठो_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstEq(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    स्थिर Ast_गाँठो_Biop* newTyped(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp);
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f== %r)"; }
    string emitC() override { फर्काऊ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { फर्काऊ "=="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstEqCase final : सार्वजनिक Ast_गाँठो_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstEqCase(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f=== %r)"; }
    string emitC() override { फर्काऊ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { फर्काऊ "=="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstEqD final : सार्वजनिक Ast_गाँठो_Bi_Com {
सार्वजनिक:
    AstEqD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "=="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstEqN final : सार्वजनिक Ast_गाँठो_Bi_Com {
सार्वजनिक:
    AstEqN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "=="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
    bool stringFlavor() अचर override { फर्काऊ true; }
};
class AstEqT final : सार्वजनिक Ast_गाँठो_Bi_Com {
    // Equal (==) for data types
सार्वजनिक:
    AstEqT(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqT;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "=="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
};
class AstLogEq final : सार्वजनिक Ast_गाँठो_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLogEq(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f<-> %r)"; }
    string emitC() override { फर्काऊ "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "<->"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() + INSTR_गन्ती_BRANCH; }
};
class AstNeq final : सार्वजनिक Ast_गाँठो_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNeq(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    स्थिर Ast_गाँठो_Biop* newTyped(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp);
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f!= %r)"; }
    string emitC() override { फर्काऊ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { फर्काऊ "!="; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (not (= %l %r)))"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstNeqCase final : सार्वजनिक Ast_गाँठो_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNeqCase(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f!== %r)"; }
    string emitC() override { फर्काऊ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { फर्काऊ "!="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
};
class AstNeqD final : सार्वजनिक Ast_गाँठो_Bi_Com {
सार्वजनिक:
    AstNeqD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "!="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstNeqN final : सार्वजनिक Ast_गाँठो_Bi_Com {
सार्वजनिक:
    AstNeqN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "!="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
    bool stringFlavor() अचर override { फर्काऊ true; }
};
class AstNeqT final : सार्वजनिक Ast_गाँठो_Bi_Com {
    // Not-equal (!=) for data types
सार्वजनिक:
    AstNeqT(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "!="; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_STR; }
};

// === AstNodeBiComAsv ===
class AstAdd final : सार्वजनिक Ast_गाँठो_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstAdd(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAdd;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f+ %r)"; }
    string emitC() override { फर्काऊ "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvadd %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "+"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
};
class AstAddD final : सार्वजनिक Ast_गाँठो_Bi_Com_Asv {
सार्वजनिक:
    AstAddD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstAddD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "+"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstAnd final : सार्वजनिक Ast_गाँठो_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstAnd(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f& %r)"; }
    string emitC() override { फर्काऊ "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvand %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "&"; }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    अचर वर्ण* widthMismatch() अचर override VL_MT_STABLE;
};
class AstMul final : सार्वजनिक Ast_गाँठो_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstMul(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f* %r)"; }
    string emitC() override { फर्काऊ "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "*"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_MUL; }
};
class AstMulD final : सार्वजनिक Ast_गाँठो_Bi_Com_Asv {
सार्वजनिक:
    AstMulD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstMulD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "*"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstMulS final : सार्वजनिक Ast_गाँठो_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstMulS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f* %r)"; }
    string emitC() override { फर्काऊ "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    bool sizeMattersRhs() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * INSTR_गन्ती_INT_MUL; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstOr final : सार्वजनिक Ast_गाँठो_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstOr(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f| %r)"; }
    string emitC() override { फर्काऊ "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvor %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "|"; }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    अचर वर्ण* widthMismatch() अचर override VL_MT_STABLE;
};
class AstXor final : सार्वजनिक Ast_गाँठो_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstXor(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f^ %r)"; }
    string emitC() override { फर्काऊ "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() अचर override { फर्काऊ "(bvxor %l %r)"; }
    string emitSimpleOperator() override { फर्काऊ "^"; }
    bool cleanOut() अचर override { फर्काऊ false; }  // Lclean && Rclean
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    अचर वर्ण* widthMismatch() अचर override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class AstDistChiSquare final : सार्वजनिक Ast_गाँठो_Dist_Biop {
सार्वजनिक:
    AstDistChiSquare(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_DistChiSquare(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistChiSquare;
    string emitVerilog() override { फर्काऊ "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { फर्काऊ "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class AstDistExponential final : सार्वजनिक Ast_गाँठो_Dist_Biop {
सार्वजनिक:
    AstDistExponential(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_DistExponential(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistExponential;
    string emitVerilog() override { फर्काऊ "%f$dist_exponential(%l, %r)"; }
    string emitC() override { फर्काऊ "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class AstDistPoisson final : सार्वजनिक Ast_गाँठो_Dist_Biop {
सार्वजनिक:
    AstDistPoisson(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_DistPoisson(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistPoisson;
    string emitVerilog() override { फर्काऊ "%f$dist_poisson(%l, %r)"; }
    string emitC() override { फर्काऊ "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : सार्वजनिक Ast_गाँठो_Dist_Biop {
सार्वजनिक:
    AstDistT(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { फर्काऊ "%f$dist_t(%l, %r)"; }
    string emitC() override { फर्काऊ "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : सार्वजनिक Ast_गाँठो_Sel {
    // @astgen makeDfgVertex
    शून्य init(अचर Ast_गाँठो* fromp) {
        यदि (fromp && VN_IS(fromp->dtypep()->skipRefp(), गाँठो_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), गाँठो_Array_DType)->subDTypep());
        }
    }

सार्वजनिक:
    AstArraySel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, पूर्णाङ्क bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() अचर override { फर्काऊ "(select %l %r)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isGateOptimizable() अचर override { फर्काऊ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
};
class AstAssocSel final : सार्वजनिक Ast_गाँठो_Sel {
    शून्य init(अचर Ast_गाँठो* fromp) {
        यदि (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

सार्वजनिक:
    AstAssocSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%k(%l%f[%r])"; }
    string emitC() override { फर्काऊ "%li%k[%ri]"; }
    string emitSMT() अचर override { फर्काऊ "(select %l %r)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isGateOptimizable() अचर override {
        फर्काऊ !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ !isLValue(); }  // AssocSel creates on miss
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
};
class AstWildcardSel final : सार्वजनिक Ast_गाँठो_Sel {
    शून्य init(अचर Ast_गाँठो* fromp) {
        यदि (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

सार्वजनिक:
    AstWildcardSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%k(%l%f[%r])"; }
    string emitC() override { फर्काऊ "%li%k[%ri]"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool isGateOptimizable() अचर override { फर्काऊ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
};
class AstWordSel final : सार्वजनिक Ast_गाँठो_Sel {
    // Select a single word from a multi-word wide value
सार्वजनिक:
    AstWordSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या&, अचर V3_सङ्ख्या&, अचर V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};

// === AstNodeStream ===
class AstStreamL final : सार्वजनिक Ast_गाँठो_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
सार्वजनिक:
    AstStreamL(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamL;
    string emitVerilog() override { फर्काऊ "%f{ << %r %k{%l} }"; }
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { फर्काऊ "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 2; }
};
class AstStreamR final : सार्वजनिक Ast_गाँठो_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
सार्वजनिक:
    AstStreamR(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamR;
    string emitVerilog() override { फर्काऊ "%f{ >> %r %k{%l} }"; }
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { फर्काऊ isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : सार्वजनिक Ast_गाँठो_System_Biop_D {
सार्वजनिक:
    AstAtan2D(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$atan2(%l,%r)"; }
    string emitC() override { फर्काऊ "atan2(%li,%ri)"; }
};
class AstHypotD final : सार्वजनिक Ast_गाँठो_System_Biop_D {
सार्वजनिक:
    AstHypotD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$hypot(%l,%r)"; }
    string emitC() override { फर्काऊ "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : सार्वजनिक Ast_गाँठो_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
सार्वजनिक:
    AstCCall(File_पङ्क्ति* fl, AstCFunc* funcp, Ast_गाँठो_अभिव्यक्ति* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    अचर VSelfPointerText& selfPointer() अचर { फर्काऊ m_selfPointer; }
    शून्य selfPointer(अचर VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) अचर {
        फर्काऊ selfPointer().protect(useSelfForThis, protect());
    }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
};
class AstCMethodCall final : सार्वजनिक Ast_गाँठो_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    AstCMethodCall(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, AstCFunc* funcp, Ast_गाँठो_अभिव्यक्ति* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : सार्वजनिक Ast_गाँठो_CCall {
    // C++ new() call
सार्वजनिक:
    AstCNew(File_पङ्क्ति* fl, AstCFunc* funcp, Ast_गाँठो_अभिव्यक्ति* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : सार्वजनिक Ast_गाँठो_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

सार्वजनिक:
    AstConsArray(File_पङ्क्ति* fl, Ast_गाँठो_DType* newDTypep, Ast_गाँठो_अभिव्यक्ति* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    अचर वर्ण* broken() अचर override;
    शून्य cloneRelink() override;
    अन्तःस्थ AstConsArrayItem* itemsp() अचर VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_गाँठो_अभिव्यक्ति* valuep(uint64_t index) अचर;
    // Add the value of the element at the index, which must not be given yet
    शून्य valuep(uint64_t index, Ast_गाँठो_अभिव्यक्ति* newp);
    // Add the elements of the array 'arrayp', starting at the index
    शून्य splice_मानहरू(uint64_t index, Ast_गाँठो_अभिव्यक्ति* arrayp);
};
class AstConsAssoc final : सार्वजनिक Ast_गाँठो_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
सार्वजनिक:
    explicit AstConsAssoc(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        फर्काऊ nullptr;
    }
};
class AstConsDynArray final : सार्वजनिक Ast_गाँठो_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
सार्वजनिक:
    explicit AstConsDynArray(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    अचर वर्ण* broken() अचर override;
};
class AstConsQueue final : सार्वजनिक Ast_गाँठो_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
सार्वजनिक:
    explicit AstConsQueue(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        फर्काऊ nullptr;
    }
};
class AstConsStruct final : सार्वजनिक Ast_गाँठो_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
सार्वजनिक:
    AstConsStruct(File_पङ्क्ति* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    अचर वर्ण* broken() अचर override;
    // The value of the member called 'name'
    Ast_गाँठो_अभिव्यक्ति* memberValuep(अचर string& name) अचर;
};
class AstConsWildcard final : सार्वजनिक Ast_गाँठो_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
सार्वजनिक:
    explicit AstConsWildcard(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        फर्काऊ nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : सार्वजनिक Ast_गाँठो_FTask_Ref {
    // A reference to a function
सार्वजनिक:
    अन्तःस्थ AstFuncRef(File_पङ्क्ति* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_पङ्क्ति* fl, अचर string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class AstMethodCall final : सार्वजनिक Ast_गाँठो_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
सार्वजनिक:
    AstMethodCall(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, VFlagChildDType, अचर string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMethodCall(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, अचर string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_AstMethodCall;
};
class AstNew final : सार्वजनिक Ast_गाँठो_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
सार्वजनिक:
    AstNew(File_पङ्क्ति* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool isImplicit() अचर { फर्काऊ m_isImplicit; }
    शून्य isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() अचर { फर्काऊ m_isScoped; }
    शून्य isScoped(bool flag) { m_isScoped = flag; }
    bool isPure() override { फर्काऊ false; }
};
class AstTaskRef final : सार्वजनिक Ast_गाँठो_FTask_Ref {
    // A reference to a task
सार्वजनिक:
    अन्तःस्थ AstTaskRef(File_पङ्क्ति* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_पङ्क्ति* fl, अचर string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtypeSetVoid();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : सार्वजनिक Ast_गाँठो_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
गुप्त:
    VAccess m_access;  // Left hand side assignment
सार्वजनिक:
    AstSelBit(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    VAccess access() अचर { फर्काऊ m_access; }
    शून्य access(अचर VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : सार्वजनिक Ast_गाँठो_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
सार्वजनिक:
    AstSelExtract(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* msbp, Ast_गाँठो_अभिव्यक्ति* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : सार्वजनिक Ast_गाँठो_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
सार्वजनिक:
    AstSelMinus(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* bitp, Ast_गाँठो_अभिव्यक्ति* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : सार्वजनिक Ast_गाँठो_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
सार्वजनिक:
    AstSelPlus(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* bitp, Ast_गाँठो_अभिव्यक्ति* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_गन्ती_Bits final : सार्वजनिक Ast_गाँठो_Quadop {
    // Number of bits set in vector
सार्वजनिक:
    Ast_गन्ती_Bits(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* ctrl1p)
        : ASTGEN_SUPER_गन्ती_Bits(fl, exprp, ctrl1p, ctrl1p->clone_रूख_Pure(false),
                                 ctrl1p->clone_रूख_Pure(false)) {
        dtypeSetInt();
    }
    Ast_गन्ती_Bits(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* ctrl1p, Ast_गाँठो_अभिव्यक्ति* ctrl2p)
        : ASTGEN_SUPER_गन्ती_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_रूख_Pure(false)) {
        dtypeSetInt();
    }
    Ast_गन्ती_Bits(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, Ast_गाँठो_अभिव्यक्ति* ctrl1p, Ast_गाँठो_अभिव्यक्ति* ctrl2p,
                 Ast_गाँठो_अभिव्यक्ति* ctrl3p)
        : ASTGEN_SUPER_गन्ती_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_गन्ती_Bits;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& अभिव्यक्ति, अचर V3_सङ्ख्या& ctrl1,
                       अचर V3_सङ्ख्या& ctrl2, अचर V3_सङ्ख्या& ctrl3) override {
        out.op_गन्ती_Bits(अभिव्यक्ति, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { फर्काऊ "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool cleanThs() अचर override { फर्काऊ true; }
    bool cleanFhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool sizeMattersThs() अचर override { फर्काऊ false; }
    bool sizeMattersFhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : सार्वजनिक Ast_गाँठो_Termop {
सार्वजनिक:
    explicit AstInferredDisable(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { फर्काऊ "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstTime final : सार्वजनिक Ast_गाँठो_Termop {
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    AstTime(File_पङ्क्ति* fl, अचर VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { फर्काऊ "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_TIME; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    VTimescale timeunit() अचर { फर्काऊ m_timeunit; }
    शून्य timeunit(अचर VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : सार्वजनिक Ast_गाँठो_Termop {
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    AstTimeD(File_पङ्क्ति* fl, अचर VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { फर्काऊ "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_TIME; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    VTimescale timeunit() अचर { फर्काऊ m_timeunit; }
    शून्य timeunit(अचर VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : सार्वजनिक Ast_गाँठो_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
सार्वजनिक:
    AstCond(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* condp, Ast_गाँठो_अभिव्यक्ति* thenp, Ast_गाँठो_अभिव्यक्ति* elsep);
    ASTGEN_MEMBERS_AstCond;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs,
                       अचर V3_सङ्ख्या& ths) override {
        out.opAssign(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { फर्काऊ "%k(%l %f? %r %k: %t)"; }
    string emitC() override { फर्काऊ "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() अचर override { फर्काऊ "(ite (__Vbool %l) %r %t)"; }
    bool cleanOut() अचर override { फर्काऊ false; }  // clean if e1 & e2 clean
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ false; }
    bool cleanThs() अचर override { फर्काऊ false; }  // Propagates up
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool sizeMattersThs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_BRANCH; }
};
class AstPutcN final : सार्वजनिक Ast_गाँठो_Triop {
    // Verilog string.putc()
सार्वजनिक:
    AstPutcN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, Ast_गाँठो_अभिव्यक्ति* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstPutcN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs,
                       अचर V3_सङ्ख्या& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ "putc"; }
    string emitVerilog() override { फर्काऊ "%k(%l.putc(%r,%t))"; }
    string emitC() override { फर्काऊ "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool cleanThs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool sizeMattersThs() अचर override { फर्काऊ false; }
};
class AstSliceSel final : सार्वजनिक Ast_गाँठो_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
सार्वजनिक:
    AstSliceSel(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* fromp, अचर VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_AstSliceSel;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& from, अचर V3_सङ्ख्या& lo,
                       अचर V3_सङ्ख्या& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool cleanThs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool sizeMattersThs() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { फर्काऊ m_declRange; }
    अचर VNumRange& declRange() अचर VL_MT_STABLE { फर्काऊ m_declRange; }
};
class AstSubstrN final : सार्वजनिक Ast_गाँठो_Triop {
    // Verilog string.substr()
सार्वजनिक:
    AstSubstrN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, Ast_गाँठो_अभिव्यक्ति* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs,
                       अचर V3_सङ्ख्या& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ "substr"; }
    string emitVerilog() override { फर्काऊ "%k(%l.substr(%r,%t))"; }
    string emitC() override { फर्काऊ "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { फर्काऊ ""; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool cleanRhs() अचर override { फर्काऊ true; }
    bool cleanThs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool sizeMattersRhs() अचर override { फर्काऊ false; }
    bool sizeMattersThs() अचर override { फर्काऊ false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : सार्वजनिक Ast_गाँठो_Dist_Triop {
सार्वजनिक:
    AstDistErlang(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, Ast_गाँठो_अभिव्यक्ति* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { फर्काऊ "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { फर्काऊ "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class AstDistNormal final : सार्वजनिक Ast_गाँठो_Dist_Triop {
सार्वजनिक:
    AstDistNormal(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, Ast_गाँठो_अभिव्यक्ति* thsp)
        : ASTGEN_SUPER_DistNormal(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistNormal;
    string emitVerilog() override { फर्काऊ "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { फर्काऊ "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class AstDistUniform final : सार्वजनिक Ast_गाँठो_Dist_Triop {
सार्वजनिक:
    AstDistUniform(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp, Ast_गाँठो_अभिव्यक्ति* thsp)
        : ASTGEN_SUPER_DistUniform(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistUniform;
    string emitVerilog() override { फर्काऊ "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { फर्काऊ "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : सार्वजनिक Ast_गाँठो_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
सार्वजनिक:
    गणना FmtType : पूर्णाङ्क { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

गुप्त:
    // dist-ast-dump-suppress  // Part of name()
    अचर FmtType m_fmtType;  // Operation type
सार्वजनिक:
    AstAtoN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtypeSetDouble() : dtypeSetInteger();
    }
    ASTGEN_MEMBERS_AstAtoN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opAtoN(lhs, m_fmtType); }
    string name() अचर override VL_MT_STABLE {
        विकल्प (m_fmtType) {
        पक्ष ATOI: फर्काऊ "atoi";
        पक्ष ATOHEX: फर्काऊ "atohex";
        पक्ष ATOOCT: फर्काऊ "atooct";
        पक्ष ATOBIN: फर्काऊ "atobin";
        पक्ष ATOREAL: फर्काऊ "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { फर्काऊ "%l." + name() + "()"; }
    string emitC() override {
        विकल्प (m_fmtType) {
        पक्ष ATOI: फर्काऊ "VL_ATOI_N(%li, 10)";
        पक्ष ATOHEX: फर्काऊ "VL_ATOI_N(%li, 16)";
        पक्ष ATOOCT: फर्काऊ "VL_ATOI_N(%li, 8)";
        पक्ष ATOBIN: फर्काऊ "VL_ATOI_N(%li, 2)";
        पक्ष ATOREAL: फर्काऊ "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    FmtType fmtType() अचर { फर्काऊ m_fmtType; }
};
class AstBitsToRealD final : सार्वजनिक Ast_गाँठो_Uniop {
सार्वजनिक:
    AstBitsToRealD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstBitsToRealD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { फर्काऊ "%f$bitstoreal(%l)"; }
    string emitC() override { फर्काऊ "VL_CVT_D_Q(%li)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }  // Eliminated before matters
    bool sizeMattersLhs() अचर override { फर्काऊ false; }  // Eliminated before matters
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstCCast final : सार्वजनिक Ast_गाँठो_Uniop {
    // Cast to C-based data type
    पूर्णाङ्क m_size;

सार्वजनिक:
    AstCCast(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, पूर्णाङ्क setwidth, पूर्णाङ्क minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_size = setwidth;
        यदि (setwidth) {
            यदि (minwidth == -1) minwidth = setwidth;
            dtypeSetBitUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_size = width();
    }
    ASTGEN_MEMBERS_AstCCast;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { फर्काऊ "%f$_CAST(%l)"; }
    string emitC() override { फर्काऊ "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }  // Special cased in V3Cast
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ size() == VN_DBG_AS(samep, CCast)->size();
    }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    //
    पूर्णाङ्क size() अचर { फर्काऊ m_size; }
};
class AstCLog2 final : सार्वजनिक Ast_गाँठो_Uniop {
सार्वजनिक:
    AstCLog2(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { फर्काऊ "%f$clog2(%l)"; }
    string emitC() override { फर्काऊ "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 16; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstCastWrap final : सार्वजनिक Ast_गाँठो_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
सार्वजनिक:
    AstCastWrap(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { फर्काऊ "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
};
class Ast_गन्ती_Ones final : सार्वजनिक Ast_गाँठो_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
सार्वजनिक:
    Ast_गन्ती_Ones(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_गन्ती_Ones(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_गन्ती_Ones;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.op_गन्ती_Ones(lhs); }
    string emitVerilog() override { फर्काऊ "%f$countones(%l)"; }
    string emitC() override { फर्काऊ "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 16; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstCvtPackString final : सार्वजनिक Ast_गाँठो_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
सार्वजनिक:
    AstCvtPackString(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstExtend final : सार्वजनिक Ast_गाँठो_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
सार्वजनिक:
    AstExtend(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, पूर्णाङ्क width, पूर्णाङ्क widthMin = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetBitOrLogicUnsized(width, widthMin ? widthMin : width, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { फर्काऊ "%l"; }
    string emitC() override { फर्काऊ "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() अचर override {
        फर्काऊ "((_ zero_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override {
        फर्काऊ false;  // Because the EXTEND operator self-casts
    }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
};
class AstExtendS final : सार्वजनिक Ast_गाँठो_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
सार्वजनिक:
    AstExtendS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, पूर्णाङ्क width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { फर्काऊ "%l"; }
    string emitC() override { फर्काऊ "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() अचर override {
        फर्काऊ "((_ sign_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override {
        फर्काऊ false;  // Because the EXTEND operator self-casts
    }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
    bool signedFlavor() अचर override { फर्काऊ true; }
};
class AstFEof final : सार्वजनिक Ast_गाँठो_Uniop {
सार्वजनिक:
    AstFEof(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%f$feof(%l)"; }
    string emitC() override { फर्काऊ "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 16; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() अचर override { फर्काऊ true; }
    Ast_गाँठो* filep() अचर { फर्काऊ lhsp(); }
};
class AstFGetC final : सार्वजनिक Ast_गाँठो_Uniop {
सार्वजनिक:
    AstFGetC(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { फर्काऊ "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 64; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_गाँठो* filep() अचर { फर्काऊ lhsp(); }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstISToRD final : सार्वजनिक Ast_गाँठो_Uniop {
    // $itor where lhs is signed
सार्वजनिक:
    AstISToRD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstISToRD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { फर्काऊ "%f$itor($signed(%l))"; }
    string emitC() override { फर्काऊ "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_शब्दहरू() override { फर्काऊ true; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstIToRD final : सार्वजनिक Ast_गाँठो_Uniop {
    // $itor where lhs is unsigned
सार्वजनिक:
    AstIToRD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstIToRD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { फर्काऊ "%f$itor(%l)"; }
    string emitC() override { फर्काऊ "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstIsUnbounded final : सार्वजनिक Ast_गाँठो_Uniop {
    // True if is unbounded ($)
सार्वजनिक:
    AstIsUnbounded(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { फर्काऊ "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstIsUnknown final : सार्वजनिक Ast_गाँठो_Uniop {
    // True if any unknown bits
सार्वजनिक:
    AstIsUnknown(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { फर्काऊ "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstLenN final : सार्वजनिक Ast_गाँठो_Uniop {
    // Length of a string
सार्वजनिक:
    AstLenN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstLenN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { फर्काऊ "%f(%l)"; }
    string emitC() override { फर्काऊ "VL_LEN_IN(%li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
};
class AstLogNot final : सार्वजनिक Ast_गाँठो_Uniop {
    // @astgen makeDfgVertex
गुप्त:
    अचर bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
सार्वजनिक:
    AstLogNot(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { फर्काऊ "%f(! %l)"; }
    string emitC() override { फर्काऊ "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() अचर override { फर्काऊ "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { फर्काऊ "!"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool fromProperty() अचर { फर्काऊ m_fromProperty; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstMostSetBitP1 final : सार्वजनिक Ast_गाँठो_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
सार्वजनिक:
    AstMostSetBitP1(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_MostSetBitP1(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstMostSetBitP1;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opMostSetBitP1(lhs); }
    string emitVerilog() override { फर्काऊ "%f$mostsetbitp1(%l)"; }
    string emitC() override { फर्काऊ "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 16; }
};
class AstNToI final : सार्वजनिक Ast_गाँठो_Uniop {
    // String to any-size integral
सार्वजनिक:
    AstNToI(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opNToI(lhs); }
    string emitVerilog() override { फर्काऊ "'(%l)"; }
    string emitC() override { फर्काऊ "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
};
class AstNegate final : सार्वजनिक Ast_गाँठो_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNegate(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { फर्काऊ "%f(- %l)"; }
    string emitC() override { फर्काऊ "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() अचर override { फर्काऊ "(bvneg %l)"; }
    string emitSimpleOperator() override { फर्काऊ "-"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
};
class AstNegateD final : सार्वजनिक Ast_गाँठो_Uniop {
सार्वजनिक:
    AstNegateD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstNegateD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { फर्काऊ "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { फर्काऊ "-"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool doubleFlavor() अचर override { फर्काऊ true; }
};
class AstNot final : सार्वजनिक Ast_गाँठो_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNot(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { फर्काऊ "%f(~ %l)"; }
    string emitC() override { फर्काऊ "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() अचर override { फर्काऊ "(bvnot %l)"; }
    string emitSimpleOperator() override { फर्काऊ "~"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ true; }
    अचर वर्ण* widthMismatch() अचर override VL_MT_STABLE;
};
class AstNullCheck final : सार्वजनिक Ast_गाँठो_Uniop {
    // Return LHS after checking that LHS is non-null
सार्वजनिक:
    AstNullCheck(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 1; }  // Rarely executes
    string emitVerilog() override { फर्काऊ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override { फर्काऊ fileline() == samep->fileline(); }
};
class AstOneHot final : सार्वजनिक Ast_गाँठो_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
सार्वजनिक:
    AstOneHot(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { फर्काऊ "%f$onehot(%l)"; }
    string emitC() override { फर्काऊ "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 4; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstOneHot0 final : सार्वजनिक Ast_गाँठो_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
सार्वजनिक:
    AstOneHot0(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { फर्काऊ "%f$onehot0(%l)"; }
    string emitC() override { फर्काऊ "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs() * 3; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstPostDec final : सार्वजनिक Ast_गाँठो_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
सार्वजनिक:
    AstPostDec(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { फर्काऊ "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() अचर override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : सार्वजनिक Ast_गाँठो_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
सार्वजनिक:
    AstPostInc(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { फर्काऊ "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() अचर override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : सार्वजनिक Ast_गाँठो_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
सार्वजनिक:
    AstPreDec(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { फर्काऊ "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() अचर override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : सार्वजनिक Ast_गाँठो_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
सार्वजनिक:
    AstPreInc(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { फर्काऊ "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() अचर override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() अचर override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : सार्वजनिक Ast_गाँठो_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
सार्वजनिक:
    AstRToIRoundS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { फर्काऊ "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        फर्काऊ isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstRToIS final : सार्वजनिक Ast_गाँठो_Uniop {
    // $rtoi(lhs)
सार्वजनिक:
    AstRToIS(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { फर्काऊ "%f$rtoi(%l)"; }
    string emitC() override { फर्काऊ "VL_RTOI_I_D(%li)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }  // Eliminated before matters
    bool sizeMattersLhs() अचर override { फर्काऊ false; }  // Eliminated before matters
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstRealToBits final : सार्वजनिक Ast_गाँठो_Uniop {
सार्वजनिक:
    AstRealToBits(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstRealToBits;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { फर्काऊ "%f$realtobits(%l)"; }
    string emitC() override { फर्काऊ "VL_CVT_Q_D(%li)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }  // Eliminated before matters
    bool sizeMattersLhs() अचर override { फर्काऊ false; }  // Eliminated before matters
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_DBL; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstRedAnd final : सार्वजनिक Ast_गाँठो_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstRedAnd(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { फर्काऊ "%f(& %l)"; }
    string emitC() override { फर्काऊ "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
};
class AstRedOr final : सार्वजनिक Ast_गाँठो_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstRedOr(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { फर्काऊ "%f(| %l)"; }
    string emitC() override { फर्काऊ "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
};
class AstRedXor final : सार्वजनिक Ast_गाँठो_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstRedXor(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { फर्काऊ "%f(^ %l)"; }
    string emitC() override { फर्काऊ "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override {
        अचर पूर्णाङ्क w = lhsp()->width();
        फर्काऊ (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 1 + V3_सङ्ख्या::log2b(width()); }
};
class AstResizeLValue final : सार्वजनिक Ast_गाँठो_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
सार्वजनिक:
    AstResizeLValue(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
};
class AstSigned final : सार्वजनिक Ast_गाँठो_Uniop {
    // $signed(lhs)
सार्वजनिक:
    AstSigned(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { फर्काऊ "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }  // Eliminated before matters
    bool sizeMattersLhs() अचर override { फर्काऊ true; }  // Eliminated before matters
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};
class AstTimeImport final : सार्वजनिक Ast_गाँठो_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    AstTimeImport(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    VTimescale timeunit() अचर { फर्काऊ m_timeunit; }
    शून्य timeunit(अचर VTimescale& flag) { m_timeunit = flag; }
};
class AstToLowerN final : सार्वजनिक Ast_गाँठो_Uniop {
    // string.tolower()
सार्वजनिक:
    AstToLowerN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToLowerN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { फर्काऊ "%l.tolower()"; }
    string emitC() override { फर्काऊ "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
};
class AstToStringN final : सार्वजनिक Ast_गाँठो_Uniop {
सार्वजनिक:
    AstToStringN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_ToStringN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToStringN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { फर्काऊ "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        फर्काऊ isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
};
class AstToUpperN final : सार्वजनिक Ast_गाँठो_Uniop {
    // string.toupper()
सार्वजनिक:
    AstToUpperN(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToUpperN;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { फर्काऊ "%l.toupper()"; }
    string emitC() override { फर्काऊ "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() अचर override { फर्काऊ true; }
    bool cleanLhs() अचर override { फर्काऊ true; }
    bool sizeMattersLhs() अचर override { फर्काऊ false; }
};
class AstUnsigned final : सार्वजनिक Ast_गाँठो_Uniop {
    // $unsigned(lhs)
सार्वजनिक:
    AstUnsigned(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { फर्काऊ "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ false; }
    bool cleanLhs() अचर override { फर्काऊ false; }  // Eliminated before matters
    bool sizeMattersLhs() अचर override { फर्काऊ true; }  // Eliminated before matters
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 0; }
    bool isSystemFunc() अचर override { फर्काऊ true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstAcosD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$acos(%l)"; }
    string emitC() override { फर्काऊ "acos(%li)"; }
};
class AstAcoshD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstAcoshD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$acosh(%l)"; }
    string emitC() override { फर्काऊ "acosh(%li)"; }
};
class AstAsinD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstAsinD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$asin(%l)"; }
    string emitC() override { फर्काऊ "asin(%li)"; }
};
class AstAsinhD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstAsinhD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$asinh(%l)"; }
    string emitC() override { फर्काऊ "asinh(%li)"; }
};
class AstAtanD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstAtanD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$atan(%l)"; }
    string emitC() override { फर्काऊ "atan(%li)"; }
};
class AstAtanhD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstAtanhD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$atanh(%l)"; }
    string emitC() override { फर्काऊ "atanh(%li)"; }
};
class AstCeilD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstCeilD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::उच्चमान(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$ceil(%l)"; }
    string emitC() override { फर्काऊ "ceil(%li)"; }
};
class AstCosD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstCosD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$cos(%l)"; }
    string emitC() override { फर्काऊ "cos(%li)"; }
};
class AstCoshD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstCoshD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$cosh(%l)"; }
    string emitC() override { फर्काऊ "cosh(%li)"; }
};
class AstExpD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstExpD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$exp(%l)"; }
    string emitC() override { फर्काऊ "exp(%li)"; }
};
class AstFloorD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstFloorD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::न्यूनमान(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$floor(%l)"; }
    string emitC() override { फर्काऊ "floor(%li)"; }
};
class AstLog10D final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstLog10D(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$log10(%l)"; }
    string emitC() override { फर्काऊ "log10(%li)"; }
};
class AstLogD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstLogD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$ln(%l)"; }
    string emitC() override { फर्काऊ "log(%li)"; }
};
class AstSinD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstSinD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$sin(%l)"; }
    string emitC() override { फर्काऊ "sin(%li)"; }
};
class AstSinhD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstSinhD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$sinh(%l)"; }
    string emitC() override { फर्काऊ "sinh(%li)"; }
};
class AstSqrtD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstSqrtD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::वर्ग_मूल(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$sqrt(%l)"; }
    string emitC() override { फर्काऊ "sqrt(%li)"; }
};
class AstTanD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstTanD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$tan(%l)"; }
    string emitC() override { फर्काऊ "tan(%li)"; }
};
class AstTanhD final : सार्वजनिक Ast_गाँठो_System_Uniop_D {
सार्वजनिक:
    AstTanhD(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    शून्य सङ्ख्या_Operate(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { फर्काऊ "%f$tanh(%l)"; }
    string emitC() override { फर्काऊ "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : सार्वजनिक Ast_गाँठो_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
सार्वजनिक:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    अन्तःस्थ AstVarRef(File_पङ्क्ति* fl, AstVar* varp, अचर VAccess& access);
    अन्तःस्थ AstVarRef(File_पङ्क्ति* fl, Ast_गाँठो_Module* classOrPackagep, AstVar* varp,
                     अचर VAccess& access);
    // This form only allowed post-link (see above)
    अन्तःस्थ AstVarRef(File_पङ्क्ति* fl, AstVarScope* varscp, अचर VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() अचर { फर्काऊ m_name; }
    अन्तःस्थ string name() अचर override;  // * = Var name
    शून्य name(अचर string& name) override { m_name = name; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    अचर वर्ण* broken() अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    अन्तःस्थ bool same_गाँठो(अचर AstVarRef* samep) अचर;
    अन्तःस्थ bool sameNoLvalue(अचर AstVarRef* samep) अचर;
    पूर्णाङ्क instr_गन्ती() अचर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
};
class AstVarXRef final : सार्वजनिक Ast_गाँठो_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
सार्वजनिक:
    AstVarXRef(File_पङ्क्ति* fl, अचर string& name, अचर string& dotted, अचर VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    अन्तःस्थ AstVarXRef(File_पङ्क्ति* fl, AstVar* varp, अचर string& dotted, अचर VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    शून्य name(अचर std::string& name) override { m_name = name; }  // * = Var name
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string dotted() अचर { फर्काऊ m_dotted; }
    शून्य dotted(अचर string& dotted) { m_dotted = dotted; }
    string inlinedDots() अचर { फर्काऊ m_inlinedDots; }
    शून्य inlinedDots(अचर string& flag) { m_inlinedDots = flag; }
    bool containsGenBlock() अचर { फर्काऊ m_containsGenBlock; }
    शून्य containsGenBlock(अचर bool flag) { m_containsGenBlock = flag; }
    bool readOnlyModport() अचर { फर्काऊ m_readOnlyModport; }
    शून्य readOnlyModport(अचर bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() अचर override { फर्काऊ true; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ widthInstrs(); }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        फर्काऊ (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#endif  // Guard
