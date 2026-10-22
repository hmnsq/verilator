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
#അന്തര്ഭാവ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_കണ്ണി_വ്യഞ്ജകം VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // An expression tree node
സംരക്ഷിത:
    Ast_കണ്ണി_വ്യഞ്ജകം(VNType t, File_വരി* fl)
        : Ast_കണ്ണി{t, fl} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_വ്യഞ്ജകം;
    // METHODS
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    // Every expression must have a data type after V3Width
    bool hasDType() സ്ഥിരം override final VL_MT_SAFE { തിരികെ true; }
    മായാ string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    മായാ string emitC() = 0;
    മായാ string emitSMT() സ്ഥിരം { തിരികെ ""; };
    മായാ string emitSimpleOperator() { തിരികെ ""; }  // "" means not ok to use
    മായാ bool emit_Check_Max_വാക്കുകൾ() { തിരികെ false; }  // Check VL_MULS_MAX_WORDS
    മായാ bool cleanOut() സ്ഥിരം = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    മായാ bool isMultiCycleSva() സ്ഥിരം { തിരികെ false; }
    സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* getVAccessTargetRecurse() സ്ഥിരം;
    Ast_കണ്ണി_വ്യഞ്ജകം* getVAccessTargetRecurse() {
        തിരികെ const_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() സ്ഥിരം;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() സ്ഥിരം { തിരികെ getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_വൃക്ഷം() സ്ഥിരം;
    // Return base var (or const) nodep dereferences
    Ast_കണ്ണി* baseFromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    വരിയിൽ Ast_Stmt_വ്യഞ്ജകം* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    മായാ സ്ഥിരം അക്ഷരം* widthMismatch() സ്ഥിരം VL_MT_STABLE { തിരികെ nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string patternString(uint32_t depth = 0) സ്ഥിരം;
    bool isFourstateOrNoDtype() സ്ഥിരം {
        എങ്കിൽ (!dtypep()) തിരികെ true;
        തിരികെ dtypep()->skipRefp()->isFourstate();
    }
    സ്ഥിതം bool isFourstateOrNoDtype(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p) {
        തിരികെ expr1p->isFourstateOrNoDtype();
    }
    സ്ഥിതം bool isFourstateOrNoDtype(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p,
                                     സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr2p) {
        തിരികെ expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    സ്ഥിതം bool isFourstateOrNoDtype(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p,
                                     സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr2p,
                                     സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr3p) {
        തിരികെ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    സ്ഥിതം bool isFourstateOrNoDtype(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p,
                                     സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr2p,
                                     സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr3p,
                                     സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr4p) {
        തിരികെ isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_കണ്ണി_Biop VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

സംരക്ഷിത:
    Ast_കണ്ണി_Biop(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    മായാ ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) = 0;
    മായാ bool cleanLhs() സ്ഥിരം = 0;  // True if LHS must have extra upper bits zero
    മായാ bool cleanRhs() സ്ഥിരം = 0;  // True if RHS must have extra upper bits zero
    മായാ bool sizeMattersLhs() സ്ഥിരം = 0;  // True if output result depends on lhs size
    മായാ bool sizeMattersRhs() സ്ഥിരം = 0;  // True if output result depends on rhs size
    മായാ bool doubleFlavor() സ്ഥിരം { തിരികെ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    മായാ bool signedFlavor() സ്ഥിരം { തിരികെ false; }
    മായാ bool stringFlavor() സ്ഥിരം { തിരികെ false; }  // N flavor of nodes with both flavors?
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    bool isPure() override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;

സ്വകാര്യ:
    bool getPurityRecurse() സ്ഥിരം { തിരികെ lhsp()->isPure() && rhsp()->isPure(); }
};
class Ast_കണ്ണി_Bi_Com VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Biop {
    // Binary expr with commutative properties
സംരക്ഷിത:
    Ast_കണ്ണി_Bi_Com(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhs, Ast_കണ്ണി_വ്യഞ്ജകം* rhs)
        : Ast_കണ്ണി_Biop{t, fl, lhs, rhs} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Bi_Com;
};
class Ast_കണ്ണി_Bi_Com_Asv VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Bi_Com {
    // Binary expr with commutative & associative properties
സംരക്ഷിത:
    Ast_കണ്ണി_Bi_Com_Asv(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhs, Ast_കണ്ണി_വ്യഞ്ജകം* rhs)
        : Ast_കണ്ണി_Bi_Com{t, fl, lhs, rhs} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Bi_Com_Asv;
};
class Ast_കണ്ണി_Dist_Biop VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    Ast_കണ്ണി_Dist_Biop(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : Ast_കണ്ണി_Biop{t, fl, lhsp, rhsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_കണ്ണി_Dist_Biop;
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL_TRIG; }
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
};
class Ast_കണ്ണി_Sel VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
സംരക്ഷിത:
    Ast_കണ്ണി_Sel(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* bitp)
        : Ast_കണ്ണി_Biop{t, fl, fromp, bitp} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Sel;
    പൂർണ്ണസംഖ്യ bitConst() സ്ഥിരം;
};
class Ast_കണ്ണി_Stream VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
സംരക്ഷിത:
    Ast_കണ്ണി_Stream(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : Ast_കണ്ണി_Biop{t, fl, lhsp, rhsp} {
        എങ്കിൽ (lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(lhsp->width(), lhsp->widthMin(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Stream;
};
class Ast_കണ്ണി_System_Biop_D VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    Ast_കണ്ണി_System_Biop_D(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : Ast_കണ്ണി_Biop{t, fl, lhsp, rhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_കണ്ണി_System_Biop_D;
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL_TRIG; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class Ast_കണ്ണി_CCall VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

സംരക്ഷിത:
    Ast_കണ്ണി_CCall(VNType t, File_വരി* fl, AstCFunc* funcp, Ast_കണ്ണി_വ്യഞ്ജകം* argsp = nullptr)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl}
        , m_funcp{funcp} {
        addArgsp(argsp);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_CCall;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_CALL; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം Ast_കണ്ണി_CCall* സ്ഥിരം asamep = VN_DBG_AS(samep, കണ്ണി_CCall);
        തിരികെ (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override;
    bool isOutputter() override { തിരികെ !isPure(); }
    AstCFunc* funcp() സ്ഥിരം { തിരികെ m_funcp; }
    ശൂന്യം funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() സ്ഥിരം { തിരികെ m_argTypes; }
    ശൂന്യം argTypes(സ്ഥിരം string& str) { m_argTypes = str; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം final override { തിരികെ true; }
    bool superReference() സ്ഥിരം { തിരികെ m_superReference; }
    ശൂന്യം superReference(bool flag) { m_superReference = flag; }
};
class Ast_കണ്ണി_Cons VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Construct a value of an aggregate type
    friend class Ast_കണ്ണി_വ്യഞ്ജകം;  // For m_knownConstantTree
    mutable bool m_known_Constant_വൃക്ഷം = false;  // Known to be 'this->isConstantTree() == true'

സംരക്ഷിത:
    Ast_കണ്ണി_Cons(VNType t, File_വരി* fl)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Cons;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം final override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം final override { തിരികെ widthInstrs(); }
};
class Ast_കണ്ണി_FTask_Ref VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_കണ്ണി_FTask_Ref;  // Gen pointers before other members for performance
സ്വകാര്യ:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

സംരക്ഷിത:
    Ast_കണ്ണി_FTask_Ref(VNType t, File_വരി* fl, സ്ഥിരം string& name, AstArg* argsp)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl}
        , m_name{name} {
        addArgsp(argsp);
    }

പൊതു:
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    bool isGateOptimizable() സ്ഥിരം override;
    string dotted() സ്ഥിരം { തിരികെ m_dotted; }  // * = Scope name or ""
    string inlinedDots() സ്ഥിരം { തിരികെ m_inlinedDots; }
    ശൂന്യം inlinedDots(സ്ഥിരം string& flag) { m_inlinedDots = flag; }
    Ast_കണ്ണി_FTask* taskp() സ്ഥിരം { തിരികെ m_taskp; }  // [After Link] Pointer to variable
    ശൂന്യം taskp(Ast_കണ്ണി_FTask* taskp) { m_taskp = taskp; }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    ശൂന്യം dotted(സ്ഥിരം string& name) { m_dotted = name; }
    Ast_കണ്ണി_Module* classOrPackagep() സ്ഥിരം { തിരികെ m_classOrPackagep; }
    ശൂന്യം classOrPackagep(Ast_കണ്ണി_Module* nodep) { m_classOrPackagep = nodep; }
    bool containsGenBlock() സ്ഥിരം { തിരികെ m_containsGenBlock; }
    ശൂന്യം containsGenBlock(സ്ഥിരം bool flag) { m_containsGenBlock = flag; }
    bool pli() സ്ഥിരം { തിരികെ m_pli; }
    ശൂന്യം pli(bool flag) { m_pli = flag; }
    bool superReference() സ്ഥിരം { തിരികെ m_superReference; }
    ശൂന്യം superReference(bool flag) { m_superReference = flag; }
    bool isPure() override;
    bool isUnlikely() സ്ഥിരം override { തിരികെ m_taskp && m_taskp->isUnlikely(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം Ast_കണ്ണി_FTask_Ref* സ്ഥിരം asamep = VN_DBG_AS(samep, കണ്ണി_FTask_Ref);
        തിരികെ taskp() == asamep->taskp()  //
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
    bool cleanOut() സ്ഥിരം final override { V3ERROR_NA_RETURN(true); }

സ്വകാര്യ:
    bool getPurityRecurse() സ്ഥിരം;
};
class Ast_കണ്ണി_Pre_Sel VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

സംരക്ഷിത:
    Ast_കണ്ണി_Pre_Sel(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി_വ്യഞ്ജകം* thsp)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Pre_Sel;
    // METHODS
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം final override { V3ERROR_NA_RETURN(true); }
    bool isPure() override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;

സ്വകാര്യ:
    bool getPurityRecurse() സ്ഥിരം {
        തിരികെ fromp()->isPure() && rhsp()->isPure() && (!thsp() || thsp()->isPure());
    }
};
class Ast_കണ്ണി_Quadop VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

സംരക്ഷിത:
    Ast_കണ്ണി_Quadop(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി_വ്യഞ്ജകം* thsp,
                  Ast_കണ്ണി_വ്യഞ്ജകം* fhsp)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    മായാ ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs,
                               സ്ഥിരം V3_സംഖ്യ& ths, സ്ഥിരം V3_സംഖ്യ& fhs)
        = 0;
    മായാ bool cleanLhs() സ്ഥിരം = 0;  // True if LHS must have extra upper bits zero
    മായാ bool cleanRhs() സ്ഥിരം = 0;  // True if RHS must have extra upper bits zero
    മായാ bool cleanThs() സ്ഥിരം = 0;  // True if THS must have extra upper bits zero
    മായാ bool cleanFhs() സ്ഥിരം = 0;  // True if THS must have extra upper bits zero
    മായാ bool sizeMattersLhs() സ്ഥിരം = 0;  // True if output result depends on lhs size
    മായാ bool sizeMattersRhs() സ്ഥിരം = 0;  // True if output result depends on rhs size
    മായാ bool sizeMattersThs() സ്ഥിരം = 0;  // True if output result depends on ths size
    മായാ bool sizeMattersFhs() സ്ഥിരം = 0;  // True if output result depends on ths size
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    bool isPure() override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;

സ്വകാര്യ:
    bool getPurityRecurse() സ്ഥിരം {
        തിരികെ lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure() && fhsp()->isPure();
    }
};
class Ast_കണ്ണി_Termop VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Terminal operator -- an operator with no "inputs"
സംരക്ഷിത:
    Ast_കണ്ണി_Termop(VNType t, File_വരി* fl)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Termop;
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class Ast_കണ്ണി_Triop VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

സംരക്ഷിത:
    Ast_കണ്ണി_Triop(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി_വ്യഞ്ജകം* thsp)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Triop;
    // METHODS
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    // Set out to evaluation of a AstConst'ed
    മായാ ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs,
                               സ്ഥിരം V3_സംഖ്യ& ths)
        = 0;
    മായാ bool cleanLhs() സ്ഥിരം = 0;  // True if LHS must have extra upper bits zero
    മായാ bool cleanRhs() സ്ഥിരം = 0;  // True if RHS must have extra upper bits zero
    മായാ bool cleanThs() സ്ഥിരം = 0;  // True if THS must have extra upper bits zero
    മായാ bool sizeMattersLhs() സ്ഥിരം = 0;  // True if output result depends on lhs size
    മായാ bool sizeMattersRhs() സ്ഥിരം = 0;  // True if output result depends on rhs size
    മായാ bool sizeMattersThs() സ്ഥിരം = 0;  // True if output result depends on ths size
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    bool isPure() override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;

സ്വകാര്യ:
    bool getPurityRecurse() സ്ഥിരം {
        തിരികെ lhsp()->isPure() && rhsp()->isPure() && thsp()->isPure();
    }
};
class Ast_കണ്ണി_Dist_Triop VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Triop {
പൊതു:
    Ast_കണ്ണി_Dist_Triop(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp,
                     Ast_കണ്ണി_വ്യഞ്ജകം* thsp)
        : Ast_കണ്ണി_Triop{t, fl, lhsp, rhsp, thsp} {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_Ast_കണ്ണി_Dist_Triop;
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanThs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersThs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL_TRIG; }
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs,
                       സ്ഥിരം V3_സംഖ്യ& ths) override {
        V3ERROR_NA;
    }
};
class Ast_കണ്ണി_Uniop VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

സംരക്ഷിത:
    Ast_കണ്ണി_Uniop(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Uniop;
    // METHODS
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    // Set out to evaluation of a AstConst'ed lhs
    മായാ ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) = 0;
    മായാ bool cleanLhs() സ്ഥിരം = 0;
    മായാ bool sizeMattersLhs() സ്ഥിരം = 0;  // True if output result depends on lhs size
    മായാ bool doubleFlavor() സ്ഥിരം { തിരികെ false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    മായാ bool signedFlavor() സ്ഥിരം { തിരികെ false; }
    മായാ bool stringFlavor() സ്ഥിരം { തിരികെ false; }  // N flavor of nodes with both flavors?
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    bool isPure() override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
};
class Ast_കണ്ണി_System_Uniop_D VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Uniop {
പൊതു:
    Ast_കണ്ണി_System_Uniop_D(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : Ast_കണ്ണി_Uniop{t, fl, lhsp} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_Ast_കണ്ണി_System_Uniop_D;
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL_TRIG; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class Ast_കണ്ണി_Var_Ref VL_NOT_FINAL : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_കണ്ണി_Var_Ref;  // Gen pointers before other members for performance
സ്വകാര്യ:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
സംരക്ഷിത:
    Ast_കണ്ണി_Var_Ref(VNType t, File_വരി* fl, AstVar* varp, സ്ഥിരം VAccess& access)
        : Ast_കണ്ണി_വ്യഞ്ജകം{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

പൊതു:
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    VAccess access() സ്ഥിരം { തിരികെ m_access; }
    ശൂന്യം access(സ്ഥിരം VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_varp; }  // [After Link] Pointer to variable
    ശൂന്യം varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_കണ്ണി*>(varp));
    }
    AstVarScope* varScopep() സ്ഥിരം { തിരികെ m_varScopep; }
    ശൂന്യം varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    സ്ഥിരം VSelfPointerText& selfPointer() സ്ഥിരം { തിരികെ m_selfPointer; }
    ശൂന്യം selfPointer(സ്ഥിരം VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) സ്ഥിരം {
        തിരികെ selfPointer().protect(useSelfForThis, protect());
    }
    Ast_കണ്ണി_Module* classOrPackagep() സ്ഥിരം { തിരികെ m_classOrPackagep; }
    ശൂന്യം classOrPackagep(Ast_കണ്ണി_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : പൊതു Ast_കണ്ണി {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
പൊതു:
    AstArg(File_വരി* fl, സ്ഥിരം std::string& name, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം std::string& name) override { m_name = name; }
    bool emptyConnectNoNext() സ്ഥിരം { തിരികെ !exprp() && name() == "" && !nextp(); }
};
class AstWith final : പൊതു Ast_കണ്ണി {
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
സ്വകാര്യ:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::set<std::string> m_restrictedNames;

പൊതു:
    AstWith(File_വരി* fl, AstLambdaArgRef* indexArgRefp, AstLambdaArgRef* മൂല്യം_Arg_Refp,
            Ast_കണ്ണി* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->indexArgRefp(indexArgRefp);
        this->മൂല്യം_Arg_Refp(മൂല്യം_Arg_Refp);
        addExprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!indexArgRefp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!മൂല്യം_Arg_Refp());  // varp needed to know lambda's arg dtype
        തിരികെ nullptr;
    }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം restricted(bool flag) { m_restricted = flag; }
    bool restricted() സ്ഥിരം { തിരികെ m_restricted; }
    bool validated() സ്ഥിരം { തിരികെ m_validated; }
    ശൂന്യം validated(bool flag) { m_validated = flag; }
    ശൂന്യം addRestrictedName(സ്ഥിരം std::string& name) { m_restrictedNames.തിരുകുക(name); }
    സ്ഥിരം std::set<std::string>& restrictedNames() സ്ഥിരം { തിരികെ m_restrictedNames; }
    // True if 'name' binds into the randomize() target class.
    bool nameResolvesToTarget(സ്ഥിരം std::string& name) സ്ഥിരം {
        തിരികെ !m_restricted || m_restrictedNames.എണ്ണം(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbortKind m_kind{VAbortKind::ACCEPT_ON};

പൊതു:
    AstAbortOn(File_വരി* fl, VAbortKind kind, Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി_വ്യഞ്ജകം* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_kind{kind} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    VAbortKind kind() സ്ഥിരം { തിരികെ m_kind; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstAddrOfCFunc final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

പൊതു:
    AstAddrOfCFunc(File_വരി* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_AstAddrOfCFunc;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    AstCFunc* funcp() സ്ഥിരം { തിരികെ m_funcp; }
};
class AstAttrOf final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
പൊതു:
    AstAttrOf(File_വരി* fl, VAttrType attrtype, Ast_കണ്ണി* fromp = nullptr,
              Ast_കണ്ണി_വ്യഞ്ജകം* dimp = nullptr)
        : ASTGEN_SUPER_AttrOf(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_AstAttrOf;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    VAttrType attrType() സ്ഥിരം { തിരികെ m_attrType; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    സ്ഥിരം bool m_pure;  // Pure optimizable

    ശൂന്യം init(സ്ഥിരം string& text, പൂർണ്ണസംഖ്യ setwidth) {
        എങ്കിൽ (!text.empty()) add(text);
        എങ്കിൽ (setwidth) {
            dtypeSetBitSized(setwidth, VSigning::UNSIGNED);
        } അല്ലെങ്കിൽ {
            dtypeSetVoid();  // Caller to override if necessary
        }
    }

പൊതു:
    class Pure {};
    explicit AstCExpr(File_വരി* fl, സ്ഥിരം string& text = "", പൂർണ്ണസംഖ്യ setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{false} {
        init(text, setwidth);
    }
    AstCExpr(File_വരി* fl, Pure, സ്ഥിരം string& text = "", പൂർണ്ണസംഖ്യ setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_pure{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ m_pure; }
    bool isOutputter() override { തിരികെ true; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ m_pure; }
    bool isPure() override { തിരികെ m_pure; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    // Add some text, or a node to this expression
    ശൂന്യം add(സ്ഥിരം std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ശൂന്യം add(Ast_കണ്ണി* nodep) { addNodesp(nodep); }
};
class AstCExprUser final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    സ്ഥിരം bool m_pure;  // Whether the function is pure
പൊതു:
    class Pure {};
    explicit AstCExprUser(File_വരി* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{false} {}
    AstCExprUser(File_വരി* fl, Pure)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_pure{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ m_pure; }
    bool isOutputter() override { തിരികെ true; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ m_pure; }
    bool isPure() override { തിരികെ m_pure; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    // Add some text, or a node to this expression
    ശൂന്യം add(സ്ഥിരം std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ശൂന്യം add(Ast_കണ്ണി* nodep) { addNodesp(nodep); }
};
class AstCFuncHard final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
പൊതു:
    AstCFuncHard(File_വരി* fl, VCFunction function, Ast_കണ്ണി_വ്യഞ്ജകം* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ function().ascii(); }
    bool isPure() override {
        എങ്കിൽ (!m_purity.isCached()) m_purity.set(getPurity());
        തിരികെ m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    VCFunction function() സ്ഥിരം VL_MT_STABLE { തിരികെ m_function; }

സ്വകാര്യ:
    bool getPurity();
};
class AstCMethodHard final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_method;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
പൊതു:
    AstCMethodHard(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, VCMethod method, Ast_കണ്ണി_വ്യഞ്ജകം* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_method{method} {
        this->fromp(fromp);
        addPinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ method().ascii(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstCMethodHard* സ്ഥിരം asamep = VN_DBG_AS(samep, CMethodHard);
        തിരികെ (m_method == asamep->m_method);
    }
    bool isPure() override {
        എങ്കിൽ (!m_purity.isCached()) m_purity.set(getPurity());
        തിരികെ m_purity.get();
    }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ m_method.isClean(); }
    bool usePtr() സ്ഥിരം { തിരികെ m_usePtr; }
    ശൂന്യം usePtr(bool flag) { m_usePtr = flag; }
    VCMethod method() സ്ഥിരം VL_MT_STABLE { തിരികെ m_method; }
    ശൂന്യം method(VCMethod മൂല്യം) { m_method = മൂല്യം; }

സ്വകാര്യ:
    bool getPurity();
};
class AstCReset final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Reset variable at startup
    സ്ഥിരം bool m_constructing;  // Previously cleared by constructor
പൊതു:
    AstCReset(File_വരി* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isPure() override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!VN_IS(backp(), കണ്ണി_Assign));  // V3Emit* assumption
        തിരികെ nullptr;
    }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() സ്ഥിരം { തിരികെ m_constructing; }
};
class AstCast final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
പൊതു:
    AstCast(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, VFlagChildDType, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { തിരികെ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
    Ast_കണ്ണി_DType* getChildDTypep() സ്ഥിരം override { തിരികെ childDTypep(); }
    Ast_കണ്ണി_DType* subDTypep() സ്ഥിരം VL_MT_STABLE { തിരികെ dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
പൊതു:
    AstCastParse(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { തിരികെ "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstCastSize final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
പൊതു:
    AstCastSize(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_CastSize(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCastSize;
    string emitVerilog() override { തിരികെ "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
പൊതു:
    AstCellArrayRef(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി_വ്യഞ്ജകം* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        addSelp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
പൊതു:
    AstCellRef(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* cellp, Ast_കണ്ണി* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
പൊതു:
    AstClassOrPackageRef(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_കണ്ണി_Module* classOrPackageSkipp(സ്ഥിരം bool doRefs = true) സ്ഥിരം;
    Ast_കണ്ണി* classOrPackageNodep() സ്ഥിരം { തിരികെ m_classOrPackageNodep; }
    ശൂന്യം classOrPackageNodep(Ast_കണ്ണി* nodep) { m_classOrPackageNodep = nodep; }
    ശൂന്യം classOrPackagep(Ast_കണ്ണി_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_കണ്ണി*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A constant
    V3_സംഖ്യ m_അക്കം;  // Constant value
    ശൂന്യം init_With_സംഖ്യ() {
        എങ്കിൽ (m_അക്കം.isDouble()) {
            dtypeSetDouble();
        } അല്ലെങ്കിൽ എങ്കിൽ (m_അക്കം.isString()) {
            dtypeSetString();
        } അല്ലെങ്കിൽ {
            dtypeSetBitOrLogicUnsized(m_അക്കം.width(), (m_അക്കം.sized() ? 0 : m_അക്കം.widthToFit()),
                                      VSigning::fromBool(m_അക്കം.isSigned()), m_അക്കം.isAnyXZ());
        }
        m_അക്കം.nodep(this);
    }

പൊതു:
    ~AstConst() override;
    AstConst(File_വരി* fl, സ്ഥിരം V3_സംഖ്യ& അക്കം)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{അക്കം} {
        init_With_സംഖ്യ();
    }
    class Widthed_മൂല്യം {};  // for creator type-overload selection
    AstConst(File_വരി* fl, Widthed_മൂല്യം, പൂർണ്ണസംഖ്യ width, uint32_t മൂല്യം)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, width, മൂല്യം) {  // Need () constructor
        init_With_സംഖ്യ();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_വരി* fl, DTyped, സ്ഥിരം Ast_കണ്ണി_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{this, nodedtypep} {
        init_With_സംഖ്യ();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(File_വരി* fl, StringToParse, സ്ഥിരം അക്ഷരം* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{this, V3_സംഖ്യ::Verilog_സംഖ്യ_Literal{}, sourcep} {
        init_With_സംഖ്യ();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_വരി* fl, VerilogStringLiteral, സ്ഥിരം string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{this, V3_സംഖ്യ::VerilogStringLiteral{}, str} {
        init_With_സംഖ്യ();
    }
    AstConst(File_വരി* fl, uint32_t അക്കം)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, 32, അക്കം) {  // Need () constructor
        dtypeSetBitUnsized(m_അക്കം.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_വരി* fl, Unsized32, uint32_t അക്കം)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, 32, അക്കം) {  // Need () constructor
        m_അക്കം.width(32, false);
        dtypeSetBitUnsized(32, m_അക്കം.widthToFit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_വരി* fl, Signed32, int32_t അക്കം)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, 32, അക്കം) {  // Need () constructor
        m_അക്കം.width(32, true);
        dtypeSetBitUnsized(32, m_അക്കം.widthToFit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_വരി* fl, Unsized64, uint64_t അക്കം)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, 64, 0) {  // Need () constructor
        m_അക്കം.setQuad(അക്കം);
        dtypeSetBitSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_വരി* fl, SizedEData, uint64_t അക്കം)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_അക്കം.setQuad(അക്കം);
        dtypeSetBitSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_വരി* fl, RealDouble, ഇരട്ട അക്കം)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, 64) {  // Need () constructor
        m_അക്കം.setDouble(അക്കം);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_വരി* fl, String, സ്ഥിരം string& അക്കം)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{this, V3_സംഖ്യ::String{}, അക്കം} {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(File_വരി* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_വരി* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, 1, 0) {  // Need () constructor
        dtypeSetBit();
        File_വരി* സ്ഥിരം newfl = new File_വരി{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_വരി* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം(this, 1, on) {  // Need () constructor
        dtypeSetBit();
    }
    class All0 {};
    AstConst(File_വരി* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{this, V3_സംഖ്യ::Verilog_സംഖ്യ_Literal{}, "'0"} {
        init_With_സംഖ്യ();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_വരി* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{this, V3_സംഖ്യ::Verilog_സംഖ്യ_Literal{}, "'1"} {
        init_With_സംഖ്യ();
        fl->warnOff(V3ErrorCode::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_വരി* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{this, V3_സംഖ്യ::Null{}} {
        dtypep(findNullDType());
        init_With_സംഖ്യ();
    }
    class OneStep {};
    AstConst(File_വരി* fl, OneStep)
        : ASTGEN_SUPER_Const(fl)
        , m_അക്കം{this, V3_സംഖ്യ::OneStep{}} {
        dtypeSetBitSized(64, VSigning::UNSIGNED);
        init_With_സംഖ്യ();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ അക്കം().ascii(); }  // * = Value
    സ്ഥിരം V3_സംഖ്യ& അക്കം() സ്ഥിരം VL_MT_SAFE { തിരികെ m_അക്കം; }  // * = Value
    V3_സംഖ്യ& അക്കം() { തിരികെ m_അക്കം; }  // * = Value
    string origParamName() സ്ഥിരം;
    ശൂന്യം origParamName(സ്ഥിരം string& name);
    uint32_t toUInt() സ്ഥിരം { തിരികെ അക്കം().toUInt(); }
    int32_t toSInt() സ്ഥിരം VL_MT_SAFE { തിരികെ അക്കം().toSInt(); }
    uint64_t toUQuad() സ്ഥിരം { തിരികെ അക്കം().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstConst* സ്ഥിരം sp = VN_DBG_AS(samep, Const);
        തിരികെ അക്കം().isCaseEq(sp->അക്കം());
    }
    ശൂന്യം cloneRelink() override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(m_അക്കം.nodep() && m_അക്കം.nodep() != this);
        തിരികെ nullptr;
    }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool isEqAllOnes() സ്ഥിരം { തിരികെ അക്കം().isEqAllOnes(width()); }
    bool isEqAllOnesV() സ്ഥിരം { തിരികെ അക്കം().isEqAllOnes(widthMinV()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_മൂല്യം_Type(സ്ഥിരം AstConst* samep) സ്ഥിരം;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    സ്ഥിതം AstConst* parseParamLiteral(File_വരി* fl, സ്ഥിരം string& literal);
};
class AstConstraintRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

പൊതു:
    AstConstraintRef(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() സ്ഥിരം override VL_MT_STABLE;
    AstConstraint* constrp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_constrp; }
    ശൂന്യം constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_കണ്ണി_Module* classOrPackagep() സ്ഥിരം VL_MT_STABLE { തിരികെ m_classOrPackagep; }
    // TODO: this setter is unused
    ശൂന്യം classOrPackagep(Ast_കണ്ണി_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം final override { V3ERROR_NA_RETURN(true); }
};
class AstCvtArrayToArray final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
സ്വകാര്യ:
    സ്ഥിരം bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_blockSize;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_dstElementBits;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_srcElementBits;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

പൊതു:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToArray(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_DType* dtp, bool reverse,
                       പൂർണ്ണസംഖ്യ blockSize, പൂർണ്ണസംഖ്യ dstElementBits, പൂർണ്ണസംഖ്യ srcElementBits)
        : ASTGEN_SUPER_CvtArrayToArray(fl)
        , m_reverse{reverse}
        , m_blockSize{blockSize}
        , m_dstElementBits{dstElementBits}
        , m_srcElementBits{srcElementBits} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToArray;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool reverse() സ്ഥിരം { തിരികെ m_reverse; }
    പൂർണ്ണസംഖ്യ blockSize() സ്ഥിരം { തിരികെ m_blockSize; }
    പൂർണ്ണസംഖ്യ dstElementBits() സ്ഥിരം { തിരികെ m_dstElementBits; }
    പൂർണ്ണസംഖ്യ srcElementBits() സ്ഥിരം { തിരികെ m_srcElementBits; }
};
class AstCvtArrayToPacked final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
പൊതു:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtArrayToPacked(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_CvtArrayToPacked(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtArrayToPacked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
};
class AstCvtPackedToArray final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
പൊതു:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtPackedToArray(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_CvtPackedToArray(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtPackedToArray;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
};
class AstCvtUnpackToQueue final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
പൊതു:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCvtUnpackToQueue(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToQueue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToQueue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { തിരികെ "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
};
class AstCvtUnpackToUnpack final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
പൊതു:
    AstCvtUnpackToUnpack(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_CvtUnpackToUnpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCvtUnpackToUnpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
};
class AstDist final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
പൊതു:
    AstDist(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { തിരികെ "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }  // NA
};
class AstDistItem final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_isWhole = false;  // True for weight ':/', false for ':='
പൊതു:
    AstDistItem(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* rangep, Ast_കണ്ണി_വ്യഞ്ജകം* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { തിരികെ "%l "s + (m_isWhole ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }  // NA
    ശൂന്യം isWhole(bool flag) { m_isWhole = flag; }
    bool isWhole() സ്ഥിരം { തിരികെ m_isWhole; }
};
class AstDot final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    സ്ഥിരം bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
പൊതു:
    AstDot(File_വരി* fl, bool colon, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newIfPkg(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* packageOrClassp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        എങ്കിൽ (!packageOrClassp) തിരികെ rhsp;
        തിരികെ new AstDot{fl, true, packageOrClassp, rhsp};
    }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool colon() സ്ഥിരം { തിരികെ m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstEmptyQueue final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
പൊതു:
    explicit AstEmptyQueue(File_വരി* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_AstEmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { തിരികെ "{}"; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
};
class AstEnumItemRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_containsGenBlock = false;  // Contains gen block reference
പൊതു:
    AstEnumItemRef(File_വരി* fl, AstEnumItem* itemp, Ast_കണ്ണി_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ itemp() ? itemp()->name() : m_name; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstEnumItemRef* സ്ഥിരം sp = VN_DBG_AS(samep, EnumItemRef);
        തിരികെ itemp() == sp->itemp() && dotted() == sp->dotted()
               && containsGenBlock() == sp->containsGenBlock();
    }
    AstEnumItem* itemp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_itemp; }
    ശൂന്യം itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() സ്ഥിരം { തിരികെ m_dotted; }
    ശൂന്യം dotted(സ്ഥിരം string& dotted) { m_dotted = dotted; }
    bool containsGenBlock() സ്ഥിരം { തിരികെ m_containsGenBlock; }
    ശൂന്യം containsGenBlock(സ്ഥിരം bool flag) { m_containsGenBlock = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    Ast_കണ്ണി_Module* classOrPackagep() സ്ഥിരം { തിരികെ m_classOrPackagep; }
    ശൂന്യം classOrPackagep(Ast_കണ്ണി_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_വ്യഞ്ജകം_Stmt final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
സ്വകാര്യ:
    bool m_hasResult = true;  // Returns result via resultp()

പൊതു:
    Ast_വ്യഞ്ജകം_Stmt(File_വരി* fl, Ast_കണ്ണി* stmtsp, Ast_കണ്ണി_വ്യഞ്ജകം* resultp)
        : ASTGEN_SUPER_വ്യഞ്ജകം_Stmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_വ്യഞ്ജകം_Stmt;
    // METHODS
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool isPure() override {
        എങ്കിൽ (Ast_കണ്ണി::afterCommentp(stmtsp())) തിരികെ false;
        തിരികെ resultp()->isPure();
    }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    bool hasResult() സ്ഥിരം { തിരികെ m_hasResult; }
    ശൂന്യം hasResult(bool flag) { m_hasResult = flag; }
};
class AstFError final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
പൊതു:
    AstFError(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* filep, Ast_കണ്ണി_വ്യഞ്ജകം* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { തിരികെ "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 64; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstFOpen final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
പൊതു:
    AstFOpen(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* filenamep, Ast_കണ്ണി_വ്യഞ്ജകം* modep)
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
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    string verilogKwd() സ്ഥിരം override { തിരികെ "$fopen"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstFOpenMcd final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op2 := filenamep : AstNodeExpr
പൊതു:
    AstFOpenMcd(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    string verilogKwd() സ്ഥിരം override { തിരികെ "$fopen"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstFRead final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
പൊതു:
    AstFRead(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* memp, Ast_കണ്ണി_വ്യഞ്ജകം* filep, Ast_കണ്ണി_വ്യഞ്ജകം* startp,
             Ast_കണ്ണി_വ്യഞ്ജകം* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: makes output
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstFRewind final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := filep : Optional[AstNodeExpr]
പൊതു:
    AstFRewind(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstFScanF final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

പൊതു:
    AstFScanF(File_വരി* fl, സ്ഥിരം string& text, Ast_കണ്ണി_വ്യഞ്ജകം* filep, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_text; }
    string verilogKwd() സ്ഥിരം override { തിരികെ "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: makes output
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() സ്ഥിരം { തിരികെ m_text; }  // * = Text to display
    ശൂന്യം text(സ്ഥിരം string& text) { m_text = text; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstFSeek final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
പൊതു:
    AstFSeek(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* filep, Ast_കണ്ണി_വ്യഞ്ജകം* offset, Ast_കണ്ണി_വ്യഞ്ജകം* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: makes output
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstFTell final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
പൊതു:
    AstFTell(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstFalling final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
പൊതു:
    AstFalling(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { തിരികെ "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstFell final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
പൊതു:
    AstFell(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_Sen_വൃക്ഷം* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { തിരികെ "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstFuture final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
പൊതു:
    AstFuture(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_Sen_വൃക്ഷം* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { തിരികെ "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstGatePin final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
പൊതു:
    AstGatePin(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { തിരികെ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
};
class AstGetInitialRandomSeed final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $get_initial_random_seed()
പൊതു:
    explicit AstGetInitialRandomSeed(File_വരി* fl)
        : ASTGEN_SUPER_GetInitialRandomSeed(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstGetInitialRandomSeed;
    string emitVerilog() override { തിരികെ "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ true; }
    bool isPure() override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstImplication final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

സ്വകാര്യ:
    സ്ഥിരം bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    സ്ഥിരം bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

പൊതു:
    AstImplication(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() സ്ഥിരം { തിരികെ m_isOverlapped; }
    bool isFollowedBy() സ്ഥിരം { തിരികെ m_isFollowedBy; }
};
class AstInside final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
പൊതു:
    AstInside(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { തിരികെ "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }  // NA
};
class AstInsideRange final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
പൊതു:
    AstInsideRange(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { തിരികെ "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_കണ്ണി_വ്യഞ്ജകം* newAndFromInside(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp);
};
class AstLambdaArgRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_index;  // Index, not value

പൊതു:
    AstLambdaArgRef(File_വരി* fl, സ്ഥിരം string& name, bool index)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_index{index} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    string emitVerilog() override { തിരികെ name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    bool index() സ്ഥിരം { തിരികെ m_index; }
    bool is_വ്യഞ്ജകം_Coverage_Eligible() സ്ഥിരം override { തിരികെ false; }
};
class AstMatchMasked final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
പൊതു:
    വരിയിൽ AstMatchMasked(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_AstMatchMasked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { തിരികെ "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    സ്ഥിതം uint32_t fold(സ്ഥിരം V3_സംഖ്യ& lhs, AstVar* matchVarp);
};
class AstMatches final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
പൊതു:
    AstMatches(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { തിരികെ "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstMemberSel final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
പൊതു:
    AstMemberSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, VFlagChildDType, സ്ഥിരം string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    VAccess access() സ്ഥിരം { തിരികെ m_access; }
    ശൂന്യം access(സ്ഥിരം VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    AstVar* varp() സ്ഥിരം { തിരികെ m_varp; }
    ശൂന്യം varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
പൊതു:
    AstNewCopy(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { തിരികെ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
};
class AstNewDynamic final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
പൊതു:
    AstNewDynamic(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* sizep, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { തിരികെ "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
};
class AstPExpr final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
പൊതു:
    explicit AstPExpr(File_വരി* fl, AstBegin* bodyp, Ast_കണ്ണി_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_വരി* fl, AstBegin* bodyp, Ast_കണ്ണി_Stmt* finalp, Ast_കണ്ണി_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
};
class AstParseHolder final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
പൊതു:
    explicit AstParseHolder(File_വരി* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

പൊതു:
    AstParseRef(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nullptr,
                Ast_കണ്ണി_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstParseRef* സ്ഥിരം asamep = VN_DBG_AS(samep, ParseRef);
        തിരികെ m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

പൊതു:
    AstPast(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* ticksp = nullptr,
            Ast_Sen_വൃക്ഷം* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() സ്ഥിരം { തിരികെ m_propertyTiming; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstPatMember final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
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

പൊതു:
    AstPatMember(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhssp, Ast_കണ്ണി* keyp, Ast_കണ്ണി_വ്യഞ്ജകം* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { തിരികെ lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 2; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    bool isConcat() സ്ഥിരം { തിരികെ m_isConcat; }
    ശൂന്യം isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() സ്ഥിരം { തിരികെ m_isDefault; }
    ശൂന്യം isDefault(bool flag) { m_isDefault = flag; }
    Ast_കണ്ണി_വ്യഞ്ജകം* താക്കോൽ_Exprp() സ്ഥിരം { തിരികെ varrefp() ? varrefp() : VN_CAST(keyp(), കണ്ണി_വ്യഞ്ജകം); }
};
class AstPattern final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
പൊതു:
    AstPattern(File_വരി* fl, Ast_കണ്ണി* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstPattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    Ast_കണ്ണി_DType* getChildDTypep() സ്ഥിരം override { തിരികെ childDTypep(); }
    Ast_കണ്ണി_DType* subDTypep() സ്ഥിരം VL_MT_STABLE { തിരികെ dtypep() ? dtypep() : childDTypep(); }
};
class AstPatternStar final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Pattern wildcard: ".*"
പൊതു:
    explicit AstPatternStar(File_വരി* fl)
        : ASTGEN_SUPER_PatternStar(fl) {}
    ASTGEN_MEMBERS_AstPatternStar;
    string emitVerilog() override { തിരികെ ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstPatternVar final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
പൊതു:
    AstPatternVar(File_വരി* fl, സ്ഥിരം string& name)
        : ASTGEN_SUPER_PatternVar(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPatternVar;
    string emitVerilog() override { തിരികെ ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_name == VN_DBG_AS(samep, PatternVar)->m_name;
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
};
class AstPropAlways final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    സ്ഥിരം bool m_isStrong = false;  // s_always
പൊതു:
    AstPropAlways(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* propp, Ast_കണ്ണി_വ്യഞ്ജകം* loBoundp, Ast_കണ്ണി_വ്യഞ്ജകം* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { V3ERROR_NA_RETURN(0); }
    bool isStrong() സ്ഥിരം { തിരികെ m_isStrong; }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstRand final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    സ്ഥിരം bool m_urandom;  // $urandom vs $random
പൊതു:
    class Reset {};
    AstRand(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_വരി* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtypeSetUInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override {
        തിരികെ seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        എങ്കിൽ (seedp()) {
            എങ്കിൽ (urandom()) തിരികെ "VL_URANDOM_SEEDED_%nq%lq(%li)";
            തിരികെ "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        എങ്കിൽ (isWide()) തിരികെ "VL_RANDOM_%nq(%nw, %P)";
        തിരികെ "VL_RANDOM_%nq()";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ !seedp(); }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool combinable(സ്ഥിരം AstRand* samep) സ്ഥിരം {
        തിരികെ !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() സ്ഥിരം { തിരികെ m_urandom; }
};
class AstRandRNG final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
പൊതു:
    AstRandRNG(File_വരി* fl, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { തിരികെ "%f$rngrandom()"; }
    string emitC() override {
        തിരികെ isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstRising final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
പൊതു:
    AstRising(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { തിരികെ "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstRose final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
പൊതു:
    AstRose(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_Sen_വൃക്ഷം* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { തിരികെ "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstSClocked final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
പൊതു:
    AstSClocked(File_വരി* fl, AstSenItem* sensesp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
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
    സ്ഥിരം bool m_unbounded = false;  // True when the upper bound is $
പൊതു:
    // Exact [*N]
    AstSConsRep(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* countp, Ast_കണ്ണി_വ്യഞ്ജകം* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { V3ERROR_NA_RETURN(0); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {  // LCOV_EXCL_LINE
        തിരികെ m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() സ്ഥിരം { തിരികെ m_unbounded; }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstSEventually final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
പൊതു:
    explicit AstSEventually(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
പൊതു:
    explicit AstSExpr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* preExprp, Ast_കണ്ണി_Stmt* delayp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_വരി* fl, Ast_കണ്ണി_Stmt* delayp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstSFormatArg final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

പൊതു:
    AstSFormatArg(File_വരി* fl, VFormatAttr formatAttr, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "$sformatarg"; }
    string emitVerilog() override { തിരികെ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        തിരികെ nullptr;
    }
    VFormatAttr formatAttr() സ്ഥിരം { തിരികെ m_formatAttr; }
    ശൂന്യം formatAttr(സ്ഥിരം VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    സ്ഥിതം VFormatAttr formatAttrDefauled(സ്ഥിരം AstSFormatArg* nodep, സ്ഥിരം Ast_കണ്ണി_DType* dtypep);
};
class AstSFormatF final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_വ്യഞ്ജകം_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    സ്ഥിരം അക്ഷരം m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
പൊതു:
    class വ്യഞ്ജകം_Format {};
    AstSFormatF(File_വരി* fl, സ്ഥിരം string& text, bool hidden, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp,
                അക്ഷരം missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    AstSFormatF(File_വരി* fl, വ്യഞ്ജകം_Format, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp, അക്ഷരം missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_വ്യഞ്ജകം_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtypeSetString();
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_text; }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_text = name; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "$sformatf"; }
    string text() സ്ഥിരം { തിരികെ m_text; }  // * = Text to display
    ശൂന്യം text(സ്ഥിരം string& text) { m_text = text; }
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(text() != "" && വ്യഞ്ജകം_Format_2());  // Expr format means no literal format
        തിരികെ nullptr;
    }
    bool formatScopeTracking() സ്ഥിരം {  // Track scopeNamep();  Ok if false positive
        എങ്കിൽ (വ്യഞ്ജകം_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            തിരികെ true;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം fmtp = VN_CAST(exprp, Const)) {
                എങ്കിൽ (fmtp->അക്കം().isFromString()) {
                    സ്ഥിരം string str = fmtp->അക്കം().toString();
                    എങ്കിൽ (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        തിരികെ true;
                }
            }
        }
        തിരികെ false;
    }
    bool hidden() സ്ഥിരം { തിരികെ m_hidden; }
    ശൂന്യം hidden(bool flag) { m_hidden = flag; }
    bool വ്യഞ്ജകം_Format_2() സ്ഥിരം { തിരികെ m_വ്യഞ്ജകം_Format; }
    ശൂന്യം വ്യഞ്ജകം_Format_2(bool flag) { m_വ്യഞ്ജകം_Format = flag; }
    bool optionalFormat() സ്ഥിരം { തിരികെ m_optionalFormat; }
    ശൂന്യം optionalFormat(bool flag) { m_optionalFormat = flag; }
    അക്ഷരം missingArgChar() സ്ഥിരം { തിരികെ m_missingArgChar; }
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstSGotoRep final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
പൊതു:
    // Exact [->N]
    AstSGotoRep(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* countp, Ast_കണ്ണി_വ്യഞ്ജകം* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstSNonConsRep final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
പൊതു:
    // Exact [=N]
    AstSNonConsRep(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* countp, Ast_കണ്ണി_വ്യഞ്ജകം* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstSScanF final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

പൊതു:
    AstSScanF(File_വരി* fl, സ്ഥിരം string& text, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->fromp(fromp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_text; }
    string verilogKwd() സ്ഥിരം override { തിരികെ "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: makes output
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstSScanF* സ്ഥിരം asamep = VN_DBG_AS(samep, SScanF);
        തിരികെ text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() സ്ഥിരം { തിരികെ m_text; }  // * = Text to display
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstSampled final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_internal : 1;  // Internally created, not from a source $sampled
പൊതു:
    AstSampled(File_വരി* fl, Ast_കണ്ണി* exprp, Ast_കണ്ണി_DType* dtypep, bool internal = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_internal{internal} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { തിരികെ "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_internal == VN_DBG_AS(samep, Sampled)->m_internal;
    }
    bool internal() സ്ഥിരം { തിരികെ m_internal; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstScopeName final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    സ്ഥിരം bool m_forFormat;  // Is for a format %m
    സ്ഥിതം std::string scopeNameFormatter(സ്ഥിരം std::string& text);
    സ്ഥിതം std::string scopePrettyNameFormatter(സ്ഥിരം std::string& text);

പൊതു:
    class ForFormat {};
    AstScopeName(File_വരി* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstScopeName* സ്ഥിരം sp = VN_DBG_AS(samep, ScopeName);
        തിരികെ m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    // ACCESSORS
    സ്ഥിരം std::string& scopeAttr() സ്ഥിരം { തിരികെ m_scopeAttr; }
    ശൂന്യം scopeAttr(സ്ഥിരം std::string& val) { m_scopeAttr = val; }
    സ്ഥിരം std::string& scopeEntr() സ്ഥിരം { തിരികെ m_scopeEntr; }
    ശൂന്യം scopeEntr(സ്ഥിരം std::string& val) { m_scopeEntr = val; }
    bool dpiExport() സ്ഥിരം { തിരികെ m_dpiExport; }
    ശൂന്യം dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() സ്ഥിരം { തിരികെ m_selfRelative; }
    ശൂന്യം selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() സ്ഥിരം { തിരികെ m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() സ്ഥിരം { തിരികെ scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() സ്ഥിരം { തിരികെ scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() സ്ഥിരം { തിരികെ scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() സ്ഥിരം { തിരികെ scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
പൊതു:
    AstStable(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_Sen_വൃക്ഷം* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { തിരികെ "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class Ast_അട്ടി_Trace_F final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // $stacktrace used as function
പൊതു:
    explicit Ast_അട്ടി_Trace_F(File_വരി* fl)
        : ASTGEN_SUPER_അട്ടി_Trace_F(fl) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_Ast_അട്ടി_Trace_F;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$stacktrace"; }
    string emitVerilog() override { തിരികെ verilogKwd(); }
    string emitC() override { തിരികെ "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstSteady final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
പൊതു:
    AstSteady(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { തിരികെ "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstStructSel final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
പൊതു:
    AstStructSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, സ്ഥിരം string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override {
        // Not a union
        തിരികെ VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstStructSel* സ്ഥിരം sp = VN_DBG_AS(samep, StructSel);
        തിരികെ m_name == sp->m_name;
    }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
};
class AstSysIgnore final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
പൊതു:
    AstSysIgnore(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$ignored"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }  // Though deleted before opt
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }  // Though deleted before opt
    bool isPure() override { തിരികെ false; }  // Though deleted before opt
    bool isOutputter() override { തിരികെ true; }  // Though deleted before opt
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
പൊതു:
    AstSystemF(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$system"; }
    string emitVerilog() override { തിരികെ verilogKwd(); }
    string emitC() override { തിരികെ "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class Ast_Tagged_വ്യഞ്ജകം final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
പൊതു:
    Ast_Tagged_വ്യഞ്ജകം(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_Tagged_വ്യഞ്ജകം(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_വ്യഞ്ജകം;
    string emitVerilog() override { തിരികെ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_name == VN_DBG_AS(samep, Tagged_വ്യഞ്ജകം)->m_name;
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
};
class AstTaggedPattern final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
പൊതു:
    AstTaggedPattern(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* patternp)
        : ASTGEN_SUPER_TaggedPattern(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstTaggedPattern;
    string emitVerilog() override { തിരികെ "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_name == VN_DBG_AS(samep, TaggedPattern)->m_name;
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
};
class AstTestPlusArgs final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
പൊതു:
    AstTestPlusArgs(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* searchp)
        : ASTGEN_SUPER_TestPlusArgs(fl) {
        this->searchp(searchp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstTestPlusArgs;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$test$plusargs"; }
    string emitVerilog() override { തിരികെ verilogKwd(); }
    string emitC() override { തിരികെ "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    // but isPure() true
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstThisRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
പൊതു:
    AstThisRef(File_വരി* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_വരി* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { തിരികെ "this"; }
    string emitVerilog() override { തിരികെ "this"; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    Ast_കണ്ണി_DType* getChildDTypep() സ്ഥിരം override { തിരികെ childDTypep(); }
};
class AstTimePrecision final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Verilog $timeprecision
പൊതു:
    explicit AstTimePrecision(File_വരി* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { തിരികെ "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstTimeUnit final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
പൊതു:
    explicit AstTimeUnit(File_വരി* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { തിരികെ "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    സ്ഥിരം std::vector<uint32_t> m_indices;  // Indices of the triggers tested
പൊതു:
    AstTriggered(File_വരി* fl, AstVarScope* trigVscp, സ്ഥിരം std::vector<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    സ്ഥിരം std::vector<uint32_t>& indices() സ്ഥിരം { തിരികെ m_indices; }
};
class AstUnbounded final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
പൊതു:
    explicit AstUnbounded(File_വരി* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { തിരികെ "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
};
class AstUnlinkedRef final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
പൊതു:
    AstUnlinkedRef(File_വരി* fl, Ast_കണ്ണി* refp, സ്ഥിരം string& name, Ast_കണ്ണി* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    സ്ഥിരം bool m_strong;  // 's_' preffix
    സ്ഥിരം bool m_overlapping;  // '_with` suffix
പൊതു:
    AstUntil(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() സ്ഥിരം override {
        തിരികെ (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
    bool isStrong() സ്ഥിരം { തിരികെ m_strong; }
    bool isOverlapping() സ്ഥിരം { തിരികെ m_overlapping; }
};
class Ast_മൂല്യം_Plus_Args final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
പൊതു:
    Ast_മൂല്യം_Plus_Args(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* searchp, Ast_കണ്ണി_വ്യഞ്ജകം* outp)
        : ASTGEN_SUPER_മൂല്യം_Plus_Args(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_മൂല്യം_Plus_Args;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$value$plusargs"; }
    string emitVerilog() override { തിരികെ "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ !outp(); }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstWithParse final : പൊതു Ast_കണ്ണി_വ്യഞ്ജകം {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
സ്വകാര്യ:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

പൊതു:
    AstWithParse(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* funcrefp, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp,
                 Ast_കണ്ണി* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        addExprsp(exprsp);
        addConstraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool restricted() സ്ഥിരം { തിരികെ m_restricted; }
    ശൂന്യം restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : പൊതു Ast_കണ്ണി_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
പൊതു:
    AstBufIf1(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstCastDynamic final : പൊതു Ast_കണ്ണി_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
പൊതു:
    AstCastDynamic(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtypeSetBitUnsized(32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { തിരികെ "%f$cast(%r, %l)"; }
    string emitC() override { തിരികെ "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 20; }
    bool isPure() override { തിരികെ false; }
};
class AstCompareNN final : പൊതു Ast_കണ്ണി_Biop {
    // Verilog str.compare() and str.icompare()
    സ്ഥിരം bool m_ignoreCase;  // True for str.icompare()
പൊതു:
    AstCompareNN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        തിരികെ m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        തിരികെ m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool ignoreCase() സ്ഥിരം { തിരികെ m_ignoreCase; }
};
class AstConcat final : പൊതു Ast_കണ്ണി_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
പൊതു:
    AstConcat(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        എങ്കിൽ (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetBitOrLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { തിരികെ "%f{%l, %k%r}"; }
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { തിരികെ "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(concat %l %r)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 2; }
};
class AstConcatN final : പൊതു Ast_കണ്ണി_Biop {
    // String concatenate
പൊതു:
    AstConcatN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { തിരികെ "%f{%l, %k%r}"; }
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { തിരികെ "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
    bool stringFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstDiv final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstDiv(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f/ %r)"; }
    string emitC() override { തിരികെ "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvudiv %l %r)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_DIV; }
};
class AstDivD final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstDivD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstDivD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "/"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL_DIV; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstDivS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstDivS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f/ %r)"; }
    string emitC() override { തിരികെ "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvsdiv %l %r)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_DIV; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstEqWild final : പൊതു Ast_കണ്ണി_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
പൊതു:
    AstEqWild(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    സ്ഥിതം Ast_കണ്ണി_Biop* newTyped(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp);
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f==? %r)"; }
    string emitC() override {
        എങ്കിൽ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } അല്ലെങ്കിൽ {
            തിരികെ "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        എങ്കിൽ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } അല്ലെങ്കിൽ {
            തിരികെ "==";
        }
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstFGetS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
പൊതു:
    AstFGetS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtypeSetInteger2State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%f$fgets(%l,%r)"; }
    string emitC() override {
        തിരികെ strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 64; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstFUngetC(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        തിരികെ "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 64; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_കണ്ണി* filep() സ്ഥിരം { തിരികെ lhsp(); }
    Ast_കണ്ണി* charp() സ്ഥിരം { തിരികെ rhsp(); }
};
class AstGetcN final : പൊതു Ast_കണ്ണി_Biop {
    // Verilog string.getc()
പൊതു:
    AstGetcN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ "getc"; }
    string emitVerilog() override { തിരികെ "%k(%l.getc(%r))"; }
    string emitC() override { തിരികെ "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstGetcRefN final : പൊതു Ast_കണ്ണി_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
പൊതു:
    AstGetcRefN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstGt final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstGt(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f> %r)"; }
    string emitC() override { തിരികെ "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { തിരികെ ">"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstGtD final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstGtD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ ">"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstGtN final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstGtN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGtN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ ">"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
    bool stringFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstGtS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstGtS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f> %r)"; }
    string emitC() override { തിരികെ "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstGte final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstGte(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f>= %r)"; }
    string emitC() override { തിരികെ "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { തിരികെ ">="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstGteD final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstGteD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ ">="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstGteN final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstGteN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstGteN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ ">="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
    bool stringFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstGteS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstGteS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f>= %r)"; }
    string emitC() override { തിരികെ "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstLogAnd final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstLogAnd(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f&& %r)"; }
    string emitC() override { തിരികെ "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvand %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "&&"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() + INSTR_എണ്ണം_BRANCH; }
};
class AstLogIf final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstLogIf(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f-> %r)"; }
    string emitC() override { തിരികെ "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { തിരികെ "->"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() + INSTR_എണ്ണം_BRANCH; }
};
class AstLogOr final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstLogOr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f|| %r)"; }
    string emitC() override { തിരികെ "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvor %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "||"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() + INSTR_എണ്ണം_BRANCH; }
};
class AstLt final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstLt(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f< %r)"; }
    string emitC() override { തിരികെ "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { തിരികെ "<"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstLtD final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstLtD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "<"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstLtN final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstLtN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLtN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "<"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
    bool stringFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstLtS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstLtS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f< %r)"; }
    string emitC() override { തിരികെ "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstLte final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstLte(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f<= %r)"; }
    string emitC() override { തിരികെ "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { തിരികെ "<="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstLteD final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstLteD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "<="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstLteN final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstLteN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstLteN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "<="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
    bool stringFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstLteS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstLteS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f<= %r)"; }
    string emitC() override { തിരികെ "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstModDiv final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstModDiv(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f%% %r)"; }
    string emitC() override { തിരികെ "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvurem %l %r)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_DIV; }
};
class AstModDivS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstModDivS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f%% %r)"; }
    string emitC() override { തിരികെ "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvsmod %l %r)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_DIV; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstNeqWild final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstNeqWild(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f!=? %r)"; }
    string emitC() override {
        എങ്കിൽ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } അല്ലെങ്കിൽ {
            തിരികെ "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        എങ്കിൽ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } അല്ലെങ്കിൽ {
            തിരികെ "!=";
        }
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstPow final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstPow(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f** %r)"; }
    string emitC() override { തിരികെ "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_MUL * 10; }
};
class AstPowD final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstPowD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstPowD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f** %r)"; }
    string emitC() override { തിരികെ "pow(%li,%ri)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL_DIV * 5; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstPowSS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstPowSS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f** %r)"; }
    string emitC() override { തിരികെ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_MUL * 10; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstPowSU final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstPowSU(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f** %r)"; }
    string emitC() override { തിരികെ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_MUL * 10; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstPowUS final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstPowUS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f** %r)"; }
    string emitC() override { തിരികെ "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_MUL * 10; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstReplicate final : പൊതു Ast_കണ്ണി_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
പൊതു:
    AstReplicate(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        എങ്കിൽ (lhsp) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(rhsp, Const)) {
                എങ്കിൽ (constp->അക്കം().isFourState()
                    || (constp->dtypep()->isSigned() && constp->അക്കം().isNegative())) {
                    dtypeSetBitOrLogicSized(lhsp->width(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } അല്ലെങ്കിൽ {
                    dtypeSetBitOrLogicSized(lhsp->width() * constp->toSInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, uint32_t rep_എണ്ണം)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_എണ്ണം}} {}
    ASTGEN_MEMBERS_AstReplicate;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%f{%r{%k%l}}"; }
    string emitC() override { തിരികെ "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override {
        തിരികെ "((_ repeat " + cvtToStr(width() / lhsp()->width()) + ") %l)";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 2; }
};
class AstReplicateN final : പൊതു Ast_കണ്ണി_Biop {
    // String replicate
പൊതു:
    AstReplicateN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%f{%r{%k%l}}"; }
    string emitC() override { തിരികെ "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 2; }
    bool stringFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstSAnd final : പൊതു Ast_കണ്ണി_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    സ്ഥിരം bool m_propertyControl;  // Parser-generated property if/case branch conjunction
പൊതു:
    AstSAnd(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() + INSTR_എണ്ണം_BRANCH; }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {  // LCOV_EXCL_LINE
        തിരികെ m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() സ്ഥിരം { തിരികെ m_propertyControl; }
};
class AstSIntersect final : പൊതു Ast_കണ്ണി_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
പൊതു:
    AstSIntersect(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() + INSTR_എണ്ണം_BRANCH; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstSOr final : പൊതു Ast_കണ്ണി_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
പൊതു:
    AstSOr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSOr;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() + INSTR_എണ്ണം_BRANCH; }
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstSThroughout final : പൊതു Ast_കണ്ണി_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
പൊതു:
    AstSThroughout(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstSWithin final : പൊതു Ast_കണ്ണി_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
പൊതു:
    AstSWithin(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    // LCOV_EXCL_STOP
    bool isMultiCycleSva() സ്ഥിരം override { തിരികെ true; }
};
class AstSel final : പൊതു Ast_കണ്ണി_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    പൂർണ്ണസംഖ്യ m_declElWidth;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    പൂർണ്ണസംഖ്യ m_widthConst;

പൊതു:
    AstSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* lsbp, പൂർണ്ണസംഖ്യ bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicSized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ bitwidth, പൂർണ്ണസംഖ്യ bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_declElWidth{1}
        , m_widthConst{bitwidth} {
        dtypeSetBitOrLogicUnsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& from, സ്ഥിരം V3_സംഖ്യ& bit) override {
        out.opSel(from, bit.toUInt() + widthConst() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        തിരികെ widthConst() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() സ്ഥിരം override { തിരികെ "((_ extract %t %r) %l)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* otherp) സ്ഥിരം override {
        തിരികെ widthConst() == VN_DBG_AS(otherp, Sel)->widthConst();
    }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override {
        തിരികെ widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    പൂർണ്ണസംഖ്യ widthConst() സ്ഥിരം { തിരികെ m_widthConst; }
    ശൂന്യം widthConst(പൂർണ്ണസംഖ്യ മൂല്യം) { m_widthConst = മൂല്യം; }
    പൂർണ്ണസംഖ്യ lsbConst() സ്ഥിരം { തിരികെ VN_AS(lsbp(), Const)->toSInt(); }
    പൂർണ്ണസംഖ്യ msbConst() സ്ഥിരം { തിരികെ lsbConst() + widthConst() - 1; }
    VNumRange& declRange() VL_MT_STABLE { തിരികെ m_declRange; }
    സ്ഥിരം VNumRange& declRange() സ്ഥിരം VL_MT_STABLE { തിരികെ m_declRange; }
    ശൂന്യം declRange(സ്ഥിരം VNumRange& flag) { m_declRange = flag; }
    പൂർണ്ണസംഖ്യ declElWidth() സ്ഥിരം { തിരികെ m_declElWidth; }
    ശൂന്യം declElWidth(പൂർണ്ണസംഖ്യ flag) { m_declElWidth = flag; }
};
class AstShiftL final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstShiftL(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, പൂർണ്ണസംഖ്യ setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        എങ്കിൽ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftL;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f<< %r)"; }
    string emitC() override { തിരികെ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        തിരികെ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstShiftLOvr final : പൊതു Ast_കണ്ണി_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
പൊതു:
    AstShiftLOvr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, പൂർണ്ണസംഖ്യ setwidth = 0)
        : ASTGEN_SUPER_ShiftLOvr(fl, lhsp, rhsp) {
        എങ്കിൽ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } അല്ലെങ്കിൽ {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftLOvr;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f<< %r)"; }
    string emitC() override { തിരികെ "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstShiftR final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstShiftR(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, പൂർണ്ണസംഖ്യ setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        എങ്കിൽ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftR;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f>> %r)"; }
    string emitC() override { തിരികെ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        തിരികെ (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstShiftROvr final : പൊതു Ast_കണ്ണി_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
പൊതു:
    AstShiftROvr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, പൂർണ്ണസംഖ്യ setwidth = 0)
        : ASTGEN_SUPER_ShiftROvr(fl, lhsp, rhsp) {
        എങ്കിൽ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } അല്ലെങ്കിൽ {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftROvr;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f>> %r)"; }
    string emitC() override { തിരികെ "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstShiftRS final : പൊതു Ast_കണ്ണി_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
പൊതു:
    AstShiftRS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, പൂർണ്ണസംഖ്യ setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        എങ്കിൽ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = lhsp->dtypep()) {
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { തിരികെ "%k(%l %f>>> %r)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvashr %l %r)"; }
    string emitC() override { തിരികെ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstShiftRSOvr final : പൊതു Ast_കണ്ണി_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
പൊതു:
    AstShiftRSOvr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, പൂർണ്ണസംഖ്യ setwidth = 0)
        : ASTGEN_SUPER_ShiftRSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        എങ്കിൽ (setwidth) {
            dtypeSetBitOrLogicUnsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } അല്ലെങ്കിൽ {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtypeSetBitOrLogicUnsized(dtypep->width(), dtypep->widthMin(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstShiftRSOvr;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { തിരികെ "%k(%l %f>>> %r)"; }
    string emitC() override { തിരികെ "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstSub final : പൊതു Ast_കണ്ണി_Biop {
    // @astgen makeDfgVertex
പൊതു:
    AstSub(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f- %r)"; }
    string emitC() override { തിരികെ "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvsub %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "-"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
};
class AstSubD final : പൊതു Ast_കണ്ണി_Biop {
പൊതു:
    AstSubD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstSubD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "-"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstURandomRange final : പൊതു Ast_കണ്ണി_Biop {
    // $urandom_range
പൊതു:
    AstURandomRange(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%f$urandom_range(%l, %r)"; }
    string emitC() override { തിരികെ "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : പൊതു Ast_കണ്ണി_Bi_Com {
    // @astgen makeDfgVertex
പൊതു:
    AstEq(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    സ്ഥിതം Ast_കണ്ണി_Biop* newTyped(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp);
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f== %r)"; }
    string emitC() override { തിരികെ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { തിരികെ "=="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstEqCase final : പൊതു Ast_കണ്ണി_Bi_Com {
    // @astgen makeDfgVertex
പൊതു:
    AstEqCase(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f=== %r)"; }
    string emitC() override { തിരികെ "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { തിരികെ "=="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstEqD final : പൊതു Ast_കണ്ണി_Bi_Com {
പൊതു:
    AstEqD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "=="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstEqN final : പൊതു Ast_കണ്ണി_Bi_Com {
പൊതു:
    AstEqN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "=="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
    bool stringFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstEqT final : പൊതു Ast_കണ്ണി_Bi_Com {
    // Equal (==) for data types
പൊതു:
    AstEqT(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstEqT;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "=="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
};
class AstLogEq final : പൊതു Ast_കണ്ണി_Bi_Com {
    // @astgen makeDfgVertex
പൊതു:
    AstLogEq(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f<-> %r)"; }
    string emitC() override { തിരികെ "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "<->"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() + INSTR_എണ്ണം_BRANCH; }
};
class AstNeq final : പൊതു Ast_കണ്ണി_Bi_Com {
    // @astgen makeDfgVertex
പൊതു:
    AstNeq(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBitOrLogic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    സ്ഥിതം Ast_കണ്ണി_Biop* newTyped(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp);
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f!= %r)"; }
    string emitC() override { തിരികെ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { തിരികെ "!="; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (not (= %l %r)))"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstNeqCase final : പൊതു Ast_കണ്ണി_Bi_Com {
    // @astgen makeDfgVertex
പൊതു:
    AstNeqCase(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f!== %r)"; }
    string emitC() override { തിരികെ "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { തിരികെ "!="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
};
class AstNeqD final : പൊതു Ast_കണ്ണി_Bi_Com {
പൊതു:
    AstNeqD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "!="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstNeqN final : പൊതു Ast_കണ്ണി_Bi_Com {
പൊതു:
    AstNeqN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "!="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
    bool stringFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstNeqT final : പൊതു Ast_കണ്ണി_Bi_Com {
    // Not-equal (!=) for data types
പൊതു:
    AstNeqT(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "!="; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_STR; }
};

// === AstNodeBiComAsv ===
class AstAdd final : പൊതു Ast_കണ്ണി_Bi_Com_Asv {
    // @astgen makeDfgVertex
പൊതു:
    AstAdd(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAdd;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f+ %r)"; }
    string emitC() override { തിരികെ "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvadd %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "+"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
};
class AstAddD final : പൊതു Ast_കണ്ണി_Bi_Com_Asv {
പൊതു:
    AstAddD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstAddD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "+"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstAnd final : പൊതു Ast_കണ്ണി_Bi_Com_Asv {
    // @astgen makeDfgVertex
പൊതു:
    AstAnd(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f& %r)"; }
    string emitC() override { തിരികെ "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvand %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "&"; }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    സ്ഥിരം അക്ഷരം* widthMismatch() സ്ഥിരം override VL_MT_STABLE;
};
class AstMul final : പൊതു Ast_കണ്ണി_Bi_Com_Asv {
    // @astgen makeDfgVertex
പൊതു:
    AstMul(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f* %r)"; }
    string emitC() override { തിരികെ "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "*"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_MUL; }
};
class AstMulD final : പൊതു Ast_കണ്ണി_Bi_Com_Asv {
പൊതു:
    AstMulD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstMulD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "*"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstMulS final : പൊതു Ast_കണ്ണി_Bi_Com_Asv {
    // @astgen makeDfgVertex
പൊതു:
    AstMulS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f* %r)"; }
    string emitC() override { തിരികെ "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvmul %l %r)"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * INSTR_എണ്ണം_INT_MUL; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstOr final : പൊതു Ast_കണ്ണി_Bi_Com_Asv {
    // @astgen makeDfgVertex
പൊതു:
    AstOr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f| %r)"; }
    string emitC() override { തിരികെ "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvor %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "|"; }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    സ്ഥിരം അക്ഷരം* widthMismatch() സ്ഥിരം override VL_MT_STABLE;
};
class AstXor final : പൊതു Ast_കണ്ണി_Bi_Com_Asv {
    // @astgen makeDfgVertex
പൊതു:
    AstXor(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f^ %r)"; }
    string emitC() override { തിരികെ "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvxor %l %r)"; }
    string emitSimpleOperator() override { തിരികെ "^"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }  // Lclean && Rclean
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    സ്ഥിരം അക്ഷരം* widthMismatch() സ്ഥിരം override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class AstDistChiSquare final : പൊതു Ast_കണ്ണി_Dist_Biop {
പൊതു:
    AstDistChiSquare(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_DistChiSquare(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistChiSquare;
    string emitVerilog() override { തിരികെ "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { തിരികെ "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class AstDistExponential final : പൊതു Ast_കണ്ണി_Dist_Biop {
പൊതു:
    AstDistExponential(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_DistExponential(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistExponential;
    string emitVerilog() override { തിരികെ "%f$dist_exponential(%l, %r)"; }
    string emitC() override { തിരികെ "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class AstDistPoisson final : പൊതു Ast_കണ്ണി_Dist_Biop {
പൊതു:
    AstDistPoisson(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_DistPoisson(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistPoisson;
    string emitVerilog() override { തിരികെ "%f$dist_poisson(%l, %r)"; }
    string emitC() override { തിരികെ "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : പൊതു Ast_കണ്ണി_Dist_Biop {
പൊതു:
    AstDistT(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { തിരികെ "%f$dist_t(%l, %r)"; }
    string emitC() override { തിരികെ "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : പൊതു Ast_കണ്ണി_Sel {
    // @astgen makeDfgVertex
    ശൂന്യം init(സ്ഥിരം Ast_കണ്ണി* fromp) {
        എങ്കിൽ (fromp && VN_IS(fromp->dtypep()->skipRefp(), കണ്ണി_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), കണ്ണി_Array_DType)->subDTypep());
        }
    }

പൊതു:
    AstArraySel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, പൂർണ്ണസംഖ്യ bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() സ്ഥിരം override { തിരികെ "(select %l %r)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
};
class AstAssocSel final : പൊതു Ast_കണ്ണി_Sel {
    ശൂന്യം init(സ്ഥിരം Ast_കണ്ണി* fromp) {
        എങ്കിൽ (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

പൊതു:
    AstAssocSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%k(%l%f[%r])"; }
    string emitC() override { തിരികെ "%li%k[%ri]"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(select %l %r)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isGateOptimizable() സ്ഥിരം override {
        തിരികെ !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ !isLValue(); }  // AssocSel creates on miss
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
};
class AstWildcardSel final : പൊതു Ast_കണ്ണി_Sel {
    ശൂന്യം init(സ്ഥിരം Ast_കണ്ണി* fromp) {
        എങ്കിൽ (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

പൊതു:
    AstWildcardSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%k(%l%f[%r])"; }
    string emitC() override { തിരികെ "%li%k[%ri]"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
};
class AstWordSel final : പൊതു Ast_കണ്ണി_Sel {
    // Select a single word from a multi-word wide value
പൊതു:
    AstWordSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&, സ്ഥിരം V3_സംഖ്യ&) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};

// === AstNodeStream ===
class AstStreamL final : പൊതു Ast_കണ്ണി_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
പൊതു:
    AstStreamL(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamL;
    string emitVerilog() override { തിരികെ "%f{ << %r %k{%l} }"; }
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { തിരികെ "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 2; }
};
class AstStreamR final : പൊതു Ast_കണ്ണി_Stream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
പൊതു:
    AstStreamR(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstStreamR;
    string emitVerilog() override { തിരികെ "%f{ >> %r %k{%l} }"; }
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { തിരികെ isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : പൊതു Ast_കണ്ണി_System_Biop_D {
പൊതു:
    AstAtan2D(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$atan2(%l,%r)"; }
    string emitC() override { തിരികെ "atan2(%li,%ri)"; }
};
class AstHypotD final : പൊതു Ast_കണ്ണി_System_Biop_D {
പൊതു:
    AstHypotD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$hypot(%l,%r)"; }
    string emitC() override { തിരികെ "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : പൊതു Ast_കണ്ണി_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::Empty()};  // Output code object
                                                        // pointer (e.g.: 'this')
പൊതു:
    AstCCall(File_വരി* fl, AstCFunc* funcp, Ast_കണ്ണി_വ്യഞ്ജകം* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    സ്ഥിരം VSelfPointerText& selfPointer() സ്ഥിരം { തിരികെ m_selfPointer; }
    ശൂന്യം selfPointer(സ്ഥിരം VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) സ്ഥിരം {
        തിരികെ selfPointer().protect(useSelfForThis, protect());
    }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
};
class AstCMethodCall final : പൊതു Ast_കണ്ണി_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
പൊതു:
    AstCMethodCall(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, AstCFunc* funcp, Ast_കണ്ണി_വ്യഞ്ജകം* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : പൊതു Ast_കണ്ണി_CCall {
    // C++ new() call
പൊതു:
    AstCNew(File_വരി* fl, AstCFunc* funcp, Ast_കണ്ണി_വ്യഞ്ജകം* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : പൊതു Ast_കണ്ണി_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

പൊതു:
    AstConsArray(File_വരി* fl, Ast_കണ്ണി_DType* newDTypep, Ast_കണ്ണി_വ്യഞ്ജകം* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    ശൂന്യം cloneRelink() override;
    വരിയിൽ AstConsArrayItem* itemsp() സ്ഥിരം VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_കണ്ണി_വ്യഞ്ജകം* valuep(uint64_t index) സ്ഥിരം;
    // Add the value of the element at the index, which must not be given yet
    ശൂന്യം valuep(uint64_t index, Ast_കണ്ണി_വ്യഞ്ജകം* newp);
    // Add the elements of the array 'arrayp', starting at the index
    ശൂന്യം splice_മൂല്യങ്ങൾ(uint64_t index, Ast_കണ്ണി_വ്യഞ്ജകം* arrayp);
};
class AstConsAssoc final : പൊതു Ast_കണ്ണി_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
പൊതു:
    explicit AstConsAssoc(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        തിരികെ nullptr;
    }
};
class AstConsDynArray final : പൊതു Ast_കണ്ണി_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
പൊതു:
    explicit AstConsDynArray(File_വരി* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
};
class AstConsQueue final : പൊതു Ast_കണ്ണി_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
പൊതു:
    explicit AstConsQueue(File_വരി* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        തിരികെ nullptr;
    }
};
class AstConsStruct final : പൊതു Ast_കണ്ണി_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
പൊതു:
    AstConsStruct(File_വരി* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    // The value of the member called 'name'
    Ast_കണ്ണി_വ്യഞ്ജകം* memberValuep(സ്ഥിരം string& name) സ്ഥിരം;
};
class AstConsWildcard final : പൊതു Ast_കണ്ണി_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
പൊതു:
    explicit AstConsWildcard(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        തിരികെ nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : പൊതു Ast_കണ്ണി_FTask_Ref {
    // A reference to a function
പൊതു:
    വരിയിൽ AstFuncRef(File_വരി* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_വരി* fl, സ്ഥിരം string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class AstMethodCall final : പൊതു Ast_കണ്ണി_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
പൊതു:
    AstMethodCall(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, VFlagChildDType, സ്ഥിരം string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMethodCall(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, സ്ഥിരം string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_MethodCall(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_AstMethodCall;
};
class AstNew final : പൊതു Ast_കണ്ണി_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
പൊതു:
    AstNew(File_വരി* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool isImplicit() സ്ഥിരം { തിരികെ m_isImplicit; }
    ശൂന്യം isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() സ്ഥിരം { തിരികെ m_isScoped; }
    ശൂന്യം isScoped(bool flag) { m_isScoped = flag; }
    bool isPure() override { തിരികെ false; }
};
class AstTaskRef final : പൊതു Ast_കണ്ണി_FTask_Ref {
    // A reference to a task
പൊതു:
    വരിയിൽ AstTaskRef(File_വരി* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_വരി* fl, സ്ഥിരം string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtypeSetVoid();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : പൊതു Ast_കണ്ണി_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
സ്വകാര്യ:
    VAccess m_access;  // Left hand side assignment
പൊതു:
    AstSelBit(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    VAccess access() സ്ഥിരം { തിരികെ m_access; }
    ശൂന്യം access(സ്ഥിരം VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : പൊതു Ast_കണ്ണി_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
പൊതു:
    AstSelExtract(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* msbp, Ast_കണ്ണി_വ്യഞ്ജകം* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : പൊതു Ast_കണ്ണി_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
പൊതു:
    AstSelMinus(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* bitp, Ast_കണ്ണി_വ്യഞ്ജകം* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : പൊതു Ast_കണ്ണി_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
പൊതു:
    AstSelPlus(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* bitp, Ast_കണ്ണി_വ്യഞ്ജകം* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_എണ്ണം_Bits final : പൊതു Ast_കണ്ണി_Quadop {
    // Number of bits set in vector
പൊതു:
    Ast_എണ്ണം_Bits(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* ctrl1p)
        : ASTGEN_SUPER_എണ്ണം_Bits(fl, exprp, ctrl1p, ctrl1p->clone_വൃക്ഷം_Pure(false),
                                 ctrl1p->clone_വൃക്ഷം_Pure(false)) {
        dtypeSetInt();
    }
    Ast_എണ്ണം_Bits(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* ctrl1p, Ast_കണ്ണി_വ്യഞ്ജകം* ctrl2p)
        : ASTGEN_SUPER_എണ്ണം_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_വൃക്ഷം_Pure(false)) {
        dtypeSetInt();
    }
    Ast_എണ്ണം_Bits(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* ctrl1p, Ast_കണ്ണി_വ്യഞ്ജകം* ctrl2p,
                 Ast_കണ്ണി_വ്യഞ്ജകം* ctrl3p)
        : ASTGEN_SUPER_എണ്ണം_Bits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_എണ്ണം_Bits;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& വ്യഞ്ജകം, സ്ഥിരം V3_സംഖ്യ& ctrl1,
                       സ്ഥിരം V3_സംഖ്യ& ctrl2, സ്ഥിരം V3_സംഖ്യ& ctrl3) override {
        out.op_എണ്ണം_Bits(വ്യഞ്ജകം, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { തിരികെ "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanThs() സ്ഥിരം override { തിരികെ true; }
    bool cleanFhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersThs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersFhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : പൊതു Ast_കണ്ണി_Termop {
പൊതു:
    explicit AstInferredDisable(File_വരി* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { തിരികെ "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstTime final : പൊതു Ast_കണ്ണി_Termop {
    VTimescale m_timeunit;  // Parent module time unit
പൊതു:
    AstTime(File_വരി* fl, സ്ഥിരം VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { തിരികെ "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_TIME; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : പൊതു Ast_കണ്ണി_Termop {
    VTimescale m_timeunit;  // Parent module time unit
പൊതു:
    AstTimeD(File_വരി* fl, സ്ഥിരം VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { തിരികെ "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_TIME; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : പൊതു Ast_കണ്ണി_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
പൊതു:
    AstCond(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി_വ്യഞ്ജകം* thenp, Ast_കണ്ണി_വ്യഞ്ജകം* elsep);
    ASTGEN_MEMBERS_AstCond;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs,
                       സ്ഥിരം V3_സംഖ്യ& ths) override {
        out.opAssign(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { തിരികെ "%k(%l %f? %r %k: %t)"; }
    string emitC() override { തിരികെ "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(ite (__Vbool %l) %r %t)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }  // clean if e1 & e2 clean
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanThs() സ്ഥിരം override { തിരികെ false; }  // Propagates up
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersThs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
};
class AstPutcN final : പൊതു Ast_കണ്ണി_Triop {
    // Verilog string.putc()
പൊതു:
    AstPutcN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി_വ്യഞ്ജകം* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstPutcN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs,
                       സ്ഥിരം V3_സംഖ്യ& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ "putc"; }
    string emitVerilog() override { തിരികെ "%k(%l.putc(%r,%t))"; }
    string emitC() override { തിരികെ "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanThs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersThs() സ്ഥിരം override { തിരികെ false; }
};
class AstSliceSel final : പൊതു Ast_കണ്ണി_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
പൊതു:
    AstSliceSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, സ്ഥിരം VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_AstSliceSel;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& from, സ്ഥിരം V3_സംഖ്യ& lo,
                       സ്ഥിരം V3_സംഖ്യ& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanThs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersThs() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { തിരികെ m_declRange; }
    സ്ഥിരം VNumRange& declRange() സ്ഥിരം VL_MT_STABLE { തിരികെ m_declRange; }
};
class AstSubstrN final : പൊതു Ast_കണ്ണി_Triop {
    // Verilog string.substr()
പൊതു:
    AstSubstrN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി_വ്യഞ്ജകം* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs,
                       സ്ഥിരം V3_സംഖ്യ& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ "substr"; }
    string emitVerilog() override { തിരികെ "%k(%l.substr(%r,%t))"; }
    string emitC() override { തിരികെ "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { തിരികെ ""; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanRhs() സ്ഥിരം override { തിരികെ true; }
    bool cleanThs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersRhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersThs() സ്ഥിരം override { തിരികെ false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : പൊതു Ast_കണ്ണി_Dist_Triop {
പൊതു:
    AstDistErlang(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി_വ്യഞ്ജകം* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { തിരികെ "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { തിരികെ "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class AstDistNormal final : പൊതു Ast_കണ്ണി_Dist_Triop {
പൊതു:
    AstDistNormal(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി_വ്യഞ്ജകം* thsp)
        : ASTGEN_SUPER_DistNormal(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistNormal;
    string emitVerilog() override { തിരികെ "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { തിരികെ "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class AstDistUniform final : പൊതു Ast_കണ്ണി_Dist_Triop {
പൊതു:
    AstDistUniform(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി_വ്യഞ്ജകം* thsp)
        : ASTGEN_SUPER_DistUniform(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistUniform;
    string emitVerilog() override { തിരികെ "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { തിരികെ "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : പൊതു Ast_കണ്ണി_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
പൊതു:
    എണ്ണൽ FmtType : പൂർണ്ണസംഖ്യ { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

സ്വകാര്യ:
    // dist-ast-dump-suppress  // Part of name()
    സ്ഥിരം FmtType m_fmtType;  // Operation type
പൊതു:
    AstAtoN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtypeSetDouble() : dtypeSetInteger();
    }
    ASTGEN_MEMBERS_AstAtoN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opAtoN(lhs, m_fmtType); }
    string name() സ്ഥിരം override VL_MT_STABLE {
        തിരഞ്ഞെടുപ്പ് (m_fmtType) {
        അവസ്ഥ ATOI: തിരികെ "atoi";
        അവസ്ഥ ATOHEX: തിരികെ "atohex";
        അവസ്ഥ ATOOCT: തിരികെ "atooct";
        അവസ്ഥ ATOBIN: തിരികെ "atobin";
        അവസ്ഥ ATOREAL: തിരികെ "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { തിരികെ "%l." + name() + "()"; }
    string emitC() override {
        തിരഞ്ഞെടുപ്പ് (m_fmtType) {
        അവസ്ഥ ATOI: തിരികെ "VL_ATOI_N(%li, 10)";
        അവസ്ഥ ATOHEX: തിരികെ "VL_ATOI_N(%li, 16)";
        അവസ്ഥ ATOOCT: തിരികെ "VL_ATOI_N(%li, 8)";
        അവസ്ഥ ATOBIN: തിരികെ "VL_ATOI_N(%li, 2)";
        അവസ്ഥ ATOREAL: തിരികെ "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    FmtType fmtType() സ്ഥിരം { തിരികെ m_fmtType; }
};
class AstBitsToRealD final : പൊതു Ast_കണ്ണി_Uniop {
പൊതു:
    AstBitsToRealD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstBitsToRealD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { തിരികെ "%f$bitstoreal(%l)"; }
    string emitC() override { തിരികെ "VL_CVT_D_Q(%li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }  // Eliminated before matters
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }  // Eliminated before matters
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstCCast final : പൊതു Ast_കണ്ണി_Uniop {
    // Cast to C-based data type
    പൂർണ്ണസംഖ്യ m_size;

പൊതു:
    AstCCast(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, പൂർണ്ണസംഖ്യ setwidth, പൂർണ്ണസംഖ്യ minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_size = setwidth;
        എങ്കിൽ (setwidth) {
            എങ്കിൽ (minwidth == -1) minwidth = setwidth;
            dtypeSetBitUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_size = width();
    }
    ASTGEN_MEMBERS_AstCCast;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { തിരികെ "%f$_CAST(%l)"; }
    string emitC() override { തിരികെ "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }  // Special cased in V3Cast
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ size() == VN_DBG_AS(samep, CCast)->size();
    }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    //
    പൂർണ്ണസംഖ്യ size() സ്ഥിരം { തിരികെ m_size; }
};
class AstCLog2 final : പൊതു Ast_കണ്ണി_Uniop {
പൊതു:
    AstCLog2(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { തിരികെ "%f$clog2(%l)"; }
    string emitC() override { തിരികെ "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 16; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstCastWrap final : പൊതു Ast_കണ്ണി_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
പൊതു:
    AstCastWrap(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { തിരികെ "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
};
class Ast_എണ്ണം_Ones final : പൊതു Ast_കണ്ണി_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
പൊതു:
    Ast_എണ്ണം_Ones(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_എണ്ണം_Ones(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_Ast_എണ്ണം_Ones;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.op_എണ്ണം_Ones(lhs); }
    string emitVerilog() override { തിരികെ "%f$countones(%l)"; }
    string emitC() override { തിരികെ "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 16; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstCvtPackString final : പൊതു Ast_കണ്ണി_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
പൊതു:
    AstCvtPackString(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstExtend final : പൊതു Ast_കണ്ണി_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
പൊതു:
    AstExtend(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetBitOrLogicUnsized(width, widthMin ? widthMin : width, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { തിരികെ "%l"; }
    string emitC() override { തിരികെ "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() സ്ഥിരം override {
        തിരികെ "((_ zero_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override {
        തിരികെ false;  // Because the EXTEND operator self-casts
    }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
};
class AstExtendS final : പൊതു Ast_കണ്ണി_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
പൊതു:
    AstExtendS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, പൂർണ്ണസംഖ്യ width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { തിരികെ "%l"; }
    string emitC() override { തിരികെ "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() സ്ഥിരം override {
        തിരികെ "((_ sign_extend " + cvtToStr(width() - lhsp()->width()) + ") %l)";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override {
        തിരികെ false;  // Because the EXTEND operator self-casts
    }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool signedFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstFEof final : പൊതു Ast_കണ്ണി_Uniop {
പൊതു:
    AstFEof(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%f$feof(%l)"; }
    string emitC() override { തിരികെ "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 16; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
    Ast_കണ്ണി* filep() സ്ഥിരം { തിരികെ lhsp(); }
};
class AstFGetC final : പൊതു Ast_കണ്ണി_Uniop {
പൊതു:
    AstFGetC(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtypeSetLogic2StateUnsized(
            32, v3Global.widthMinUsage() == VWidthMinUsage::MATCHES_WIDTH ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { തിരികെ "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 64; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    Ast_കണ്ണി* filep() സ്ഥിരം { തിരികെ lhsp(); }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstISToRD final : പൊതു Ast_കണ്ണി_Uniop {
    // $itor where lhs is signed
പൊതു:
    AstISToRD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstISToRD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { തിരികെ "%f$itor($signed(%l))"; }
    string emitC() override { തിരികെ "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_വാക്കുകൾ() override { തിരികെ true; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstIToRD final : പൊതു Ast_കണ്ണി_Uniop {
    // $itor where lhs is unsigned
പൊതു:
    AstIToRD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstIToRD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { തിരികെ "%f$itor(%l)"; }
    string emitC() override { തിരികെ "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstIsUnbounded final : പൊതു Ast_കണ്ണി_Uniop {
    // True if is unbounded ($)
പൊതു:
    AstIsUnbounded(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { തിരികെ "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstIsUnknown final : പൊതു Ast_കണ്ണി_Uniop {
    // True if any unknown bits
പൊതു:
    AstIsUnknown(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { തിരികെ "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstLenN final : പൊതു Ast_കണ്ണി_Uniop {
    // Length of a string
പൊതു:
    AstLenN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetInt();
    }
    ASTGEN_MEMBERS_AstLenN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { തിരികെ "%f(%l)"; }
    string emitC() override { തിരികെ "VL_LEN_IN(%li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
};
class AstLogNot final : പൊതു Ast_കണ്ണി_Uniop {
    // @astgen makeDfgVertex
സ്വകാര്യ:
    സ്ഥിരം bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
പൊതു:
    AstLogNot(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { തിരികെ "%f(! %l)"; }
    string emitC() override { തിരികെ "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { തിരികെ "!"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool fromProperty() സ്ഥിരം { തിരികെ m_fromProperty; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstMostSetBitP1 final : പൊതു Ast_കണ്ണി_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
പൊതു:
    AstMostSetBitP1(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_MostSetBitP1(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstMostSetBitP1;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opMostSetBitP1(lhs); }
    string emitVerilog() override { തിരികെ "%f$mostsetbitp1(%l)"; }
    string emitC() override { തിരികെ "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 16; }
};
class AstNToI final : പൊതു Ast_കണ്ണി_Uniop {
    // String to any-size integral
പൊതു:
    AstNToI(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opNToI(lhs); }
    string emitVerilog() override { തിരികെ "'(%l)"; }
    string emitC() override { തിരികെ "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
};
class AstNegate final : പൊതു Ast_കണ്ണി_Uniop {
    // @astgen makeDfgVertex
പൊതു:
    AstNegate(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { തിരികെ "%f(- %l)"; }
    string emitC() override { തിരികെ "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvneg %l)"; }
    string emitSimpleOperator() override { തിരികെ "-"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
};
class AstNegateD final : പൊതു Ast_കണ്ണി_Uniop {
പൊതു:
    AstNegateD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AstNegateD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { തിരികെ "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { തിരികെ "-"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool doubleFlavor() സ്ഥിരം override { തിരികെ true; }
};
class AstNot final : പൊതു Ast_കണ്ണി_Uniop {
    // @astgen makeDfgVertex
പൊതു:
    AstNot(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { തിരികെ "%f(~ %l)"; }
    string emitC() override { തിരികെ "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() സ്ഥിരം override { തിരികെ "(bvnot %l)"; }
    string emitSimpleOperator() override { തിരികെ "~"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }
    സ്ഥിരം അക്ഷരം* widthMismatch() സ്ഥിരം override VL_MT_STABLE;
};
class AstNullCheck final : പൊതു Ast_കണ്ണി_Uniop {
    // Return LHS after checking that LHS is non-null
പൊതു:
    AstNullCheck(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { V3ERROR_NA; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 1; }  // Rarely executes
    string emitVerilog() override { തിരികെ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ fileline() == samep->fileline(); }
};
class AstOneHot final : പൊതു Ast_കണ്ണി_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
പൊതു:
    AstOneHot(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { തിരികെ "%f$onehot(%l)"; }
    string emitC() override { തിരികെ "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 4; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstOneHot0 final : പൊതു Ast_കണ്ണി_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
പൊതു:
    AstOneHot0(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { തിരികെ "%f$onehot0(%l)"; }
    string emitC() override { തിരികെ "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() * 3; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstPostDec final : പൊതു Ast_കണ്ണി_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
പൊതു:
    AstPostDec(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { തിരികെ "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : പൊതു Ast_കണ്ണി_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
പൊതു:
    AstPostInc(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { തിരികെ "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : പൊതു Ast_കണ്ണി_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
പൊതു:
    AstPreDec(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { തിരികെ "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : പൊതു Ast_കണ്ണി_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
പൊതു:
    AstPreInc(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { തിരികെ "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() സ്ഥിരം override { V3ERROR_NA_RETURN(false); }
    bool sizeMattersLhs() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : പൊതു Ast_കണ്ണി_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
പൊതു:
    AstRToIRoundS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { തിരികെ "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        തിരികെ isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstRToIS final : പൊതു Ast_കണ്ണി_Uniop {
    // $rtoi(lhs)
പൊതു:
    AstRToIS(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetInteger2State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { തിരികെ "%f$rtoi(%l)"; }
    string emitC() override { തിരികെ "VL_RTOI_I_D(%li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }  // Eliminated before matters
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }  // Eliminated before matters
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstRealToBits final : പൊതു Ast_കണ്ണി_Uniop {
പൊതു:
    AstRealToBits(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_AstRealToBits;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { തിരികെ "%f$realtobits(%l)"; }
    string emitC() override { തിരികെ "VL_CVT_Q_D(%li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }  // Eliminated before matters
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }  // Eliminated before matters
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_DBL; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstRedAnd final : പൊതു Ast_കണ്ണി_Uniop {
    // @astgen makeDfgVertex
പൊതു:
    AstRedAnd(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { തിരികെ "%f(& %l)"; }
    string emitC() override { തിരികെ "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
};
class AstRedOr final : പൊതു Ast_കണ്ണി_Uniop {
    // @astgen makeDfgVertex
പൊതു:
    AstRedOr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { തിരികെ "%f(| %l)"; }
    string emitC() override { തിരികെ "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
};
class AstRedXor final : പൊതു Ast_കണ്ണി_Uniop {
    // @astgen makeDfgVertex
പൊതു:
    AstRedXor(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBitOrLogic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { തിരികെ "%f(^ %l)"; }
    string emitC() override { തിരികെ "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override {
        സ്ഥിരം പൂർണ്ണസംഖ്യ w = lhsp()->width();
        തിരികെ (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 1 + V3_സംഖ്യ::log2b(width()); }
};
class AstResizeLValue final : പൊതു Ast_കണ്ണി_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
പൊതു:
    AstResizeLValue(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
};
class AstSigned final : പൊതു Ast_കണ്ണി_Uniop {
    // $signed(lhs)
പൊതു:
    AstSigned(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { തിരികെ "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }  // Eliminated before matters
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }  // Eliminated before matters
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};
class AstTimeImport final : പൊതു Ast_കണ്ണി_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
പൊതു:
    AstTimeImport(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }
};
class AstToLowerN final : പൊതു Ast_കണ്ണി_Uniop {
    // string.tolower()
പൊതു:
    AstToLowerN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToLowerN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { തിരികെ "%l.tolower()"; }
    string emitC() override { തിരികെ "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
};
class AstToStringN final : പൊതു Ast_കണ്ണി_Uniop {
പൊതു:
    AstToStringN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_ToStringN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToStringN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { തിരികെ "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        തിരികെ isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
};
class AstToUpperN final : പൊതു Ast_കണ്ണി_Uniop {
    // string.toupper()
പൊതു:
    AstToUpperN(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_AstToUpperN;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { തിരികെ "%l.toupper()"; }
    string emitC() override { തിരികെ "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ true; }
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ false; }
};
class AstUnsigned final : പൊതു Ast_കണ്ണി_Uniop {
    // $unsigned(lhs)
പൊതു:
    AstUnsigned(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { തിരികെ "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ false; }
    bool cleanLhs() സ്ഥിരം override { തിരികെ false; }  // Eliminated before matters
    bool sizeMattersLhs() സ്ഥിരം override { തിരികെ true; }  // Eliminated before matters
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool isSystemFunc() സ്ഥിരം override { തിരികെ true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstAcosD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$acos(%l)"; }
    string emitC() override { തിരികെ "acos(%li)"; }
};
class AstAcoshD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstAcoshD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$acosh(%l)"; }
    string emitC() override { തിരികെ "acosh(%li)"; }
};
class AstAsinD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstAsinD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$asin(%l)"; }
    string emitC() override { തിരികെ "asin(%li)"; }
};
class AstAsinhD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstAsinhD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$asinh(%l)"; }
    string emitC() override { തിരികെ "asinh(%li)"; }
};
class AstAtanD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstAtanD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$atan(%l)"; }
    string emitC() override { തിരികെ "atan(%li)"; }
};
class AstAtanhD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstAtanhD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$atanh(%l)"; }
    string emitC() override { തിരികെ "atanh(%li)"; }
};
class AstCeilD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstCeilD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::ceil(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$ceil(%l)"; }
    string emitC() override { തിരികെ "ceil(%li)"; }
};
class AstCosD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstCosD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$cos(%l)"; }
    string emitC() override { തിരികെ "cos(%li)"; }
};
class AstCoshD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstCoshD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$cosh(%l)"; }
    string emitC() override { തിരികെ "cosh(%li)"; }
};
class AstExpD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstExpD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$exp(%l)"; }
    string emitC() override { തിരികെ "exp(%li)"; }
};
class AstFloorD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstFloorD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::floor(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$floor(%l)"; }
    string emitC() override { തിരികെ "floor(%li)"; }
};
class AstLog10D final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstLog10D(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$log10(%l)"; }
    string emitC() override { തിരികെ "log10(%li)"; }
};
class AstLogD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstLogD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$ln(%l)"; }
    string emitC() override { തിരികെ "log(%li)"; }
};
class AstSinD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstSinD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$sin(%l)"; }
    string emitC() override { തിരികെ "sin(%li)"; }
};
class AstSinhD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstSinhD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$sinh(%l)"; }
    string emitC() override { തിരികെ "sinh(%li)"; }
};
class AstSqrtD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstSqrtD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::sqrt(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$sqrt(%l)"; }
    string emitC() override { തിരികെ "sqrt(%li)"; }
};
class AstTanD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstTanD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$tan(%l)"; }
    string emitC() override { തിരികെ "tan(%li)"; }
};
class AstTanhD final : പൊതു Ast_കണ്ണി_System_Uniop_D {
പൊതു:
    AstTanhD(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    ശൂന്യം സംഖ്യ_Operate(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { തിരികെ "%f$tanh(%l)"; }
    string emitC() override { തിരികെ "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : പൊതു Ast_കണ്ണി_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
പൊതു:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    വരിയിൽ AstVarRef(File_വരി* fl, AstVar* varp, സ്ഥിരം VAccess& access);
    വരിയിൽ AstVarRef(File_വരി* fl, Ast_കണ്ണി_Module* classOrPackagep, AstVar* varp,
                     സ്ഥിരം VAccess& access);
    // This form only allowed post-link (see above)
    വരിയിൽ AstVarRef(File_വരി* fl, AstVarScope* varscp, സ്ഥിരം VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() സ്ഥിരം { തിരികെ m_name; }
    വരിയിൽ string name() സ്ഥിരം override;  // * = Var name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override;
    വരിയിൽ bool same_കണ്ണി(സ്ഥിരം AstVarRef* samep) സ്ഥിരം;
    വരിയിൽ bool sameNoLvalue(സ്ഥിരം AstVarRef* samep) സ്ഥിരം;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
};
class AstVarXRef final : പൊതു Ast_കണ്ണി_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_containsGenBlock = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
പൊതു:
    AstVarXRef(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& dotted, സ്ഥിരം VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    വരിയിൽ AstVarXRef(File_വരി* fl, AstVar* varp, സ്ഥിരം string& dotted, സ്ഥിരം VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    ശൂന്യം name(സ്ഥിരം std::string& name) override { m_name = name; }  // * = Var name
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string dotted() സ്ഥിരം { തിരികെ m_dotted; }
    ശൂന്യം dotted(സ്ഥിരം string& dotted) { m_dotted = dotted; }
    string inlinedDots() സ്ഥിരം { തിരികെ m_inlinedDots; }
    ശൂന്യം inlinedDots(സ്ഥിരം string& flag) { m_inlinedDots = flag; }
    bool containsGenBlock() സ്ഥിരം { തിരികെ m_containsGenBlock; }
    ശൂന്യം containsGenBlock(സ്ഥിരം bool flag) { m_containsGenBlock = flag; }
    bool readOnlyModport() സ്ഥിരം { തിരികെ m_readOnlyModport; }
    ശൂന്യം readOnlyModport(സ്ഥിരം bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        തിരികെ (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#endif  // Guard
