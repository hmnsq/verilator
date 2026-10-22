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

#अगर_अघोषित VERILATOR_V3ASTNODEEXPR_H_
#घोषणा VERILATOR_V3ASTNODEEXPR_H_

#अगर_अघोषित VERILATOR_V3AST_H_
#त्रुटि "Use V3Ast.h as the include"
#समावेश "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#घोषणा VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#पूर्ण_अगर

// === Abstract base node types (AstNode*) =====================================

class Ast_गाँठ_व्यंजक VL_NOT_FINAL : खुला Ast_गाँठ {
    // An expression tree node
रक्षित:
    Ast_गाँठ_व्यंजक(VNType t, File_पंक्ति* fl)
        : Ast_गाँठ{t, fl} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_व्यंजक;
    // METHODS
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    // Every expression must have a data type after V3Width
    bool hasDType() स्थिर override final VL_MT_SAFE { वापस true; }
    भव string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    भव string emitC() = 0;
    भव string emitSMT() स्थिर { वापस ""; };
    भव string emitSimpleOperator() { वापस ""; }  // "" means not ok to use
    भव bool emit_Check_Max_शब्दगण() { वापस false; }  // Check VL_MULS_MAX_WORDS
    भव bool cleanOut() स्थिर = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    भव bool isMultiCycleSva() स्थिर { वापस false; }
    स्थिर Ast_गाँठ_व्यंजक* getVAccessTargetRecurse() स्थिर;
    Ast_गाँठ_व्यंजक* getVAccessTargetRecurse() {
        वापस const_cast<Ast_गाँठ_व्यंजक*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<स्थिर Ast_गाँठ_व्यंजक*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() स्थिर;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() स्थिर { वापस getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_वृक्ष() स्थिर;
    // Return base var (or const) nodep dereferences
    Ast_गाँठ* baseFromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    अंतरभूत Ast_Stmt_व्यंजक* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    भव स्थिर अक्षर* widthMismatch() स्थिर VL_MT_STABLE { वापस nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string patternString(uint32_t depth = 0) स्थिर;
    bool isFourstateOrNoDtype() स्थिर {
        अगर (!dtypep()) वापस true;
        वापस dtypep()->skipRefp()->isFourstate();
    }
    अटल bool isFourstateOrNoDtype(स्थिर Ast_गाँठ_व्यंजक* स्थिर expr1p) {
        वापस expr1p->isFourstateOrNoDtype();
    }
    अटल bool isFourstateOrNoDtype(स्थिर Ast_गाँठ_व्यंजक* स्थिर expr1p,
                                     स्थिर Ast_गाँठ_व्यंजक* स्थिर expr2p) {
        वापस expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    अटल bool isFourstateOrNoDtype(स्थिर Ast_गाँठ_व्यंजक* स्थिर expr1p,
                                     स्थिर Ast_गाँठ_व्यंजक* स्थिर expr2p,
                                     स्थिर Ast_गाँठ_व्यंजक* स्थिर expr3p) {
        वापस isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    अटल bool isFourstateOrNoDtype(स्थिर Ast_गाँठ_व्यंजक* स्थिर expr1p,
                                     स्थिर Ast_गाँठ_व्यंजक* स्थिर expr2p,
                                     स्थिर Ast_गाँठ_व्यंजक* स्थिर expr3p,
                                     स्थिर Ast_गाँठ_व्यंजक* स्थिर expr4p) {
        वापस isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_गाँठ_Biop VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठ_Biop(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : Ast_गाँठ_व्यंजक{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    भव व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) = 0;
    भव bool cleanLhs() स्थिर = 0;  // True if LHS must have extra upper bits zero
    भव bool cleanRhs() स्थिर = 0;  // True if RHS must have extra upper bits zero
    भव bool आकार_Matters_Lhs() स्थिर = 0;  // True if output result depends on lhs size
    भव bool आकार_Matters_Rhs() स्थिर = 0;  // True if output result depends on rhs size
    भव bool doubleFlavor() स्थिर { वापस false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    भव bool signedFlavor() स्थिर { वापस false; }
    भव bool stringFlavor() स्थिर { वापस false; }  // N flavor of nodes with both flavors?
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    bool isPure() override;
    स्थिर अक्षर* broken() स्थिर override;

निजी:
    bool getPurityRecurse() स्थिर { वापस lhsp()->isPure() && rhsp()->isPure(); }
};
class Ast_गाँठ_Bi_Com VL_NOT_FINAL : खुला Ast_गाँठ_Biop {
    // Binary expr with commutative properties
रक्षित:
    Ast_गाँठ_Bi_Com(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhs, Ast_गाँठ_व्यंजक* rhs)
        : Ast_गाँठ_Biop{t, fl, lhs, rhs} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Bi_Com;
};
class Ast_गाँठ_Bi_Com_Asv VL_NOT_FINAL : खुला Ast_गाँठ_Bi_Com {
    // Binary expr with commutative & associative properties
रक्षित:
    Ast_गाँठ_Bi_Com_Asv(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhs, Ast_गाँठ_व्यंजक* rhs)
        : Ast_गाँठ_Bi_Com{t, fl, lhs, rhs} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Bi_Com_Asv;
};
class Ast_गाँठ_Dist_Biop VL_NOT_FINAL : खुला Ast_गाँठ_Biop {
खुला:
    Ast_गाँठ_Dist_Biop(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : Ast_गाँठ_Biop{t, fl, lhsp, rhsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_गाँठ_Dist_Biop;
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL_TRIG; }
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
};
class Ast_गाँठ_Sel VL_NOT_FINAL : खुला Ast_गाँठ_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
रक्षित:
    Ast_गाँठ_Sel(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* bitp)
        : Ast_गाँठ_Biop{t, fl, fromp, bitp} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Sel;
    पूर्णांक bitConst() स्थिर;
};
class Ast_गाँठ_Stream VL_NOT_FINAL : खुला Ast_गाँठ_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
रक्षित:
    Ast_गाँठ_Stream(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : Ast_गाँठ_Biop{t, fl, lhsp, rhsp} {
        अगर (lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(lhsp->width(), lhsp->widthMin(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Stream;
};
class Ast_गाँठ_System_Biop_D VL_NOT_FINAL : खुला Ast_गाँठ_Biop {
खुला:
    Ast_गाँठ_System_Biop_D(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : Ast_गाँठ_Biop{t, fl, lhsp, rhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_गाँठ_System_Biop_D;
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL_TRIG; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class Ast_गाँठ_CCall VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

रक्षित:
    Ast_गाँठ_CCall(VNType t, File_पंक्ति* fl, AstCFunc* funcp, Ast_गाँठ_व्यंजक* argsp = nullptr)
        : Ast_गाँठ_व्यंजक{t, fl}
        , m_funcp{funcp} {
        addArgsp(argsp);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_CCall;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_CALL; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर Ast_गाँठ_CCall* स्थिर asamep = VN_DBG_AS(samep, गाँठ_CCall);
        वापस (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override;
    bool isOutputter() override { वापस !isPure(); }
    AstCFunc* funcp() स्थिर { वापस m_funcp; }
    व्योम funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() स्थिर { वापस m_argTypes; }
    व्योम argTypes(स्थिर string& str) { m_argTypes = str; }
    bool isUnlikely() स्थिर override { वापस m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर final override { वापस true; }
    bool superReference() स्थिर { वापस m_superReference; }
    व्योम superReference(bool flag) { m_superReference = flag; }
};
class Ast_गाँठ_Cons VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // Construct a value of an aggregate type
    मित्र class Ast_गाँठ_व्यंजक;  // For m_knownConstantTree
    mutable bool m_known_Constant_वृक्ष = false;  // Known to be 'this->isConstantTree() == true'

रक्षित:
    Ast_गाँठ_Cons(VNType t, File_पंक्ति* fl)
        : Ast_गाँठ_व्यंजक{t, fl} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Cons;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर final override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर final override { वापस widthInstrs(); }
};
class Ast_गाँठ_FTask_Ref VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_गाँठ_FTask_Ref;  // Gen pointers before other members for performance
निजी:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठ_FTask_Ref(VNType t, File_पंक्ति* fl, स्थिर string& name, AstArg* argsp)
        : Ast_गाँठ_व्यंजक{t, fl}
        , m_name{name} {
        addArgsp(argsp);
    }

खुला:
    स्थिर अक्षर* broken() स्थिर override;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    bool isGateOptimizable() स्थिर override;
    string dotted() स्थिर { वापस m_dotted; }  // * = Scope name or ""
    string inlinedDots() स्थिर { वापस m_inlinedDots; }
    व्योम inlinedDots(स्थिर string& flag) { m_inlinedDots = flag; }
    Ast_गाँठ_FTask* taskp() स्थिर { वापस m_taskp; }  // [After Link] Pointer to variable
    व्योम taskp(Ast_गाँठ_FTask* taskp) { m_taskp = taskp; }
    व्योम name(स्थिर string& name) override { m_name = name; }
    व्योम dotted(स्थिर string& name) { m_dotted = name; }
    Ast_गाँठ_Module* classOrPackagep() स्थिर { वापस m_classOrPackagep; }
    व्योम classOrPackagep(Ast_गाँठ_Module* nodep) { m_classOrPackagep = nodep; }
    bool containsGenBlock() स्थिर { वापस m_containsGenBlock; }
    व्योम containsGenBlock(स्थिर bool flag) { m_containsGenBlock = flag; }
    bool pli() स्थिर { वापस m_pli; }
    व्योम pli(bool flag) { m_pli = flag; }
    bool superReference() स्थिर { वापस m_superReference; }
    व्योम superReference(bool flag) { m_superReference = flag; }
    bool isPure() override;
    bool isUnlikely() स्थिर override { वापस m_taskp && m_taskp->isUnlikely(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर Ast_गाँठ_FTask_Ref* स्थिर asamep = VN_DBG_AS(samep, गाँठ_FTask_Ref);
        वापस taskp() == asamep->taskp()  //
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
    bool cleanOut() स्थिर final override { V3ERROR_NA_RETURN(true); }

निजी:
    bool getPurityRecurse() स्थिर;
};
class Ast_गाँठ_Pre_Sel VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठ_Pre_Sel(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ_व्यंजक* thsp)
        : Ast_गाँठ_व्यंजक{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Pre_Sel;
    // METHODS
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर final override { V3ERROR_NA_RETURN(true); }
    bool isPure() override;
    स्थिर अक्षर* broken() स्थिर override;

निजी:
    bool getPurityRecurse() स्थिर {
        वापस fromp()->isPure() && rhsp()->isPure() && (!thsp() || thsp()->isPure());
    }
};
class Ast_गाँठ_Quadop VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठ_Quadop(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ_व्यंजक* thsp,
                  Ast_गाँठ_व्यंजक* fhsp)
        : Ast_गाँठ_व्यंजक{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    भव व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs,
                               स्थिर V3_संख्या& ths, स्थिर V3_संख्या& fhs)
        = 0;
    भव bool cleanLhs() स्थिर = 0;  // True if LHS must have extra upper bits zero
    भव bool cleanRhs() स्थिर = 0;  // True if RHS must have extra upper bits zero
    भव bool cleanThs() स्थिर = 0;  // True if THS must have extra upper bits zero
    भव bool cleanFhs() स्थिर = 0;  // True if THS must have extra upper bits zero
    भव bool आकार_Matters_Lhs() स्थिर = 0;  // True if output result depends on lhs size
    भव bool आकार_Matters_Rhs() स्थिर = 0;  // True if output result depends on rhs size
    भव bool आकार_Matters_Ths() स्थिर = 0;  // True if output result depends on ths size
    भव bool आकार_Matters_Fhs() स्थिर = 0;  // True if output result depends on ths size
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    bool isPure() override;
    स्थिर अक्षर* broken() स्थिर override;

निजी:
    bool getPurityRecurse() स्थिर {
        वापस lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure() && fhsp()->isPure();
    }
};
class Ast_गाँठ_Termop VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // Terminal operator -- an operator with no "inputs"
रक्षित:
    Ast_गाँठ_Termop(VNType t, File_पंक्ति* fl)
        : Ast_गाँठ_व्यंजक{t, fl} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Termop;
    bool isSystemFunc() स्थिर override { वापस true; }
};
class Ast_गाँठ_Triop VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठ_Triop(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ_व्यंजक* thsp)
        : Ast_गाँठ_व्यंजक{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Triop;
    // METHODS
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    // Set out to evaluation of a AstConst'ed
    भव व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs,
                               स्थिर V3_संख्या& ths)
        = 0;
    भव bool cleanLhs() स्थिर = 0;  // True if LHS must have extra upper bits zero
    भव bool cleanRhs() स्थिर = 0;  // True if RHS must have extra upper bits zero
    भव bool cleanThs() स्थिर = 0;  // True if THS must have extra upper bits zero
    भव bool आकार_Matters_Lhs() स्थिर = 0;  // True if output result depends on lhs size
    भव bool आकार_Matters_Rhs() स्थिर = 0;  // True if output result depends on rhs size
    भव bool आकार_Matters_Ths() स्थिर = 0;  // True if output result depends on ths size
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    bool isPure() override;
    स्थिर अक्षर* broken() स्थिर override;

निजी:
    bool getPurityRecurse() स्थिर {
        वापस lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure();
    }
};
class Ast_गाँठ_Dist_Triop VL_NOT_FINAL : खुला Ast_गाँठ_Triop {
खुला:
    Ast_गाँठ_Dist_Triop(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp,
                     Ast_गाँठ_व्यंजक* thsp)
        : Ast_गाँठ_Triop{t, fl, lhsp, rhsp, thsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_गाँठ_Dist_Triop;
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool cleanThs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool आकार_Matters_Ths() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL_TRIG; }
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs,
                       स्थिर V3_संख्या& ths) override {
        V3ERROR_NA;
    }
};
class Ast_गाँठ_Uniop VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_गाँठ_Uniop(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : Ast_गाँठ_व्यंजक{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Uniop;
    // METHODS
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    // Set out to evaluation of a AstConst'ed lhs
    भव व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) = 0;
    भव bool cleanLhs() स्थिर = 0;
    भव bool आकार_Matters_Lhs() स्थिर = 0;  // True if output result depends on lhs size
    भव bool doubleFlavor() स्थिर { वापस false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    भव bool signedFlavor() स्थिर { वापस false; }
    भव bool stringFlavor() स्थिर { वापस false; }  // N flavor of nodes with both flavors?
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    bool isPure() override;
    स्थिर अक्षर* broken() स्थिर override;
};
class Ast_गाँठ_System_Uniop_D VL_NOT_FINAL : खुला Ast_गाँठ_Uniop {
खुला:
    Ast_गाँठ_System_Uniop_D(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : Ast_गाँठ_Uniop{t, fl, lhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_गाँठ_System_Uniop_D;
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL_TRIG; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class Ast_गाँठ_Var_Ref VL_NOT_FINAL : खुला Ast_गाँठ_व्यंजक {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_गाँठ_Var_Ref;  // Gen pointers before other members for performance
निजी:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
रक्षित:
    Ast_गाँठ_Var_Ref(VNType t, File_पंक्ति* fl, AstVar* varp, स्थिर VAccess& access)
        : Ast_गाँठ_व्यंजक{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

खुला:
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    VAccess access() स्थिर { वापस m_access; }
    व्योम access(स्थिर VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() स्थिर VL_MT_STABLE { वापस m_varp; }  // [After Link] Pointer to variable
    व्योम varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_गाँठ*>(varp));
    }
    AstVarScope* varScopep() स्थिर { वापस m_varScopep; }
    व्योम varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    स्थिर VSelfPointerText& selfPointer() स्थिर { वापस m_selfPointer; }
    व्योम selfPointer(स्थिर VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) स्थिर {
        वापस selfPointer().protect(useSelfForThis, protect());
    }
    Ast_गाँठ_Module* classOrPackagep() स्थिर { वापस m_classOrPackagep; }
    व्योम classOrPackagep(Ast_गाँठ_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : खुला Ast_गाँठ {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
खुला:
    AstArg(File_पंक्ति* fl, स्थिर std::string& name, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर std::string& name) override { m_name = name; }
    bool empty_Connect_No_अग्र() स्थिर { वापस !exprp() && name() == "" && !nextp(); }
};
class AstWith final : खुला Ast_गाँठ {
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
निजी:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::set<std::string> m_restrictedNames;

खुला:
    AstWith(File_पंक्ति* fl, AstLambdaArgRef* सूचकांक_Arg_Refp, AstLambdaArgRef* मूल्य_Arg_Refp,
            Ast_गाँठ* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->सूचकांक_Arg_Refp(सूचकांक_Arg_Refp);
        this->मूल्य_Arg_Refp(मूल्य_Arg_Refp);
        addExprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!सूचकांक_Arg_Refp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!मूल्य_Arg_Refp());  // varp needed to know lambda's arg dtype
        वापस nullptr;
    }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    व्योम restricted(bool flag) { m_restricted = flag; }
    bool restricted() स्थिर { वापस m_restricted; }
    bool validated() स्थिर { वापस m_validated; }
    व्योम validated(bool flag) { m_validated = flag; }
    व्योम addRestrictedName(स्थिर std::string& name) { m_restrictedNames.जोड़ो(name); }
    स्थिर std::set<std::string>& restrictedNames() स्थिर { वापस m_restrictedNames; }
    // True if 'name' binds into the randomize() target class.
    bool nameResolvesToTarget(स्थिर std::string& name) स्थिर {
        वापस !m_restricted || m_restrictedNames.गिनती(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : खुला Ast_गाँठ_व्यंजक {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbortKind m_kind{VAbortKind::ACCEPT_ON};

खुला:
    AstAbortOn(File_पंक्ति* fl, VAbortKind kind, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ_व्यंजक* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_kind{kind} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    VAbortKind kind() स्थिर { वापस m_kind; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstAddrOfCFunc final : खुला Ast_गाँठ_व्यंजक {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

खुला:
    AstAddrOfCFunc(File_पंक्ति* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_AstAddrOfCFunc;
    व्योम dump(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    AstCFunc* funcp() स्थिर { वापस m_funcp; }
};
class AstAttrOf final : खुला Ast_गाँठ_व्यंजक {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
खुला:
    AstAttrOf(File_पंक्ति* fl, VAttrType attrtype, Ast_गाँठ* fromp = nullptr,
              Ast_गाँठ_व्यंजक* dimp = nullptr)
        : ASTGEN_SUPER_AttrOf(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_AstAttrOf;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    व्योम name(स्थिर string& name) override { m_name = name; }
    VAttrType attrType() स्थिर { वापस m_attrType; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : खुला Ast_गाँठ_व्यंजक {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    स्थिर bool m_pure;  // Pure optimizable

    व्योम init(स्थिर string& text, पूर्णांक setwidth) {
        अगर (!text.empty()) add(text);
        अगर (setwidth) {
            dtypeSetBitSized(setwidth, VSigning::UNSIGNED);
        } अन्यथा {
            dtypeSetVoid();  // Caller to override if necessary
        }
    }

खुला:
    class Pure {};
    explicit AstCExpr(File_पंक्ति* fl, स्थिर string& text = "", पूर्णांक setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{false} {
        init(text, setwidth);
    }
    AstCExpr(File_पंक्ति* fl, Pure, स्थिर string& text = "", पूर्णांक setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    bool cleanOut() स्थिर override { वापस true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस m_pure; }
    bool isOutputter() override { वापस true; }
    bool isPredictOptimizable() स्थिर override { वापस m_pure; }
    bool isPure() override { वापस m_pure; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    // Add some text, or a node to this expression
    व्योम add(स्थिर std::string& text) { addNodesp(new AstText{fileline(), text}); }
    व्योम add(Ast_गाँठ* nodep) { addNodesp(nodep); }
};
class AstCExprUser final : खुला Ast_गाँठ_व्यंजक {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    स्थिर bool m_pure;  // Whether the function is pure
खुला:
    class Pure {};
    explicit AstCExprUser(File_पंक्ति* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{false} {}
    AstCExprUser(File_पंक्ति* fl, Pure)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    bool cleanOut() स्थिर override { वापस false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस m_pure; }
    bool isOutputter() override { वापस true; }
    bool isPredictOptimizable() स्थिर override { वापस m_pure; }
    bool isPure() override { वापस m_pure; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    // Add some text, or a node to this expression
    व्योम add(स्थिर std::string& text) { addNodesp(new AstText{fileline(), text}); }
    व्योम add(Ast_गाँठ* nodep) { addNodesp(nodep); }
};
class AstCFuncHard final : खुला Ast_गाँठ_व्यंजक {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
खुला:
    AstCFuncHard(File_पंक्ति* fl, VCFunction function, Ast_गाँठ_व्यंजक* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस function().ascii(); }
    bool isPure() override {
        अगर (!m_purity.isCached()) m_purity.set(getPurity());
        वापस m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    VCFunction function() स्थिर VL_MT_STABLE { वापस m_function; }

निजी:
    bool getPurity();
};
class AstCMethodHard final : खुला Ast_गाँठ_व्यंजक {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_method;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
खुला:
    AstCMethodHard(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, VCMethod method, Ast_गाँठ_व्यंजक* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_method{method} {
        this->fromp(fromp);
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस method().ascii(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstCMethodHard* स्थिर asamep = VN_DBG_AS(samep, CMethodHard);
        वापस (m_method == asamep->m_method);
    }
    bool isPure() override {
        अगर (!m_purity.isCached()) m_purity.set(getPurity());
        वापस m_purity.get();
    }
    पूर्णांक instr_गिनती() स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस m_method.isClean(); }
    bool usePtr() स्थिर { वापस m_usePtr; }
    व्योम usePtr(bool flag) { m_usePtr = flag; }
    VCMethod method() स्थिर VL_MT_STABLE { वापस m_method; }
    व्योम method(VCMethod मूल्य) { m_method = मूल्य; }

निजी:
    bool getPurity();
};
class AstCReset final : खुला Ast_गाँठ_व्यंजक {
    // Reset variable at startup
    स्थिर bool m_constructing;  // Previously cleared by constructor
खुला:
    AstCReset(File_पंक्ति* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isPure() override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!VN_IS(backp(), गाँठ_Assign));  // V3Emit* assumption
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() स्थिर { वापस m_constructing; }
};
class AstCast final : खुला Ast_गाँठ_व्यंजक {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
खुला:
    AstCast(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, VFlagChildDType, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { वापस "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर VL_MT_STABLE { वापस dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : खुला Ast_गाँठ_व्यंजक {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
खुला:
    AstCastParse(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { वापस "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cast_आकार final : खुला Ast_गाँठ_व्यंजक {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
खुला:
    Ast_Cast_आकार(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_Cast_आकार(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_Ast_Cast_आकार;
    string emitVerilog() override { वापस "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : खुला Ast_गाँठ_व्यंजक {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
खुला:
    AstCellArrayRef(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_व्यंजक* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        addSelp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : खुला Ast_गाँठ_व्यंजक {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
खुला:
    AstCellRef(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* cellp, Ast_गाँठ* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
खुला:
    AstClassOrPackageRef(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_गाँठ_Module* classOrPackageSkipp(स्थिर bool doRefs = true) स्थिर;
    Ast_गाँठ* classOrPackageNodep() स्थिर { वापस m_classOrPackageNodep; }
    व्योम classOrPackageNodep(Ast_गाँठ* nodep) { m_classOrPackageNodep = nodep; }
    व्योम classOrPackagep(Ast_गाँठ_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_गाँठ*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : खुला Ast_गाँठ_व्यंजक {
    // A constant
    V3_संख्या m_अंक;  // Constant value
    व्योम init_With_संख्या() {
        अगर (m_अंक.isDouble()) {
            dtypeSetDouble();
        } अन्यथा अगर (m_अंक.isString()) {
            dtypeSetString();
        } अन्यथा {
            dtypeSetBitOrLogicUnsized(m_अंक.width(), (m_अंक.sized() ? 0 : m_अंक.widthToFit()),
                                      VSigning::fromBool(m_अंक.isSigned()), m_अंक.isAnyXZ());
        }
        m_अंक.nodep(this);
    }

खुला:
    ~AstConst() override;
    AstConst(File_पंक्ति* fl, स्थिर V3_संख्या& अंक)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{अंक} {
        init_With_संख्या();
    }
    class Widthed_मूल्य {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, Widthed_मूल्य, पूर्णांक width, uint32_t मूल्य)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, width, मूल्य) {  // Need () constructor
        init_With_संख्या();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_पंक्ति* fl, DTyped, स्थिर Ast_गाँठ_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{this, nodedtypep} {
        init_With_संख्या();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, StringToParse, स्थिर अक्षर* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{this, V3_संख्या::Verilog_संख्या_Literal{}, sourcep} {
        init_With_संख्या();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, VerilogStringLiteral, स्थिर string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{this, V3_संख्या::VerilogStringLiteral{}, str} {
        init_With_संख्या();
    }
    AstConst(File_पंक्ति* fl, uint32_t अंक)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, 32, अंक) {  // Need () constructor
        dtypeSetBitUnsized(m_अंक.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, Unsized32, uint32_t अंक)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, 32, अंक) {  // Need () constructor
        m_अंक.width(32, false);
        dtypeSetBitUnsized(32, m_अंक.widthToFit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, Signed32, int32_t अंक)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, 32, अंक) {  // Need () constructor
        m_अंक.width(32, true);
        dtypeSetBitUnsized(32, m_अंक.widthToFit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, Unsized64, uint64_t अंक)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, 64, 0) {  // Need () constructor
        m_अंक.setQuad(अंक);
        dtypeSetBitSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, SizedEData, uint64_t अंक)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_अंक.setQuad(अंक);
        dtypeSetBitSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, RealDouble, द्विगुन अंक)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, 64) {  // Need () constructor
        m_अंक.setDouble(अंक);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_पंक्ति* fl, String, स्थिर string& अंक)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{this, V3_संख्या::String{}, अंक} {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(File_पंक्ति* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_पंक्ति* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
        File_पंक्ति* स्थिर newfl = new File_पंक्ति{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_पंक्ति* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक(this, 1, on) {  // Need () constructor
        dtypeSetBit();
    }
    class All0 {};
    AstConst(File_पंक्ति* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{this, V3_संख्या::Verilog_संख्या_Literal{}, "'0"} {
        init_With_संख्या();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_पंक्ति* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{this, V3_संख्या::Verilog_संख्या_Literal{}, "'1"} {
        init_With_संख्या();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_पंक्ति* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{this, V3_संख्या::Null{}} {
        dtypep(findNullDType());
        init_With_संख्या();
    }
    class OneStep {};
    AstConst(File_पंक्ति* fl, OneStep)
        : ASTGEN_SUPER_Const(fl)
        , m_अंक{this, V3_संख्या::OneStep{}} {
        dtypeSetBitSized(64, VSigning::UNSIGNED);
        init_With_संख्या();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() स्थिर override VL_MT_STABLE { वापस अंक().ascii(); }  // * = Value
    स्थिर V3_संख्या& अंक() स्थिर VL_MT_SAFE { वापस m_अंक; }  // * = Value
    V3_संख्या& अंक() { वापस m_अंक; }  // * = Value
    string origParamName() स्थिर;
    व्योम origParamName(स्थिर string& name);
    uint32_t toUInt() स्थिर { वापस अंक().toUInt(); }
    int32_t toSInt() स्थिर VL_MT_SAFE { वापस अंक().toSInt(); }
    uint64_t toUQuad() स्थिर { वापस अंक().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstConst* स्थिर sp = VN_DBG_AS(samep, Const);
        वापस अंक().isCaseEq(sp->अंक());
    }
    व्योम cloneRelink() override;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(m_अंक.nodep() && m_अंक.nodep() != this);
        वापस nullptr;
    }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool isEqAllOnes() स्थिर { वापस अंक().isEqAllOnes(width()); }
    bool isEqAllOnesV() स्थिर { वापस अंक().isEqAllOnes(widthMinV()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_मूल्य_Type(स्थिर AstConst* samep) स्थिर;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    अटल AstConst* parseParamLiteral(File_पंक्ति* fl, स्थिर string& literal);
};
class AstConstraintRef final : खुला Ast_गाँठ_व्यंजक {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

खुला:
    AstConstraintRef(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() स्थिर override VL_MT_STABLE;
    AstConstraint* constrp() स्थिर VL_MT_STABLE { वापस m_constrp; }
    व्योम constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_गाँठ_Module* classOrPackagep() स्थिर VL_MT_STABLE { वापस m_classOrPackagep; }
    // TODO: this setter is unused
    व्योम classOrPackagep(Ast_गाँठ_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर final override { V3ERROR_NA_RETURN(true); }
};
class AstCvtArrayToArray final : खुला Ast_गाँठ_व्यंजक {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
निजी:
    स्थिर bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    स्थिर पूर्णांक m_block_आकार;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    स्थिर पूर्णांक m_dstElementBits;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    स्थिर पूर्णांक m_srcElementBits;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

खुला:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToArray(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_DType* dtp, bool reverse,
                       पूर्णांक block_आकार, पूर्णांक dstElementBits, पूर्णांक srcElementBits)
        : ASTGEN_SUPER_CvtArrayToArray(fl)
        , m_reverse{reverse}
        , m_block_आकार{block_आकार}
        , m_dstElementBits{dstElementBits}
        , m_srcElementBits{srcElementBits} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToArray;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool reverse() स्थिर { वापस m_reverse; }
    पूर्णांक block_आकार() स्थिर { वापस m_block_आकार; }
    पूर्णांक dstElementBits() स्थिर { वापस m_dstElementBits; }
    पूर्णांक srcElementBits() स्थिर { वापस m_srcElementBits; }
};
class AstCvtArrayToPacked final : खुला Ast_गाँठ_व्यंजक {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
खुला:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToPacked(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_CvtArrayToPacked(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToPacked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
};
class AstCvtPackedToArray final : खुला Ast_गाँठ_व्यंजक {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
खुला:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtPackedToArray(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_CvtPackedToArray(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtPackedToArray;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
};
class AstCvtUnpackToQueue final : खुला Ast_गाँठ_व्यंजक {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
खुला:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtUnpackToQueue(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToQueue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToQueue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { वापस "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool cleanOut() स्थिर override { वापस true; }
};
class AstCvtUnpackToUnpack final : खुला Ast_गाँठ_व्यंजक {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
खुला:
    AstCvtUnpackToUnpack(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToUnpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToUnpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
};
class AstDist final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
खुला:
    AstDist(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { वापस "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }  // NA
};
class AstDistItem final : खुला Ast_गाँठ_व्यंजक {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_isWhole = false;  // True for weight ':/', false for ':='
खुला:
    AstDistItem(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* rangep, Ast_गाँठ_व्यंजक* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { वापस "%l "s + (m_isWhole ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }  // NA
    व्योम isWhole(bool flag) { m_isWhole = flag; }
    bool isWhole() स्थिर { वापस m_isWhole; }
};
class AstDot final : खुला Ast_गाँठ_व्यंजक {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    स्थिर bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
खुला:
    AstDot(File_पंक्ति* fl, bool colon, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    अटल Ast_गाँठ_व्यंजक* newIfPkg(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* packageOrClassp, Ast_गाँठ_व्यंजक* rhsp) {
        अगर (!packageOrClassp) वापस rhsp;
        वापस new AstDot{fl, true, packageOrClassp, rhsp};
    }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool colon() स्थिर { वापस m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstEmptyQueue final : खुला Ast_गाँठ_व्यंजक {
खुला:
    explicit AstEmptyQueue(File_पंक्ति* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_AstEmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { वापस "{}"; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool cleanOut() स्थिर override { वापस true; }
};
class AstEnumItemRef final : खुला Ast_गाँठ_व्यंजक {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_containsGenBlock = false;  // Contains gen block reference
खुला:
    AstEnumItemRef(File_पंक्ति* fl, AstEnumItem* itemp, Ast_गाँठ_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    स्थिर अक्षर* broken() स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस itemp() ? itemp()->name() : m_name; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstEnumItemRef* स्थिर sp = VN_DBG_AS(samep, EnumItemRef);
        वापस itemp() == sp->itemp() && dotted() == sp->dotted()
               && containsGenBlock() == sp->containsGenBlock();
    }
    AstEnumItem* itemp() स्थिर VL_MT_STABLE { वापस m_itemp; }
    व्योम itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() स्थिर { वापस m_dotted; }
    व्योम dotted(स्थिर string& dotted) { m_dotted = dotted; }
    bool containsGenBlock() स्थिर { वापस m_containsGenBlock; }
    व्योम containsGenBlock(स्थिर bool flag) { m_containsGenBlock = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    Ast_गाँठ_Module* classOrPackagep() स्थिर { वापस m_classOrPackagep; }
    व्योम classOrPackagep(Ast_गाँठ_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_व्यंजक_Stmt final : खुला Ast_गाँठ_व्यंजक {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
निजी:
    bool m_has_परिणाम = true;  // Returns result via resultp()

खुला:
    Ast_व्यंजक_Stmt(File_पंक्ति* fl, Ast_गाँठ* stmtsp, Ast_गाँठ_व्यंजक* resultp)
        : ASTGEN_SUPER_व्यंजक_Stmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_व्यंजक_Stmt;
    // METHODS
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool isPure() override {
        अगर (Ast_गाँठ::afterCommentp(stmtsp())) वापस false;
        वापस resultp()->isPure();
    }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    bool has_परिणाम() स्थिर { वापस m_has_परिणाम; }
    व्योम has_परिणाम(bool flag) { m_has_परिणाम = flag; }
};
class AstFError final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
खुला:
    AstFError(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* filep, Ast_गाँठ_व्यंजक* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { वापस "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 64; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstFOpen final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
खुला:
    AstFOpen(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* filenamep, Ast_गाँठ_व्यंजक* modep)
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
    bool cleanOut() स्थिर override { वापस true; }
    string verilogKwd() स्थिर override { वापस "$fopen"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstFOpenMcd final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op2 := filenamep : AstNodeExpr
खुला:
    AstFOpenMcd(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    string verilogKwd() स्थिर override { वापस "$fopen"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstFRead final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
खुला:
    AstFRead(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* memp, Ast_गाँठ_व्यंजक* filep, Ast_गाँठ_व्यंजक* startp,
             Ast_गाँठ_व्यंजक* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() स्थिर override { वापस "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: makes output
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstFRewind final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := filep : Optional[AstNodeExpr]
खुला:
    AstFRewind(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() स्थिर override { वापस "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstFScanF final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

खुला:
    AstFScanF(File_पंक्ति* fl, स्थिर string& text, Ast_गाँठ_व्यंजक* filep, Ast_गाँठ_व्यंजक* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() स्थिर override VL_MT_STABLE { वापस m_text; }
    string verilogKwd() स्थिर override { वापस "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: makes output
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() स्थिर { वापस m_text; }  // * = Text to display
    व्योम text(स्थिर string& text) { m_text = text; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstFSeek final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
खुला:
    AstFSeek(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* filep, Ast_गाँठ_व्यंजक* offset, Ast_गाँठ_व्यंजक* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() स्थिर override { वापस "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: makes output
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstFTell final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
खुला:
    AstFTell(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() स्थिर override { वापस "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstFalling final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
खुला:
    AstFalling(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { वापस "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstFell final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
खुला:
    AstFell(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_Sen_वृक्ष* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { वापस "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstFuture final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
खुला:
    AstFuture(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_Sen_वृक्ष* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { वापस "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstGatePin final : खुला Ast_गाँठ_व्यंजक {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
खुला:
    AstGatePin(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { वापस "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
};
class AstGetInitialRandomSeed final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $get_initial_random_seed()
खुला:
    explicit AstGetInitialRandomSeed(File_पंक्ति* fl)
        : ASTGEN_SUPER_GetInitialRandomSeed(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstGetInitialRandomSeed;
    string emitVerilog() override { वापस "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस true; }
    bool isPure() override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstImplication final : खुला Ast_गाँठ_व्यंजक {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

निजी:
    स्थिर bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    स्थिर bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

खुला:
    AstImplication(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool isMultiCycleSva() स्थिर override { वापस m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() स्थिर { वापस m_isOverlapped; }
    bool isFollowedBy() स्थिर { वापस m_isFollowedBy; }
};
class AstInside final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
खुला:
    AstInside(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { वापस "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }  // NA
};
class AstInsideRange final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
खुला:
    AstInsideRange(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { वापस "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_गाँठ_व्यंजक* newAndFromInside(Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp);
};
class AstLambdaArgRef final : खुला Ast_गाँठ_व्यंजक {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_सूचकांक;  // Index, not value

खुला:
    AstLambdaArgRef(File_पंक्ति* fl, स्थिर string& name, bool सूचकांक_2)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_सूचकांक{सूचकांक_2} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    string emitVerilog() override { वापस name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    व्योम name(स्थिर string& name) override { m_name = name; }
    bool सूचकांक_2() स्थिर { वापस m_सूचकांक; }
    bool is_व्यंजक_Coverage_Eligible() स्थिर override { वापस false; }
};
class AstMatchMasked final : खुला Ast_गाँठ_व्यंजक {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
खुला:
    अंतरभूत AstMatchMasked(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_AstMatchMasked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { वापस "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool cleanOut() स्थिर override { वापस true; }
    अटल uint32_t fold(स्थिर V3_संख्या& lhs, AstVar* matchVarp);
};
class AstMatches final : खुला Ast_गाँठ_व्यंजक {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
खुला:
    AstMatches(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { वापस "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstMemberSel final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
खुला:
    AstMemberSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, VFlagChildDType, स्थिर string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& name) override { m_name = name; }
    VAccess access() स्थिर { वापस m_access; }
    व्योम access(स्थिर VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    AstVar* varp() स्थिर { वापस m_varp; }
    व्योम varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : खुला Ast_गाँठ_व्यंजक {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
खुला:
    AstNewCopy(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { वापस "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
};
class AstNewDynamic final : खुला Ast_गाँठ_व्यंजक {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
खुला:
    AstNewDynamic(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* sizep, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { वापस "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
};
class AstPExpr final : खुला Ast_गाँठ_व्यंजक {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
खुला:
    explicit AstPExpr(File_पंक्ति* fl, AstBegin* bodyp, Ast_गाँठ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_पंक्ति* fl, AstBegin* bodyp, Ast_गाँठ_Stmt* finalp, Ast_गाँठ_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
};
class AstParseHolder final : खुला Ast_गाँठ_व्यंजक {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
खुला:
    explicit AstParseHolder(File_पंक्ति* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : खुला Ast_गाँठ_व्यंजक {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

खुला:
    AstParseRef(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_व्यंजक* lhsp = nullptr,
                Ast_गाँठ_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    व्योम name(स्थिर string& name) override { m_name = name; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstParseRef* स्थिर asamep = VN_DBG_AS(samep, ParseRef);
        वापस m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

खुला:
    AstPast(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* ticksp = nullptr,
            Ast_Sen_वृक्ष* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() स्थिर { वापस m_propertyTiming; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstPatMember final : खुला Ast_गाँठ_व्यंजक {
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

खुला:
    AstPatMember(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhssp, Ast_गाँठ* keyp, Ast_गाँठ_व्यंजक* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { वापस lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 2; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    bool isConcat() स्थिर { वापस m_isConcat; }
    व्योम isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() स्थिर { वापस m_isDefault; }
    व्योम isDefault(bool flag) { m_isDefault = flag; }
    Ast_गाँठ_व्यंजक* कुंजी_Exprp() स्थिर { वापस varrefp() ? varrefp() : VN_CAST(keyp(), गाँठ_व्यंजक); }
};
class AstPattern final : खुला Ast_गाँठ_व्यंजक {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
खुला:
    AstPattern(File_पंक्ति* fl, Ast_गाँठ* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstPattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर VL_MT_STABLE { वापस dtypep() ? dtypep() : childDTypep(); }
};
class AstPatternStar final : खुला Ast_गाँठ_व्यंजक {
    // Pattern wildcard: ".*"
खुला:
    explicit AstPatternStar(File_पंक्ति* fl)
        : ASTGEN_SUPER_PatternStar(fl) {}
    ASTGEN_MEMBERS_AstPatternStar;
    string emitVerilog() override { वापस ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstPatternVar final : खुला Ast_गाँठ_व्यंजक {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
खुला:
    AstPatternVar(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_PatternVar(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPatternVar;
    string emitVerilog() override { वापस ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_name == VN_DBG_AS(samep, PatternVar)->m_name;
    }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
};
class AstPropAlways final : खुला Ast_गाँठ_व्यंजक {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    स्थिर bool m_isStrong = false;  // s_always
खुला:
    AstPropAlways(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* propp, Ast_गाँठ_व्यंजक* loBoundp, Ast_गाँठ_व्यंजक* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { V3ERROR_NA_RETURN(0); }
    bool isStrong() स्थिर { वापस m_isStrong; }
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstRand final : खुला Ast_गाँठ_व्यंजक {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    स्थिर bool m_urandom;  // $urandom vs $random
खुला:
    class Reset {};
    AstRand(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_पंक्ति* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtypeSetUInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override {
        वापस seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        अगर (seedp()) {
            अगर (urandom()) वापस "VL_URANDOM_SEEDED_%nq%lq(%li)";
            वापस "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        अगर (isWide()) वापस "VL_RANDOM_%nq(%nw, %P)";
        वापस "VL_RANDOM_%nq()";
    }
    bool cleanOut() स्थिर override { वापस false; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस !seedp(); }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool combinable(स्थिर AstRand* samep) स्थिर {
        वापस !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() स्थिर { वापस m_urandom; }
};
class AstRandRNG final : खुला Ast_गाँठ_व्यंजक {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
खुला:
    AstRandRNG(File_पंक्ति* fl, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { वापस "%f$rngrandom()"; }
    string emitC() override {
        वापस isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool cleanOut() स्थिर override { वापस false; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstRising final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
खुला:
    AstRising(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { वापस "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstRose final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
खुला:
    AstRose(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_Sen_वृक्ष* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { वापस "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstSClocked final : खुला Ast_गाँठ_व्यंजक {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
खुला:
    AstSClocked(File_पंक्ति* fl, AstSenItem* sensesp, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : खुला Ast_गाँठ_व्यंजक {
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
    स्थिर bool m_unbounded = false;  // True when the upper bound is $
खुला:
    // Exact [*N]
    AstSConsRep(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* countp, Ast_गाँठ_व्यंजक* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { V3ERROR_NA_RETURN(0); }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {  // LCOV_EXCL_LINE
        वापस m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() स्थिर { वापस m_unbounded; }
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstSEventually final : खुला Ast_गाँठ_व्यंजक {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
खुला:
    explicit AstSEventually(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : खुला Ast_गाँठ_व्यंजक {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
खुला:
    explicit AstSExpr(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* preExprp, Ast_गाँठ_Stmt* delayp, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_पंक्ति* fl, Ast_गाँठ_Stmt* delayp, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstSFormatArg final : खुला Ast_गाँठ_व्यंजक {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

खुला:
    AstSFormatArg(File_पंक्ति* fl, VFormatAttr formatAttr, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() स्थिर override { वापस "$sformatarg"; }
    string emitVerilog() override { वापस "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        वापस nullptr;
    }
    VFormatAttr formatAttr() स्थिर { वापस m_formatAttr; }
    व्योम formatAttr(स्थिर VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    अटल VFormatAttr formatAttrDefauled(स्थिर AstSFormatArg* nodep, स्थिर Ast_गाँठ_DType* dtypep);
};
class AstSFormatF final : खुला Ast_गाँठ_व्यंजक {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_व्यंजक_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    स्थिर अक्षर m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
खुला:
    class व्यंजक_Format {};
    AstSFormatF(File_पंक्ति* fl, स्थिर string& text, bool hidden, Ast_गाँठ_व्यंजक* exprsp,
                अक्षर missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    AstSFormatF(File_पंक्ति* fl, व्यंजक_Format, Ast_गाँठ_व्यंजक* exprsp, अक्षर missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_व्यंजक_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_text; }
    व्योम name(स्थिर string& name) override { m_text = name; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() स्थिर override { वापस "$sformatf"; }
    string text() स्थिर { वापस m_text; }  // * = Text to display
    व्योम text(स्थिर string& text) { m_text = text; }
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(text() != "" && व्यंजक_Format_2());  // Expr format means no literal format
        वापस nullptr;
    }
    bool formatScopeTracking() स्थिर {  // Track scopeNamep();  Ok if false positive
        अगर (व्यंजक_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            वापस true;
        क्रम (स्थिर Ast_गाँठ* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            अगर (स्थिर AstConst* स्थिर fmtp = VN_CAST(exprp, Const)) {
                अगर (fmtp->अंक().isFromString()) {
                    स्थिर string str = fmtp->अंक().toString();
                    अगर (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        वापस true;
                }
            }
        }
        वापस false;
    }
    bool hidden() स्थिर { वापस m_hidden; }
    व्योम hidden(bool flag) { m_hidden = flag; }
    bool व्यंजक_Format_2() स्थिर { वापस m_व्यंजक_Format; }
    व्योम व्यंजक_Format_2(bool flag) { m_व्यंजक_Format = flag; }
    bool optionalFormat() स्थिर { वापस m_optionalFormat; }
    व्योम optionalFormat(bool flag) { m_optionalFormat = flag; }
    अक्षर missingArgChar() स्थिर { वापस m_missingArgChar; }
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstSGotoRep final : खुला Ast_गाँठ_व्यंजक {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
खुला:
    // Exact [->N]
    AstSGotoRep(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* countp, Ast_गाँठ_व्यंजक* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstSNonConsRep final : खुला Ast_गाँठ_व्यंजक {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
खुला:
    // Exact [=N]
    AstSNonConsRep(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* countp, Ast_गाँठ_व्यंजक* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstSScanF final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

खुला:
    AstSScanF(File_पंक्ति* fl, स्थिर string& text, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->fromp(fromp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() स्थिर override VL_MT_STABLE { वापस m_text; }
    string verilogKwd() स्थिर override { वापस "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: makes output
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstSScanF* स्थिर asamep = VN_DBG_AS(samep, SScanF);
        वापस text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() स्थिर { वापस m_text; }  // * = Text to display
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstSampled final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_internal : 1;  // Internally created, not from a source $sampled
खुला:
    AstSampled(File_पंक्ति* fl, Ast_गाँठ* exprp, Ast_गाँठ_DType* dtypep, bool internal = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_internal{internal} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { वापस "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_internal == VN_DBG_AS(samep, Sampled)->m_internal;
    }
    bool internal() स्थिर { वापस m_internal; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstScopeName final : खुला Ast_गाँठ_व्यंजक {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    स्थिर bool m_forFormat;  // Is for a format %m
    अटल std::string scopeNameFormatter(स्थिर std::string& text);
    अटल std::string scopePrettyNameFormatter(स्थिर std::string& text);

खुला:
    class ForFormat {};
    AstScopeName(File_पंक्ति* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstScopeName* स्थिर sp = VN_DBG_AS(samep, ScopeName);
        वापस m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    // ACCESSORS
    स्थिर std::string& scopeAttr() स्थिर { वापस m_scopeAttr; }
    व्योम scopeAttr(स्थिर std::string& val) { m_scopeAttr = val; }
    स्थिर std::string& scopeEntr() स्थिर { वापस m_scopeEntr; }
    व्योम scopeEntr(स्थिर std::string& val) { m_scopeEntr = val; }
    bool dpiExport() स्थिर { वापस m_dpiExport; }
    व्योम dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() स्थिर { वापस m_selfRelative; }
    व्योम selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() स्थिर { वापस m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() स्थिर { वापस scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() स्थिर { वापस scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() स्थिर { वापस scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() स्थिर { वापस scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
खुला:
    AstStable(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_Sen_वृक्ष* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { वापस "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class Ast_ढेर_Trace_F final : खुला Ast_गाँठ_व्यंजक {
    // $stacktrace used as function
खुला:
    explicit Ast_ढेर_Trace_F(File_पंक्ति* fl)
        : ASTGEN_SUPER_ढेर_Trace_F(fl) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_Ast_ढेर_Trace_F;
    string verilogKwd() स्थिर override { वापस "$stacktrace"; }
    string emitVerilog() override { वापस verilogKwd(); }
    string emitC() override { वापस "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool cleanOut() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstSteady final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
खुला:
    AstSteady(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { वापस "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstStructSel final : खुला Ast_गाँठ_व्यंजक {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
खुला:
    AstStructSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, स्थिर string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override {
        // Not a union
        वापस VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstStructSel* स्थिर sp = VN_DBG_AS(samep, StructSel);
        वापस m_name == sp->m_name;
    }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
};
class AstSysIgnore final : खुला Ast_गाँठ_व्यंजक {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
खुला:
    AstSysIgnore(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() स्थिर override { वापस "$ignored"; }
    bool isGateOptimizable() स्थिर override { वापस false; }  // Though deleted before opt
    bool isPredictOptimizable() स्थिर override { वापस false; }  // Though deleted before opt
    bool isPure() override { वापस false; }  // Though deleted before opt
    bool isOutputter() override { वापस true; }  // Though deleted before opt
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : खुला Ast_गाँठ_व्यंजक {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
खुला:
    AstSystemF(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() स्थिर override { वापस "$system"; }
    string emitVerilog() override { वापस verilogKwd(); }
    string emitC() override { वापस "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
    bool isUnlikely() स्थिर override { वापस true; }
    bool cleanOut() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class Ast_Tagged_व्यंजक final : खुला Ast_गाँठ_व्यंजक {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
खुला:
    Ast_Tagged_व्यंजक(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_Tagged_व्यंजक(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_व्यंजक;
    string emitVerilog() override { वापस "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_name == VN_DBG_AS(samep, Tagged_व्यंजक)->m_name;
    }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
};
class AstTaggedPattern final : खुला Ast_गाँठ_व्यंजक {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
खुला:
    AstTaggedPattern(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* patternp)
        : ASTGEN_SUPER_TaggedPattern(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstTaggedPattern;
    string emitVerilog() override { वापस "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_name == VN_DBG_AS(samep, TaggedPattern)->m_name;
    }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
};
class AstTestPlusArgs final : खुला Ast_गाँठ_व्यंजक {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
खुला:
    AstTestPlusArgs(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* searchp)
        : ASTGEN_SUPER_TestPlusArgs(fl) {
        this->searchp(searchp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstTestPlusArgs;
    string verilogKwd() स्थिर override { वापस "$test$plusargs"; }
    string emitVerilog() override { वापस verilogKwd(); }
    string emitC() override { वापस "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    // but isPure() true
    bool isSystemFunc() स्थिर override { वापस true; }
    bool cleanOut() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstThisRef final : खुला Ast_गाँठ_व्यंजक {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
खुला:
    AstThisRef(File_पंक्ति* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_पंक्ति* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { वापस "this"; }
    string emitVerilog() override { वापस "this"; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool cleanOut() स्थिर override { वापस true; }
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
};
class AstTimePrecision final : खुला Ast_गाँठ_व्यंजक {
    // Verilog $timeprecision
खुला:
    explicit AstTimePrecision(File_पंक्ति* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { वापस "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstTimeUnit final : खुला Ast_गाँठ_व्यंजक {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
खुला:
    explicit AstTimeUnit(File_पंक्ति* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { वापस "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : खुला Ast_गाँठ_व्यंजक {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    स्थिर std::vector<uint32_t> m_indices;  // Indices of the triggers tested
खुला:
    AstTriggered(File_पंक्ति* fl, AstVarScope* trigVscp, स्थिर std::vector<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    स्थिर std::vector<uint32_t>& indices() स्थिर { वापस m_indices; }
};
class AstUnbounded final : खुला Ast_गाँठ_व्यंजक {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
खुला:
    explicit AstUnbounded(File_पंक्ति* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { वापस "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
};
class AstUnlinkedRef final : खुला Ast_गाँठ_व्यंजक {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
खुला:
    AstUnlinkedRef(File_पंक्ति* fl, Ast_गाँठ* refp, स्थिर string& name, Ast_गाँठ* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : खुला Ast_गाँठ_व्यंजक {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    स्थिर bool m_strong;  // 's_' preffix
    स्थिर bool m_overlapping;  // '_with` suffix
खुला:
    AstUntil(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() स्थिर override {
        वापस (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isMultiCycleSva() स्थिर override { वापस true; }
    bool isStrong() स्थिर { वापस m_strong; }
    bool isOverlapping() स्थिर { वापस m_overlapping; }
};
class Ast_मूल्य_Plus_Args final : खुला Ast_गाँठ_व्यंजक {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
खुला:
    Ast_मूल्य_Plus_Args(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* searchp, Ast_गाँठ_व्यंजक* outp)
        : ASTGEN_SUPER_मूल्य_Plus_Args(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_मूल्य_Plus_Args;
    string verilogKwd() स्थिर override { वापस "$value$plusargs"; }
    string emitVerilog() override { वापस "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस !outp(); }
    bool isSystemFunc() स्थिर override { वापस true; }
    bool cleanOut() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstWithParse final : खुला Ast_गाँठ_व्यंजक {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
निजी:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

खुला:
    AstWithParse(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* funcrefp, Ast_गाँठ_व्यंजक* exprsp,
                 Ast_गाँठ* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        addExprsp(exprsp);
        addConstraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool restricted() स्थिर { वापस m_restricted; }
    व्योम restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : खुला Ast_गाँठ_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
खुला:
    AstBufIf1(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { वापस "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstCastDynamic final : खुला Ast_गाँठ_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
खुला:
    AstCastDynamic(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtypeSetBitUnsized(32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { वापस "%f$cast(%r, %l)"; }
    string emitC() override { वापस "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 20; }
    bool isPure() override { वापस false; }
};
class AstCompareNN final : खुला Ast_गाँठ_Biop {
    // Verilog str.compare() and str.icompare()
    स्थिर bool m_ignoreCase;  // True for str.icompare()
खुला:
    AstCompareNN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() स्थिर override VL_MT_STABLE { वापस m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        वापस m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        वापस m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool ignoreCase() स्थिर { वापस m_ignoreCase; }
};
class AstConcat final : खुला Ast_गाँठ_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
खुला:
    AstConcat(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        अगर (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetBitOrLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { वापस "%f{%l, %k%r}"; }
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { वापस "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(concat %l %r)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 2; }
};
class AstConcatN final : खुला Ast_गाँठ_Biop {
    // String concatenate
खुला:
    AstConcatN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { वापस "%f{%l, %k%r}"; }
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { वापस "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
    bool stringFlavor() स्थिर override { वापस true; }
};
class AstDiv final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstDiv(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f/ %r)"; }
    string emitC() override { वापस "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvudiv %l %r)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_DIV; }
};
class AstDivD final : खुला Ast_गाँठ_Biop {
खुला:
    AstDivD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstDivD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "/"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL_DIV; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstDivS final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstDivS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f/ %r)"; }
    string emitC() override { वापस "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvsdiv %l %r)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_DIV; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstEqWild final : खुला Ast_गाँठ_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
खुला:
    AstEqWild(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    अटल Ast_गाँठ_Biop* newTyped(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp);
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f==? %r)"; }
    string emitC() override {
        अगर (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            वापस "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() स्थिर override { वापस "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        अगर (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            वापस "==";
        }
    }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstFGetS final : खुला Ast_गाँठ_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
खुला:
    AstFGetS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%f$fgets(%l,%r)"; }
    string emitC() override {
        वापस strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 64; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : खुला Ast_गाँठ_Biop {
खुला:
    AstFUngetC(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        वापस "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 64; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    Ast_गाँठ* filep() स्थिर { वापस lhsp(); }
    Ast_गाँठ* charp() स्थिर { वापस rhsp(); }
};
class AstGetcN final : खुला Ast_गाँठ_Biop {
    // Verilog string.getc()
खुला:
    AstGetcN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() स्थिर override VL_MT_STABLE { वापस "getc"; }
    string emitVerilog() override { वापस "%k(%l.getc(%r))"; }
    string emitC() override { वापस "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstGetcRefN final : खुला Ast_गाँठ_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
खुला:
    AstGetcRefN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstGt final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstGt(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f> %r)"; }
    string emitC() override { वापस "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { वापस ">"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstGtD final : खुला Ast_गाँठ_Biop {
खुला:
    AstGtD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस ">"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstGtN final : खुला Ast_गाँठ_Biop {
खुला:
    AstGtN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस ">"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
    bool stringFlavor() स्थिर override { वापस true; }
};
class AstGtS final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstGtS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f> %r)"; }
    string emitC() override { वापस "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstGte final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstGte(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f>= %r)"; }
    string emitC() override { वापस "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { वापस ">="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstGteD final : खुला Ast_गाँठ_Biop {
खुला:
    AstGteD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस ">="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstGteN final : खुला Ast_गाँठ_Biop {
खुला:
    AstGteN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस ">="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
    bool stringFlavor() स्थिर override { वापस true; }
};
class AstGteS final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstGteS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f>= %r)"; }
    string emitC() override { वापस "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstLogAnd final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstLogAnd(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f&& %r)"; }
    string emitC() override { वापस "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvand %l %r)"; }
    string emitSimpleOperator() override { वापस "&&"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() + INSTR_गिनती_BRANCH; }
};
class AstLogIf final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstLogIf(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f-> %r)"; }
    string emitC() override { वापस "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { वापस "->"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() + INSTR_गिनती_BRANCH; }
};
class AstLogOr final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstLogOr(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f|| %r)"; }
    string emitC() override { वापस "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvor %l %r)"; }
    string emitSimpleOperator() override { वापस "||"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() + INSTR_गिनती_BRANCH; }
};
class AstLt final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstLt(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f< %r)"; }
    string emitC() override { वापस "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { वापस "<"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstLtD final : खुला Ast_गाँठ_Biop {
खुला:
    AstLtD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "<"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstLtN final : खुला Ast_गाँठ_Biop {
खुला:
    AstLtN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "<"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
    bool stringFlavor() स्थिर override { वापस true; }
};
class AstLtS final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstLtS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f< %r)"; }
    string emitC() override { वापस "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstLte final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstLte(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f<= %r)"; }
    string emitC() override { वापस "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { वापस "<="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstLteD final : खुला Ast_गाँठ_Biop {
खुला:
    AstLteD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "<="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstLteN final : खुला Ast_गाँठ_Biop {
खुला:
    AstLteN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "<="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
    bool stringFlavor() स्थिर override { वापस true; }
};
class AstLteS final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstLteS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f<= %r)"; }
    string emitC() override { वापस "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstModDiv final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstModDiv(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f%% %r)"; }
    string emitC() override { वापस "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvurem %l %r)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_DIV; }
};
class AstModDivS final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstModDivS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f%% %r)"; }
    string emitC() override { वापस "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvsmod %l %r)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_DIV; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstNeqWild final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstNeqWild(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f!=? %r)"; }
    string emitC() override {
        अगर (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            वापस "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        अगर (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            वापस "!=";
        }
    }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstPow final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstPow(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f** %r)"; }
    string emitC() override { वापस "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_MUL * 10; }
};
class AstPowD final : खुला Ast_गाँठ_Biop {
खुला:
    AstPowD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstPowD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f** %r)"; }
    string emitC() override { वापस "pow(%li,%ri)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL_DIV * 5; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstPowSS final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstPowSS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f** %r)"; }
    string emitC() override { वापस "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_MUL * 10; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstPowSU final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstPowSU(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f** %r)"; }
    string emitC() override { वापस "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_MUL * 10; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstPowUS final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstPowUS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f** %r)"; }
    string emitC() override { वापस "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_MUL * 10; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstReplicate final : खुला Ast_गाँठ_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
खुला:
    AstReplicate(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        अगर (lhsp) {
            अगर (स्थिर AstConst* स्थिर constp = VN_CAST(rhsp, Const)) {
                अगर (constp->अंक().isFourState()
                    || (constp->dtypep()->isSigned() && constp->अंक().isNegative())) {
                    dtypeSetBitOrLogicSized(lhsp->width(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } अन्यथा {
                    dtypeSetBitOrLogicSized(lhsp->width() * constp->toSInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, uint32_t rep_गिनती)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_गिनती}} {}
    ASTGEN_MEMBERS_AstReplicate;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { वापस "%f{%r{%k%l}}"; }
    string emitC() override { वापस "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override {
        वापस "((_ repeat " + cvtToStr(width() / lhsp()->width()) + ") %l)";
    }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 2; }
};
class AstReplicateN final : खुला Ast_गाँठ_Biop {
    // String replicate
खुला:
    AstReplicateN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { वापस "%f{%r{%k%l}}"; }
    string emitC() override { वापस "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 2; }
    bool stringFlavor() स्थिर override { वापस true; }
};
class AstSAnd final : खुला Ast_गाँठ_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    स्थिर bool m_propertyControl;  // Parser-generated property if/case branch conjunction
खुला:
    AstSAnd(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() + INSTR_गिनती_BRANCH; }
    bool isMultiCycleSva() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {  // LCOV_EXCL_LINE
        वापस m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() स्थिर { वापस m_propertyControl; }
};
class AstSIntersect final : खुला Ast_गाँठ_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
खुला:
    AstSIntersect(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() + INSTR_गिनती_BRANCH; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstSOr final : खुला Ast_गाँठ_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
खुला:
    AstSOr(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSOr;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() + INSTR_गिनती_BRANCH; }
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstSThroughout final : खुला Ast_गाँठ_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
खुला:
    AstSThroughout(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstSWithin final : खुला Ast_गाँठ_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
खुला:
    AstSWithin(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() स्थिर override { वापस true; }
};
class AstSel final : खुला Ast_गाँठ_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    पूर्णांक m_declElWidth;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    पूर्णांक m_widthConst;

खुला:
    AstSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* lsbp, पूर्णांक bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicSized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, पूर्णांक lsb, पूर्णांक bitwidth, पूर्णांक bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicUnsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& from, स्थिर V3_संख्या& bit) override {
        out.opSel(from, bit.toUInt() + widthConst() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        वापस widthConst() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() स्थिर override { वापस "((_ extract %t %r) %l)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* otherp) स्थिर override {
        वापस widthConst() == VN_DBG_AS(otherp, Sel)->widthConst();
    }
    पूर्णांक instr_गिनती() स्थिर override {
        वापस widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    पूर्णांक widthConst() स्थिर { वापस m_widthConst; }
    व्योम widthConst(पूर्णांक मूल्य) { m_widthConst = मूल्य; }
    पूर्णांक lsbConst() स्थिर { वापस VN_AS(lsbp(), Const)->toSInt(); }
    पूर्णांक msbConst() स्थिर { वापस lsbConst() + widthConst() - 1; }
    VNumRange& declRange() VL_MT_STABLE { वापस m_declRange; }
    स्थिर VNumRange& declRange() स्थिर VL_MT_STABLE { वापस m_declRange; }
    व्योम declRange(स्थिर VNumRange& flag) { m_declRange = flag; }
    पूर्णांक declElWidth() स्थिर { वापस m_declElWidth; }
    व्योम declElWidth(पूर्णांक flag) { m_declElWidth = flag; }
};
class AstShiftL final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstShiftL(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, पूर्णांक setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        अगर (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा अगर (स्थिर Ast_गाँठ_DType* स्थिर dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftL;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f<< %r)"; }
    string emitC() override { वापस "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        वापस (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstShiftLOvr final : खुला Ast_गाँठ_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
खुला:
    AstShiftLOvr(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, पूर्णांक setwidth = 0)
        : ASTGEN_SUPER_ShiftLOvr(fl, lhsp, rhsp) {
        अगर (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            स्थिर Ast_गाँठ_DType* स्थिर dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftLOvr;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f<< %r)"; }
    string emitC() override { वापस "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstShiftR final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstShiftR(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, पूर्णांक setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        अगर (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा अगर (स्थिर Ast_गाँठ_DType* स्थिर dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftR;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f>> %r)"; }
    string emitC() override { वापस "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        वापस (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    // LHS size might be > output size, so don't want to force size
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstShiftROvr final : खुला Ast_गाँठ_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
खुला:
    AstShiftROvr(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, पूर्णांक setwidth = 0)
        : ASTGEN_SUPER_ShiftROvr(fl, lhsp, rhsp) {
        अगर (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            स्थिर Ast_गाँठ_DType* स्थिर dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftROvr;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f>> %r)"; }
    string emitC() override { वापस "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    // LHS size might be > output size, so don't want to force size
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstShiftRS final : खुला Ast_गाँठ_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
खुला:
    AstShiftRS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, पूर्णांक setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        अगर (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा अगर (स्थिर Ast_गाँठ_DType* स्थिर dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { वापस "%k(%l %f>>> %r)"; }
    string emitSMT() स्थिर override { वापस "(bvashr %l %r)"; }
    string emitC() override { वापस "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstShiftRSOvr final : खुला Ast_गाँठ_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
खुला:
    AstShiftRSOvr(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, पूर्णांक setwidth = 0)
        : ASTGEN_SUPER_ShiftRSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        अगर (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            स्थिर Ast_गाँठ_DType* स्थिर dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRSOvr;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { वापस "%k(%l %f>>> %r)"; }
    string emitC() override { वापस "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstSub final : खुला Ast_गाँठ_Biop {
    // @astgen makeDfgVertex
खुला:
    AstSub(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f- %r)"; }
    string emitC() override { वापस "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvsub %l %r)"; }
    string emitSimpleOperator() override { वापस "-"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
};
class AstSubD final : खुला Ast_गाँठ_Biop {
खुला:
    AstSubD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstSubD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "-"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstURandomRange final : खुला Ast_गाँठ_Biop {
    // $urandom_range
खुला:
    AstURandomRange(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%f$urandom_range(%l, %r)"; }
    string emitC() override { वापस "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : खुला Ast_गाँठ_Bi_Com {
    // @astgen makeDfgVertex
खुला:
    AstEq(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    अटल Ast_गाँठ_Biop* newTyped(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp);
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f== %r)"; }
    string emitC() override { वापस "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { वापस "=="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstEqCase final : खुला Ast_गाँठ_Bi_Com {
    // @astgen makeDfgVertex
खुला:
    AstEqCase(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f=== %r)"; }
    string emitC() override { वापस "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { वापस "=="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstEqD final : खुला Ast_गाँठ_Bi_Com {
खुला:
    AstEqD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "=="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstEqN final : खुला Ast_गाँठ_Bi_Com {
खुला:
    AstEqN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "=="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
    bool stringFlavor() स्थिर override { वापस true; }
};
class AstEqT final : खुला Ast_गाँठ_Bi_Com {
    // Equal (==) for data types
खुला:
    AstEqT(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqT;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "=="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
};
class AstLogEq final : खुला Ast_गाँठ_Bi_Com {
    // @astgen makeDfgVertex
खुला:
    AstLogEq(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f<-> %r)"; }
    string emitC() override { वापस "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { वापस "<->"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() + INSTR_गिनती_BRANCH; }
};
class AstNeq final : खुला Ast_गाँठ_Bi_Com {
    // @astgen makeDfgVertex
खुला:
    AstNeq(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    अटल Ast_गाँठ_Biop* newTyped(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp);
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f!= %r)"; }
    string emitC() override { वापस "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { वापस "!="; }
    string emitSMT() स्थिर override { वापस "(__Vbv (not (= %l %r)))"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstNeqCase final : खुला Ast_गाँठ_Bi_Com {
    // @astgen makeDfgVertex
खुला:
    AstNeqCase(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f!== %r)"; }
    string emitC() override { वापस "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { वापस "!="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
};
class AstNeqD final : खुला Ast_गाँठ_Bi_Com {
खुला:
    AstNeqD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "!="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstNeqN final : खुला Ast_गाँठ_Bi_Com {
खुला:
    AstNeqN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "!="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
    bool stringFlavor() स्थिर override { वापस true; }
};
class AstNeqT final : खुला Ast_गाँठ_Bi_Com {
    // Not-equal (!=) for data types
खुला:
    AstNeqT(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "!="; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_STR; }
};

// === AstNodeBiComAsv ===
class AstAdd final : खुला Ast_गाँठ_Bi_Com_Asv {
    // @astgen makeDfgVertex
खुला:
    AstAdd(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAdd;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f+ %r)"; }
    string emitC() override { वापस "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvadd %l %r)"; }
    string emitSimpleOperator() override { वापस "+"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
};
class AstAddD final : खुला Ast_गाँठ_Bi_Com_Asv {
खुला:
    AstAddD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstAddD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "+"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstAnd final : खुला Ast_गाँठ_Bi_Com_Asv {
    // @astgen makeDfgVertex
खुला:
    AstAnd(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f& %r)"; }
    string emitC() override { वापस "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvand %l %r)"; }
    string emitSimpleOperator() override { वापस "&"; }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    स्थिर अक्षर* widthMismatch() स्थिर override VL_MT_STABLE;
};
class AstMul final : खुला Ast_गाँठ_Bi_Com_Asv {
    // @astgen makeDfgVertex
खुला:
    AstMul(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f* %r)"; }
    string emitC() override { वापस "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvmul %l %r)"; }
    string emitSimpleOperator() override { वापस "*"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_MUL; }
};
class AstMulD final : खुला Ast_गाँठ_Bi_Com_Asv {
खुला:
    AstMulD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstMulD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "*"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstMulS final : खुला Ast_गाँठ_Bi_Com_Asv {
    // @astgen makeDfgVertex
खुला:
    AstMulS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f* %r)"; }
    string emitC() override { वापस "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvmul %l %r)"; }
    string emitSimpleOperator() override { वापस ""; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    bool आकार_Matters_Rhs() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * INSTR_गिनती_INT_MUL; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstOr final : खुला Ast_गाँठ_Bi_Com_Asv {
    // @astgen makeDfgVertex
खुला:
    AstOr(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f| %r)"; }
    string emitC() override { वापस "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvor %l %r)"; }
    string emitSimpleOperator() override { वापस "|"; }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    स्थिर अक्षर* widthMismatch() स्थिर override VL_MT_STABLE;
};
class AstXor final : खुला Ast_गाँठ_Bi_Com_Asv {
    // @astgen makeDfgVertex
खुला:
    AstXor(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { वापस "%k(%l %f^ %r)"; }
    string emitC() override { वापस "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() स्थिर override { वापस "(bvxor %l %r)"; }
    string emitSimpleOperator() override { वापस "^"; }
    bool cleanOut() स्थिर override { वापस false; }  // Lclean && Rclean
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    स्थिर अक्षर* widthMismatch() स्थिर override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class AstDistChiSquare final : खुला Ast_गाँठ_Dist_Biop {
खुला:
    AstDistChiSquare(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_DistChiSquare(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistChiSquare;
    string emitVerilog() override { वापस "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { वापस "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class AstDistExponential final : खुला Ast_गाँठ_Dist_Biop {
खुला:
    AstDistExponential(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_DistExponential(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistExponential;
    string emitVerilog() override { वापस "%f$dist_exponential(%l, %r)"; }
    string emitC() override { वापस "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class AstDistPoisson final : खुला Ast_गाँठ_Dist_Biop {
खुला:
    AstDistPoisson(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_DistPoisson(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistPoisson;
    string emitVerilog() override { वापस "%f$dist_poisson(%l, %r)"; }
    string emitC() override { वापस "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : खुला Ast_गाँठ_Dist_Biop {
खुला:
    AstDistT(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { वापस "%f$dist_t(%l, %r)"; }
    string emitC() override { वापस "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : खुला Ast_गाँठ_Sel {
    // @astgen makeDfgVertex
    व्योम init(स्थिर Ast_गाँठ* fromp) {
        अगर (fromp && VN_IS(fromp->dtypep()->skipRefp(), गाँठ_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), गाँठ_Array_DType)->subDTypep());
        }
    }

खुला:
    AstArraySel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, पूर्णांक bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() स्थिर override { वापस "(select %l %r)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isGateOptimizable() स्थिर override { वापस true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
};
class AstAssocSel final : खुला Ast_गाँठ_Sel {
    व्योम init(स्थिर Ast_गाँठ* fromp) {
        अगर (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

खुला:
    AstAssocSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%k(%l%f[%r])"; }
    string emitC() override { वापस "%li%k[%ri]"; }
    string emitSMT() स्थिर override { वापस "(select %l %r)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isGateOptimizable() स्थिर override {
        वापस !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस !isLValue(); }  // AssocSel creates on miss
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
};
class AstWildcardSel final : खुला Ast_गाँठ_Sel {
    व्योम init(स्थिर Ast_गाँठ* fromp) {
        अगर (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

खुला:
    AstWildcardSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%k(%l%f[%r])"; }
    string emitC() override { वापस "%li%k[%ri]"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool isGateOptimizable() स्थिर override { वापस true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
};
class AstWordSel final : खुला Ast_गाँठ_Sel {
    // Select a single word from a multi-word wide value
खुला:
    AstWordSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    व्योम संख्या_Operate(V3_संख्या&, स्थिर V3_संख्या&, स्थिर V3_संख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};

// === AstNodeStream ===
class AstStreamL final : खुला Ast_गाँठ_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
खुला:
    AstStreamL(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamL;
    string emitVerilog() override { वापस "%f{ << %r %k{%l} }"; }
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { वापस "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 2; }
};
class AstStreamR final : खुला Ast_गाँठ_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
खुला:
    AstStreamR(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamR;
    string emitVerilog() override { वापस "%f{ >> %r %k{%l} }"; }
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { वापस isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : खुला Ast_गाँठ_System_Biop_D {
खुला:
    AstAtan2D(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$atan2(%l,%r)"; }
    string emitC() override { वापस "atan2(%li,%ri)"; }
};
class AstHypotD final : खुला Ast_गाँठ_System_Biop_D {
खुला:
    AstHypotD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$hypot(%l,%r)"; }
    string emitC() override { वापस "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : खुला Ast_गाँठ_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
खुला:
    AstCCall(File_पंक्ति* fl, AstCFunc* funcp, Ast_गाँठ_व्यंजक* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    स्थिर VSelfPointerText& selfPointer() स्थिर { वापस m_selfPointer; }
    व्योम selfPointer(स्थिर VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) स्थिर {
        वापस selfPointer().protect(useSelfForThis, protect());
    }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
};
class AstCMethodCall final : खुला Ast_गाँठ_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
खुला:
    AstCMethodCall(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, AstCFunc* funcp, Ast_गाँठ_व्यंजक* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : खुला Ast_गाँठ_CCall {
    // C++ new() call
खुला:
    AstCNew(File_पंक्ति* fl, AstCFunc* funcp, Ast_गाँठ_व्यंजक* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : खुला Ast_गाँठ_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

खुला:
    AstConsArray(File_पंक्ति* fl, Ast_गाँठ_DType* newDTypep, Ast_गाँठ_व्यंजक* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    स्थिर अक्षर* broken() स्थिर override;
    व्योम cloneRelink() override;
    अंतरभूत AstConsArrayItem* itemsp() स्थिर VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_गाँठ_व्यंजक* valuep(uint64_t सूचकांक_2) स्थिर;
    // Add the value of the element at the index, which must not be given yet
    व्योम valuep(uint64_t सूचकांक_2, Ast_गाँठ_व्यंजक* newp);
    // Add the elements of the array 'arrayp', starting at the index
    व्योम splice_मूल्यावली(uint64_t सूचकांक_2, Ast_गाँठ_व्यंजक* arrayp);
};
class AstConsAssoc final : खुला Ast_गाँठ_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
खुला:
    explicit AstConsAssoc(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        वापस nullptr;
    }
};
class AstConsDynArray final : खुला Ast_गाँठ_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
खुला:
    explicit AstConsDynArray(File_पंक्ति* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    स्थिर अक्षर* broken() स्थिर override;
};
class AstConsQueue final : खुला Ast_गाँठ_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
खुला:
    explicit AstConsQueue(File_पंक्ति* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        वापस nullptr;
    }
};
class AstConsStruct final : खुला Ast_गाँठ_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
खुला:
    AstConsStruct(File_पंक्ति* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    स्थिर अक्षर* broken() स्थिर override;
    // The value of the member called 'name'
    Ast_गाँठ_व्यंजक* memberValuep(स्थिर string& name) स्थिर;
};
class AstConsWildcard final : खुला Ast_गाँठ_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
खुला:
    explicit AstConsWildcard(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        वापस nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : खुला Ast_गाँठ_FTask_Ref {
    // A reference to a function
खुला:
    अंतरभूत AstFuncRef(File_पंक्ति* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_पंक्ति* fl, स्थिर string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class AstMethodCall final : खुला Ast_गाँठ_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
खुला:
    AstMethodCall(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, VFlagChildDType, स्थिर string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMethodCall(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, स्थिर string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_AstMethodCall;
};
class AstNew final : खुला Ast_गाँठ_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
खुला:
    AstNew(File_पंक्ति* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool isImplicit() स्थिर { वापस m_isImplicit; }
    व्योम isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() स्थिर { वापस m_isScoped; }
    व्योम isScoped(bool flag) { m_isScoped = flag; }
    bool isPure() override { वापस false; }
};
class AstTaskRef final : खुला Ast_गाँठ_FTask_Ref {
    // A reference to a task
खुला:
    अंतरभूत AstTaskRef(File_पंक्ति* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_पंक्ति* fl, स्थिर string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtypeSetVoid();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : खुला Ast_गाँठ_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
निजी:
    VAccess m_access;  // Left hand side assignment
खुला:
    AstSelBit(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    VAccess access() स्थिर { वापस m_access; }
    व्योम access(स्थिर VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : खुला Ast_गाँठ_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
खुला:
    AstSelExtract(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* msbp, Ast_गाँठ_व्यंजक* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : खुला Ast_गाँठ_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
खुला:
    AstSelMinus(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* bitp, Ast_गाँठ_व्यंजक* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : खुला Ast_गाँठ_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
खुला:
    AstSelPlus(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_व्यंजक* bitp, Ast_गाँठ_व्यंजक* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_गिनती_Bits final : खुला Ast_गाँठ_Quadop {
    // Number of bits set in vector
खुला:
    Ast_गिनती_Bits(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* ctrl1p)
        : ASTGEN_SUPER_गिनती_Bits(fl, exprp, ctrl1p, ctrl1p->clone_वृक्ष_Pure(false),
                                 ctrl1p->clone_वृक्ष_Pure(false)) {
        dtypeSetInt();
    }
    Ast_गिनती_Bits(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* ctrl1p, Ast_गाँठ_व्यंजक* ctrl2p)
        : ASTGEN_SUPER_गिनती_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_वृक्ष_Pure(false)) {
        dtypeSetInt();
    }
    Ast_गिनती_Bits(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, Ast_गाँठ_व्यंजक* ctrl1p, Ast_गाँठ_व्यंजक* ctrl2p,
                 Ast_गाँठ_व्यंजक* ctrl3p)
        : ASTGEN_SUPER_गिनती_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_गिनती_Bits;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& व्यंजक, स्थिर V3_संख्या& ctrl1,
                       स्थिर V3_संख्या& ctrl2, स्थिर V3_संख्या& ctrl3) override {
        out.op_गिनती_Bits(व्यंजक, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { वापस "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool cleanThs() स्थिर override { वापस true; }
    bool cleanFhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool आकार_Matters_Ths() स्थिर override { वापस false; }
    bool आकार_Matters_Fhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : खुला Ast_गाँठ_Termop {
खुला:
    explicit AstInferredDisable(File_पंक्ति* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { वापस "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool isSystemFunc() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstTime final : खुला Ast_गाँठ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
खुला:
    AstTime(File_पंक्ति* fl, स्थिर VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { वापस "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_TIME; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : खुला Ast_गाँठ_Termop {
    VTimescale m_timeunit;  // Parent module time unit
खुला:
    AstTimeD(File_पंक्ति* fl, स्थिर VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { वापस "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_TIME; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : खुला Ast_गाँठ_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
खुला:
    AstCond(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ_व्यंजक* thenp, Ast_गाँठ_व्यंजक* elsep);
    ASTGEN_MEMBERS_AstCond;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs,
                       स्थिर V3_संख्या& ths) override {
        out.opAssign(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { वापस "%k(%l %f? %r %k: %t)"; }
    string emitC() override { वापस "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() स्थिर override { वापस "(ite (__Vbool %l) %r %t)"; }
    bool cleanOut() स्थिर override { वापस false; }  // clean if e1 & e2 clean
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस false; }
    bool cleanThs() स्थिर override { वापस false; }  // Propagates up
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool आकार_Matters_Ths() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_BRANCH; }
};
class AstPutcN final : खुला Ast_गाँठ_Triop {
    // Verilog string.putc()
खुला:
    AstPutcN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ_व्यंजक* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstPutcN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs,
                       स्थिर V3_संख्या& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() स्थिर override VL_MT_STABLE { वापस "putc"; }
    string emitVerilog() override { वापस "%k(%l.putc(%r,%t))"; }
    string emitC() override { वापस "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool cleanThs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool आकार_Matters_Ths() स्थिर override { वापस false; }
};
class AstSliceSel final : खुला Ast_गाँठ_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
खुला:
    AstSliceSel(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* fromp, स्थिर VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_AstSliceSel;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& from, स्थिर V3_संख्या& lo,
                       स्थिर V3_संख्या& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool cleanThs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool आकार_Matters_Ths() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { वापस m_declRange; }
    स्थिर VNumRange& declRange() स्थिर VL_MT_STABLE { वापस m_declRange; }
};
class AstSubstrN final : खुला Ast_गाँठ_Triop {
    // Verilog string.substr()
खुला:
    AstSubstrN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ_व्यंजक* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs,
                       स्थिर V3_संख्या& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() स्थिर override VL_MT_STABLE { वापस "substr"; }
    string emitVerilog() override { वापस "%k(%l.substr(%r,%t))"; }
    string emitC() override { वापस "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { वापस ""; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool cleanRhs() स्थिर override { वापस true; }
    bool cleanThs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool आकार_Matters_Rhs() स्थिर override { वापस false; }
    bool आकार_Matters_Ths() स्थिर override { वापस false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : खुला Ast_गाँठ_Dist_Triop {
खुला:
    AstDistErlang(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ_व्यंजक* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { वापस "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { वापस "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class AstDistNormal final : खुला Ast_गाँठ_Dist_Triop {
खुला:
    AstDistNormal(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ_व्यंजक* thsp)
        : ASTGEN_SUPER_DistNormal(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistNormal;
    string emitVerilog() override { वापस "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { वापस "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class AstDistUniform final : खुला Ast_गाँठ_Dist_Triop {
खुला:
    AstDistUniform(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp, Ast_गाँठ_व्यंजक* thsp)
        : ASTGEN_SUPER_DistUniform(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistUniform;
    string emitVerilog() override { वापस "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { वापस "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : खुला Ast_गाँठ_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
खुला:
    क्रमागत FmtType : पूर्णांक { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

निजी:
    // dist-ast-dump-suppress  // Part of name()
    स्थिर FmtType m_fmtType;  // Operation type
खुला:
    AstAtoN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtypeSetDouble() : dtypeSetInteger();
    }
    ASTGEN_MEMBERS_AstAtoN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opAtoN(lhs, m_fmtType); }
    string name() स्थिर override VL_MT_STABLE {
        चयन (m_fmtType) {
        हाल ATOI: वापस "atoi";
        हाल ATOHEX: वापस "atohex";
        हाल ATOOCT: वापस "atooct";
        हाल ATOBIN: वापस "atobin";
        हाल ATOREAL: वापस "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { वापस "%l." + name() + "()"; }
    string emitC() override {
        चयन (m_fmtType) {
        हाल ATOI: वापस "VL_ATOI_N(%li, 10)";
        हाल ATOHEX: वापस "VL_ATOI_N(%li, 16)";
        हाल ATOOCT: वापस "VL_ATOI_N(%li, 8)";
        हाल ATOBIN: वापस "VL_ATOI_N(%li, 2)";
        हाल ATOREAL: वापस "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    FmtType fmtType() स्थिर { वापस m_fmtType; }
};
class AstBitsToRealD final : खुला Ast_गाँठ_Uniop {
खुला:
    AstBitsToRealD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstBitsToRealD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { वापस "%f$bitstoreal(%l)"; }
    string emitC() override { वापस "VL_CVT_D_Q(%li)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }  // Eliminated before matters
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstCCast final : खुला Ast_गाँठ_Uniop {
    // Cast to C-based data type
    पूर्णांक m_आकार;

खुला:
    AstCCast(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, पूर्णांक setwidth, पूर्णांक minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_आकार = setwidth;
        अगर (setwidth) {
            अगर (minwidth == -1) minwidth = setwidth;
            dtypeSetBitUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_आकार = width();
    }
    ASTGEN_MEMBERS_AstCCast;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { वापस "%f$_CAST(%l)"; }
    string emitC() override { वापस "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }  // Special cased in V3Cast
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस आकार() == VN_DBG_AS(samep, CCast)->आकार();
    }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    //
    पूर्णांक आकार() स्थिर { वापस m_आकार; }
};
class AstCLog2 final : खुला Ast_गाँठ_Uniop {
खुला:
    AstCLog2(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { वापस "%f$clog2(%l)"; }
    string emitC() override { वापस "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 16; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstCastWrap final : खुला Ast_गाँठ_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
खुला:
    AstCastWrap(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { वापस "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
};
class Ast_गिनती_Ones final : खुला Ast_गाँठ_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
खुला:
    Ast_गिनती_Ones(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_गिनती_Ones(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_गिनती_Ones;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.op_गिनती_Ones(lhs); }
    string emitVerilog() override { वापस "%f$countones(%l)"; }
    string emitC() override { वापस "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 16; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstCvtPackString final : खुला Ast_गाँठ_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
खुला:
    AstCvtPackString(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstExtend final : खुला Ast_गाँठ_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
खुला:
    AstExtend(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, पूर्णांक width, पूर्णांक widthMin = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetBitOrLogicUnsized(width, widthMin ? widthMin : width, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { वापस "%l"; }
    string emitC() override { वापस "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() स्थिर override {
        वापस "((_ zero_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override {
        वापस false;  // Because the EXTEND operator self-casts
    }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
};
class AstExtendS final : खुला Ast_गाँठ_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
खुला:
    AstExtendS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, पूर्णांक width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { वापस "%l"; }
    string emitC() override { वापस "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() स्थिर override {
        वापस "((_ sign_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override {
        वापस false;  // Because the EXTEND operator self-casts
    }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool signedFlavor() स्थिर override { वापस true; }
};
class AstFEof final : खुला Ast_गाँठ_Uniop {
खुला:
    AstFEof(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%f$feof(%l)"; }
    string emitC() override { वापस "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 16; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() स्थिर override { वापस true; }
    Ast_गाँठ* filep() स्थिर { वापस lhsp(); }
};
class AstFGetC final : खुला Ast_गाँठ_Uniop {
खुला:
    AstFGetC(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { वापस "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 64; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    Ast_गाँठ* filep() स्थिर { वापस lhsp(); }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstISToRD final : खुला Ast_गाँठ_Uniop {
    // $itor where lhs is signed
खुला:
    AstISToRD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstISToRD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { वापस "%f$itor($signed(%l))"; }
    string emitC() override { वापस "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_शब्दगण() override { वापस true; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstIToRD final : खुला Ast_गाँठ_Uniop {
    // $itor where lhs is unsigned
खुला:
    AstIToRD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstIToRD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { वापस "%f$itor(%l)"; }
    string emitC() override { वापस "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstIsUnbounded final : खुला Ast_गाँठ_Uniop {
    // True if is unbounded ($)
खुला:
    AstIsUnbounded(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { वापस "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstIsUnknown final : खुला Ast_गाँठ_Uniop {
    // True if any unknown bits
खुला:
    AstIsUnknown(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { वापस "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstLenN final : खुला Ast_गाँठ_Uniop {
    // Length of a string
खुला:
    AstLenN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstLenN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { वापस "%f(%l)"; }
    string emitC() override { वापस "VL_LEN_IN(%li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
};
class AstLogNot final : खुला Ast_गाँठ_Uniop {
    // @astgen makeDfgVertex
निजी:
    स्थिर bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
खुला:
    AstLogNot(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { वापस "%f(! %l)"; }
    string emitC() override { वापस "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() स्थिर override { वापस "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { वापस "!"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool fromProperty() स्थिर { वापस m_fromProperty; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstMostSetBitP1 final : खुला Ast_गाँठ_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
खुला:
    AstMostSetBitP1(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_MostSetBitP1(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstMostSetBitP1;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opMostSetBitP1(lhs); }
    string emitVerilog() override { वापस "%f$mostsetbitp1(%l)"; }
    string emitC() override { वापस "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 16; }
};
class AstNToI final : खुला Ast_गाँठ_Uniop {
    // String to any-size integral
खुला:
    AstNToI(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opNToI(lhs); }
    string emitVerilog() override { वापस "'(%l)"; }
    string emitC() override { वापस "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
};
class AstNegate final : खुला Ast_गाँठ_Uniop {
    // @astgen makeDfgVertex
खुला:
    AstNegate(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { वापस "%f(- %l)"; }
    string emitC() override { वापस "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() स्थिर override { वापस "(bvneg %l)"; }
    string emitSimpleOperator() override { वापस "-"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
};
class AstNegateD final : खुला Ast_गाँठ_Uniop {
खुला:
    AstNegateD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstNegateD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { वापस "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { वापस "-"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool doubleFlavor() स्थिर override { वापस true; }
};
class AstNot final : खुला Ast_गाँठ_Uniop {
    // @astgen makeDfgVertex
खुला:
    AstNot(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { वापस "%f(~ %l)"; }
    string emitC() override { वापस "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() स्थिर override { वापस "(bvnot %l)"; }
    string emitSimpleOperator() override { वापस "~"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }
    स्थिर अक्षर* widthMismatch() स्थिर override VL_MT_STABLE;
};
class AstNullCheck final : खुला Ast_गाँठ_Uniop {
    // Return LHS after checking that LHS is non-null
खुला:
    AstNullCheck(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { V3ERROR_NA; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 1; }  // Rarely executes
    string emitVerilog() override { वापस "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override { वापस fileline() == samep->fileline(); }
};
class AstOneHot final : खुला Ast_गाँठ_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
खुला:
    AstOneHot(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { वापस "%f$onehot(%l)"; }
    string emitC() override { वापस "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 4; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstOneHot0 final : खुला Ast_गाँठ_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
खुला:
    AstOneHot0(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { वापस "%f$onehot0(%l)"; }
    string emitC() override { वापस "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs() * 3; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstPostDec final : खुला Ast_गाँठ_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
खुला:
    AstPostDec(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { वापस "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool आकार_Matters_Lhs() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : खुला Ast_गाँठ_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
खुला:
    AstPostInc(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { वापस "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool आकार_Matters_Lhs() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : खुला Ast_गाँठ_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
खुला:
    AstPreDec(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { वापस "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool आकार_Matters_Lhs() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : खुला Ast_गाँठ_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
खुला:
    AstPreInc(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { वापस "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() स्थिर override { V3ERROR_NA_RETURN(false); }
    bool आकार_Matters_Lhs() स्थिर override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : खुला Ast_गाँठ_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
खुला:
    AstRToIRoundS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { वापस "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        वापस isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstRToIS final : खुला Ast_गाँठ_Uniop {
    // $rtoi(lhs)
खुला:
    AstRToIS(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { वापस "%f$rtoi(%l)"; }
    string emitC() override { वापस "VL_RTOI_I_D(%li)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }  // Eliminated before matters
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstRealToBits final : खुला Ast_गाँठ_Uniop {
खुला:
    AstRealToBits(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstRealToBits;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { वापस "%f$realtobits(%l)"; }
    string emitC() override { वापस "VL_CVT_Q_D(%li)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }  // Eliminated before matters
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_DBL; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstRedAnd final : खुला Ast_गाँठ_Uniop {
    // @astgen makeDfgVertex
खुला:
    AstRedAnd(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { वापस "%f(& %l)"; }
    string emitC() override { वापस "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
};
class AstRedOr final : खुला Ast_गाँठ_Uniop {
    // @astgen makeDfgVertex
खुला:
    AstRedOr(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { वापस "%f(| %l)"; }
    string emitC() override { वापस "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
};
class AstRedXor final : खुला Ast_गाँठ_Uniop {
    // @astgen makeDfgVertex
खुला:
    AstRedXor(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { वापस "%f(^ %l)"; }
    string emitC() override { वापस "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override {
        स्थिर पूर्णांक w = lhsp()->width();
        वापस (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 1 + V3_संख्या::log2b(width()); }
};
class AstResizeLValue final : खुला Ast_गाँठ_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
खुला:
    AstResizeLValue(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
};
class AstSigned final : खुला Ast_गाँठ_Uniop {
    // $signed(lhs)
खुला:
    AstSigned(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { वापस "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }  // Eliminated before matters
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool isSystemFunc() स्थिर override { वापस true; }
};
class AstTimeImport final : खुला Ast_गाँठ_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
खुला:
    AstTimeImport(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }
};
class AstToLowerN final : खुला Ast_गाँठ_Uniop {
    // string.tolower()
खुला:
    AstToLowerN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToLowerN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { वापस "%l.tolower()"; }
    string emitC() override { वापस "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
};
class AstToStringN final : खुला Ast_गाँठ_Uniop {
खुला:
    AstToStringN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_ToStringN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToStringN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { वापस "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        वापस isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
};
class AstToUpperN final : खुला Ast_गाँठ_Uniop {
    // string.toupper()
खुला:
    AstToUpperN(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToUpperN;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { वापस "%l.toupper()"; }
    string emitC() override { वापस "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() स्थिर override { वापस true; }
    bool cleanLhs() स्थिर override { वापस true; }
    bool आकार_Matters_Lhs() स्थिर override { वापस false; }
};
class AstUnsigned final : खुला Ast_गाँठ_Uniop {
    // $unsigned(lhs)
खुला:
    AstUnsigned(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { वापस "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस false; }
    bool cleanLhs() स्थिर override { वापस false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() स्थिर override { वापस true; }  // Eliminated before matters
    पूर्णांक instr_गिनती() स्थिर override { वापस 0; }
    bool isSystemFunc() स्थिर override { वापस true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstAcosD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$acos(%l)"; }
    string emitC() override { वापस "acos(%li)"; }
};
class AstAcoshD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstAcoshD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$acosh(%l)"; }
    string emitC() override { वापस "acosh(%li)"; }
};
class AstAsinD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstAsinD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$asin(%l)"; }
    string emitC() override { वापस "asin(%li)"; }
};
class AstAsinhD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstAsinhD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$asinh(%l)"; }
    string emitC() override { वापस "asinh(%li)"; }
};
class AstAtanD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstAtanD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$atan(%l)"; }
    string emitC() override { वापस "atan(%li)"; }
};
class AstAtanhD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstAtanhD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$atanh(%l)"; }
    string emitC() override { वापस "atanh(%li)"; }
};
class AstCeilD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstCeilD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::उच्चमान(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$ceil(%l)"; }
    string emitC() override { वापस "ceil(%li)"; }
};
class AstCosD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstCosD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$cos(%l)"; }
    string emitC() override { वापस "cos(%li)"; }
};
class AstCoshD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstCoshD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$cosh(%l)"; }
    string emitC() override { वापस "cosh(%li)"; }
};
class AstExpD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstExpD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$exp(%l)"; }
    string emitC() override { वापस "exp(%li)"; }
};
class AstFloorD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstFloorD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::न्यूनमान(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$floor(%l)"; }
    string emitC() override { वापस "floor(%li)"; }
};
class AstLog10D final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstLog10D(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$log10(%l)"; }
    string emitC() override { वापस "log10(%li)"; }
};
class AstLogD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstLogD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$ln(%l)"; }
    string emitC() override { वापस "log(%li)"; }
};
class AstSinD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstSinD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$sin(%l)"; }
    string emitC() override { वापस "sin(%li)"; }
};
class AstSinhD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstSinhD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$sinh(%l)"; }
    string emitC() override { वापस "sinh(%li)"; }
};
class AstSqrtD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstSqrtD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::वर्ग_मूल(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$sqrt(%l)"; }
    string emitC() override { वापस "sqrt(%li)"; }
};
class AstTanD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstTanD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$tan(%l)"; }
    string emitC() override { वापस "tan(%li)"; }
};
class AstTanhD final : खुला Ast_गाँठ_System_Uniop_D {
खुला:
    AstTanhD(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    व्योम संख्या_Operate(V3_संख्या& out, स्थिर V3_संख्या& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { वापस "%f$tanh(%l)"; }
    string emitC() override { वापस "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : खुला Ast_गाँठ_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
खुला:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    अंतरभूत AstVarRef(File_पंक्ति* fl, AstVar* varp, स्थिर VAccess& access);
    अंतरभूत AstVarRef(File_पंक्ति* fl, Ast_गाँठ_Module* classOrPackagep, AstVar* varp,
                     स्थिर VAccess& access);
    // This form only allowed post-link (see above)
    अंतरभूत AstVarRef(File_पंक्ति* fl, AstVarScope* varscp, स्थिर VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() स्थिर { वापस m_name; }
    अंतरभूत string name() स्थिर override;  // * = Var name
    व्योम name(स्थिर string& name) override { m_name = name; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    स्थिर अक्षर* broken() स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    अंतरभूत bool same_गाँठ(स्थिर AstVarRef* samep) स्थिर;
    अंतरभूत bool sameNoLvalue(स्थिर AstVarRef* samep) स्थिर;
    पूर्णांक instr_गिनती() स्थिर override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
};
class AstVarXRef final : खुला Ast_गाँठ_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
खुला:
    AstVarXRef(File_पंक्ति* fl, स्थिर string& name, स्थिर string& dotted, स्थिर VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    अंतरभूत AstVarXRef(File_पंक्ति* fl, AstVar* varp, स्थिर string& dotted, स्थिर VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    व्योम name(स्थिर std::string& name) override { m_name = name; }  // * = Var name
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string dotted() स्थिर { वापस m_dotted; }
    व्योम dotted(स्थिर string& dotted) { m_dotted = dotted; }
    string inlinedDots() स्थिर { वापस m_inlinedDots; }
    व्योम inlinedDots(स्थिर string& flag) { m_inlinedDots = flag; }
    bool containsGenBlock() स्थिर { वापस m_containsGenBlock; }
    व्योम containsGenBlock(स्थिर bool flag) { m_containsGenBlock = flag; }
    bool readOnlyModport() स्थिर { वापस m_readOnlyModport; }
    व्योम readOnlyModport(स्थिर bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() स्थिर override { वापस true; }
    पूर्णांक instr_गिनती() स्थिर override { वापस widthInstrs(); }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        वापस (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#पूर्ण_अगर  // Guard
