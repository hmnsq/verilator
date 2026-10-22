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

#నిర్వచితం_కాకపోతే VERILATOR_V3ASTNODEEXPR_H_
#నిర్వచించు VERILATOR_V3ASTNODEEXPR_H_

#నిర్వచితం_కాకపోతే VERILATOR_V3AST_H_
#దోషం "Use V3Ast.h as the include"
#అన్తర్భావ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#నిర్వచించు VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#అయితే_ముగింపు

// === Abstract base node types (AstNode*) =====================================

class Ast_కణుపు_సమాసం VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // An expression tree node
రక్షిత:
    Ast_కణుపు_సమాసం(VNType t, File_పంక్తి* fl)
        : Ast_కణుపు{t, fl} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_సమాసం;
    // METHODS
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    // Every expression must have a data type after V3Width
    bool hasDType() స్థిరరాశి override final VL_MT_సురక్షిత { తిరిగి true; }
    మాయా string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    మాయా string emitC() = 0;
    మాయా string emitSMT() స్థిరరాశి { తిరిగి ""; };
    మాయా string emitSimpleOperator() { తిరిగి ""; }  // "" means not ok to use
    మాయా bool emit_Check_Max_పదాలు() { తిరిగి false; }  // Check VL_MULS_MAX_WORDS
    మాయా bool clean_బహిర్గతం() స్థిరరాశి = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    మాయా bool is_Multi_చక్రం_Sva() స్థిరరాశి { తిరిగి false; }
    స్థిరరాశి Ast_కణుపు_సమాసం* get_VAccess_లక్ష్యం_Recurse() స్థిరరాశి;
    Ast_కణుపు_సమాసం* get_VAccess_లక్ష్యం_Recurse() {
        తిరిగి const_cast<Ast_కణుపు_సమాసం*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<స్థిరరాశి Ast_కణుపు_సమాసం*>(this)->get_VAccess_లక్ష్యం_Recurse());
    }
    VAccess getVAccessRecurse() స్థిరరాశి;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() స్థిరరాశి { తిరిగి getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_వృక్షం() స్థిరరాశి;
    // Return base var (or const) nodep dereferences
    Ast_కణుపు* పునాది_Fromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    అంతఃవరుస Ast_Stmt_సమాసం* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    మాయా స్థిరరాశి అక్షరం* వెడల్పు_Mismatch() స్థిరరాశి VL_MT_STABLE { తిరిగి nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string నమూనా_String(uint32_t లోతు = 0) స్థిరరాశి;
    bool isFourstateOrNoDtype() స్థిరరాశి {
        ఒకవేళ (!dtypep()) తిరిగి true;
        తిరిగి dtypep()->skipRefp()->isFourstate();
    }
    స్థిర bool isFourstateOrNoDtype(స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr1p) {
        తిరిగి expr1p->isFourstateOrNoDtype();
    }
    స్థిర bool isFourstateOrNoDtype(స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr1p,
                                     స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr2p) {
        తిరిగి expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    స్థిర bool isFourstateOrNoDtype(స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr1p,
                                     స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr2p,
                                     స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr3p) {
        తిరిగి isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    స్థిర bool isFourstateOrNoDtype(స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr1p,
                                     స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr2p,
                                     స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr3p,
                                     స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి expr4p) {
        తిరిగి isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_కణుపు_Biop VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

రక్షిత:
    Ast_కణుపు_Biop(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : Ast_కణుపు_సమాసం{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    మాయా శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) = 0;
    మాయా bool cleanLhs() స్థిరరాశి = 0;  // True if LHS must have extra upper bits zero
    మాయా bool cleanRhs() స్థిరరాశి = 0;  // True if RHS must have extra upper bits zero
    మాయా bool కొలత_Matters_Lhs() స్థిరరాశి = 0;  // True if output result depends on lhs size
    మాయా bool కొలత_Matters_Rhs() స్థిరరాశి = 0;  // True if output result depends on rhs size
    మాయా bool doubleFlavor() స్థిరరాశి { తిరిగి false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    మాయా bool signedFlavor() స్థిరరాశి { తిరిగి false; }
    మాయా bool stringFlavor() స్థిరరాశి { తిరిగి false; }  // N flavor of nodes with both flavors?
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    bool is_శుద్ధ() override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;

అంతరంగ:
    bool getPurityRecurse() స్థిరరాశి { తిరిగి lhsp()->is_శుద్ధ() && rhsp()->is_శుద్ధ(); }
};
class Ast_కణుపు_Bi_Com VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Biop {
    // Binary expr with commutative properties
రక్షిత:
    Ast_కణుపు_Bi_Com(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhs, Ast_కణుపు_సమాసం* rhs)
        : Ast_కణుపు_Biop{t, fl, lhs, rhs} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Bi_Com;
};
class Ast_కణుపు_Bi_Com_Asv VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Bi_Com {
    // Binary expr with commutative & associative properties
రక్షిత:
    Ast_కణుపు_Bi_Com_Asv(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhs, Ast_కణుపు_సమాసం* rhs)
        : Ast_కణుపు_Bi_Com{t, fl, lhs, rhs} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Bi_Com_Asv;
};
class Ast_కణుపు_Dist_Biop VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    Ast_కణుపు_Dist_Biop(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : Ast_కణుపు_Biop{t, fl, lhsp, rhsp} {
        dtype_సమితి_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_కణుపు_Dist_Biop;
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL_TRIG; }
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
};
class Ast_కణుపు_Sel VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
రక్షిత:
    Ast_కణుపు_Sel(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* bitp)
        : Ast_కణుపు_Biop{t, fl, fromp, bitp} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Sel;
    పూర్ణాంకం బిట్టు_Const() స్థిరరాశి;
};
class Ast_కణుపు_ప్రవాహం VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
రక్షిత:
    Ast_కణుపు_ప్రవాహం(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : Ast_కణుపు_Biop{t, fl, lhsp, rhsp} {
        ఒకవేళ (lhsp->dtypep()) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(lhsp->వెడల్పు(), lhsp->వెడల్పు_Min(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_ప్రవాహం;
};
class Ast_కణుపు_System_Biop_D VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    Ast_కణుపు_System_Biop_D(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : Ast_కణుపు_Biop{t, fl, lhsp, rhsp} {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_Ast_కణుపు_System_Biop_D;
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL_TRIG; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class Ast_కణుపు_CCall VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

రక్షిత:
    Ast_కణుపు_CCall(VNType t, File_పంక్తి* fl, AstCFunc* funcp, Ast_కణుపు_సమాసం* argsp = nullptr)
        : Ast_కణుపు_సమాసం{t, fl}
        , m_funcp{funcp} {
        కూడు_Argsp(argsp);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_CCall;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_CALL; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_కణుపు_CCall* స్థిరరాశి asamep = VN_DBG_AS(samep, కణుపు_CCall);
        తిరిగి (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override;
    bool isOutputter() override { తిరిగి !is_శుద్ధ(); }
    AstCFunc* funcp() స్థిరరాశి { తిరిగి m_funcp; }
    శూన్య funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() స్థిరరాశి { తిరిగి m_argTypes; }
    శూన్య argTypes(స్థిరరాశి string& str) { m_argTypes = str; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి final override { తిరిగి true; }
    bool superReference() స్థిరరాశి { తిరిగి m_superReference; }
    శూన్య superReference(bool flag) { m_superReference = flag; }
};
class Ast_కణుపు_Cons VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // Construct a value of an aggregate type
    మిత్ర class Ast_కణుపు_సమాసం;  // For m_knownConstantTree
    mutable bool m_తెలిసిన_Constant_వృక్షం = false;  // Known to be 'this->isConstantTree() == true'

రక్షిత:
    Ast_కణుపు_Cons(VNType t, File_పంక్తి* fl)
        : Ast_కణుపు_సమాసం{t, fl} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Cons;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి final override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి final override { తిరిగి వెడల్పు_Instrs(); }
};
class Ast_కణుపు_FTask_Ref VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_కణుపు_FTask_Ref;  // Gen pointers before other members for performance
అంతరంగ:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_కలిగియుంది_Gen_ఖండం = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

రక్షిత:
    Ast_కణుపు_FTask_Ref(VNType t, File_పంక్తి* fl, స్థిరరాశి string& name, AstArg* argsp)
        : Ast_కణుపు_సమాసం{t, fl}
        , m_name{name} {
        కూడు_Argsp(argsp);
    }

బహిరంగ:
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    bool isGateOptimizable() స్థిరరాశి override;
    string dotted() స్థిరరాశి { తిరిగి m_dotted; }  // * = Scope name or ""
    string inlinedDots() స్థిరరాశి { తిరిగి m_inlinedDots; }
    శూన్య inlinedDots(స్థిరరాశి string& flag) { m_inlinedDots = flag; }
    Ast_కణుపు_FTask* taskp() స్థిరరాశి { తిరిగి m_taskp; }  // [After Link] Pointer to variable
    శూన్య taskp(Ast_కణుపు_FTask* taskp) { m_taskp = taskp; }
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    శూన్య dotted(స్థిరరాశి string& name) { m_dotted = name; }
    Ast_కణుపు_Module* classOrPackagep() స్థిరరాశి { తిరిగి m_classOrPackagep; }
    శూన్య classOrPackagep(Ast_కణుపు_Module* nodep) { m_classOrPackagep = nodep; }
    bool కలిగియుంది_Gen_ఖండం() స్థిరరాశి { తిరిగి m_కలిగియుంది_Gen_ఖండం; }
    శూన్య కలిగియుంది_Gen_ఖండం(స్థిరరాశి bool flag) { m_కలిగియుంది_Gen_ఖండం = flag; }
    bool pli() స్థిరరాశి { తిరిగి m_pli; }
    శూన్య pli(bool flag) { m_pli = flag; }
    bool superReference() స్థిరరాశి { తిరిగి m_superReference; }
    శూన్య superReference(bool flag) { m_superReference = flag; }
    bool is_శుద్ధ() override;
    bool isUnlikely() స్థిరరాశి override { తిరిగి m_taskp && m_taskp->isUnlikely(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_కణుపు_FTask_Ref* స్థిరరాశి asamep = VN_DBG_AS(samep, కణుపు_FTask_Ref);
        తిరిగి taskp() == asamep->taskp()  //
               && classOrPackagep() == asamep->classOrPackagep()  //
               && name() == asamep->name()  //
               && dotted() == asamep->dotted()  //
               && inlinedDots() == asamep->inlinedDots()  //
               && pli() == asamep->pli()  //
               && కలిగియుంది_Gen_ఖండం() == asamep->కలిగియుంది_Gen_ఖండం()
               && superReference() == asamep->superReference();
    }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి final override { V3ERROR_NA_RETURN(true); }

అంతరంగ:
    bool getPurityRecurse() స్థిరరాశి;
};
class Ast_కణుపు_Pre_Sel VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

రక్షిత:
    Ast_కణుపు_Pre_Sel(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు_సమాసం* thsp)
        : Ast_కణుపు_సమాసం{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Pre_Sel;
    // METHODS
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి final override { V3ERROR_NA_RETURN(true); }
    bool is_శుద్ధ() override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;

అంతరంగ:
    bool getPurityRecurse() స్థిరరాశి {
        తిరిగి fromp()->is_శుద్ధ() && rhsp()->is_శుద్ధ() && (!thsp() || thsp()->is_శుద్ధ());
    }
};
class Ast_కణుపు_Quadop VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

రక్షిత:
    Ast_కణుపు_Quadop(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు_సమాసం* thsp,
                  Ast_కణుపు_సమాసం* fhsp)
        : Ast_కణుపు_సమాసం{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    మాయా శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs,
                               స్థిరరాశి V3_సంఖ్య& ths, స్థిరరాశి V3_సంఖ్య& fhs)
        = 0;
    మాయా bool cleanLhs() స్థిరరాశి = 0;  // True if LHS must have extra upper bits zero
    మాయా bool cleanRhs() స్థిరరాశి = 0;  // True if RHS must have extra upper bits zero
    మాయా bool cleanThs() స్థిరరాశి = 0;  // True if THS must have extra upper bits zero
    మాయా bool cleanFhs() స్థిరరాశి = 0;  // True if THS must have extra upper bits zero
    మాయా bool కొలత_Matters_Lhs() స్థిరరాశి = 0;  // True if output result depends on lhs size
    మాయా bool కొలత_Matters_Rhs() స్థిరరాశి = 0;  // True if output result depends on rhs size
    మాయా bool కొలత_Matters_Ths() స్థిరరాశి = 0;  // True if output result depends on ths size
    మాయా bool కొలత_Matters_Fhs() స్థిరరాశి = 0;  // True if output result depends on ths size
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    bool is_శుద్ధ() override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;

అంతరంగ:
    bool getPurityRecurse() స్థిరరాశి {
        తిరిగి lhsp()->is_శుద్ధ() && rhsp()->is_శుద్ధ() && thsp()->is_శుద్ధ() && fhsp()->is_శుద్ధ();
    }
};
class Ast_కణుపు_Termop VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // Terminal operator -- an operator with no "inputs"
రక్షిత:
    Ast_కణుపు_Termop(VNType t, File_పంక్తి* fl)
        : Ast_కణుపు_సమాసం{t, fl} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Termop;
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class Ast_కణుపు_Triop VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

రక్షిత:
    Ast_కణుపు_Triop(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు_సమాసం* thsp)
        : Ast_కణుపు_సమాసం{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Triop;
    // METHODS
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    // Set out to evaluation of a AstConst'ed
    మాయా శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs,
                               స్థిరరాశి V3_సంఖ్య& ths)
        = 0;
    మాయా bool cleanLhs() స్థిరరాశి = 0;  // True if LHS must have extra upper bits zero
    మాయా bool cleanRhs() స్థిరరాశి = 0;  // True if RHS must have extra upper bits zero
    మాయా bool cleanThs() స్థిరరాశి = 0;  // True if THS must have extra upper bits zero
    మాయా bool కొలత_Matters_Lhs() స్థిరరాశి = 0;  // True if output result depends on lhs size
    మాయా bool కొలత_Matters_Rhs() స్థిరరాశి = 0;  // True if output result depends on rhs size
    మాయా bool కొలత_Matters_Ths() స్థిరరాశి = 0;  // True if output result depends on ths size
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    bool is_శుద్ధ() override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;

అంతరంగ:
    bool getPurityRecurse() స్థిరరాశి {
        తిరిగి lhsp()->is_శుద్ధ() && rhsp()->is_శుద్ధ() && thsp()->is_శుద్ధ();
    }
};
class Ast_కణుపు_Dist_Triop VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Triop {
బహిరంగ:
    Ast_కణుపు_Dist_Triop(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp,
                     Ast_కణుపు_సమాసం* thsp)
        : Ast_కణుపు_Triop{t, fl, lhsp, rhsp, thsp} {
        dtype_సమితి_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_కణుపు_Dist_Triop;
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanThs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Ths() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL_TRIG; }
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs,
                       స్థిరరాశి V3_సంఖ్య& ths) override {
        V3ERROR_NA;
    }
};
class Ast_కణుపు_Uniop VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

రక్షిత:
    Ast_కణుపు_Uniop(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : Ast_కణుపు_సమాసం{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Uniop;
    // METHODS
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    // Set out to evaluation of a AstConst'ed lhs
    మాయా శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) = 0;
    మాయా bool cleanLhs() స్థిరరాశి = 0;
    మాయా bool కొలత_Matters_Lhs() స్థిరరాశి = 0;  // True if output result depends on lhs size
    మాయా bool doubleFlavor() స్థిరరాశి { తిరిగి false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    మాయా bool signedFlavor() స్థిరరాశి { తిరిగి false; }
    మాయా bool stringFlavor() స్థిరరాశి { తిరిగి false; }  // N flavor of nodes with both flavors?
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    bool is_శుద్ధ() override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
};
class Ast_కణుపు_System_Uniop_D VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Uniop {
బహిరంగ:
    Ast_కణుపు_System_Uniop_D(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : Ast_కణుపు_Uniop{t, fl, lhsp} {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_Ast_కణుపు_System_Uniop_D;
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL_TRIG; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class Ast_కణుపు_Var_Ref VL_NOT_FINAL : బహిరంగ Ast_కణుపు_సమాసం {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_కణుపు_Var_Ref;  // Gen pointers before other members for performance
అంతరంగ:
    VAccess m_access;  // Left hand side assignment
    VSelf_సూచిక_పాఠ్యం m_self_సూచిక
        = VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::ఖాళీలు_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
రక్షిత:
    Ast_కణుపు_Var_Ref(VNType t, File_పంక్తి* fl, AstVar* varp, స్థిరరాశి VAccess& access)
        : Ast_కణుపు_సమాసం{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

బహిరంగ:
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    VAccess access() స్థిరరాశి { తిరిగి m_access; }
    శూన్య access(స్థిరరాశి VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_varp; }  // [After Link] Pointer to variable
    శూన్య varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_కణుపు*>(varp));
    }
    AstVarScope* varScopep() స్థిరరాశి { తిరిగి m_varScopep; }
    శూన్య varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    స్థిరరాశి VSelf_సూచిక_పాఠ్యం& self_సూచిక() స్థిరరాశి { తిరిగి m_self_సూచిక; }
    శూన్య self_సూచిక(స్థిరరాశి VSelf_సూచిక_పాఠ్యం& self_సూచిక) { m_self_సూచిక = self_సూచిక; }
    string self_సూచిక_Protect(bool useSelfForThis) స్థిరరాశి {
        తిరిగి self_సూచిక().protect(useSelfForThis, protect());
    }
    Ast_కణుపు_Module* classOrPackagep() స్థిరరాశి { తిరిగి m_classOrPackagep; }
    శూన్య classOrPackagep(Ast_కణుపు_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : బహిరంగ Ast_కణుపు {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
బహిరంగ:
    AstArg(File_పంక్తి* fl, స్థిరరాశి std::string& name, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి std::string& name) override { m_name = name; }
    bool ఖాళీలు_Connect_No_తదుపరి() స్థిరరాశి { తిరిగి !exprp() && name() == "" && !nextp(); }
};
class AstWith final : బహిరంగ Ast_కణుపు {
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
అంతరంగ:
    // 'with (identifier_list) {...}' restricted form (IEEE 1800-2023 18.7).
    bool m_restricted = false;
    // dist-ast-dump-suppress  // V3LinkDot temporary use only
    bool m_validated = false;  // identifier_list typo / unused checks already run
    std::సమితి<std::string> m_restricted_పేర్లు;

బహిరంగ:
    AstWith(File_పంక్తి* fl, AstLambdaArgRef* సూచీ_Arg_Refp, AstLambdaArgRef* విలువ_Arg_Refp,
            Ast_కణుపు* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->సూచీ_Arg_Refp(సూచీ_Arg_Refp);
        this->విలువ_Arg_Refp(విలువ_Arg_Refp);
        కూడు_Exprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!సూచీ_Arg_Refp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!విలువ_Arg_Refp());  // varp needed to know lambda's arg dtype
        తిరిగి nullptr;
    }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    శూన్య restricted(bool flag) { m_restricted = flag; }
    bool restricted() స్థిరరాశి { తిరిగి m_restricted; }
    bool validated() స్థిరరాశి { తిరిగి m_validated; }
    శూన్య validated(bool flag) { m_validated = flag; }
    శూన్య కూడు_Restricted_Name(స్థిరరాశి std::string& name) { m_restricted_పేర్లు.చొప్పించు(name); }
    స్థిరరాశి std::సమితి<std::string>& restricted_పేర్లు() స్థిరరాశి { తిరిగి m_restricted_పేర్లు; }
    // True if 'name' binds into the randomize() target class.
    bool name_Resolves_గమ్యం_లక్ష్యం(స్థిరరాశి std::string& name) స్థిరరాశి {
        తిరిగి !m_restricted || m_restricted_పేర్లు.లెక్క(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : బహిరంగ Ast_కణుపు_సమాసం {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbort_రకం m_రకం{VAbort_రకం::ACCEPT_ON};

బహిరంగ:
    AstAbortOn(File_పంక్తి* fl, VAbort_రకం రకం, Ast_కణుపు_సమాసం* condp, Ast_కణుపు_సమాసం* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_రకం{రకం} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    VAbort_రకం రకం() స్థిరరాశి { తిరిగి m_రకం; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class Ast_Addr_యొక్క_CFunc final : బహిరంగ Ast_కణుపు_సమాసం {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

బహిరంగ:
    Ast_Addr_యొక్క_CFunc(File_పంక్తి* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_Addr_యొక్క_CFunc(fl)
        , m_funcp{funcp} {
        dtypep(కనుగొను_CHandle_DType());
    }

    ASTGEN_MEMBERS_Ast_Addr_యొక్క_CFunc;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    AstCFunc* funcp() స్థిరరాశి { తిరిగి m_funcp; }
};
class Ast_Attr_యొక్క final : బహిరంగ Ast_కణుపు_సమాసం {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
బహిరంగ:
    Ast_Attr_యొక్క(File_పంక్తి* fl, VAttrType attrtype, Ast_కణుపు* fromp = nullptr,
              Ast_కణుపు_సమాసం* dimp = nullptr)
        : ASTGEN_SUPER_Attr_యొక్క(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_Ast_Attr_యొక్క;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    VAttrType attrType() స్థిరరాశి { తిరిగి m_attrType; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : బహిరంగ Ast_కణుపు_సమాసం {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    స్థిరరాశి bool m_శుద్ధ;  // Pure optimizable

    శూన్య init(స్థిరరాశి string& పాఠ్యం, పూర్ణాంకం setwidth) {
        ఒకవేళ (!పాఠ్యం.ఖాళీలు()) కూడు(పాఠ్యం);
        ఒకవేళ (setwidth) {
            dtype_సమితి_బిట్టు_Sized(setwidth, VSigning::UNSIGNED);
        } లేకపోతే {
            dtype_సమితి_Void();  // Caller to override if necessary
        }
    }

బహిరంగ:
    class శుద్ధ {};
    explicit AstCExpr(File_పంక్తి* fl, స్థిరరాశి string& పాఠ్యం = "", పూర్ణాంకం setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_శుద్ధ{false} {
        init(పాఠ్యం, setwidth);
    }
    AstCExpr(File_పంక్తి* fl, శుద్ధ, స్థిరరాశి string& పాఠ్యం = "", పూర్ణాంకం setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_శుద్ధ{true} {
        init(పాఠ్యం, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి m_శుద్ధ; }
    bool isOutputter() override { తిరిగి true; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి m_శుద్ధ; }
    bool is_శుద్ధ() override { తిరిగి m_శుద్ధ; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    // Add some text, or a node to this expression
    శూన్య కూడు(స్థిరరాశి std::string& పాఠ్యం) { కూడు_Nodesp(new Ast_పాఠ్యం{fileline(), పాఠ్యం}); }
    శూన్య కూడు(Ast_కణుపు* nodep) { కూడు_Nodesp(nodep); }
};
class AstCExprUser final : బహిరంగ Ast_కణుపు_సమాసం {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    స్థిరరాశి bool m_శుద్ధ;  // Whether the function is pure
బహిరంగ:
    class శుద్ధ {};
    explicit AstCExprUser(File_పంక్తి* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_శుద్ధ{false} {}
    AstCExprUser(File_పంక్తి* fl, శుద్ధ)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_శుద్ధ{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి m_శుద్ధ; }
    bool isOutputter() override { తిరిగి true; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి m_శుద్ధ; }
    bool is_శుద్ధ() override { తిరిగి m_శుద్ధ; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    // Add some text, or a node to this expression
    శూన్య కూడు(స్థిరరాశి std::string& పాఠ్యం) { కూడు_Nodesp(new Ast_పాఠ్యం{fileline(), పాఠ్యం}); }
    శూన్య కూడు(Ast_కణుపు* nodep) { కూడు_Nodesp(nodep); }
};
class AstCFuncHard final : బహిరంగ Ast_కణుపు_సమాసం {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
బహిరంగ:
    AstCFuncHard(File_పంక్తి* fl, VCFunction function, Ast_కణుపు_సమాసం* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        కూడు_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి function().ascii(); }
    bool is_శుద్ధ() override {
        ఒకవేళ (!m_purity.isCached()) m_purity.సమితి(getPurity());
        తిరిగి m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    VCFunction function() స్థిరరాశి VL_MT_STABLE { తిరిగి m_function; }

అంతరంగ:
    bool getPurity();
};
class AstCMethodHard final : బహిరంగ Ast_కణుపు_సమాసం {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_పద్ధతి;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
బహిరంగ:
    AstCMethodHard(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, VCMethod పద్ధతి, Ast_కణుపు_సమాసం* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_పద్ధతి{పద్ధతి} {
        this->fromp(fromp);
        కూడు_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి పద్ధతి().ascii(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstCMethodHard* స్థిరరాశి asamep = VN_DBG_AS(samep, CMethodHard);
        తిరిగి (m_పద్ధతి == asamep->m_పద్ధతి);
    }
    bool is_శుద్ధ() override {
        ఒకవేళ (!m_purity.isCached()) m_purity.సమితి(getPurity());
        తిరిగి m_purity.get();
    }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి m_పద్ధతి.isClean(); }
    bool usePtr() స్థిరరాశి { తిరిగి m_usePtr; }
    శూన్య usePtr(bool flag) { m_usePtr = flag; }
    VCMethod పద్ధతి() స్థిరరాశి VL_MT_STABLE { తిరిగి m_పద్ధతి; }
    శూన్య పద్ధతి(VCMethod విలువ) { m_పద్ధతి = విలువ; }

అంతరంగ:
    bool getPurity();
};
class AstCReset final : బహిరంగ Ast_కణుపు_సమాసం {
    // Reset variable at startup
    స్థిరరాశి bool m_constructing;  // Previously cleared by constructor
బహిరంగ:
    AstCReset(File_పంక్తి* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool is_శుద్ధ() override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!VN_IS(backp(), కణుపు_కేటాయించు));  // V3Emit* assumption
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() స్థిరరాశి { తిరిగి m_constructing; }
};
class AstCast final : బహిరంగ Ast_కణుపు_సమాసం {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
బహిరంగ:
    AstCast(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        బిడ్డ_DTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { తిరిగి "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి VL_MT_STABLE { తిరిగి dtypep() ? dtypep() : బిడ్డ_DTypep(); }
};
class AstCastParse final : బహిరంగ Ast_కణుపు_సమాసం {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
బహిరంగ:
    AstCastParse(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { తిరిగి "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cast_కొలత final : బహిరంగ Ast_కణుపు_సమాసం {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
బహిరంగ:
    Ast_Cast_కొలత(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_Cast_కొలత(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_Ast_Cast_కొలత;
    string emitVerilog() override { తిరిగి "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : బహిరంగ Ast_కణుపు_సమాసం {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
బహిరంగ:
    AstCellArrayRef(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_సమాసం* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        కూడు_Selp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : బహిరంగ Ast_కణుపు_సమాసం {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
బహిరంగ:
    AstCellRef(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* cellp, Ast_కణుపు* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
బహిరంగ:
    AstClassOrPackageRef(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        కూడు_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_కణుపు_Module* classOrPackageSkipp(స్థిరరాశి bool doRefs = true) స్థిరరాశి;
    Ast_కణుపు* classOrPackageNodep() స్థిరరాశి { తిరిగి m_classOrPackageNodep; }
    శూన్య classOrPackageNodep(Ast_కణుపు* nodep) { m_classOrPackageNodep = nodep; }
    శూన్య classOrPackagep(Ast_కణుపు_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_కణుపు*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : బహిరంగ Ast_కణుపు_సమాసం {
    // A constant
    V3_సంఖ్య m_అంకె;  // Constant value
    శూన్య init_With_సంఖ్య() {
        ఒకవేళ (m_అంకె.isDouble()) {
            dtype_సమితి_Double();
        } లేకపోతే ఒకవేళ (m_అంకె.isString()) {
            dtype_సమితి_String();
        } లేకపోతే {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(m_అంకె.వెడల్పు(), (m_అంకె.sized() ? 0 : m_అంకె.వెడల్పు_గమ్యం_Fit()),
                                      VSigning::fromBool(m_అంకె.isSigned()), m_అంకె.isAnyXZ());
        }
        m_అంకె.nodep(this);
    }

బహిరంగ:
    ~AstConst() override;
    AstConst(File_పంక్తి* fl, స్థిరరాశి V3_సంఖ్య& అంకె)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{అంకె} {
        init_With_సంఖ్య();
    }
    class Widthed_విలువ {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, Widthed_విలువ, పూర్ణాంకం వెడల్పు, uint32_t విలువ)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, వెడల్పు, విలువ) {  // Need () constructor
        init_With_సంఖ్య();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_పంక్తి* fl, DTyped, స్థిరరాశి Ast_కణుపు_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{this, nodedtypep} {
        init_With_సంఖ్య();
    }
    class String_గమ్యం_Parse {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, String_గమ్యం_Parse, స్థిరరాశి అక్షరం* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{this, V3_సంఖ్య::Verilog_సంఖ్య_Literal{}, sourcep} {
        init_With_సంఖ్య();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, VerilogStringLiteral, స్థిరరాశి string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{this, V3_సంఖ్య::VerilogStringLiteral{}, str} {
        init_With_సంఖ్య();
    }
    AstConst(File_పంక్తి* fl, uint32_t అంకె)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, 32, అంకె) {  // Need () constructor
        dtype_సమితి_బిట్టు_Unsized(m_అంకె.వెడల్పు(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, Unsized32, uint32_t అంకె)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, 32, అంకె) {  // Need () constructor
        m_అంకె.వెడల్పు(32, false);
        dtype_సమితి_బిట్టు_Unsized(32, m_అంకె.వెడల్పు_గమ్యం_Fit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, Signed32, int32_t అంకె)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, 32, అంకె) {  // Need () constructor
        m_అంకె.వెడల్పు(32, true);
        dtype_సమితి_బిట్టు_Unsized(32, m_అంకె.వెడల్పు_గమ్యం_Fit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, Unsized64, uint64_t అంకె)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, 64, 0) {  // Need () constructor
        m_అంకె.సమితి_Quad(అంకె);
        dtype_సమితి_బిట్టు_Sized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, SizedEData, uint64_t అంకె)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_అంకె.సమితి_Quad(అంకె);
        dtype_సమితి_బిట్టు_Sized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, RealDouble, ద్విగుణ అంకె)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, 64) {  // Need () constructor
        m_అంకె.సమితి_Double(అంకె);
        dtype_సమితి_Double();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_పంక్తి* fl, String, స్థిరరాశి string& అంకె)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{this, V3_సంఖ్య::String{}, అంకె} {
        dtype_సమితి_String();
    }
    class బిట్టు_False {};
    AstConst(File_పంక్తి* fl, బిట్టు_False)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, 1, 0) {  // Need () constructor
        dtype_సమితి_బిట్టు();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class బిట్టు_False_Erroring {};
    AstConst(File_పంక్తి* fl, బిట్టు_False_Erroring)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, 1, 0) {  // Need () constructor
        dtype_సమితి_బిట్టు();
        File_పంక్తి* స్థిరరాశి newfl = new File_పంక్తి{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class బిట్టు_True {};
    AstConst(File_పంక్తి* fl, బిట్టు_True, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె(this, 1, on) {  // Need () constructor
        dtype_సమితి_బిట్టు();
    }
    class All0 {};
    AstConst(File_పంక్తి* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{this, V3_సంఖ్య::Verilog_సంఖ్య_Literal{}, "'0"} {
        init_With_సంఖ్య();
        fl->warnOff(V3_Error_సంకేతం::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_పంక్తి* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{this, V3_సంఖ్య::Verilog_సంఖ్య_Literal{}, "'1"} {
        init_With_సంఖ్య();
        fl->warnOff(V3_Error_సంకేతం::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_పంక్తి* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{this, V3_సంఖ్య::Null{}} {
        dtypep(కనుగొను_Null_DType());
        init_With_సంఖ్య();
    }
    class One_అడుగు {};
    AstConst(File_పంక్తి* fl, One_అడుగు)
        : ASTGEN_SUPER_Const(fl)
        , m_అంకె{this, V3_సంఖ్య::One_అడుగు{}} {
        dtype_సమితి_బిట్టు_Sized(64, VSigning::UNSIGNED);
        init_With_సంఖ్య();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి అంకె().ascii(); }  // * = Value
    స్థిరరాశి V3_సంఖ్య& అంకె() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_అంకె; }  // * = Value
    V3_సంఖ్య& అంకె() { తిరిగి m_అంకె; }  // * = Value
    string origParamName() స్థిరరాశి;
    శూన్య origParamName(స్థిరరాశి string& name);
    uint32_t గమ్యం_UInt() స్థిరరాశి { తిరిగి అంకె().గమ్యం_UInt(); }
    int32_t గమ్యం_SInt() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి అంకె().గమ్యం_SInt(); }
    uint64_t గమ్యం_UQuad() స్థిరరాశి { తిరిగి అంకె().గమ్యం_UQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstConst* స్థిరరాశి sp = VN_DBG_AS(samep, Const);
        తిరిగి అంకె().isCaseEq(sp->అంకె());
    }
    శూన్య cloneRelink() override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(m_అంకె.nodep() && m_అంకె.nodep() != this);
        తిరిగి nullptr;
    }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool isEqAllOnes() స్థిరరాశి { తిరిగి అంకె().isEqAllOnes(వెడల్పు()); }
    bool isEqAllOnesV() స్థిరరాశి { తిరిగి అంకె().isEqAllOnes(వెడల్పు_Min_V()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_విలువ_Type(స్థిరరాశి AstConst* samep) స్థిరరాశి;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    స్థిర AstConst* parseParamLiteral(File_పంక్తి* fl, స్థిరరాశి string& literal);
};
class AstConstraintRef final : బహిరంగ Ast_కణుపు_సమాసం {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

బహిరంగ:
    AstConstraintRef(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(కనుగొను_Constraint_Ref_DType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() స్థిరరాశి override VL_MT_STABLE;
    AstConstraint* constrp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_constrp; }
    శూన్య constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_కణుపు_Module* classOrPackagep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_classOrPackagep; }
    // TODO: this setter is unused
    శూన్య classOrPackagep(Ast_కణుపు_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి final override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cvt_Array_గమ్యం_Array final : బహిరంగ Ast_కణుపు_సమాసం {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
అంతరంగ:
    స్థిరరాశి bool m_తిరగేయి;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    స్థిరరాశి పూర్ణాంకం m_ఖండం_కొలత;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    స్థిరరాశి పూర్ణాంకం m_dst_Element_బిట్లు;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    స్థిరరాశి పూర్ణాంకం m_src_Element_బిట్లు;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

బహిరంగ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_గమ్యం_Array(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_DType* dtp, bool తిరగేయి,
                       పూర్ణాంకం ఖండం_కొలత, పూర్ణాంకం dst_Element_బిట్లు, పూర్ణాంకం src_Element_బిట్లు)
        : ASTGEN_SUPER_Cvt_Array_గమ్యం_Array(fl)
        , m_తిరగేయి{తిరగేయి}
        , m_ఖండం_కొలత{ఖండం_కొలత}
        , m_dst_Element_బిట్లు{dst_Element_బిట్లు}
        , m_src_Element_బిట్లు{src_Element_బిట్లు} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_గమ్యం_Array;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool తిరగేయి() స్థిరరాశి { తిరిగి m_తిరగేయి; }
    పూర్ణాంకం ఖండం_కొలత() స్థిరరాశి { తిరిగి m_ఖండం_కొలత; }
    పూర్ణాంకం dst_Element_బిట్లు() స్థిరరాశి { తిరిగి m_dst_Element_బిట్లు; }
    పూర్ణాంకం src_Element_బిట్లు() స్థిరరాశి { తిరిగి m_src_Element_బిట్లు; }
};
class Ast_Cvt_Array_గమ్యం_Packed final : బహిరంగ Ast_కణుపు_సమాసం {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
బహిరంగ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_గమ్యం_Packed(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Cvt_Array_గమ్యం_Packed(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_గమ్యం_Packed;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
};
class Ast_Cvt_Packed_గమ్యం_Array final : బహిరంగ Ast_కణుపు_సమాసం {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
బహిరంగ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Packed_గమ్యం_Array(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Cvt_Packed_గమ్యం_Array(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Packed_గమ్యం_Array;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
};
class Ast_Cvt_Unpack_గమ్యం_వరుస final : బహిరంగ Ast_కణుపు_సమాసం {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
బహిరంగ:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Unpack_గమ్యం_వరుస(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_గమ్యం_వరుస(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_గమ్యం_వరుస;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { తిరిగి "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
};
class Ast_Cvt_Unpack_గమ్యం_Unpack final : బహిరంగ Ast_కణుపు_సమాసం {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
బహిరంగ:
    Ast_Cvt_Unpack_గమ్యం_Unpack(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_గమ్యం_Unpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_గమ్యం_Unpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
};
class AstDist final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
బహిరంగ:
    AstDist(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_Dist_అంశం* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        కూడు_Itemsp(itemsp);
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { తిరిగి "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }  // NA
};
class Ast_Dist_అంశం final : బహిరంగ Ast_కణుపు_సమాసం {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_is_సంపూర్ణ = false;  // True for weight ':/', false for ':='
బహిరంగ:
    Ast_Dist_అంశం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* rangep, Ast_కణుపు_సమాసం* weightp)
        : ASTGEN_SUPER_Dist_అంశం(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_Ast_Dist_అంశం;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { తిరిగి "%l "s + (m_is_సంపూర్ణ ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }  // NA
    శూన్య is_సంపూర్ణ(bool flag) { m_is_సంపూర్ణ = flag; }
    bool is_సంపూర్ణ() స్థిరరాశి { తిరిగి m_is_సంపూర్ణ; }
};
class AstDot final : బహిరంగ Ast_కణుపు_సమాసం {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    స్థిరరాశి bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
బహిరంగ:
    AstDot(File_పంక్తి* fl, bool colon, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    స్థిర Ast_కణుపు_సమాసం* newIfPkg(File_పంక్తి* fl, Ast_కణుపు_సమాసం* packageOrClassp, Ast_కణుపు_సమాసం* rhsp) {
        ఒకవేళ (!packageOrClassp) తిరిగి rhsp;
        తిరిగి new AstDot{fl, true, packageOrClassp, rhsp};
    }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool colon() స్థిరరాశి { తిరిగి m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class Ast_ఖాళీలు_వరుస final : బహిరంగ Ast_కణుపు_సమాసం {
బహిరంగ:
    explicit Ast_ఖాళీలు_వరుస(File_పంక్తి* fl)
        : ASTGEN_SUPER_ఖాళీలు_వరుస(fl) {}
    ASTGEN_MEMBERS_Ast_ఖాళీలు_వరుస;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { తిరిగి "{}"; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
};
class Ast_Enum_అంశం_Ref final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_కలిగియుంది_Gen_ఖండం = false;  // Contains gen block reference
బహిరంగ:
    Ast_Enum_అంశం_Ref(File_పంక్తి* fl, Ast_Enum_అంశం* itemp, Ast_కణుపు_Module* classOrPackagep)
        : ASTGEN_SUPER_Enum_అంశం_Ref(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_Ast_Enum_అంశం_Ref;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి itemp() ? itemp()->name() : m_name; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_Enum_అంశం_Ref* స్థిరరాశి sp = VN_DBG_AS(samep, Enum_అంశం_Ref);
        తిరిగి itemp() == sp->itemp() && dotted() == sp->dotted()
               && కలిగియుంది_Gen_ఖండం() == sp->కలిగియుంది_Gen_ఖండం();
    }
    Ast_Enum_అంశం* itemp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_itemp; }
    శూన్య itemp(Ast_Enum_అంశం* nodep) { m_itemp = nodep; }
    string dotted() స్థిరరాశి { తిరిగి m_dotted; }
    శూన్య dotted(స్థిరరాశి string& dotted) { m_dotted = dotted; }
    bool కలిగియుంది_Gen_ఖండం() స్థిరరాశి { తిరిగి m_కలిగియుంది_Gen_ఖండం; }
    శూన్య కలిగియుంది_Gen_ఖండం(స్థిరరాశి bool flag) { m_కలిగియుంది_Gen_ఖండం = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    Ast_కణుపు_Module* classOrPackagep() స్థిరరాశి { తిరిగి m_classOrPackagep; }
    శూన్య classOrPackagep(Ast_కణుపు_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_సమాసం_Stmt final : బహిరంగ Ast_కణుపు_సమాసం {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
అంతరంగ:
    bool m_has_ఫలితం = true;  // Returns result via resultp()

బహిరంగ:
    Ast_సమాసం_Stmt(File_పంక్తి* fl, Ast_కణుపు* stmtsp, Ast_కణుపు_సమాసం* resultp)
        : ASTGEN_SUPER_సమాసం_Stmt(fl) {
        కూడు_Stmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_సమాసం_Stmt;
    // METHODS
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool is_శుద్ధ() override {
        ఒకవేళ (Ast_కణుపు::afterCommentp(stmtsp())) తిరిగి false;
        తిరిగి resultp()->is_శుద్ధ();
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    bool has_ఫలితం() స్థిరరాశి { తిరిగి m_has_ఫలితం; }
    శూన్య has_ఫలితం(bool flag) { m_has_ఫలితం = flag; }
};
class AstFError final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
బహిరంగ:
    AstFError(File_పంక్తి* fl, Ast_కణుపు_సమాసం* filep, Ast_కణుపు_సమాసం* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { తిరిగి "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 64; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstFOpen final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
బహిరంగ:
    AstFOpen(File_పంక్తి* fl, Ast_కణుపు_సమాసం* filenamep, Ast_కణుపు_సమాసం* modep)
        : ASTGEN_SUPER_FOpen(fl) {
        this->filenamep(filenamep);
        this->modep(modep);
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpen;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    string verilogKwd() స్థిరరాశి override { తిరిగి "$fopen"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstFOpenMcd final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op2 := filenamep : AstNodeExpr
బహిరంగ:
    AstFOpenMcd(File_పంక్తి* fl, Ast_కణుపు_సమాసం* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    string verilogKwd() స్థిరరాశి override { తిరిగి "$fopen"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstFRead final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
బహిరంగ:
    AstFRead(File_పంక్తి* fl, Ast_కణుపు_సమాసం* memp, Ast_కణుపు_సమాసం* filep, Ast_కణుపు_సమాసం* startp,
             Ast_కణుపు_సమాసం* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtype_సమితి_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: makes output
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstFRewind final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := filep : Optional[AstNodeExpr]
బహిరంగ:
    AstFRewind(File_పంక్తి* fl, Ast_కణుపు_సమాసం* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstFScanF final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_పాఠ్యం;

బహిరంగ:
    AstFScanF(File_పంక్తి* fl, స్థిరరాశి string& పాఠ్యం, Ast_కణుపు_సమాసం* filep, Ast_కణుపు_సమాసం* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_పాఠ్యం{పాఠ్యం} {
        కూడు_Exprsp(exprsp);
        this->filep(filep);
        dtype_సమితి_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_పాఠ్యం; }
    string verilogKwd() స్థిరరాశి override { తిరిగి "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: makes output
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి పాఠ్యం() == VN_DBG_AS(samep, FScanF)->పాఠ్యం();
    }
    string పాఠ్యం() స్థిరరాశి { తిరిగి m_పాఠ్యం; }  // * = Text to display
    శూన్య పాఠ్యం(స్థిరరాశి string& పాఠ్యం) { m_పాఠ్యం = పాఠ్యం; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstFSeek final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
బహిరంగ:
    AstFSeek(File_పంక్తి* fl, Ast_కణుపు_సమాసం* filep, Ast_కణుపు_సమాసం* స్థానభ్రంశం, Ast_కణుపు_సమాసం* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->స్థానభ్రంశం(స్థానభ్రంశం);
        this->operation(operation);
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: makes output
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstFTell final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
బహిరంగ:
    AstFTell(File_పంక్తి* fl, Ast_కణుపు_సమాసం* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstFalling final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
బహిరంగ:
    AstFalling(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { తిరిగి "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstFell final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
బహిరంగ:
    AstFell(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_Sen_వృక్షం* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { తిరిగి "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstFuture final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
బహిరంగ:
    AstFuture(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_Sen_వృక్షం* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { తిరిగి "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstGatePin final : బహిరంగ Ast_కణుపు_సమాసం {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
బహిరంగ:
    AstGatePin(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { తిరిగి "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
};
class Ast_Get_Initial_Random_విత్తనం final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $get_initial_random_seed()
బహిరంగ:
    explicit Ast_Get_Initial_Random_విత్తనం(File_పంక్తి* fl)
        : ASTGEN_SUPER_Get_Initial_Random_విత్తనం(fl) {
        dtype_సమితి_Int();
    }
    ASTGEN_MEMBERS_Ast_Get_Initial_Random_విత్తనం;
    string emitVerilog() override { తిరిగి "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి true; }
    bool is_శుద్ధ() override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstImplication final : బహిరంగ Ast_కణుపు_సమాసం {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

అంతరంగ:
    స్థిరరాశి bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    స్థిరరాశి bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

బహిరంగ:
    AstImplication(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() స్థిరరాశి { తిరిగి m_isOverlapped; }
    bool isFollowedBy() స్థిరరాశి { తిరిగి m_isFollowedBy; }
};
class AstInside final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
బహిరంగ:
    AstInside(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        కూడు_Itemsp(itemsp);
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { తిరిగి "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }  // NA
};
class AstInsideRange final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
బహిరంగ:
    AstInsideRange(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { తిరిగి "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_కణుపు_సమాసం* newAndFromInside(Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp);
};
class AstLambdaArgRef final : బహిరంగ Ast_కణుపు_సమాసం {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_సూచీ;  // Index, not value

బహిరంగ:
    AstLambdaArgRef(File_పంక్తి* fl, స్థిరరాశి string& name, bool సూచీ)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_సూచీ{సూచీ} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    string emitVerilog() override { తిరిగి name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    bool సూచీ() స్థిరరాశి { తిరిగి m_సూచీ; }
    bool is_సమాసం_Coverage_Eligible() స్థిరరాశి override { తిరిగి false; }
};
class Ast_సరిపోలిక_Masked final : బహిరంగ Ast_కణుపు_సమాసం {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
బహిరంగ:
    అంతఃవరుస Ast_సరిపోలిక_Masked(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_Ast_సరిపోలిక_Masked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { తిరిగి "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    స్థిర uint32_t fold(స్థిరరాశి V3_సంఖ్య& lhs, AstVar* సరిపోలిక_Varp);
};
class AstMatches final : బహిరంగ Ast_కణుపు_సమాసం {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
బహిరంగ:
    AstMatches(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { తిరిగి "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstMemberSel final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
బహిరంగ:
    AstMemberSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, VFlag_బిడ్డ_DType, స్థిరరాశి string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    VAccess access() స్థిరరాశి { తిరిగి m_access; }
    శూన్య access(స్థిరరాశి VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    AstVar* varp() స్థిరరాశి { తిరిగి m_varp; }
    శూన్య varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : బహిరంగ Ast_కణుపు_సమాసం {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
బహిరంగ:
    AstNewCopy(File_పంక్తి* fl, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { తిరిగి "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
};
class AstNewDynamic final : బహిరంగ Ast_కణుపు_సమాసం {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
బహిరంగ:
    AstNewDynamic(File_పంక్తి* fl, Ast_కణుపు_సమాసం* sizep, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { తిరిగి "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
};
class AstPExpr final : బహిరంగ Ast_కణుపు_సమాసం {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
బహిరంగ:
    explicit AstPExpr(File_పంక్తి* fl, AstBegin* bodyp, Ast_కణుపు_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_పంక్తి* fl, AstBegin* bodyp, Ast_కణుపు_Stmt* finalp, Ast_కణుపు_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
};
class AstParseHolder final : బహిరంగ Ast_కణుపు_సమాసం {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
బహిరంగ:
    explicit AstParseHolder(File_పంక్తి* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : బహిరంగ Ast_కణుపు_సమాసం {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

బహిరంగ:
    AstParseRef(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_సమాసం* lhsp = nullptr,
                Ast_కణుపు_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstParseRef* స్థిరరాశి asamep = VN_DBG_AS(samep, ParseRef);
        తిరిగి m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $past
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := ticksp : Optional[AstNodeExpr]
    // @astgen op3 := sentreep : Optional[AstSenTree]
    // @astgen op4 := initp : Optional[AstNodeExpr]  // Default sampled expression
    // True only for history synthesized as property-evaluation bookkeeping (for example, |=> or
    // NFA local-variable capture). Source $past and AstPast nodes lowering IEEE sampled-value
    // functions such as $rose/$fell must remain false so they receive default initialization.
    bool m_propertyTiming : 1;

బహిరంగ:
    AstPast(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* ticksp = nullptr,
            Ast_Sen_వృక్షం* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() స్థిరరాశి { తిరిగి m_propertyTiming; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstPatMember final : బహిరంగ Ast_కణుపు_సమాసం {
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

బహిరంగ:
    AstPatMember(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhssp, Ast_కణుపు* keyp, Ast_కణుపు_సమాసం* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        కూడు_Lhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { తిరిగి lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 2; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    bool isConcat() స్థిరరాశి { తిరిగి m_isConcat; }
    శూన్య isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() స్థిరరాశి { తిరిగి m_isDefault; }
    శూన్య isDefault(bool flag) { m_isDefault = flag; }
    Ast_కణుపు_సమాసం* కీలకం_Exprp() స్థిరరాశి { తిరిగి varrefp() ? varrefp() : VN_CAST(keyp(), కణుపు_సమాసం); }
};
class Ast_నమూనా final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
బహిరంగ:
    Ast_నమూనా(File_పంక్తి* fl, Ast_కణుపు* itemsp)
        : ASTGEN_SUPER_నమూనా(fl) {
        కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_నమూనా;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి VL_MT_STABLE { తిరిగి dtypep() ? dtypep() : బిడ్డ_DTypep(); }
};
class Ast_నమూనా_నక్షత్రం final : బహిరంగ Ast_కణుపు_సమాసం {
    // Pattern wildcard: ".*"
బహిరంగ:
    explicit Ast_నమూనా_నక్షత్రం(File_పంక్తి* fl)
        : ASTGEN_SUPER_నమూనా_నక్షత్రం(fl) {}
    ASTGEN_MEMBERS_Ast_నమూనా_నక్షత్రం;
    string emitVerilog() override { తిరిగి ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_నమూనా_Var final : బహిరంగ Ast_కణుపు_సమాసం {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
బహిరంగ:
    Ast_నమూనా_Var(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_నమూనా_Var(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_నమూనా_Var;
    string emitVerilog() override { తిరిగి ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_name == VN_DBG_AS(samep, నమూనా_Var)->m_name;
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
};
class AstPropAlways final : బహిరంగ Ast_కణుపు_సమాసం {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    స్థిరరాశి bool m_isStrong = false;  // s_always
బహిరంగ:
    AstPropAlways(File_పంక్తి* fl, Ast_కణుపు_సమాసం* propp, Ast_కణుపు_సమాసం* loBoundp, Ast_కణుపు_సమాసం* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { V3ERROR_NA_RETURN(0); }
    bool isStrong() స్థిరరాశి { తిరిగి m_isStrong; }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstRand final : బహిరంగ Ast_కణుపు_సమాసం {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    స్థిరరాశి bool m_urandom;  // $urandom vs $random
బహిరంగ:
    class Reset {};
    AstRand(File_పంక్తి* fl, Ast_కణుపు_సమాసం* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_పంక్తి* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtype_సమితి_UInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override {
        తిరిగి seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        ఒకవేళ (seedp()) {
            ఒకవేళ (urandom()) తిరిగి "VL_URANDOM_SEEDED_%nq%lq(%li)";
            తిరిగి "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        ఒకవేళ (isWide()) తిరిగి "VL_RANDOM_%nq(%nw, %P)";
        తిరిగి "VL_RANDOM_%nq()";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి !seedp(); }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool combinable(స్థిరరాశి AstRand* samep) స్థిరరాశి {
        తిరిగి !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() స్థిరరాశి { తిరిగి m_urandom; }
};
class AstRandRNG final : బహిరంగ Ast_కణుపు_సమాసం {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
బహిరంగ:
    AstRandRNG(File_పంక్తి* fl, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { తిరిగి "%f$rngrandom()"; }
    string emitC() override {
        తిరిగి isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstRising final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
బహిరంగ:
    AstRising(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { తిరిగి "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstRose final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
బహిరంగ:
    AstRose(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_Sen_వృక్షం* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { తిరిగి "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstSClocked final : బహిరంగ Ast_కణుపు_సమాసం {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
బహిరంగ:
    AstSClocked(File_పంక్తి* fl, Ast_Sen_అంశం* sensesp, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : బహిరంగ Ast_కణుపు_సమాసం {
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
    స్థిరరాశి bool m_unbounded = false;  // True when the upper bound is $
బహిరంగ:
    // Exact [*N]
    AstSConsRep(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* countp, Ast_కణుపు_సమాసం* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { V3ERROR_NA_RETURN(0); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {  // LCOV_EXCL_LINE
        తిరిగి m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() స్థిరరాశి { తిరిగి m_unbounded; }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstSEventually final : బహిరంగ Ast_కణుపు_సమాసం {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
బహిరంగ:
    explicit AstSEventually(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : బహిరంగ Ast_కణుపు_సమాసం {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
బహిరంగ:
    explicit AstSExpr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* preExprp, Ast_కణుపు_Stmt* delayp, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_పంక్తి* fl, Ast_కణుపు_Stmt* delayp, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstSFormatArg final : బహిరంగ Ast_కణుపు_సమాసం {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

బహిరంగ:
    AstSFormatArg(File_పంక్తి* fl, VFormatAttr formatAttr, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "$sformatarg"; }
    string emitVerilog() override { తిరిగి "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && మొదటి_Abovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        తిరిగి nullptr;
    }
    VFormatAttr formatAttr() స్థిరరాశి { తిరిగి m_formatAttr; }
    శూన్య formatAttr(స్థిరరాశి VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    స్థిర VFormatAttr formatAttrDefauled(స్థిరరాశి AstSFormatArg* nodep, స్థిరరాశి Ast_కణుపు_DType* dtypep);
};
class AstSFormatF final : బహిరంగ Ast_కణుపు_సమాసం {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_పాఠ్యం;
    bool m_hidden;  // Under display, etc
    bool m_సమాసం_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    స్థిరరాశి అక్షరం m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
బహిరంగ:
    class సమాసం_Format {};
    AstSFormatF(File_పంక్తి* fl, స్థిరరాశి string& పాఠ్యం, bool hidden, Ast_కణుపు_సమాసం* exprsp,
                అక్షరం missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_పాఠ్యం{పాఠ్యం}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtype_సమితి_String();
        కూడు_Exprsp(exprsp);
    }
    AstSFormatF(File_పంక్తి* fl, సమాసం_Format, Ast_కణుపు_సమాసం* exprsp, అక్షరం missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_పాఠ్యం{""}
        , m_hidden{hidden}
        , m_సమాసం_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtype_సమితి_String();
        కూడు_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_పాఠ్యం; }
    శూన్య name(స్థిరరాశి string& name) override { m_పాఠ్యం = name; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి పాఠ్యం() == VN_DBG_AS(samep, SFormatF)->పాఠ్యం();
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "$sformatf"; }
    string పాఠ్యం() స్థిరరాశి { తిరిగి m_పాఠ్యం; }  // * = Text to display
    శూన్య పాఠ్యం(స్థిరరాశి string& పాఠ్యం) { m_పాఠ్యం = పాఠ్యం; }
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(పాఠ్యం() != "" && సమాసం_Format_2());  // Expr format means no literal format
        తిరిగి nullptr;
    }
    bool formatScopeTracking() స్థిరరాశి {  // Track scopeNamep();  Ok if false positive
        ఒకవేళ (సమాసం_Format_2() || name().కనుగొను("%m") != string::npos || name().కనుగొను("%M") != string::npos)
            తిరిగి true;
        కొరకు (స్థిరరాశి Ast_కణుపు* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి fmtp = VN_CAST(exprp, Const)) {
                ఒకవేళ (fmtp->అంకె().isFromString()) {
                    స్థిరరాశి string str = fmtp->అంకె().గమ్యం_String();
                    ఒకవేళ (str.కనుగొను("%m") != string::npos || str.కనుగొను("%M") != string::npos)
                        తిరిగి true;
                }
            }
        }
        తిరిగి false;
    }
    bool hidden() స్థిరరాశి { తిరిగి m_hidden; }
    శూన్య hidden(bool flag) { m_hidden = flag; }
    bool సమాసం_Format_2() స్థిరరాశి { తిరిగి m_సమాసం_Format; }
    శూన్య సమాసం_Format_2(bool flag) { m_సమాసం_Format = flag; }
    bool optionalFormat() స్థిరరాశి { తిరిగి m_optionalFormat; }
    శూన్య optionalFormat(bool flag) { m_optionalFormat = flag; }
    అక్షరం missingArgChar() స్థిరరాశి { తిరిగి m_missingArgChar; }
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstSGotoRep final : బహిరంగ Ast_కణుపు_సమాసం {
    // Goto repetition: expr [-> count] or [-> count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [->N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
బహిరంగ:
    // Exact [->N]
    AstSGotoRep(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* countp, Ast_కణుపు_సమాసం* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstSNonConsRep final : బహిరంగ Ast_కణుపు_సమాసం {
    // Nonconsecutive repetition: expr [= count] or [= count : maxCount]
    // IEEE 1800-2023 16.9.2
    // op1 := exprp -- the repeated expression
    // op2 := countp -- min repetition count (M); positive constant after V3Width
    // op3 := maxCountp -- max repetition count (N); nullptr for exact [=N]
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := countp : AstNodeExpr
    // @astgen op3 := maxCountp : Optional[AstNodeExpr]
బహిరంగ:
    // Exact [=N]
    AstSNonConsRep(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* countp, Ast_కణుపు_సమాసం* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstSScanF final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_పాఠ్యం;
    VTimescale m_timeunit;  // Parent module time unit

బహిరంగ:
    AstSScanF(File_పంక్తి* fl, స్థిరరాశి string& పాఠ్యం, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_పాఠ్యం{పాఠ్యం} {
        కూడు_Exprsp(exprsp);
        this->fromp(fromp);
        dtype_సమితి_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_పాఠ్యం; }
    string verilogKwd() స్థిరరాశి override { తిరిగి "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: makes output
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstSScanF* స్థిరరాశి asamep = VN_DBG_AS(samep, SScanF);
        తిరిగి పాఠ్యం() == asamep->పాఠ్యం() && timeunit() == asamep->timeunit();
    }
    string పాఠ్యం() స్థిరరాశి { తిరిగి m_పాఠ్యం; }  // * = Text to display
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstSampled final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_అంతర్గత : 1;  // Internally created, not from a source $sampled
బహిరంగ:
    AstSampled(File_పంక్తి* fl, Ast_కణుపు* exprp, Ast_కణుపు_DType* dtypep, bool అంతర్గత_2 = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_అంతర్గత{అంతర్గత_2} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { తిరిగి "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_అంతర్గత == VN_DBG_AS(samep, Sampled)->m_అంతర్గత;
    }
    bool అంతర్గత_2() స్థిరరాశి { తిరిగి m_అంతర్గత; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstScopeName final : బహిరంగ Ast_కణుపు_సమాసం {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    స్థిరరాశి bool m_forFormat;  // Is for a format %m
    స్థిర std::string scopeNameFormatter(స్థిరరాశి std::string& పాఠ్యం);
    స్థిర std::string scopePrettyNameFormatter(స్థిరరాశి std::string& పాఠ్యం);

బహిరంగ:
    class ForFormat {};
    AstScopeName(File_పంక్తి* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtype_సమితి_UInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstScopeName* స్థిరరాశి sp = VN_DBG_AS(samep, ScopeName);
        తిరిగి m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    // ACCESSORS
    స్థిరరాశి std::string& scopeAttr() స్థిరరాశి { తిరిగి m_scopeAttr; }
    శూన్య scopeAttr(స్థిరరాశి std::string& val) { m_scopeAttr = val; }
    స్థిరరాశి std::string& scopeEntr() స్థిరరాశి { తిరిగి m_scopeEntr; }
    శూన్య scopeEntr(స్థిరరాశి std::string& val) { m_scopeEntr = val; }
    bool dpiExport() స్థిరరాశి { తిరిగి m_dpiExport; }
    శూన్య dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() స్థిరరాశి { తిరిగి m_selfRelative; }
    శూన్య selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() స్థిరరాశి { తిరిగి m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() స్థిరరాశి { తిరిగి scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() స్థిరరాశి { తిరిగి scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() స్థిరరాశి { తిరిగి scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() స్థిరరాశి { తిరిగి scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
బహిరంగ:
    AstStable(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_Sen_వృక్షం* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { తిరిగి "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class Ast_దొంతర_Trace_F final : బహిరంగ Ast_కణుపు_సమాసం {
    // $stacktrace used as function
బహిరంగ:
    explicit Ast_దొంతర_Trace_F(File_పంక్తి* fl)
        : ASTGEN_SUPER_దొంతర_Trace_F(fl) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_Ast_దొంతర_Trace_F;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$stacktrace"; }
    string emitVerilog() override { తిరిగి verilogKwd(); }
    string emitC() override { తిరిగి "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstSteady final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
బహిరంగ:
    AstSteady(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { తిరిగి "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstStructSel final : బహిరంగ Ast_కణుపు_సమాసం {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
బహిరంగ:
    AstStructSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, స్థిరరాశి string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override {
        // Not a union
        తిరిగి VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstStructSel* స్థిరరాశి sp = VN_DBG_AS(samep, StructSel);
        తిరిగి m_name == sp->m_name;
    }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
};
class AstSysIgnore final : బహిరంగ Ast_కణుపు_సమాసం {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
బహిరంగ:
    AstSysIgnore(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        కూడు_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$ignored"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }  // Though deleted before opt
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }  // Though deleted before opt
    bool is_శుద్ధ() override { తిరిగి false; }  // Though deleted before opt
    bool isOutputter() override { తిరిగి true; }  // Though deleted before opt
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : బహిరంగ Ast_కణుపు_సమాసం {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
బహిరంగ:
    AstSystemF(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtype_సమితి_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$system"; }
    string emitVerilog() override { తిరిగి verilogKwd(); }
    string emitC() override { తిరిగి "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class Ast_Tagged_సమాసం final : బహిరంగ Ast_కణుపు_సమాసం {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
బహిరంగ:
    Ast_Tagged_సమాసం(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_Tagged_సమాసం(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_సమాసం;
    string emitVerilog() override { తిరిగి "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_name == VN_DBG_AS(samep, Tagged_సమాసం)->m_name;
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
};
class Ast_Tagged_నమూనా final : బహిరంగ Ast_కణుపు_సమాసం {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
బహిరంగ:
    Ast_Tagged_నమూనా(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* patternp)
        : ASTGEN_SUPER_Tagged_నమూనా(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_నమూనా;
    string emitVerilog() override { తిరిగి "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_name == VN_DBG_AS(samep, Tagged_నమూనా)->m_name;
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
};
class Ast_Test_Plus_వాదనలు final : బహిరంగ Ast_కణుపు_సమాసం {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
బహిరంగ:
    Ast_Test_Plus_వాదనలు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* searchp)
        : ASTGEN_SUPER_Test_Plus_వాదనలు(fl) {
        this->searchp(searchp);
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_Test_Plus_వాదనలు;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$test$plusargs"; }
    string emitVerilog() override { తిరిగి verilogKwd(); }
    string emitC() override { తిరిగి "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    // but isPure() true
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstThisRef final : బహిరంగ Ast_కణుపు_సమాసం {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
బహిరంగ:
    AstThisRef(File_పంక్తి* fl, VFlag_బిడ్డ_DType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        బిడ్డ_DTypep(dtypep);
    }
    AstThisRef(File_పంక్తి* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { తిరిగి "this"; }
    string emitVerilog() override { తిరిగి "this"; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
};
class Ast_సమయం_Precision final : బహిరంగ Ast_కణుపు_సమాసం {
    // Verilog $timeprecision
బహిరంగ:
    explicit Ast_సమయం_Precision(File_పంక్తి* fl)
        : ASTGEN_SUPER_సమయం_Precision(fl) {
        dtype_సమితి_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_సమయం_Precision;
    string emitVerilog() override { తిరిగి "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_సమయం_Unit final : బహిరంగ Ast_కణుపు_సమాసం {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
బహిరంగ:
    explicit Ast_సమయం_Unit(File_పంక్తి* fl)
        : ASTGEN_SUPER_సమయం_Unit(fl) {
        dtype_సమితి_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_సమయం_Unit;
    string emitVerilog() override { తిరిగి "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : బహిరంగ Ast_కణుపు_సమాసం {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    స్థిరరాశి std::సదిశ<uint32_t> m_indices;  // Indices of the triggers tested
బహిరంగ:
    AstTriggered(File_పంక్తి* fl, AstVarScope* trigVscp, స్థిరరాశి std::సదిశ<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    స్థిరరాశి std::సదిశ<uint32_t>& indices() స్థిరరాశి { తిరిగి m_indices; }
};
class AstUnbounded final : బహిరంగ Ast_కణుపు_సమాసం {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
బహిరంగ:
    explicit AstUnbounded(File_పంక్తి* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtype_సమితి_Int();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { తిరిగి "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
};
class AstUnlinkedRef final : బహిరంగ Ast_కణుపు_సమాసం {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
బహిరంగ:
    AstUnlinkedRef(File_పంక్తి* fl, Ast_కణుపు* refp, స్థిరరాశి string& name, Ast_కణుపు* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : బహిరంగ Ast_కణుపు_సమాసం {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    స్థిరరాశి bool m_strong;  // 's_' preffix
    స్థిరరాశి bool m_overlapping;  // '_with` suffix
బహిరంగ:
    AstUntil(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() స్థిరరాశి override {
        తిరిగి (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
    bool isStrong() స్థిరరాశి { తిరిగి m_strong; }
    bool isOverlapping() స్థిరరాశి { తిరిగి m_overlapping; }
};
class Ast_విలువ_Plus_వాదనలు final : బహిరంగ Ast_కణుపు_సమాసం {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
బహిరంగ:
    Ast_విలువ_Plus_వాదనలు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* searchp, Ast_కణుపు_సమాసం* outp)
        : ASTGEN_SUPER_విలువ_Plus_వాదనలు(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_విలువ_Plus_వాదనలు;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$value$plusargs"; }
    string emitVerilog() override { తిరిగి "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి !outp(); }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstWithParse final : బహిరంగ Ast_కణుపు_సమాసం {
    // In early parse, FUNC(index) WITH equation-using-index
    // Replaced with AstWith
    // Parents: expr|stmt
    // Children: funcref, expr
    // @astgen op1 := funcrefp : AstNodeExpr
    // @astgen op3 := exprsp : List[AstNodeExpr]  // With's parenthesis part
    // @astgen op4 := constraintsp : List[AstNode]  // With's braces part
అంతరంగ:
    // True for the 'with (identifier_list) {...}' form, including 'with () {...}'.
    bool m_restricted = false;

బహిరంగ:
    AstWithParse(File_పంక్తి* fl, Ast_కణుపు_సమాసం* funcrefp, Ast_కణుపు_సమాసం* exprsp,
                 Ast_కణుపు* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        కూడు_Exprsp(exprsp);
        కూడు_Constraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool restricted() స్థిరరాశి { తిరిగి m_restricted; }
    శూన్య restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : బహిరంగ Ast_కణుపు_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
బహిరంగ:
    AstBufIf1(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstCastDynamic final : బహిరంగ Ast_కణుపు_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
బహిరంగ:
    AstCastDynamic(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Unsized(32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { తిరిగి "%f$cast(%r, %l)"; }
    string emitC() override { తిరిగి "VL_DYNAMIC_CAST(%r, %l)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 20; }
    bool is_శుద్ధ() override { తిరిగి false; }
};
class AstCompareNN final : బహిరంగ Ast_కణుపు_Biop {
    // Verilog str.compare() and str.icompare()
    స్థిరరాశి bool m_ignoreCase;  // True for str.icompare()
బహిరంగ:
    AstCompareNN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtype_సమితి_Int();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        తిరిగి m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        తిరిగి m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool ignoreCase() స్థిరరాశి { తిరిగి m_ignoreCase; }
};
class AstConcat final : బహిరంగ Ast_కణుపు_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
బహిరంగ:
    AstConcat(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        ఒకవేళ (lhsp->dtypep() && rhsp->dtypep()) {
            dtype_సమితి_బిట్టు_Or_Logic_Sized(lhsp->dtypep()->వెడల్పు() + rhsp->dtypep()->వెడల్పు(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { తిరిగి "%f{%l, %k%r}"; }
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opConcat(lhs, rhs);
    }
    string emitC() override { తిరిగి "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(concat %l %r)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 2; }
};
class AstConcatN final : బహిరంగ Ast_కణుపు_Biop {
    // String concatenate
బహిరంగ:
    AstConcatN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { తిరిగి "%f{%l, %k%r}"; }
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opConcatN(lhs, rhs);
    }
    string emitC() override { తిరిగి "VL_CONCATN_NNN(%li, %ri)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
    bool stringFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstDiv final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstDiv(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opDiv(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f/ %r)"; }
    string emitC() override { తిరిగి "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvudiv %l %r)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_DIV; }
};
class AstDivD final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstDivD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_AstDivD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opDivD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "/"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL_DIV; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstDivS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstDivS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opDivS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f/ %r)"; }
    string emitC() override { తిరిగి "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvsdiv %l %r)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_DIV; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstEqWild final : బహిరంగ Ast_కణుపు_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
బహిరంగ:
    AstEqWild(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    స్థిర Ast_కణుపు_Biop* newTyped(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp);
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f==? %r)"; }
    string emitC() override {
        ఒకవేళ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } లేకపోతే {
            తిరిగి "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        ఒకవేళ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } లేకపోతే {
            తిరిగి "==";
        }
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstFGetS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
బహిరంగ:
    AstFGetS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtype_సమితి_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%f$fgets(%l,%r)"; }
    string emitC() override {
        తిరిగి strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 64; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstFUngetC(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        తిరిగి "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 64; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    Ast_కణుపు* filep() స్థిరరాశి { తిరిగి lhsp(); }
    Ast_కణుపు* charp() స్థిరరాశి { తిరిగి rhsp(); }
};
class AstGetcN final : బహిరంగ Ast_కణుపు_Biop {
    // Verilog string.getc()
బహిరంగ:
    AstGetcN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGetcN(lhs, rhs);
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి "getc"; }
    string emitVerilog() override { తిరిగి "%k(%l.getc(%r))"; }
    string emitC() override { తిరిగి "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstGetcRefN final : బహిరంగ Ast_కణుపు_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
బహిరంగ:
    AstGetcRefN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstGt final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstGt(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGt(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f> %r)"; }
    string emitC() override { తిరిగి "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { తిరిగి ">"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstGtD final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstGtD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstGtD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGtD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి ">"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstGtN final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstGtN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstGtN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGtN(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి ">"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
    bool stringFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstGtS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstGtS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGtS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f> %r)"; }
    string emitC() override { తిరిగి "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstGte final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstGte(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGte(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f>= %r)"; }
    string emitC() override { తిరిగి "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { తిరిగి ">="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstGteD final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstGteD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstGteD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGteD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి ">="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstGteN final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstGteN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstGteN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGteN(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి ">="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
    bool stringFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstGteS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstGteS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opGteS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f>= %r)"; }
    string emitC() override { తిరిగి "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstLogAnd final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstLogAnd(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f&& %r)"; }
    string emitC() override { తిరిగి "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvand %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "&&"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() + INSTR_లెక్క_BRANCH; }
};
class AstLogIf final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstLogIf(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f-> %r)"; }
    string emitC() override { తిరిగి "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { తిరిగి "->"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() + INSTR_లెక్క_BRANCH; }
};
class AstLogOr final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstLogOr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f|| %r)"; }
    string emitC() override { తిరిగి "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvor %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "||"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() + INSTR_లెక్క_BRANCH; }
};
class AstLt final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstLt(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLt(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f< %r)"; }
    string emitC() override { తిరిగి "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { తిరిగి "<"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstLtD final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstLtD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstLtD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLtD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "<"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstLtN final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstLtN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstLtN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLtN(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "<"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
    bool stringFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstLtS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstLtS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLtS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f< %r)"; }
    string emitC() override { తిరిగి "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstLte final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstLte(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLte(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f<= %r)"; }
    string emitC() override { తిరిగి "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { తిరిగి "<="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstLteD final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstLteD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstLteD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLteD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "<="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstLteN final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstLteN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstLteN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLteN(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "<="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
    bool stringFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstLteS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstLteS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLteS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f<= %r)"; }
    string emitC() override { తిరిగి "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstModDiv final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstModDiv(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f%% %r)"; }
    string emitC() override { తిరిగి "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvurem %l %r)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_DIV; }
};
class AstModDivS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstModDivS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f%% %r)"; }
    string emitC() override { తిరిగి "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvsmod %l %r)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_DIV; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstNeqWild final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstNeqWild(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f!=? %r)"; }
    string emitC() override {
        ఒకవేళ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } లేకపోతే {
            తిరిగి "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        ఒకవేళ (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } లేకపోతే {
            తిరిగి "!=";
        }
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstPow final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstPow(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opPow(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f** %r)"; }
    string emitC() override { తిరిగి "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_MUL * 10; }
};
class AstPowD final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstPowD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_AstPowD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opPowD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f** %r)"; }
    string emitC() override { తిరిగి "pow(%li,%ri)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL_DIV * 5; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstPowSS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstPowSS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f** %r)"; }
    string emitC() override { తిరిగి "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_MUL * 10; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstPowSU final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstPowSU(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f** %r)"; }
    string emitC() override { తిరిగి "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_MUL * 10; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstPowUS final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstPowUS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f** %r)"; }
    string emitC() override { తిరిగి "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_MUL * 10; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstReplicate final : బహిరంగ Ast_కణుపు_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
బహిరంగ:
    AstReplicate(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        ఒకవేళ (lhsp) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(rhsp, Const)) {
                ఒకవేళ (constp->అంకె().isFourState()
                    || (constp->dtypep()->isSigned() && constp->అంకె().isNegative())) {
                    dtype_సమితి_బిట్టు_Or_Logic_Sized(lhsp->వెడల్పు(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } లేకపోతే {
                    dtype_సమితి_బిట్టు_Or_Logic_Sized(lhsp->వెడల్పు() * constp->గమ్యం_SInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, uint32_t rep_లెక్క)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_లెక్క}} {}
    ASTGEN_MEMBERS_AstReplicate;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opRepl(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%f{%r{%k%l}}"; }
    string emitC() override { తిరిగి "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override {
        తిరిగి "((_ repeat " + cvt_గమ్యం_Str(వెడల్పు() / lhsp()->వెడల్పు()) + ") %l)";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 2; }
};
class AstReplicateN final : బహిరంగ Ast_కణుపు_Biop {
    // String replicate
బహిరంగ:
    AstReplicateN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opReplN(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%f{%r{%k%l}}"; }
    string emitC() override { తిరిగి "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 2; }
    bool stringFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstSAnd final : బహిరంగ Ast_కణుపు_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    స్థిరరాశి bool m_propertyControl;  // Parser-generated property if/case branch conjunction
బహిరంగ:
    AstSAnd(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstSAnd;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() + INSTR_లెక్క_BRANCH; }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {  // LCOV_EXCL_LINE
        తిరిగి m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() స్థిరరాశి { తిరిగి m_propertyControl; }
};
class AstSIntersect final : బహిరంగ Ast_కణుపు_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
బహిరంగ:
    AstSIntersect(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() + INSTR_లెక్క_BRANCH; }
    // LCOV_EXCL_STOP
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstSOr final : బహిరంగ Ast_కణుపు_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
బహిరంగ:
    AstSOr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstSOr;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() + INSTR_లెక్క_BRANCH; }
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstSThroughout final : బహిరంగ Ast_కణుపు_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
బహిరంగ:
    AstSThroughout(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    // LCOV_EXCL_STOP
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstSWithin final : బహిరంగ Ast_కణుపు_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
బహిరంగ:
    AstSWithin(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    // LCOV_EXCL_STOP
    bool is_Multi_చక్రం_Sva() స్థిరరాశి override { తిరిగి true; }
};
class AstSel final : బహిరంగ Ast_కణుపు_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    పూర్ణాంకం m_decl_El_వెడల్పు;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    పూర్ణాంకం m_వెడల్పు_Const;

బహిరంగ:
    AstSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* lsbp, పూర్ణాంకం bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_decl_El_వెడల్పు{1}
        , m_వెడల్పు_Const{bitwidth} {
        dtype_సమితి_బిట్టు_Or_Logic_Sized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, పూర్ణాంకం lsb, పూర్ణాంకం bitwidth, పూర్ణాంకం bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_decl_El_వెడల్పు{1}
        , m_వెడల్పు_Const{bitwidth} {
        dtype_సమితి_బిట్టు_Or_Logic_Unsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& from, స్థిరరాశి V3_సంఖ్య& బిట్టు_2) override {
        బహిర్గతం.opSel(from, బిట్టు_2.గమ్యం_UInt() + వెడల్పు_Const() - 1, బిట్టు_2.గమ్యం_UInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        తిరిగి వెడల్పు_Const() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() స్థిరరాశి override { తిరిగి "((_ extract %t %r) %l)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* otherp) స్థిరరాశి override {
        తిరిగి వెడల్పు_Const() == VN_DBG_AS(otherp, Sel)->వెడల్పు_Const();
    }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override {
        తిరిగి వెడల్పు_Instrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    పూర్ణాంకం వెడల్పు_Const() స్థిరరాశి { తిరిగి m_వెడల్పు_Const; }
    శూన్య వెడల్పు_Const(పూర్ణాంకం విలువ) { m_వెడల్పు_Const = విలువ; }
    పూర్ణాంకం lsbConst() స్థిరరాశి { తిరిగి VN_AS(lsbp(), Const)->గమ్యం_SInt(); }
    పూర్ణాంకం msbConst() స్థిరరాశి { తిరిగి lsbConst() + వెడల్పు_Const() - 1; }
    VNumRange& declRange() VL_MT_STABLE { తిరిగి m_declRange; }
    స్థిరరాశి VNumRange& declRange() స్థిరరాశి VL_MT_STABLE { తిరిగి m_declRange; }
    శూన్య declRange(స్థిరరాశి VNumRange& flag) { m_declRange = flag; }
    పూర్ణాంకం decl_El_వెడల్పు() స్థిరరాశి { తిరిగి m_decl_El_వెడల్పు; }
    శూన్య decl_El_వెడల్పు(పూర్ణాంకం flag) { m_decl_El_వెడల్పు = flag; }
};
class Ast_జరుపు_L final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    Ast_జరుపు_L(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, పూర్ణాంకం setwidth = 0)
        : ASTGEN_SUPER_జరుపు_L(fl, lhsp, rhsp) {
        ఒకవేళ (setwidth) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = lhsp->dtypep()) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(dtypep->వెడల్పు(), dtypep->వెడల్పు_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_జరుపు_L;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_జరుపు_L(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f<< %r)"; }
    string emitC() override { తిరిగి "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        తిరిగి (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class Ast_జరుపు_LOvr final : బహిరంగ Ast_కణుపు_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
బహిరంగ:
    Ast_జరుపు_LOvr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, పూర్ణాంకం setwidth = 0)
        : ASTGEN_SUPER_జరుపు_LOvr(fl, lhsp, rhsp) {
        ఒకవేళ (setwidth) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } లేకపోతే {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(dtypep->వెడల్పు(), dtypep->వెడల్పు_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_జరుపు_LOvr;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_జరుపు_L(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f<< %r)"; }
    string emitC() override { తిరిగి "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class Ast_జరుపు_R final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    Ast_జరుపు_R(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, పూర్ణాంకం setwidth = 0)
        : ASTGEN_SUPER_జరుపు_R(fl, lhsp, rhsp) {
        ఒకవేళ (setwidth) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = lhsp->dtypep()) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(dtypep->వెడల్పు(), dtypep->వెడల్పు_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_జరుపు_R;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_జరుపు_R(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f>> %r)"; }
    string emitC() override { తిరిగి "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        తిరిగి (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    // LHS size might be > output size, so don't want to force size
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class Ast_జరుపు_ROvr final : బహిరంగ Ast_కణుపు_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
బహిరంగ:
    Ast_జరుపు_ROvr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, పూర్ణాంకం setwidth = 0)
        : ASTGEN_SUPER_జరుపు_ROvr(fl, lhsp, rhsp) {
        ఒకవేళ (setwidth) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } లేకపోతే {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(dtypep->వెడల్పు(), dtypep->వెడల్పు_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_జరుపు_ROvr;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_జరుపు_R(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f>> %r)"; }
    string emitC() override { తిరిగి "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    // LHS size might be > output size, so don't want to force size
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class Ast_జరుపు_RS final : బహిరంగ Ast_కణుపు_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
బహిరంగ:
    Ast_జరుపు_RS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, పూర్ణాంకం setwidth = 0)
        : ASTGEN_SUPER_జరుపు_RS(fl, lhsp, rhsp) {
        ఒకవేళ (setwidth) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = lhsp->dtypep()) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(dtypep->వెడల్పు(), dtypep->వెడల్పు_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_జరుపు_RS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_జరుపు_RS(lhs, rhs, lhsp()->వెడల్పు_Min_V());
    }
    string emitVerilog() override { తిరిగి "%k(%l %f>>> %r)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvashr %l %r)"; }
    string emitC() override { తిరిగి "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class Ast_జరుపు_RSOvr final : బహిరంగ Ast_కణుపు_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
బహిరంగ:
    Ast_జరుపు_RSOvr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, పూర్ణాంకం setwidth = 0)
        : ASTGEN_SUPER_జరుపు_RSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        ఒకవేళ (setwidth) {
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } లేకపోతే {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_సమితి_బిట్టు_Or_Logic_Unsized(dtypep->వెడల్పు(), dtypep->వెడల్పు_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_జరుపు_RSOvr;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_జరుపు_RS(lhs, rhs, lhsp()->వెడల్పు_Min_V());
    }
    string emitVerilog() override { తిరిగి "%k(%l %f>>> %r)"; }
    string emitC() override { తిరిగి "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstSub final : బహిరంగ Ast_కణుపు_Biop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstSub(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opSub(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f- %r)"; }
    string emitC() override { తిరిగి "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvsub %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "-"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
};
class AstSubD final : బహిరంగ Ast_కణుపు_Biop {
బహిరంగ:
    AstSubD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_AstSubD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opSubD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "-"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstURandomRange final : బహిరంగ Ast_కణుపు_Biop {
    // $urandom_range
బహిరంగ:
    AstURandomRange(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtype_సమితి_UInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%f$urandom_range(%l, %r)"; }
    string emitC() override { తిరిగి "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : బహిరంగ Ast_కణుపు_Bi_Com {
    // @astgen makeDfgVertex
బహిరంగ:
    AstEq(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    స్థిర Ast_కణుపు_Biop* newTyped(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp);
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opEq(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f== %r)"; }
    string emitC() override { తిరిగి "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { తిరిగి "=="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstEqCase final : బహిరంగ Ast_కణుపు_Bi_Com {
    // @astgen makeDfgVertex
బహిరంగ:
    AstEqCase(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstEqCase;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f=== %r)"; }
    string emitC() override { తిరిగి "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { తిరిగి "=="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstEqD final : బహిరంగ Ast_కణుపు_Bi_Com {
బహిరంగ:
    AstEqD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstEqD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opEqD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "=="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstEqN final : బహిరంగ Ast_కణుపు_Bi_Com {
బహిరంగ:
    AstEqN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstEqN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opEqN(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "=="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
    bool stringFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstEqT final : బహిరంగ Ast_కణుపు_Bi_Com {
    // Equal (==) for data types
బహిరంగ:
    AstEqT(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstEqT;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "=="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
};
class AstLogEq final : బహిరంగ Ast_కణుపు_Bi_Com {
    // @astgen makeDfgVertex
బహిరంగ:
    AstLogEq(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f<-> %r)"; }
    string emitC() override { తిరిగి "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "<->"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() + INSTR_లెక్క_BRANCH; }
};
class AstNeq final : బహిరంగ Ast_కణుపు_Bi_Com {
    // @astgen makeDfgVertex
బహిరంగ:
    AstNeq(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    స్థిర Ast_కణుపు_Biop* newTyped(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp);
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opNeq(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f!= %r)"; }
    string emitC() override { తిరిగి "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { తిరిగి "!="; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (not (= %l %r)))"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstNeqCase final : బహిరంగ Ast_కణుపు_Bi_Com {
    // @astgen makeDfgVertex
బహిరంగ:
    AstNeqCase(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f!== %r)"; }
    string emitC() override { తిరిగి "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { తిరిగి "!="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
};
class AstNeqD final : బహిరంగ Ast_కణుపు_Bi_Com {
బహిరంగ:
    AstNeqD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstNeqD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "!="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstNeqN final : బహిరంగ Ast_కణుపు_Bi_Com {
బహిరంగ:
    AstNeqN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstNeqN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "!="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
    bool stringFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstNeqT final : బహిరంగ Ast_కణుపు_Bi_Com {
    // Not-equal (!=) for data types
బహిరంగ:
    AstNeqT(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstNeqT;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "!="; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_STR; }
};

// === AstNodeBiComAsv ===
class Ast_కూడు final : బహిరంగ Ast_కణుపు_Bi_Com_Asv {
    // @astgen makeDfgVertex
బహిరంగ:
    Ast_కూడు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_కూడు(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_కూడు;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_కూడు(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f+ %r)"; }
    string emitC() override { తిరిగి "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvadd %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "+"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
};
class Ast_కూడు_D final : బహిరంగ Ast_కణుపు_Bi_Com_Asv {
బహిరంగ:
    Ast_కూడు_D(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_కూడు_D(fl, lhsp, rhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_Ast_కూడు_D;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_కూడు_D(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "+"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstAnd final : బహిరంగ Ast_కణుపు_Bi_Com_Asv {
    // @astgen makeDfgVertex
బహిరంగ:
    AstAnd(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opAnd(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f& %r)"; }
    string emitC() override { తిరిగి "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvand %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "&"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    స్థిరరాశి అక్షరం* వెడల్పు_Mismatch() స్థిరరాశి override VL_MT_STABLE;
};
class AstMul final : బహిరంగ Ast_కణుపు_Bi_Com_Asv {
    // @astgen makeDfgVertex
బహిరంగ:
    AstMul(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opMul(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f* %r)"; }
    string emitC() override { తిరిగి "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvmul %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "*"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_MUL; }
};
class AstMulD final : బహిరంగ Ast_కణుపు_Bi_Com_Asv {
బహిరంగ:
    AstMulD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_AstMulD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opMulD(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "*"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstMulS final : బహిరంగ Ast_కణుపు_Bi_Com_Asv {
    // @astgen makeDfgVertex
బహిరంగ:
    AstMulS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opMulS(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f* %r)"; }
    string emitC() override { తిరిగి "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvmul %l %r)"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * INSTR_లెక్క_INT_MUL; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstOr final : బహిరంగ Ast_కణుపు_Bi_Com_Asv {
    // @astgen makeDfgVertex
బహిరంగ:
    AstOr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opOr(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f| %r)"; }
    string emitC() override { తిరిగి "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvor %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "|"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    స్థిరరాశి అక్షరం* వెడల్పు_Mismatch() స్థిరరాశి override VL_MT_STABLE;
};
class AstXor final : బహిరంగ Ast_కణుపు_Bi_Com_Asv {
    // @astgen makeDfgVertex
బహిరంగ:
    AstXor(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.opXor(lhs, rhs);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f^ %r)"; }
    string emitC() override { తిరిగి "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvxor %l %r)"; }
    string emitSimpleOperator() override { తిరిగి "^"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }  // Lclean && Rclean
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    స్థిరరాశి అక్షరం* వెడల్పు_Mismatch() స్థిరరాశి override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class Ast_Dist_Chi_వర్గం final : బహిరంగ Ast_కణుపు_Dist_Biop {
బహిరంగ:
    Ast_Dist_Chi_వర్గం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Dist_Chi_వర్గం(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_Chi_వర్గం;
    string emitVerilog() override { తిరిగి "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { తిరిగి "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class Ast_Dist_ఘాతాంక final : బహిరంగ Ast_కణుపు_Dist_Biop {
బహిరంగ:
    Ast_Dist_ఘాతాంక(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Dist_ఘాతాంక(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ఘాతాంక;
    string emitVerilog() override { తిరిగి "%f$dist_exponential(%l, %r)"; }
    string emitC() override { తిరిగి "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class Ast_Dist_పాయిసాన్ final : బహిరంగ Ast_కణుపు_Dist_Biop {
బహిరంగ:
    Ast_Dist_పాయిసాన్(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Dist_పాయిసాన్(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_పాయిసాన్;
    string emitVerilog() override { తిరిగి "%f$dist_poisson(%l, %r)"; }
    string emitC() override { తిరిగి "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : బహిరంగ Ast_కణుపు_Dist_Biop {
బహిరంగ:
    AstDistT(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { తిరిగి "%f$dist_t(%l, %r)"; }
    string emitC() override { తిరిగి "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : బహిరంగ Ast_కణుపు_Sel {
    // @astgen makeDfgVertex
    శూన్య init(స్థిరరాశి Ast_కణుపు* fromp) {
        ఒకవేళ (fromp && VN_IS(fromp->dtypep()->skipRefp(), కణుపు_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), కణుపు_Array_DType)->subDTypep());
        }
    }

బహిరంగ:
    AstArraySel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, పూర్ణాంకం బిట్టు_2)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, బిట్టు_2)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() స్థిరరాశి override { తిరిగి "(select %l %r)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
};
class AstAssocSel final : బహిరంగ Ast_కణుపు_Sel {
    శూన్య init(స్థిరరాశి Ast_కణుపు* fromp) {
        ఒకవేళ (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

బహిరంగ:
    AstAssocSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%k(%l%f[%r])"; }
    string emitC() override { తిరిగి "%li%k[%ri]"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(select %l %r)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isGateOptimizable() స్థిరరాశి override {
        తిరిగి !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి !isLValue(); }  // AssocSel creates on miss
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
};
class AstWildcardSel final : బహిరంగ Ast_కణుపు_Sel {
    శూన్య init(స్థిరరాశి Ast_కణుపు* fromp) {
        ఒకవేళ (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

బహిరంగ:
    AstWildcardSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%k(%l%f[%r])"; }
    string emitC() override { తిరిగి "%li%k[%ri]"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
};
class Ast_మాట_Sel final : బహిరంగ Ast_కణుపు_Sel {
    // Select a single word from a multi-word wide value
బహిరంగ:
    Ast_మాట_Sel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* bitp)
        : ASTGEN_SUPER_మాట_Sel(fl, fromp, bitp) {
        dtype_సమితి_UInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_Ast_మాట_Sel;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&, స్థిరరాశి V3_సంఖ్య&) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};

// === AstNodeStream ===
class Ast_ప్రవాహం_L final : బహిరంగ Ast_కణుపు_ప్రవాహం {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
బహిరంగ:
    Ast_ప్రవాహం_L(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_ప్రవాహం_L(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_ప్రవాహం_L;
    string emitVerilog() override { తిరిగి "%f{ << %r %k{%l} }"; }
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_ప్రవాహం_L(lhs, rhs);
    }
    string emitC() override { తిరిగి "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 2; }
};
class Ast_ప్రవాహం_R final : బహిరంగ Ast_కణుపు_ప్రవాహం {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
బహిరంగ:
    Ast_ప్రవాహం_R(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_ప్రవాహం_R(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_ప్రవాహం_R;
    string emitVerilog() override { తిరిగి "%f{ >> %r %k{%l} }"; }
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.op_కేటాయించు(lhs);
    }
    string emitC() override { తిరిగి isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : బహిరంగ Ast_కణుపు_System_Biop_D {
బహిరంగ:
    AstAtan2D(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.సమితి_Double(std::atan2(lhs.గమ్యం_Double(), rhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$atan2(%l,%r)"; }
    string emitC() override { తిరిగి "atan2(%li,%ri)"; }
};
class AstHypotD final : బహిరంగ Ast_కణుపు_System_Biop_D {
బహిరంగ:
    AstHypotD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) override {
        బహిర్గతం.సమితి_Double(std::hypot(lhs.గమ్యం_Double(), rhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$hypot(%l,%r)"; }
    string emitC() override { తిరిగి "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : బహిరంగ Ast_కణుపు_CCall {
    // C++ function call
    VSelf_సూచిక_పాఠ్యం m_self_సూచిక
        = VSelf_సూచిక_పాఠ్యం{VSelf_సూచిక_పాఠ్యం::ఖాళీలు_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
బహిరంగ:
    AstCCall(File_పంక్తి* fl, AstCFunc* funcp, Ast_కణుపు_సమాసం* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    స్థిరరాశి VSelf_సూచిక_పాఠ్యం& self_సూచిక() స్థిరరాశి { తిరిగి m_self_సూచిక; }
    శూన్య self_సూచిక(స్థిరరాశి VSelf_సూచిక_పాఠ్యం& self_సూచిక) { m_self_సూచిక = self_సూచిక; }
    string self_సూచిక_Protect(bool useSelfForThis) స్థిరరాశి {
        తిరిగి self_సూచిక().protect(useSelfForThis, protect());
    }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
};
class AstCMethodCall final : బహిరంగ Ast_కణుపు_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
బహిరంగ:
    AstCMethodCall(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, AstCFunc* funcp, Ast_కణుపు_సమాసం* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : బహిరంగ Ast_కణుపు_CCall {
    // C++ new() call
బహిరంగ:
    AstCNew(File_పంక్తి* fl, AstCFunc* funcp, Ast_కణుపు_సమాసం* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : బహిరంగ Ast_కణుపు_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, Ast_Cons_Array_అంశం*> m_itemps;  // The item at each index given

బహిరంగ:
    AstConsArray(File_పంక్తి* fl, Ast_కణుపు_DType* newDTypep, Ast_కణుపు_సమాసం* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    శూన్య cloneRelink() override;
    అంతఃవరుస Ast_Cons_Array_అంశం* itemsp() స్థిరరాశి VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_కణుపు_సమాసం* valuep(uint64_t సూచీ) స్థిరరాశి;
    // Add the value of the element at the index, which must not be given yet
    శూన్య valuep(uint64_t సూచీ, Ast_కణుపు_సమాసం* newp);
    // Add the elements of the array 'arrayp', starting at the index
    శూన్య splice_విలువలు(uint64_t సూచీ, Ast_కణుపు_సమాసం* arrayp);
};
class AstConsAssoc final : బహిరంగ Ast_కణుపు_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
బహిరంగ:
    explicit AstConsAssoc(File_పంక్తి* fl, Ast_కణుపు_సమాసం* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        తిరిగి nullptr;
    }
};
class AstConsDynArray final : బహిరంగ Ast_కణుపు_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
బహిరంగ:
    explicit AstConsDynArray(File_పంక్తి* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
};
class Ast_Cons_వరుస final : బహిరంగ Ast_కణుపు_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
బహిరంగ:
    explicit Ast_Cons_వరుస(File_పంక్తి* fl)
        : ASTGEN_SUPER_Cons_వరుస(fl) {}
    ASTGEN_MEMBERS_Ast_Cons_వరుస;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), వరుస_DType));
        తిరిగి nullptr;
    }
};
class AstConsStruct final : బహిరంగ Ast_కణుపు_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
బహిరంగ:
    AstConsStruct(File_పంక్తి* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    // The value of the member called 'name'
    Ast_కణుపు_సమాసం* memberValuep(స్థిరరాశి string& name) స్థిరరాశి;
};
class AstConsWildcard final : బహిరంగ Ast_కణుపు_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
బహిరంగ:
    explicit AstConsWildcard(File_పంక్తి* fl, Ast_కణుపు_సమాసం* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        తిరిగి nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : బహిరంగ Ast_కణుపు_FTask_Ref {
    // A reference to a function
బహిరంగ:
    అంతఃవరుస AstFuncRef(File_పంక్తి* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_పంక్తి* fl, స్థిరరాశి string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class Ast_పద్ధతి_Call final : బహిరంగ Ast_కణుపు_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
బహిరంగ:
    Ast_పద్ధతి_Call(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, VFlag_బిడ్డ_DType, స్థిరరాశి string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_పద్ధతి_Call(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_పద్ధతి_Call(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, స్థిరరాశి string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_పద్ధతి_Call(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_Ast_పద్ధతి_Call;
};
class AstNew final : బహిరంగ Ast_కణుపు_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
బహిరంగ:
    AstNew(File_పంక్తి* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool isImplicit() స్థిరరాశి { తిరిగి m_isImplicit; }
    శూన్య isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() స్థిరరాశి { తిరిగి m_isScoped; }
    శూన్య isScoped(bool flag) { m_isScoped = flag; }
    bool is_శుద్ధ() override { తిరిగి false; }
};
class AstTaskRef final : బహిరంగ Ast_కణుపు_FTask_Ref {
    // A reference to a task
బహిరంగ:
    అంతఃవరుస AstTaskRef(File_పంక్తి* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_పంక్తి* fl, స్థిరరాశి string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtype_సమితి_Void();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class Ast_Sel_బిట్టు final : బహిరంగ Ast_కణుపు_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
అంతరంగ:
    VAccess m_access;  // Left hand side assignment
బహిరంగ:
    Ast_Sel_బిట్టు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* bitp)
        : ASTGEN_SUPER_Sel_బిట్టు(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_Ast_Sel_బిట్టు;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    VAccess access() స్థిరరాశి { తిరిగి m_access; }
    శూన్య access(స్థిరరాశి VAccess& flag) { m_access = flag; }
};
class Ast_Sel_వెలికితీయి final : బహిరంగ Ast_కణుపు_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
బహిరంగ:
    Ast_Sel_వెలికితీయి(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* msbp, Ast_కణుపు_సమాసం* lsbp)
        : ASTGEN_SUPER_Sel_వెలికితీయి(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_Ast_Sel_వెలికితీయి;
};
class AstSelMinus final : బహిరంగ Ast_కణుపు_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
బహిరంగ:
    AstSelMinus(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* bitp, Ast_కణుపు_సమాసం* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : బహిరంగ Ast_కణుపు_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
బహిరంగ:
    AstSelPlus(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* bitp, Ast_కణుపు_సమాసం* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_లెక్క_బిట్లు final : బహిరంగ Ast_కణుపు_Quadop {
    // Number of bits set in vector
బహిరంగ:
    Ast_లెక్క_బిట్లు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* ctrl1p)
        : ASTGEN_SUPER_లెక్క_బిట్లు(fl, exprp, ctrl1p, ctrl1p->clone_వృక్షం_శుద్ధ(false),
                                 ctrl1p->clone_వృక్షం_శుద్ధ(false)) {
        dtype_సమితి_Int();
    }
    Ast_లెక్క_బిట్లు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* ctrl1p, Ast_కణుపు_సమాసం* ctrl2p)
        : ASTGEN_SUPER_లెక్క_బిట్లు(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_వృక్షం_శుద్ధ(false)) {
        dtype_సమితి_Int();
    }
    Ast_లెక్క_బిట్లు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* ctrl1p, Ast_కణుపు_సమాసం* ctrl2p,
                 Ast_కణుపు_సమాసం* ctrl3p)
        : ASTGEN_SUPER_లెక్క_బిట్లు(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtype_సమితి_Int();
    }
    ASTGEN_MEMBERS_Ast_లెక్క_బిట్లు;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& సమాసం, స్థిరరాశి V3_సంఖ్య& ctrl1,
                       స్థిరరాశి V3_సంఖ్య& ctrl2, స్థిరరాశి V3_సంఖ్య& ctrl3) override {
        బహిర్గతం.op_లెక్క_బిట్లు(సమాసం, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { తిరిగి "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanThs() స్థిరరాశి override { తిరిగి true; }
    bool cleanFhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Ths() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Fhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : బహిరంగ Ast_కణుపు_Termop {
బహిరంగ:
    explicit AstInferredDisable(File_పంక్తి* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { తిరిగి "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_సమయం final : బహిరంగ Ast_కణుపు_Termop {
    VTimescale m_timeunit;  // Parent module time unit
బహిరంగ:
    Ast_సమయం(File_పంక్తి* fl, స్థిరరాశి VTimescale& timeunit)
        : ASTGEN_SUPER_సమయం(fl)
        , m_timeunit{timeunit} {
        dtype_సమితి_UInt64();
    }
    ASTGEN_MEMBERS_Ast_సమయం;
    string emitVerilog() override { తిరిగి "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_సమయం; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }
};
class Ast_సమయం_D final : బహిరంగ Ast_కణుపు_Termop {
    VTimescale m_timeunit;  // Parent module time unit
బహిరంగ:
    Ast_సమయం_D(File_పంక్తి* fl, స్థిరరాశి VTimescale& timeunit)
        : ASTGEN_SUPER_సమయం_D(fl)
        , m_timeunit{timeunit} {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_Ast_సమయం_D;
    string emitVerilog() override { తిరిగి "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_సమయం; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : బహిరంగ Ast_కణుపు_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
బహిరంగ:
    AstCond(File_పంక్తి* fl, Ast_కణుపు_సమాసం* condp, Ast_కణుపు_సమాసం* thenp, Ast_కణుపు_సమాసం* elsep);
    ASTGEN_MEMBERS_AstCond;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs,
                       స్థిరరాశి V3_సంఖ్య& ths) override {
        బహిర్గతం.op_కేటాయించు(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { తిరిగి "%k(%l %f? %r %k: %t)"; }
    string emitC() override { తిరిగి "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(ite (__Vbool %l) %r %t)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }  // clean if e1 & e2 clean
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanThs() స్థిరరాశి override { తిరిగి false; }  // Propagates up
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Ths() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
};
class AstPutcN final : బహిరంగ Ast_కణుపు_Triop {
    // Verilog string.putc()
బహిరంగ:
    AstPutcN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు_సమాసం* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_AstPutcN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs,
                       స్థిరరాశి V3_సంఖ్య& ths) override {
        బహిర్గతం.opPutcN(lhs, rhs, ths);
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి "putc"; }
    string emitVerilog() override { తిరిగి "%k(%l.putc(%r,%t))"; }
    string emitC() override { తిరిగి "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanThs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Ths() స్థిరరాశి override { తిరిగి false; }
};
class Ast_ముక్క_Sel final : బహిరంగ Ast_కణుపు_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
బహిరంగ:
    Ast_ముక్క_Sel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, స్థిరరాశి VNumRange& declRange)
        : ASTGEN_SUPER_ముక్క_Sel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_Ast_ముక్క_Sel;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& from, స్థిరరాశి V3_సంఖ్య& lo,
                       స్థిరరాశి V3_సంఖ్య& వెడల్పు) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanThs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Ths() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { తిరిగి m_declRange; }
    స్థిరరాశి VNumRange& declRange() స్థిరరాశి VL_MT_STABLE { తిరిగి m_declRange; }
};
class AstSubstrN final : బహిరంగ Ast_కణుపు_Triop {
    // Verilog string.substr()
బహిరంగ:
    AstSubstrN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు_సమాసం* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs,
                       స్థిరరాశి V3_సంఖ్య& ths) override {
        బహిర్గతం.opSubstrN(lhs, rhs, ths);
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి "substr"; }
    string emitVerilog() override { తిరిగి "%k(%l.substr(%r,%t))"; }
    string emitC() override { తిరిగి "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { తిరిగి ""; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanRhs() స్థిరరాశి override { తిరిగి true; }
    bool cleanThs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Rhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Ths() స్థిరరాశి override { తిరిగి false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : బహిరంగ Ast_కణుపు_Dist_Triop {
బహిరంగ:
    AstDistErlang(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు_సమాసం* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { తిరిగి "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { తిరిగి "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class Ast_Dist_సాధారణ final : బహిరంగ Ast_కణుపు_Dist_Triop {
బహిరంగ:
    Ast_Dist_సాధారణ(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు_సమాసం* thsp)
        : ASTGEN_SUPER_Dist_సాధారణ(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_సాధారణ;
    string emitVerilog() override { తిరిగి "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { తిరిగి "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class Ast_Dist_ఏకరీతి final : బహిరంగ Ast_కణుపు_Dist_Triop {
బహిరంగ:
    Ast_Dist_ఏకరీతి(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు_సమాసం* thsp)
        : ASTGEN_SUPER_Dist_ఏకరీతి(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_ఏకరీతి;
    string emitVerilog() override { తిరిగి "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { తిరిగి "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : బహిరంగ Ast_కణుపు_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
బహిరంగ:
    గణన FmtType : పూర్ణాంకం { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

అంతరంగ:
    // dist-ast-dump-suppress  // Part of name()
    స్థిరరాశి FmtType m_fmtType;  // Operation type
బహిరంగ:
    AstAtoN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtype_సమితి_Double() : dtype_సమితి_Integer();
    }
    ASTGEN_MEMBERS_AstAtoN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opAtoN(lhs, m_fmtType); }
    string name() స్థిరరాశి override VL_MT_STABLE {
        ఎంపిక (m_fmtType) {
        స్థితి ATOI: తిరిగి "atoi";
        స్థితి ATOHEX: తిరిగి "atohex";
        స్థితి ATOOCT: తిరిగి "atooct";
        స్థితి ATOBIN: తిరిగి "atobin";
        స్థితి ATOREAL: తిరిగి "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { తిరిగి "%l." + name() + "()"; }
    string emitC() override {
        ఎంపిక (m_fmtType) {
        స్థితి ATOI: తిరిగి "VL_ATOI_N(%li, 10)";
        స్థితి ATOHEX: తిరిగి "VL_ATOI_N(%li, 16)";
        స్థితి ATOOCT: తిరిగి "VL_ATOI_N(%li, 8)";
        స్థితి ATOBIN: తిరిగి "VL_ATOI_N(%li, 2)";
        స్థితి ATOREAL: తిరిగి "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    FmtType fmtType() స్థిరరాశి { తిరిగి m_fmtType; }
};
class Ast_బిట్లు_గమ్యం_Real_D final : బహిరంగ Ast_కణుపు_Uniop {
బహిరంగ:
    Ast_బిట్లు_గమ్యం_Real_D(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_బిట్లు_గమ్యం_Real_D(fl, lhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_Ast_బిట్లు_గమ్యం_Real_D;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_బిట్లు_గమ్యం_Real_D(lhs); }
    string emitVerilog() override { తిరిగి "%f$bitstoreal(%l)"; }
    string emitC() override { తిరిగి "VL_CVT_D_Q(%li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }  // Eliminated before matters
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }  // Eliminated before matters
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstCCast final : బహిరంగ Ast_కణుపు_Uniop {
    // Cast to C-based data type
    పూర్ణాంకం m_కొలత;

బహిరంగ:
    AstCCast(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, పూర్ణాంకం setwidth, పూర్ణాంకం minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_కొలత = setwidth;
        ఒకవేళ (setwidth) {
            ఒకవేళ (minwidth == -1) minwidth = setwidth;
            dtype_సమితి_బిట్టు_Unsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_కొలత = వెడల్పు();
    }
    ASTGEN_MEMBERS_AstCCast;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_కేటాయించు(lhs); }
    string emitVerilog() override { తిరిగి "%f$_CAST(%l)"; }
    string emitC() override { తిరిగి "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }  // Special cased in V3Cast
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి కొలత() == VN_DBG_AS(samep, CCast)->కొలత();
    }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    //
    పూర్ణాంకం కొలత() స్థిరరాశి { తిరిగి m_కొలత; }
};
class AstCLog2 final : బహిరంగ Ast_కణుపు_Uniop {
బహిరంగ:
    AstCLog2(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtype_సమితి_Integer2_State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opCLog2(lhs); }
    string emitVerilog() override { తిరిగి "%f$clog2(%l)"; }
    string emitC() override { తిరిగి "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 16; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstCastWrap final : బహిరంగ Ast_కణుపు_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
బహిరంగ:
    AstCastWrap(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_కేటాయించు(lhs); }
    string emitVerilog() override { తిరిగి "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
};
class Ast_లెక్క_Ones final : బహిరంగ Ast_కణుపు_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
బహిరంగ:
    Ast_లెక్క_Ones(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_లెక్క_Ones(fl, lhsp) {
        dtype_సమితి_Int();
    }
    ASTGEN_MEMBERS_Ast_లెక్క_Ones;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_లెక్క_Ones(lhs); }
    string emitVerilog() override { తిరిగి "%f$countones(%l)"; }
    string emitC() override { తిరిగి "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 16; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstCvtPackString final : బహిరంగ Ast_కణుపు_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
బహిరంగ:
    AstCvtPackString(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstExtend final : బహిరంగ Ast_కణుపు_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
బహిరంగ:
    AstExtend(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtype_సమితి_బిట్టు_Or_Logic_Unsized(వెడల్పు, వెడల్పు_Min ? వెడల్పు_Min : వెడల్పు, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_కేటాయించు(lhs); }
    string emitVerilog() override { తిరిగి "%l"; }
    string emitC() override { తిరిగి "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() స్థిరరాశి override {
        తిరిగి "((_ zero_extend " + cvt_గమ్యం_Str(వెడల్పు() - lhsp()->వెడల్పు()) + ") %l)";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override {
        తిరిగి false;  // Because the EXTEND operator self-casts
    }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
};
class AstExtendS final : బహిరంగ Ast_కణుపు_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
బహిరంగ:
    AstExtendS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, పూర్ణాంకం వెడల్పు)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtype_సమితి_బిట్టు_Or_Logic_Sized(వెడల్పు, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.opExtendS(lhs, lhsp()->వెడల్పు_Min_V());
    }
    string emitVerilog() override { తిరిగి "%l"; }
    string emitC() override { తిరిగి "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() స్థిరరాశి override {
        తిరిగి "((_ sign_extend " + cvt_గమ్యం_Str(వెడల్పు() - lhsp()->వెడల్పు()) + ") %l)";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override {
        తిరిగి false;  // Because the EXTEND operator self-casts
    }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool signedFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstFEof final : బహిరంగ Ast_కణుపు_Uniop {
బహిరంగ:
    AstFEof(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%f$feof(%l)"; }
    string emitC() override { తిరిగి "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 16; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
    Ast_కణుపు* filep() స్థిరరాశి { తిరిగి lhsp(); }
};
class AstFGetC final : బహిరంగ Ast_కణుపు_Uniop {
బహిరంగ:
    AstFGetC(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtype_సమితి_Logic2_State_Unsized(
            32, v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::MATCHES_వెడల్పు ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { తిరిగి "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 64; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    Ast_కణుపు* filep() స్థిరరాశి { తిరిగి lhsp(); }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstISToRD final : బహిరంగ Ast_కణుపు_Uniop {
    // $itor where lhs is signed
బహిరంగ:
    AstISToRD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_AstISToRD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opISToRD(lhs); }
    string emitVerilog() override { తిరిగి "%f$itor($signed(%l))"; }
    string emitC() override { తిరిగి "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_పదాలు() override { తిరిగి true; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstIToRD final : బహిరంగ Ast_కణుపు_Uniop {
    // $itor where lhs is unsigned
బహిరంగ:
    AstIToRD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_AstIToRD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opIToRD(lhs); }
    string emitVerilog() override { తిరిగి "%f$itor(%l)"; }
    string emitC() override { తిరిగి "VL_ITOR_D_%lq(%lw, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstIsUnbounded final : బహిరంగ Ast_కణుపు_Uniop {
    // True if is unbounded ($)
బహిరంగ:
    AstIsUnbounded(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య&) override {
        // Any constant isn't unbounded
        బహిర్గతం.సమితి_Zero();
    }
    string emitVerilog() override { తిరిగి "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstIsUnknown final : బహిరంగ Ast_కణుపు_Uniop {
    // True if any unknown bits
బహిరంగ:
    AstIsUnknown(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opIsUnknown(lhs); }
    string emitVerilog() override { తిరిగి "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstLenN final : బహిరంగ Ast_కణుపు_Uniop {
    // Length of a string
బహిరంగ:
    AstLenN(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtype_సమితి_Int();
    }
    ASTGEN_MEMBERS_AstLenN;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opLenN(lhs); }
    string emitVerilog() override { తిరిగి "%f(%l)"; }
    string emitC() override { తిరిగి "VL_LEN_IN(%li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
};
class AstLogNot final : బహిరంగ Ast_కణుపు_Uniop {
    // @astgen makeDfgVertex
అంతరంగ:
    స్థిరరాశి bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
బహిరంగ:
    AstLogNot(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtype_సమితి_బిట్టు_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opLogNot(lhs); }
    string emitVerilog() override { తిరిగి "%f(! %l)"; }
    string emitC() override { తిరిగి "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { తిరిగి "!"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool fromProperty() స్థిరరాశి { తిరిగి m_fromProperty; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class Ast_Most_సమితి_బిట్టు_P1 final : బహిరంగ Ast_కణుపు_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
బహిరంగ:
    Ast_Most_సమితి_బిట్టు_P1(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Most_సమితి_బిట్టు_P1(fl, lhsp) {
        dtype_సమితి_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_Most_సమితి_బిట్టు_P1;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_Most_సమితి_బిట్టు_P1(lhs); }
    string emitVerilog() override { తిరిగి "%f$mostsetbitp1(%l)"; }
    string emitC() override { తిరిగి "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 16; }
};
class AstNToI final : బహిరంగ Ast_కణుపు_Uniop {
    // String to any-size integral
బహిరంగ:
    AstNToI(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opNToI(lhs); }
    string emitVerilog() override { తిరిగి "'(%l)"; }
    string emitC() override { తిరిగి "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
};
class AstNegate final : బహిరంగ Ast_కణుపు_Uniop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstNegate(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opNegate(lhs); }
    string emitVerilog() override { తిరిగి "%f(- %l)"; }
    string emitC() override { తిరిగి "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvneg %l)"; }
    string emitSimpleOperator() override { తిరిగి "-"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
};
class AstNegateD final : బహిరంగ Ast_కణుపు_Uniop {
బహిరంగ:
    AstNegateD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtype_సమితి_Double();
    }
    ASTGEN_MEMBERS_AstNegateD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opNegateD(lhs); }
    string emitVerilog() override { తిరిగి "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { తిరిగి "-"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool doubleFlavor() స్థిరరాశి override { తిరిగి true; }
};
class AstNot final : బహిరంగ Ast_కణుపు_Uniop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstNot(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opNot(lhs); }
    string emitVerilog() override { తిరిగి "%f(~ %l)"; }
    string emitC() override { తిరిగి "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() స్థిరరాశి override { తిరిగి "(bvnot %l)"; }
    string emitSimpleOperator() override { తిరిగి "~"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }
    స్థిరరాశి అక్షరం* వెడల్పు_Mismatch() స్థిరరాశి override VL_MT_STABLE;
};
class AstNullCheck final : బహిరంగ Ast_కణుపు_Uniop {
    // Return LHS after checking that LHS is non-null
బహిరంగ:
    AstNullCheck(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { V3ERROR_NA; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 1; }  // Rarely executes
    string emitVerilog() override { తిరిగి "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి fileline() == samep->fileline(); }
};
class AstOneHot final : బహిరంగ Ast_కణుపు_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
బహిరంగ:
    AstOneHot(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstOneHot;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opOneHot(lhs); }
    string emitVerilog() override { తిరిగి "%f$onehot(%l)"; }
    string emitC() override { తిరిగి "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 4; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstOneHot0 final : బహిరంగ Ast_కణుపు_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
బహిరంగ:
    AstOneHot0(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtype_సమితి_బిట్టు();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opOneHot0(lhs); }
    string emitVerilog() override { తిరిగి "%f$onehot0(%l)"; }
    string emitC() override { తిరిగి "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() * 3; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstPostDec final : బహిరంగ Ast_కణుపు_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
బహిరంగ:
    AstPostDec(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { తిరిగి "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : బహిరంగ Ast_కణుపు_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
బహిరంగ:
    AstPostInc(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { తిరిగి "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : బహిరంగ Ast_కణుపు_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
బహిరంగ:
    AstPreDec(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { తిరిగి "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : బహిరంగ Ast_కణుపు_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
బహిరంగ:
    AstPreInc(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { తిరిగి "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : బహిరంగ Ast_కణుపు_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
బహిరంగ:
    AstRToIRoundS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtype_సమితి_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opRToIRoundS(lhs); }
    string emitVerilog() override { తిరిగి "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        తిరిగి isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstRToIS final : బహిరంగ Ast_కణుపు_Uniop {
    // $rtoi(lhs)
బహిరంగ:
    AstRToIS(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtype_సమితి_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opRToIS(lhs); }
    string emitVerilog() override { తిరిగి "%f$rtoi(%l)"; }
    string emitC() override { తిరిగి "VL_RTOI_I_D(%li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }  // Eliminated before matters
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }  // Eliminated before matters
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class Ast_Real_గమ్యం_బిట్లు final : బహిరంగ Ast_కణుపు_Uniop {
బహిరంగ:
    Ast_Real_గమ్యం_బిట్లు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Real_గమ్యం_బిట్లు(fl, lhsp) {
        dtype_సమితి_UInt64();
    }
    ASTGEN_MEMBERS_Ast_Real_గమ్యం_బిట్లు;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_Real_గమ్యం_బిట్లు(lhs); }
    string emitVerilog() override { తిరిగి "%f$realtobits(%l)"; }
    string emitC() override { తిరిగి "VL_CVT_Q_D(%li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }  // Eliminated before matters
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }  // Eliminated before matters
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_DBL; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class AstRedAnd final : బహిరంగ Ast_కణుపు_Uniop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstRedAnd(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opRedAnd(lhs); }
    string emitVerilog() override { తిరిగి "%f(& %l)"; }
    string emitC() override { తిరిగి "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
};
class AstRedOr final : బహిరంగ Ast_కణుపు_Uniop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstRedOr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opRedOr(lhs); }
    string emitVerilog() override { తిరిగి "%f(| %l)"; }
    string emitC() override { తిరిగి "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
};
class AstRedXor final : బహిరంగ Ast_కణుపు_Uniop {
    // @astgen makeDfgVertex
బహిరంగ:
    AstRedXor(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtype_సమితి_బిట్టు_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.opRedXor(lhs); }
    string emitVerilog() override { తిరిగి "%f(^ %l)"; }
    string emitC() override { తిరిగి "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override {
        స్థిరరాశి పూర్ణాంకం w = lhsp()->వెడల్పు();
        తిరిగి (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 1 + V3_సంఖ్య::log2b(వెడల్పు()); }
};
class AstResizeLValue final : బహిరంగ Ast_కణుపు_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
బహిరంగ:
    AstResizeLValue(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
};
class AstSigned final : బహిరంగ Ast_కణుపు_Uniop {
    // $signed(lhs)
బహిరంగ:
    AstSigned(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.op_కేటాయించు(lhs);
        బహిర్గతం.isSigned(false);
    }
    string emitVerilog() override { తిరిగి "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }  // Eliminated before matters
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }  // Eliminated before matters
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};
class Ast_సమయం_Import final : బహిరంగ Ast_కణుపు_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
బహిరంగ:
    Ast_సమయం_Import(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_సమయం_Import(fl, lhsp) {}
    ASTGEN_MEMBERS_Ast_సమయం_Import;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }
};
class Ast_గమ్యం_క్రింది_N final : బహిరంగ Ast_కణుపు_Uniop {
    // string.tolower()
బహిరంగ:
    Ast_గమ్యం_క్రింది_N(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_గమ్యం_క్రింది_N(fl, lhsp) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_Ast_గమ్యం_క్రింది_N;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_గమ్యం_క్రింది_N(lhs); }
    string emitVerilog() override { తిరిగి "%l.tolower()"; }
    string emitC() override { తిరిగి "VL_TOLOWER_NN(%li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
};
class Ast_గమ్యం_String_N final : బహిరంగ Ast_కణుపు_Uniop {
బహిరంగ:
    Ast_గమ్యం_String_N(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_గమ్యం_String_N(fl, lhsp) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_Ast_గమ్యం_String_N;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { తిరిగి "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        తిరిగి isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
};
class Ast_గమ్యం_పై_N final : బహిరంగ Ast_కణుపు_Uniop {
    // string.toupper()
బహిరంగ:
    Ast_గమ్యం_పై_N(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_గమ్యం_పై_N(fl, lhsp) {
        dtype_సమితి_String();
    }
    ASTGEN_MEMBERS_Ast_గమ్యం_పై_N;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override { బహిర్గతం.op_గమ్యం_పై_N(lhs); }
    string emitVerilog() override { తిరిగి "%l.toupper()"; }
    string emitC() override { తిరిగి "VL_TOUPPER_NN(%li)"; }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి true; }
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి false; }
};
class AstUnsigned final : బహిరంగ Ast_కణుపు_Uniop {
    // $unsigned(lhs)
బహిరంగ:
    AstUnsigned(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.op_కేటాయించు(lhs);
        బహిర్గతం.isSigned(false);
    }
    string emitVerilog() override { తిరిగి "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి false; }
    bool cleanLhs() స్థిరరాశి override { తిరిగి false; }  // Eliminated before matters
    bool కొలత_Matters_Lhs() స్థిరరాశి override { తిరిగి true; }  // Eliminated before matters
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool isSystemFunc() స్థిరరాశి override { తిరిగి true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstAcosD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::acos(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$acos(%l)"; }
    string emitC() override { తిరిగి "acos(%li)"; }
};
class AstAcoshD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstAcoshD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::acosh(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$acosh(%l)"; }
    string emitC() override { తిరిగి "acosh(%li)"; }
};
class AstAsinD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstAsinD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::asin(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$asin(%l)"; }
    string emitC() override { తిరిగి "asin(%li)"; }
};
class AstAsinhD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstAsinhD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::asinh(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$asinh(%l)"; }
    string emitC() override { తిరిగి "asinh(%li)"; }
};
class AstAtanD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstAtanD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::atan(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$atan(%l)"; }
    string emitC() override { తిరిగి "atan(%li)"; }
};
class AstAtanhD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstAtanhD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::atanh(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$atanh(%l)"; }
    string emitC() override { తిరిగి "atanh(%li)"; }
};
class AstCeilD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstCeilD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::ceil(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$ceil(%l)"; }
    string emitC() override { తిరిగి "ceil(%li)"; }
};
class AstCosD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstCosD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::కోజ్యా(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$cos(%l)"; }
    string emitC() override { తిరిగి "cos(%li)"; }
};
class AstCoshD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstCoshD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::cosh(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$cosh(%l)"; }
    string emitC() override { తిరిగి "cosh(%li)"; }
};
class AstExpD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstExpD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::ఘాతాంకం(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$exp(%l)"; }
    string emitC() override { తిరిగి "exp(%li)"; }
};
class AstFloorD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstFloorD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::floor(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$floor(%l)"; }
    string emitC() override { తిరిగి "floor(%li)"; }
};
class AstLog10D final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstLog10D(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::log10(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$log10(%l)"; }
    string emitC() override { తిరిగి "log10(%li)"; }
};
class AstLogD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstLogD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::సంవర్గమానం(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$ln(%l)"; }
    string emitC() override { తిరిగి "log(%li)"; }
};
class AstSinD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstSinD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::జ్యా(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$sin(%l)"; }
    string emitC() override { తిరిగి "sin(%li)"; }
};
class AstSinhD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstSinhD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::sinh(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$sinh(%l)"; }
    string emitC() override { తిరిగి "sinh(%li)"; }
};
class AstSqrtD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstSqrtD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::వర్గమూలం(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$sqrt(%l)"; }
    string emitC() override { తిరిగి "sqrt(%li)"; }
};
class AstTanD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstTanD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::tan(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$tan(%l)"; }
    string emitC() override { తిరిగి "tan(%li)"; }
};
class AstTanhD final : బహిరంగ Ast_కణుపు_System_Uniop_D {
బహిరంగ:
    AstTanhD(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    శూన్య సంఖ్య_నిర్వహించు(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs) override {
        బహిర్గతం.సమితి_Double(std::tanh(lhs.గమ్యం_Double()));
    }
    string emitVerilog() override { తిరిగి "%f$tanh(%l)"; }
    string emitC() override { తిరిగి "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : బహిరంగ Ast_కణుపు_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
బహిరంగ:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    అంతఃవరుస AstVarRef(File_పంక్తి* fl, AstVar* varp, స్థిరరాశి VAccess& access);
    అంతఃవరుస AstVarRef(File_పంక్తి* fl, Ast_కణుపు_Module* classOrPackagep, AstVar* varp,
                     స్థిరరాశి VAccess& access);
    // This form only allowed post-link (see above)
    అంతఃవరుస AstVarRef(File_పంక్తి* fl, AstVarScope* varscp, స్థిరరాశి VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() స్థిరరాశి { తిరిగి m_name; }
    అంతఃవరుస string name() స్థిరరాశి override;  // * = Var name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    అంతఃవరుస bool same_కణుపు(స్థిరరాశి AstVarRef* samep) స్థిరరాశి;
    అంతఃవరుస bool sameNoLvalue(స్థిరరాశి AstVarRef* samep) స్థిరరాశి;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
};
class AstVarXRef final : బహిరంగ Ast_కణుపు_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_కలిగియుంది_Gen_ఖండం = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
బహిరంగ:
    AstVarXRef(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& dotted, స్థిరరాశి VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    అంతఃవరుస AstVarXRef(File_పంక్తి* fl, AstVar* varp, స్థిరరాశి string& dotted, స్థిరరాశి VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    శూన్య name(స్థిరరాశి std::string& name) override { m_name = name; }  // * = Var name
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string dotted() స్థిరరాశి { తిరిగి m_dotted; }
    శూన్య dotted(స్థిరరాశి string& dotted) { m_dotted = dotted; }
    string inlinedDots() స్థిరరాశి { తిరిగి m_inlinedDots; }
    శూన్య inlinedDots(స్థిరరాశి string& flag) { m_inlinedDots = flag; }
    bool కలిగియుంది_Gen_ఖండం() స్థిరరాశి { తిరిగి m_కలిగియుంది_Gen_ఖండం; }
    శూన్య కలిగియుంది_Gen_ఖండం(స్థిరరాశి bool flag) { m_కలిగియుంది_Gen_ఖండం = flag; }
    bool readOnlyModport() స్థిరరాశి { తిరిగి m_readOnlyModport; }
    శూన్య readOnlyModport(స్థిరరాశి bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_బహిర్గతం() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        తిరిగి (self_సూచిక() == asamep->self_సూచిక() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#అయితే_ముగింపు  // Guard
