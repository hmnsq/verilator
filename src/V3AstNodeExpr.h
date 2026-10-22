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

#यदि_अपरिभाषितम् VERILATOR_V3ASTNODEEXPR_H_
#परिभाषय VERILATOR_V3ASTNODEEXPR_H_

#यदि_अपरिभाषितम् VERILATOR_V3AST_H_
#दोषः "Use V3Ast.h as the include"
#अन्तर्भाव "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#परिभाषय VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#यदि_अन्तः

// === Abstract base node types (AstNode*) =====================================

class Ast_ग्रन्थिः_व्यञ्जकम् VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // An expression tree node
रक्षित:
    Ast_ग्रन्थिः_व्यञ्जकम्(VNType t, File_पङ्क्तिः* fl)
        : Ast_ग्रन्थिः{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_व्यञ्जकम्;
    // METHODS
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    // Every expression must have a data type after V3Width
    bool hasDType() ध्रुव override final VL_MT_SAFE { प्रत्यागम true; }
    आभास string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    आभास string emitC() = 0;
    आभास string emitSMT() ध्रुव { प्रत्यागम ""; };
    आभास string emitSimpleOperator() { प्रत्यागम ""; }  // "" means not ok to use
    आभास bool emit_Check_Max_शब्दाः() { प्रत्यागम false; }  // Check VL_MULS_MAX_WORDS
    आभास bool clean_बहिर्गमनम्() ध्रुव = 0;  // True if output has extra upper bits zero
    // True for SVA multi-cycle sequence nodes (SExpr, SConsRep, etc.)
    आभास bool is_Multi_आवृत्तिः_Sva() ध्रुव { प्रत्यागम false; }
    ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* getVAccessTargetRecurse() ध्रुव;
    Ast_ग्रन्थिः_व्यञ्जकम्* getVAccessTargetRecurse() {
        प्रत्यागम const_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(  // casting constness away is safe since this function is
                                          // non-const itself therefore, caller guarantees that
                                          // this object is non-const
            static_cast<ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्*>(this)->getVAccessTargetRecurse());
    }
    VAccess getVAccessRecurse() ध्रुव;
    // TODO: this actually means it's a write or RW, not that it's an LValue
    bool isLValue() ध्रुव { प्रत्यागम getVAccessRecurse().isWriteOrRW(); }
    // A constant value known at compile time: a Const, or a NodeCons of only constants
    bool is_Constant_वृक्षः() ध्रुव;
    // Return base var (or const) nodep dereferences
    Ast_ग्रन्थिः* आधारः_Fromp(bool overMembers);

    // Wrap This expression into an AstStmtExpr to denote it occurs in statement position
    अन्तःस्थ Ast_Stmt_व्यञ्जकम्* makeStmt();
    // Returns an error message if widthMin() is not correct otherwise returns nullptr like
    // broken()
    आभास ध्रुव वर्ण* विस्तारः_Mismatch() ध्रुव VL_MT_STABLE { प्रत्यागम nullptr; }

    // S-expression inspired dump of node and operands for debugging
    std::string आदर्शः_String(uint32_t गभीरता = 0) ध्रुव;
    bool isFourstateOrNoDtype() ध्रुव {
        यदि (!dtypep()) प्रत्यागम true;
        प्रत्यागम dtypep()->skipRefp()->isFourstate();
    }
    स्थिर bool isFourstateOrNoDtype(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p) {
        प्रत्यागम expr1p->isFourstateOrNoDtype();
    }
    स्थिर bool isFourstateOrNoDtype(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p,
                                     ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr2p) {
        प्रत्यागम expr1p->isFourstateOrNoDtype() || expr2p->isFourstateOrNoDtype();
    }
    स्थिर bool isFourstateOrNoDtype(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p,
                                     ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr2p,
                                     ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr3p) {
        प्रत्यागम isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p);
    }
    स्थिर bool isFourstateOrNoDtype(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p,
                                     ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr2p,
                                     ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr3p,
                                     ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr4p) {
        प्रत्यागम isFourstateOrNoDtype(expr1p, expr2p) || isFourstateOrNoDtype(expr3p, expr4p);
    }
};
class Ast_ग्रन्थिः_Biop VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Binary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_ग्रन्थिः_Biop(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Biop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    आभास शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) = 0;
    आभास bool cleanLhs() ध्रुव = 0;  // True if LHS must have extra upper bits zero
    आभास bool cleanRhs() ध्रुव = 0;  // True if RHS must have extra upper bits zero
    आभास bool आकार_Matters_Lhs() ध्रुव = 0;  // True if output result depends on lhs size
    आभास bool आकार_Matters_Rhs() ध्रुव = 0;  // True if output result depends on rhs size
    आभास bool doubleFlavor() ध्रुव { प्रत्यागम false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    आभास bool signedFlavor() ध्रुव { प्रत्यागम false; }
    आभास bool stringFlavor() ध्रुव { प्रत्यागम false; }  // N flavor of nodes with both flavors?
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    bool is_विशुद्धम्() override;
    ध्रुव वर्ण* broken() ध्रुव override;

गुप्त:
    bool getPurityRecurse() ध्रुव { प्रत्यागम lhsp()->is_विशुद्धम्() && rhsp()->is_विशुद्धम्(); }
};
class Ast_ग्रन्थिः_Bi_Com VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Binary expr with commutative properties
रक्षित:
    Ast_ग्रन्थिः_Bi_Com(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhs, Ast_ग्रन्थिः_व्यञ्जकम्* rhs)
        : Ast_ग्रन्थिः_Biop{t, fl, lhs, rhs} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Bi_Com;
};
class Ast_ग्रन्थिः_Bi_Com_Asv VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
    // Binary expr with commutative & associative properties
रक्षित:
    Ast_ग्रन्थिः_Bi_Com_Asv(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhs, Ast_ग्रन्थिः_व्यञ्जकम्* rhs)
        : Ast_ग्रन्थिः_Bi_Com{t, fl, lhs, rhs} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Bi_Com_Asv;
};
class Ast_ग्रन्थिः_Dist_Biop VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    Ast_ग्रन्थिः_Dist_Biop(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : Ast_ग्रन्थिः_Biop{t, fl, lhsp, rhsp} {
        dtype_समुच्चयः_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Dist_Biop;
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL_TRIG; }
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
};
class Ast_ग्रन्थिः_Sel VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TODO: rename to idxp
रक्षित:
    Ast_ग्रन्थिः_Sel(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* bitp)
        : Ast_ग्रन्थिः_Biop{t, fl, fromp, bitp} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Sel;
    पूर्णाङ्क bitConst() ध्रुव;
};
class Ast_ग्रन्थिः_प्रवाहः VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
रक्षित:
    Ast_ग्रन्थिः_प्रवाहः(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : Ast_ग्रन्थिः_Biop{t, fl, lhsp, rhsp} {
        यदि (lhsp->dtypep()) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(lhsp->विस्तारः(), lhsp->विस्तारः_Min(), VSigning::UNSIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_प्रवाहः;
};
class Ast_ग्रन्थिः_System_Biop_D VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    Ast_ग्रन्थिः_System_Biop_D(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : Ast_ग्रन्थिः_Biop{t, fl, lhsp, rhsp} {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_System_Biop_D;
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL_TRIG; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class Ast_ग्रन्थिः_CCall VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A call of a C++ function, perhaps a AstCFunc or perhaps globally named
    // @astgen op2 := argsp : List[AstNodeExpr]  // Note: op1 used by some sub-types only
    //
    // @astgen ptr := m_funcp : AstCFunc  // Function being called
    // dist-ast-dump-suppress  // Too verbose
    string m_argTypes;
    bool m_superReference = false;  // Called with super reference

रक्षित:
    Ast_ग्रन्थिः_CCall(VNType t, File_पङ्क्तिः* fl, AstCFunc* funcp, Ast_ग्रन्थिः_व्यञ्जकम्* argsp = nullptr)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl}
        , m_funcp{funcp} {
        सङ्कलय_Argsp(argsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_CCall;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_CALL; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव Ast_ग्रन्थिः_CCall* ध्रुव asamep = VN_DBG_AS(samep, ग्रन्थिः_CCall);
        प्रत्यागम (funcp() == asamep->funcp() && argTypes() == asamep->argTypes());
    }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override;
    bool isOutputter() override { प्रत्यागम !is_विशुद्धम्(); }
    AstCFunc* funcp() ध्रुव { प्रत्यागम m_funcp; }
    शून्य funcp(AstCFunc* funcp) { m_funcp = funcp; }
    string argTypes() ध्रुव { प्रत्यागम m_argTypes; }
    शून्य argTypes(ध्रुव string& str) { m_argTypes = str; }
    bool isUnlikely() ध्रुव override { प्रत्यागम m_funcp && m_funcp->isUnlikely(); }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव final override { प्रत्यागम true; }
    bool superReference() ध्रुव { प्रत्यागम m_superReference; }
    शून्य superReference(bool flag) { m_superReference = flag; }
};
class Ast_ग्रन्थिः_Cons VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Construct a value of an aggregate type
    मित्रम् class Ast_ग्रन्थिः_व्यञ्जकम्;  // For m_knownConstantTree
    mutable bool m_ज्ञातम्_Constant_वृक्षः = false;  // Known to be 'this->isConstantTree() == true'

रक्षित:
    Ast_ग्रन्थिः_Cons(VNType t, File_पङ्क्तिः* fl)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Cons;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    std::string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    std::string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव final override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव final override { प्रत्यागम विस्तारः_Instrs(); }
};
class Ast_ग्रन्थिः_FTask_Ref VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A reference to a task (or function)
    // op1 used by some sub-types only
    // @astgen op2 := argsp : List[AstArg]
    // @astgen op3 := withp : Optional[AstWith]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    //
    // @astgen ptr := m_taskp : Optional[AstNodeFTask]  // [AfterLink] Pointer to task referenced
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the task
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_FTask_Ref;  // Gen pointers before other members for performance
गुप्त:
    string m_name;  // Name of variable
    string m_dotted;  // Dotted part of scope the name()ed task/func is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_अन्तर्भवति_Gen_खण्डम् = false;  // Contains gen block reference
    bool m_pli = false;  // Pli system call ($name)
    bool m_superReference = false;  // Called with super reference
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_ग्रन्थिः_FTask_Ref(VNType t, File_पङ्क्तिः* fl, ध्रुव string& name, AstArg* argsp)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl}
        , m_name{name} {
        सङ्कलय_Argsp(argsp);
    }

सार्वजनिक:
    ध्रुव वर्ण* broken() ध्रुव override;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Var name
    bool isGateOptimizable() ध्रुव override;
    string dotted() ध्रुव { प्रत्यागम m_dotted; }  // * = Scope name or ""
    string inlinedDots() ध्रुव { प्रत्यागम m_inlinedDots; }
    शून्य inlinedDots(ध्रुव string& flag) { m_inlinedDots = flag; }
    Ast_ग्रन्थिः_FTask* taskp() ध्रुव { प्रत्यागम m_taskp; }  // [After Link] Pointer to variable
    शून्य taskp(Ast_ग्रन्थिः_FTask* taskp) { m_taskp = taskp; }
    शून्य name(ध्रुव string& name) override { m_name = name; }
    शून्य dotted(ध्रुव string& name) { m_dotted = name; }
    Ast_ग्रन्थिः_Module* classOrPackagep() ध्रुव { प्रत्यागम m_classOrPackagep; }
    शून्य classOrPackagep(Ast_ग्रन्थिः_Module* nodep) { m_classOrPackagep = nodep; }
    bool अन्तर्भवति_Gen_खण्डम्() ध्रुव { प्रत्यागम m_अन्तर्भवति_Gen_खण्डम्; }
    शून्य अन्तर्भवति_Gen_खण्डम्(ध्रुव bool flag) { m_अन्तर्भवति_Gen_खण्डम् = flag; }
    bool pli() ध्रुव { प्रत्यागम m_pli; }
    शून्य pli(bool flag) { m_pli = flag; }
    bool superReference() ध्रुव { प्रत्यागम m_superReference; }
    शून्य superReference(bool flag) { m_superReference = flag; }
    bool is_विशुद्धम्() override;
    bool isUnlikely() ध्रुव override { प्रत्यागम m_taskp && m_taskp->isUnlikely(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव Ast_ग्रन्थिः_FTask_Ref* ध्रुव asamep = VN_DBG_AS(samep, ग्रन्थिः_FTask_Ref);
        प्रत्यागम taskp() == asamep->taskp()  //
               && classOrPackagep() == asamep->classOrPackagep()  //
               && name() == asamep->name()  //
               && dotted() == asamep->dotted()  //
               && inlinedDots() == asamep->inlinedDots()  //
               && pli() == asamep->pli()  //
               && अन्तर्भवति_Gen_खण्डम्() == asamep->अन्तर्भवति_Gen_खण्डम्()
               && superReference() == asamep->superReference();
    }
    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव final override { V3ERROR_NA_RETURN(true); }

गुप्त:
    bool getPurityRecurse() ध्रुव;
};
class Ast_ग्रन्थिः_Pre_Sel VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Something that becomes an AstSel
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : Optional[AstNodeExpr]
    // @astgen op4 := attrp : Optional[AstAttrOf]
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_ग्रन्थिः_Pre_Sel(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः_व्यञ्जकम्* thsp)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl} {
        this->fromp(fromp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Pre_Sel;
    // METHODS
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव final override { V3ERROR_NA_RETURN(true); }
    bool is_विशुद्धम्() override;
    ध्रुव वर्ण* broken() ध्रुव override;

गुप्त:
    bool getPurityRecurse() ध्रुव {
        प्रत्यागम fromp()->is_विशुद्धम्() && rhsp()->is_विशुद्धम्() && (!thsp() || thsp()->is_विशुद्धम्());
    }
};
class Ast_ग्रन्थिः_Quadop VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    // @astgen op4 := fhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_ग्रन्थिः_Quadop(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः_व्यञ्जकम्* thsp,
                  Ast_ग्रन्थिः_व्यञ्जकम्* fhsp)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Quadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    आभास शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs,
                               ध्रुव V3_सङ्ख्या& ths, ध्रुव V3_सङ्ख्या& fhs)
        = 0;
    आभास bool cleanLhs() ध्रुव = 0;  // True if LHS must have extra upper bits zero
    आभास bool cleanRhs() ध्रुव = 0;  // True if RHS must have extra upper bits zero
    आभास bool cleanThs() ध्रुव = 0;  // True if THS must have extra upper bits zero
    आभास bool cleanFhs() ध्रुव = 0;  // True if THS must have extra upper bits zero
    आभास bool आकार_Matters_Lhs() ध्रुव = 0;  // True if output result depends on lhs size
    आभास bool आकार_Matters_Rhs() ध्रुव = 0;  // True if output result depends on rhs size
    आभास bool आकार_Matters_Ths() ध्रुव = 0;  // True if output result depends on ths size
    आभास bool आकार_Matters_Fhs() ध्रुव = 0;  // True if output result depends on ths size
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    bool is_विशुद्धम्() override;
    ध्रुव वर्ण* broken() ध्रुव override;

गुप्त:
    bool getPurityRecurse() ध्रुव {
        प्रत्यागम lhsp()->is_विशुद्धम्() && rhsp()->is_विशुद्धम्() && thsp()->is_विशुद्धम्() && fhsp()->is_विशुद्धम्();
    }
};
class Ast_ग्रन्थिः_Termop VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Terminal operator -- an operator with no "inputs"
रक्षित:
    Ast_ग्रन्थिः_Termop(VNType t, File_पङ्क्तिः* fl)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Termop;
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class Ast_ग्रन्थिः_Triop VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Ternary expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := thsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_ग्रन्थिः_Triop(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः_व्यञ्जकम्* thsp)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Triop;
    // METHODS
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    // Set out to evaluation of a AstConst'ed
    आभास शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs,
                               ध्रुव V3_सङ्ख्या& ths)
        = 0;
    आभास bool cleanLhs() ध्रुव = 0;  // True if LHS must have extra upper bits zero
    आभास bool cleanRhs() ध्रुव = 0;  // True if RHS must have extra upper bits zero
    आभास bool cleanThs() ध्रुव = 0;  // True if THS must have extra upper bits zero
    आभास bool आकार_Matters_Lhs() ध्रुव = 0;  // True if output result depends on lhs size
    आभास bool आकार_Matters_Rhs() ध्रुव = 0;  // True if output result depends on rhs size
    आभास bool आकार_Matters_Ths() ध्रुव = 0;  // True if output result depends on ths size
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    bool is_विशुद्धम्() override;
    ध्रुव वर्ण* broken() ध्रुव override;

गुप्त:
    bool getPurityRecurse() ध्रुव {
        प्रत्यागम lhsp()->is_विशुद्धम्() && rhsp()->is_विशुद्धम्() && thsp()->is_विशुद्धम्();
    }
};
class Ast_ग्रन्थिः_Dist_Triop VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Triop {
सार्वजनिक:
    Ast_ग्रन्थिः_Dist_Triop(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp,
                     Ast_ग्रन्थिः_व्यञ्जकम्* thsp)
        : Ast_ग्रन्थिः_Triop{t, fl, lhsp, rhsp, thsp} {
        dtype_समुच्चयः_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Dist_Triop;
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool cleanThs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Ths() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL_TRIG; }
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs,
                       ध्रुव V3_सङ्ख्या& ths) override {
        V3ERROR_NA;
    }
};
class Ast_ग्रन्थिः_Uniop VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Unary expression
    // @astgen op1 := lhsp : AstNodeExpr
    VIsCached m_purity;  // Pure state

रक्षित:
    Ast_ग्रन्थिः_Uniop(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Uniop;
    // METHODS
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    // Set out to evaluation of a AstConst'ed lhs
    आभास शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) = 0;
    आभास bool cleanLhs() ध्रुव = 0;
    आभास bool आकार_Matters_Lhs() ध्रुव = 0;  // True if output result depends on lhs size
    आभास bool doubleFlavor() ध्रुव { प्रत्यागम false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    आभास bool signedFlavor() ध्रुव { प्रत्यागम false; }
    आभास bool stringFlavor() ध्रुव { प्रत्यागम false; }  // N flavor of nodes with both flavors?
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    bool is_विशुद्धम्() override;
    ध्रुव वर्ण* broken() ध्रुव override;
};
class Ast_ग्रन्थिः_System_Uniop_D VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_Uniop {
सार्वजनिक:
    Ast_ग्रन्थिः_System_Uniop_D(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : Ast_ग्रन्थिः_Uniop{t, fl, lhsp} {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_System_Uniop_D;
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL_TRIG; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class Ast_ग्रन्थिः_Var_Ref VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // An AstVarRef or AstVarXRef
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    // @astgen ptr := m_varScopep : Optional[AstVarScope]  // Varscope for hierarchy
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of the variable
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Var_Ref;  // Gen pointers before other members for performance
गुप्त:
    VAccess m_access;  // Left hand side assignment
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::रिक्तानि_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
रक्षित:
    Ast_ग्रन्थिः_Var_Ref(VNType t, File_पङ्क्तिः* fl, AstVar* varp, ध्रुव VAccess& access)
        : Ast_ग्रन्थिः_व्यञ्जकम्{t, fl}
        , m_access{access} {
        // May have varp==nullptr
        this->varp(varp);
    }

सार्वजनिक:
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    VAccess access() ध्रुव { प्रत्यागम m_access; }
    शून्य access(ध्रुव VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() ध्रुव VL_MT_STABLE { प्रत्यागम m_varp; }  // [After Link] Pointer to variable
    शून्य varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom(reinterpret_cast<Ast_ग्रन्थिः*>(varp));
    }
    AstVarScope* varScopep() ध्रुव { प्रत्यागम m_varScopep; }
    शून्य varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    ध्रुव VSelfPointerText& selfPointer() ध्रुव { प्रत्यागम m_selfPointer; }
    शून्य selfPointer(ध्रुव VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) ध्रुव {
        प्रत्यागम selfPointer().protect(useSelfForThis, protect());
    }
    Ast_ग्रन्थिः_Module* classOrPackagep() ध्रुव { प्रत्यागम m_classOrPackagep; }
    शून्य classOrPackagep(Ast_ग्रन्थिः_Module* nodep) { m_classOrPackagep = nodep; }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstArg final : सार्वजनिक Ast_ग्रन्थिः {
    // An argument to a function/task, which is either an expression, or is a placeholder for an
    // omitted argument.
    // @astgen op1 := exprp : Optional[AstNodeExpr] // nullptr if omitted
    std::string m_name;  // Argument name, or "" for number based interconnect
सार्वजनिक:
    AstArg(File_पङ्क्तिः* fl, ध्रुव std::string& name, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_Arg(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstArg;
    std::string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
    शून्य name(ध्रुव std::string& name) override { m_name = name; }
    bool रिक्तानि_Connect_No_अग्र() ध्रुव { प्रत्यागम !exprp() && name() == "" && !nextp(); }
};
class AstWith final : सार्वजनिक Ast_ग्रन्थिः {
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
    std::समुच्चयः<std::string> m_restricted_नामानि;

सार्वजनिक:
    AstWith(File_पङ्क्तिः* fl, AstLambdaArgRef* सूचकांक_Arg_Refp, AstLambdaArgRef* मानम्_Arg_Refp,
            Ast_ग्रन्थिः* exprp)
        : ASTGEN_SUPER_With(fl) {
        this->सूचकांक_Arg_Refp(सूचकांक_Arg_Refp);
        this->मानम्_Arg_Refp(मानम्_Arg_Refp);
        सङ्कलय_Exprp(exprp);
    }
    ASTGEN_MEMBERS_AstWith;
    bool hasDType() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!सूचकांक_Arg_Refp());  // varp needed to know lambda's arg dtype
        BROKEN_RTN(!मानम्_Arg_Refp());  // varp needed to know lambda's arg dtype
        प्रत्यागम nullptr;
    }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    शून्य restricted(bool flag) { m_restricted = flag; }
    bool restricted() ध्रुव { प्रत्यागम m_restricted; }
    bool validated() ध्रुव { प्रत्यागम m_validated; }
    शून्य validated(bool flag) { m_validated = flag; }
    शून्य सङ्कलय_Restricted_Name(ध्रुव std::string& name) { m_restricted_नामानि.निवेशय(name); }
    ध्रुव std::समुच्चयः<std::string>& restricted_नामानि() ध्रुव { प्रत्यागम m_restricted_नामानि; }
    // True if 'name' binds into the randomize() target class.
    bool name_Resolves_गन्तव्यम्_Target(ध्रुव std::string& name) ध्रुव {
        प्रत्यागम !m_restricted || m_restricted_नामानि.गणनम्(name);
    }
};

// === AstNodeExpr ===
class AstAbortOn final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // IEEE 1800-2023 16.12.14: accept_on/reject_on/sync_accept_on/sync_reject_on
    // (cond) prop. The four operators share an identical AST shape and lowering
    // scaffolding -- VAbortKind selects between Accept/Reject verdict and
    // Async/Sync sampling.
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := propp : AstNodeExpr
    VAbort_विधा m_विधा{VAbort_विधा::ACCEPT_ON};

सार्वजनिक:
    AstAbortOn(File_पङ्क्तिः* fl, VAbort_विधा विधा, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः_व्यञ्जकम्* propp)
        : ASTGEN_SUPER_AbortOn(fl)
        , m_विधा{विधा} {
        this->condp(condp);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstAbortOn;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    VAbort_विधा विधा() ध्रुव { प्रत्यागम m_विधा; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class Ast_Addr_सम्बन्धि_CFunc final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Get address of CFunc
    // @astgen ptr := m_funcp : AstCFunc  // Pointer to function itself

सार्वजनिक:
    Ast_Addr_सम्बन्धि_CFunc(File_पङ्क्तिः* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_Addr_सम्बन्धि_CFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

    ASTGEN_MEMBERS_Ast_Addr_सम्बन्धि_CFunc;
    शून्य dump(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    AstCFunc* funcp() ध्रुव { प्रत्यागम m_funcp; }
};
class Ast_Attr_सम्बन्धि final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Return a value of a attribute, for example a LSB or array LSB of a signal
    // @astgen op1 := fromp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := dimp : Optional[AstNodeExpr]
    VAttrType m_attrType;  // What sort of extraction
    string m_name;  // Name for some attributes
सार्वजनिक:
    Ast_Attr_सम्बन्धि(File_पङ्क्तिः* fl, VAttrType attrtype, Ast_ग्रन्थिः* fromp = nullptr,
              Ast_ग्रन्थिः_व्यञ्जकम्* dimp = nullptr)
        : ASTGEN_SUPER_Attr_सम्बन्धि(fl) {
        this->fromp(fromp);
        this->dimp(dimp);
        m_attrType = attrtype;
    }
    ASTGEN_MEMBERS_Ast_Attr_सम्बन्धि;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Var name
    शून्य name(ध्रुव string& name) override { m_name = name; }
    VAttrType attrType() ध्रुव { प्रत्यागम m_attrType; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstCExpr final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // C expression emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ध्रुव bool m_विशुद्धम्;  // Pure optimizable

    शून्य init(ध्रुव string& text, पूर्णाङ्क setwidth) {
        यदि (!text.रिक्तानि()) सङ्कलय(text);
        यदि (setwidth) {
            dtype_समुच्चयः_Bit_Sized(setwidth, VSigning::UNSIGNED);
        } अन्यथा {
            dtype_समुच्चयः_Void();  // Caller to override if necessary
        }
    }

सार्वजनिक:
    class विशुद्धम् {};
    explicit AstCExpr(File_पङ्क्तिः* fl, ध्रुव string& text = "", पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_विशुद्धम्{false} {
        init(text, setwidth);
    }
    AstCExpr(File_पङ्क्तिः* fl, विशुद्धम्, ध्रुव string& text = "", पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_CExpr(fl)
        , m_विशुद्धम्{true} {
        init(text, setwidth);
    }
    ASTGEN_MEMBERS_AstCExpr;
    // METHODS
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम m_विशुद्धम्; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम m_विशुद्धम्; }
    bool is_विशुद्धम्() override { प्रत्यागम m_विशुद्धम्; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    // Add some text, or a node to this expression
    शून्य सङ्कलय(ध्रुव std::string& text) { सङ्कलय_Nodesp(new AstText{fileline(), text}); }
    शून्य सङ्कलय(Ast_ग्रन्थिः* nodep) { सङ्कलय_Nodesp(nodep); }
};
class AstCExprUser final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // User '$c' or '$cpure' expression - Like AstCStmt, with text tracking and optimizations
    // disabled.
    //
    // Use AstCExpr instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ध्रुव bool m_विशुद्धम्;  // Whether the function is pure
सार्वजनिक:
    class विशुद्धम् {};
    explicit AstCExprUser(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_विशुद्धम्{false} {}
    AstCExprUser(File_पङ्क्तिः* fl, विशुद्धम्)
        : ASTGEN_SUPER_CExprUser(fl)
        , m_विशुद्धम्{true} {}
    ASTGEN_MEMBERS_AstCExprUser;
    // METHODS
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    std::string emitC() override { V3ERROR_NA_RETURN(""); }
    std::string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम m_विशुद्धम्; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम m_विशुद्धम्; }
    bool is_विशुद्धम्() override { प्रत्यागम m_विशुद्धम्; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    // Add some text, or a node to this expression
    शून्य सङ्कलय(ध्रुव std::string& text) { सङ्कलय_Nodesp(new AstText{fileline(), text}); }
    शून्य सङ्कलय(Ast_ग्रन्थिः* nodep) { सङ्कलय_Nodesp(nodep); }
};
class AstCFuncHard final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A call to a C++ function in the runtime library (include/verilated*)
    // PARENTS: stmt/expr
    // @astgen op1 := pinsp : List[AstNodeExpr] // Arguments
    VIsCached m_purity;  // Pure optimizable
    VCFunction m_function;  // Which function to call
सार्वजनिक:
    AstCFuncHard(File_पङ्क्तिः* fl, VCFunction function, Ast_ग्रन्थिः_व्यञ्जकम्* pinsp = nullptr)
        : ASTGEN_SUPER_CFuncHard(fl)
        , m_function{function} {
        सङ्कलय_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCFuncHard;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम function().ascii(); }
    bool is_विशुद्धम्() override {
        यदि (!m_purity.isCached()) m_purity.समुच्चयः(getPurity());
        प्रत्यागम m_purity.get();
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    VCFunction function() ध्रुव VL_MT_STABLE { प्रत्यागम m_function; }

गुप्त:
    bool getPurity();
};
class AstCMethodHard final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A reference to a "C" hardcoded member task (or function)
    // PARENTS: stmt/expr
    // @astgen op1 := fromp : AstNodeExpr // Subject of method call
    // @astgen op2 := pinsp : List[AstNodeExpr] // Arguments
    // @astgen op3 := withp : Optional[AstWith] // With clause
    VCMethod m_विधानम्;  // Which method to call
    VIsCached m_purity;  // Pure optimizable
    bool m_usePtr = false;  // Use '->' not '.'
सार्वजनिक:
    AstCMethodHard(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, VCMethod विधानम्, Ast_ग्रन्थिः_व्यञ्जकम्* pinsp = nullptr)
        : ASTGEN_SUPER_CMethodHard(fl)
        , m_विधानम्{विधानम्} {
        this->fromp(fromp);
        सङ्कलय_Pinsp(pinsp);
    }
    ASTGEN_MEMBERS_AstCMethodHard;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम विधानम्().ascii(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstCMethodHard* ध्रुव asamep = VN_DBG_AS(samep, CMethodHard);
        प्रत्यागम (m_विधानम् == asamep->m_विधानम्);
    }
    bool is_विशुद्धम्() override {
        यदि (!m_purity.isCached()) m_purity.समुच्चयः(getPurity());
        प्रत्यागम m_purity.get();
    }
    पूर्णाङ्क instr_गणनम्() ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम m_विधानम्.isClean(); }
    bool usePtr() ध्रुव { प्रत्यागम m_usePtr; }
    शून्य usePtr(bool flag) { m_usePtr = flag; }
    VCMethod विधानम्() ध्रुव VL_MT_STABLE { प्रत्यागम m_विधानम्; }
    शून्य विधानम्(VCMethod मानम्) { m_विधानम् = मानम्; }

गुप्त:
    bool getPurity();
};
class AstCReset final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Reset variable at startup
    ध्रुव bool m_constructing;  // Previously cleared by constructor
सार्वजनिक:
    AstCReset(File_पङ्क्तिः* fl, AstVar* varp, bool constructing)
        : ASTGEN_SUPER_CReset(fl)
        , m_constructing{constructing} {
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstCReset;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool is_विशुद्धम्() override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!VN_IS(backp(), ग्रन्थिः_नियोजय));  // V3Emit* assumption
        प्रत्यागम nullptr;
    }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम constructing() == VN_DBG_AS(samep, CReset)->constructing();
    }
    bool constructing() ध्रुव { प्रत्यागम m_constructing; }
};
class AstCast final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Cast to appropriate data type
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := childDTypep : Optional[AstNodeDType]
सार्वजनिक:
    AstCast(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, VFlagChildDType, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        childDTypep(dtp);
        dtypeFrom(dtp);
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCast(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Cast(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_AstCast;
    string emitVerilog() override { प्रत्यागम "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
    Ast_ग्रन्थिः_DType* getChildDTypep() ध्रुव override { प्रत्यागम childDTypep(); }
    Ast_ग्रन्थिः_DType* subDTypep() ध्रुव VL_MT_STABLE { प्रत्यागम dtypep() ? dtypep() : childDTypep(); }
};
class AstCastParse final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Cast to appropriate type, where we haven't determined yet what the data type is
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := dtp : AstNode
सार्वजनिक:
    AstCastParse(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः* dtp)
        : ASTGEN_SUPER_CastParse(fl) {
        this->lhsp(lhsp);
        this->dtp(dtp);
    }
    ASTGEN_MEMBERS_AstCastParse;
    string emitVerilog() override { प्रत्यागम "((%d)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cast_आकार final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Cast to specific size; signed/twostate inherited from lower element per IEEE
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstConst
सार्वजनिक:
    Ast_Cast_आकार(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, AstConst* rhsp)
        : ASTGEN_SUPER_Cast_आकार(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_Ast_Cast_आकार;
    string emitVerilog() override { प्रत्यागम "((%r)'(%l))"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstCellArrayRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // As-of-yet unlinkable reference into an array of cells
    // @astgen op1 := selp : List[AstNodeExpr] // Select expression
    string m_name;  // Array name
सार्वजनिक:
    AstCellArrayRef(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_व्यञ्जकम्* selp)
        : ASTGEN_SUPER_CellArrayRef(fl)
        , m_name{name} {
        सङ्कलय_Selp(selp);
    }
    ASTGEN_MEMBERS_AstCellArrayRef;
    // ACCESSORS
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstCellRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // As-of-yet unlinkable reference into a cell
    // @astgen op1 := cellp : AstNode
    // @astgen op2 := exprp : AstNode<AstNodeExpr|AstNodeDType>
    string m_name;  // Cell name
सार्वजनिक:
    AstCellRef(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* cellp, Ast_ग्रन्थिः* exprp)
        : ASTGEN_SUPER_CellRef(fl)
        , m_name{name} {
        this->cellp(cellp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCellRef;
    // ACCESSORS
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Array name

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstClassOrPackageRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := paramsp : List[AstPin]
    string m_name;
    // Node not NodeModule to appease some early parser usage
    // @astgen ptr := m_classOrPackageNodep : Optional[AstNode]  // Class/package referenced
सार्वजनिक:
    AstClassOrPackageRef(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* classOrPackageNodep,
                         AstPin* paramsp)
        : ASTGEN_SUPER_ClassOrPackageRef(fl)
        , m_name{name}
        , m_classOrPackageNodep{classOrPackageNodep} {
        सङ्कलय_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassOrPackageRef;
    // METHODS
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम (m_classOrPackageNodep
                == VN_DBG_AS(samep, ClassOrPackageRef)->m_classOrPackageNodep);
    }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Var name
    // There's no classOrPackagep(); use classOrPackageNodep() to get Node,
    // or iterating to package with classOrPackageSkipp()
    Ast_ग्रन्थिः_Module* classOrPackageSkipp(ध्रुव bool doRefs = true) ध्रुव;
    Ast_ग्रन्थिः* classOrPackageNodep() ध्रुव { प्रत्यागम m_classOrPackageNodep; }
    शून्य classOrPackageNodep(Ast_ग्रन्थिः* nodep) { m_classOrPackageNodep = nodep; }
    शून्य classOrPackagep(Ast_ग्रन्थिः_Module* nodep) {
        m_classOrPackageNodep = reinterpret_cast<Ast_ग्रन्थिः*>(nodep);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstConst final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A constant
    V3_सङ्ख्या m_अङ्कः;  // Constant value
    शून्य init_With_सङ्ख्या() {
        यदि (m_अङ्कः.isDouble()) {
            dtype_समुच्चयः_Double();
        } अन्यथा यदि (m_अङ्कः.isString()) {
            dtype_समुच्चयः_String();
        } अन्यथा {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(m_अङ्कः.विस्तारः(), (m_अङ्कः.sized() ? 0 : m_अङ्कः.विस्तारः_गन्तव्यम्_Fit()),
                                      VSigning::fromBool(m_अङ्कः.isSigned()), m_अङ्कः.isAnyXZ());
        }
        m_अङ्कः.nodep(this);
    }

सार्वजनिक:
    ~AstConst() override;
    AstConst(File_पङ्क्तिः* fl, ध्रुव V3_सङ्ख्या& अङ्कः)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{अङ्कः} {
        init_With_सङ्ख्या();
    }
    class Widthed_मानम् {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, Widthed_मानम्, पूर्णाङ्क विस्तारः, uint32_t मानम्)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, विस्तारः, मानम्) {  // Need () constructor
        init_With_सङ्ख्या();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(File_पङ्क्तिः* fl, DTyped, ध्रुव Ast_ग्रन्थिः_DType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{this, nodedtypep} {
        init_With_सङ्ख्या();
    }
    class String_गन्तव्यम्_Parse {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, String_गन्तव्यम्_Parse, ध्रुव वर्ण* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{this, V3_सङ्ख्या::Verilog_सङ्ख्या_Literal{}, sourcep} {
        init_With_सङ्ख्या();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, VerilogStringLiteral, ध्रुव string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{this, V3_सङ्ख्या::VerilogStringLiteral{}, str} {
        init_With_सङ्ख्या();
    }
    AstConst(File_पङ्क्तिः* fl, uint32_t अङ्कः)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, 32, अङ्कः) {  // Need () constructor
        dtype_समुच्चयः_Bit_Unsized(m_अङ्कः.विस्तारः(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, Unsized32, uint32_t अङ्कः)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, 32, अङ्कः) {  // Need () constructor
        m_अङ्कः.विस्तारः(32, false);
        dtype_समुच्चयः_Bit_Unsized(32, m_अङ्कः.विस्तारः_गन्तव्यम्_Fit(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, Signed32, int32_t अङ्कः)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, 32, अङ्कः) {  // Need () constructor
        m_अङ्कः.विस्तारः(32, true);
        dtype_समुच्चयः_Bit_Unsized(32, m_अङ्कः.विस्तारः_गन्तव्यम्_Fit(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, Unsized64, uint64_t अङ्कः)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, 64, 0) {  // Need () constructor
        m_अङ्कः.समुच्चयः_Quad(अङ्कः);
        dtype_समुच्चयः_Bit_Sized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, SizedEData, uint64_t अङ्कः)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, VL_EDATASIZE, 0) {  // Need () constructor
        m_अङ्कः.समुच्चयः_Quad(अङ्कः);
        dtype_समुच्चयः_Bit_Sized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, RealDouble, द्विगुण अङ्कः)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, 64) {  // Need () constructor
        m_अङ्कः.समुच्चयः_Double(अङ्कः);
        dtype_समुच्चयः_Double();
    }
    class String {};  // for creator type-overload selection
    AstConst(File_पङ्क्तिः* fl, String, ध्रुव string& अङ्कः)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{this, V3_सङ्ख्या::String{}, अङ्कः} {
        dtype_समुच्चयः_String();
    }
    class BitFalse {};
    AstConst(File_पङ्क्तिः* fl, BitFalse)  // Shorthand const 0, dtype should be a bit of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, 1, 0) {  // Need () constructor
        dtype_समुच्चयः_Bit();
    }
    // False but created due to reporting earlier error; suppress some later errors
    class BitFalseErroring {};
    AstConst(File_पङ्क्तिः* fl, BitFalseErroring)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, 1, 0) {  // Need () constructor
        dtype_समुच्चयः_Bit();
        File_पङ्क्तिः* ध्रुव newfl = new File_पङ्क्तिः{fileline()};
        newfl->erroringOn(true);
        fileline(newfl);
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a bit of size 1
    class BitTrue {};
    AstConst(File_पङ्क्तिः* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः(this, 1, on) {  // Need () constructor
        dtype_समुच्चयः_Bit();
    }
    class All0 {};
    AstConst(File_पङ्क्तिः* fl, All0)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{this, V3_सङ्ख्या::Verilog_सङ्ख्या_Literal{}, "'0"} {
        init_With_सङ्ख्या();
        fl->warnOff(V3_Error_संहिता::NEWERSTD, true);
    }
    class All1 {};
    AstConst(File_पङ्क्तिः* fl, All1)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{this, V3_सङ्ख्या::Verilog_सङ्ख्या_Literal{}, "'1"} {
        init_With_सङ्ख्या();
        fl->warnOff(V3_Error_संहिता::NEWERSTD, true);
    }
    class Null {};
    AstConst(File_पङ्क्तिः* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{this, V3_सङ्ख्या::Null{}} {
        dtypep(findNullDType());
        init_With_सङ्ख्या();
    }
    class One_चरणः {};
    AstConst(File_पङ्क्तिः* fl, One_चरणः)
        : ASTGEN_SUPER_Const(fl)
        , m_अङ्कः{this, V3_सङ्ख्या::One_चरणः{}} {
        dtype_समुच्चयः_Bit_Sized(64, VSigning::UNSIGNED);
        init_With_सङ्ख्या();
    }
    ASTGEN_MEMBERS_AstConst;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम अङ्कः().ascii(); }  // * = Value
    ध्रुव V3_सङ्ख्या& अङ्कः() ध्रुव VL_MT_SAFE { प्रत्यागम m_अङ्कः; }  // * = Value
    V3_सङ्ख्या& अङ्कः() { प्रत्यागम m_अङ्कः; }  // * = Value
    string origParamName() ध्रुव;
    शून्य origParamName(ध्रुव string& name);
    uint32_t गन्तव्यम्_UInt() ध्रुव { प्रत्यागम अङ्कः().गन्तव्यम्_UInt(); }
    int32_t गन्तव्यम्_SInt() ध्रुव VL_MT_SAFE { प्रत्यागम अङ्कः().गन्तव्यम्_SInt(); }
    uint64_t गन्तव्यम्_UQuad() ध्रुव { प्रत्यागम अङ्कः().गन्तव्यम्_UQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstConst* ध्रुव sp = VN_DBG_AS(samep, Const);
        प्रत्यागम अङ्कः().isCaseEq(sp->अङ्कः());
    }
    शून्य cloneRelink() override;
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(m_अङ्कः.nodep() && m_अङ्कः.nodep() != this);
        प्रत्यागम nullptr;
    }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool isEqAllOnes() ध्रुव { प्रत्यागम अङ्कः().isEqAllOnes(विस्तारः()); }
    bool isEqAllOnesV() ध्रुव { प्रत्यागम अङ्कः().isEqAllOnes(विस्तारः_Min_V()); }
    // Whether a parameter declared without a data type takes the same type from this value
    // as from 'samep': both real, both string, or both integral with the same width and
    // signedness. Unlike AstNodeDType::similarDType, ignores whether two-state or four-state.
    bool same_मानम्_Type(ध्रुव AstConst* samep) ध्रुव;
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    स्थिर AstConst* parseParamLiteral(File_पङ्क्तिः* fl, ध्रुव string& literal);
};
class AstConstraintRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A reference to a constraint identifier
    // @astgen op1 := fromp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_constrp : AstConstraint  // The constraint pointed to
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package of constraint

सार्वजनिक:
    AstConstraintRef(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, AstConstraint* constrp)
        : ASTGEN_SUPER_ConstraintRef(fl) {
        this->fromp(fromp);
        this->constrp(constrp);
        dtypep(findConstraintRefDType());
    }
    ASTGEN_MEMBERS_AstConstraintRef;
    string name() ध्रुव override VL_MT_STABLE;
    AstConstraint* constrp() ध्रुव VL_MT_STABLE { प्रत्यागम m_constrp; }
    शून्य constrp(AstConstraint* nodep) { m_constrp = nodep; }
    Ast_ग्रन्थिः_Module* classOrPackagep() ध्रुव VL_MT_STABLE { प्रत्यागम m_classOrPackagep; }
    // TODO: this setter is unused
    शून्य classOrPackagep(Ast_ग्रन्थिः_Module* nodep) { m_classOrPackagep = nodep; }

    string emitVerilog() final override { V3ERROR_NA_RETURN(""); }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव final override { V3ERROR_NA_RETURN(true); }
};
class Ast_Cvt_Array_गन्तव्यम्_Array final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Copy/Cast from dynamic/unpacked types to dynamic/unpacked types
    // @astgen op1 := fromp : AstNodeExpr
गुप्त:
    ध्रुव bool m_reverse;  // whether ordering gets reversed in this operation (ex: {<<{expr}})
    ध्रुव पूर्णाङ्क m_खण्डम्_आकार;  // num bits per block in a streaming operation (ex: 4 in {<<4{expr}}))
    ध्रुव पूर्णाङ्क m_dst_Element_द्वयङ्काः;  // num bits in lhs (ex: 8 if to byte-queue, 1 if to bit-queue)
    ध्रुव पूर्णाङ्क m_src_Element_द्वयङ्काः;  // num bits in rhs (ex 8 if from byte-queue, 1 if from bit-queue)

सार्वजनिक:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_गन्तव्यम्_Array(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_DType* dtp, bool reverse,
                       पूर्णाङ्क खण्डम्_आकार, पूर्णाङ्क dst_Element_द्वयङ्काः, पूर्णाङ्क src_Element_द्वयङ्काः)
        : ASTGEN_SUPER_Cvt_Array_गन्तव्यम्_Array(fl)
        , m_reverse{reverse}
        , m_खण्डम्_आकार{खण्डम्_आकार}
        , m_dst_Element_द्वयङ्काः{dst_Element_द्वयङ्काः}
        , m_src_Element_द्वयङ्काः{src_Element_द्वयङ्काः} {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_गन्तव्यम्_Array;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool reverse() ध्रुव { प्रत्यागम m_reverse; }
    पूर्णाङ्क खण्डम्_आकार() ध्रुव { प्रत्यागम m_खण्डम्_आकार; }
    पूर्णाङ्क dst_Element_द्वयङ्काः() ध्रुव { प्रत्यागम m_dst_Element_द्वयङ्काः; }
    पूर्णाङ्क src_Element_द्वयङ्काः() ध्रुव { प्रत्यागम m_src_Element_द्वयङ्काः; }
};
class Ast_Cvt_Array_गन्तव्यम्_Packed final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Cast from dynamic queue data type to packed array
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Array_गन्तव्यम्_Packed(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Cvt_Array_गन्तव्यम्_Packed(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Array_गन्तव्यम्_Packed;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
};
class Ast_Cvt_Packed_गन्तव्यम्_Array final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Cast from packed array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Packed_गन्तव्यम्_Array(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Cvt_Packed_गन्तव्यम्_Array(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Packed_गन्तव्यम्_Array;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
};
class Ast_Cvt_Unpack_गन्तव्यम्_Queue final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Cast from unpacked array to dynamic/unpacked queue data type
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    Ast_Cvt_Unpack_गन्तव्यम्_Queue(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_गन्तव्यम्_Queue(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_गन्तव्यम्_Queue;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { प्रत्यागम "VL_CVT_UNPACK_TO_Q(%P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
};
class Ast_Cvt_Unpack_गन्तव्यम्_Unpack final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Convert an unpacked array to one with the same shape but different ascending/descending
    // direction in at least one dimension (IEEE 1800-2023 7.6).
    // Any combination of dimensions is reversed by the one conversion, which dimensions follow
    // from comparing the two types dimension by dimension. E.g. from 'int a[2:1][1:2]', to
    // 'int b[2:1][2:1]' reverses only the inner dimension, and to 'int c[1:2][1:2]' only the
    // outer one. A conversion of an inner dimension is not inserted lower down instead, which
    // would need the arrays split into elements.
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    Ast_Cvt_Unpack_गन्तव्यम्_Unpack(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Cvt_Unpack_गन्तव्यम्_Unpack(fl) {
        this->fromp(fromp);
        dtypeFrom(dtp);
    }
    ASTGEN_MEMBERS_Ast_Cvt_Unpack_गन्तव्यम्_Unpack;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
};
class AstDist final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstDistItem]
सार्वजनिक:
    AstDist(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstDistItem* itemsp)
        : ASTGEN_SUPER_Dist(fl) {
        this->exprp(exprp);
        सङ्कलय_Itemsp(itemsp);
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstDist;
    string emitVerilog() override { प्रत्यागम "%l dist { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }  // NA
};
class AstDistItem final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Constraint distribution item
    // @astgen op1 := rangep : AstNodeExpr
    // @astgen op2 := weightp : AstNodeExpr
    bool m_is_सम्पूर्णम् = false;  // True for weight ':/', false for ':='
सार्वजनिक:
    AstDistItem(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* rangep, Ast_ग्रन्थिः_व्यञ्जकम्* weightp)
        : ASTGEN_SUPER_DistItem(fl) {
        this->rangep(rangep);
        this->weightp(weightp);
    }
    ASTGEN_MEMBERS_AstDistItem;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { प्रत्यागम "%l "s + (m_is_सम्पूर्णम् ? ":/" : ":=") + " %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }  // NA
    शून्य is_सम्पूर्णम्(bool flag) { m_is_सम्पूर्णम् = flag; }
    bool is_सम्पूर्णम्() ध्रुव { प्रत्यागम m_is_सम्पूर्णम्; }
};
class AstDot final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A dot separating paths in an AstVarXRef, AstFuncRef or AstTaskRef
    // These are eliminated in the link stage
    // @astgen op1 := lhsp : AstNode<AstNodeExpr|AstNodeDType>
    // @astgen op2 := rhsp : AstNode<AstNodeExpr|AstNodeDType>
    //
    // We don't have a list of elements as it's probably legal to do '(foo.bar).(baz.bap)'
    ध्रुव bool m_colon;  // Is a "::" instead of a "." (lhs must be package/class)
सार्वजनिक:
    AstDot(File_पङ्क्तिः* fl, bool colon, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Dot(fl)
        , m_colon{colon} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstDot;
    // For parser, make only if non-null package
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newIfPkg(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* packageOrClassp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        यदि (!packageOrClassp) प्रत्यागम rhsp;
        प्रत्यागम new AstDot{fl, true, packageOrClassp, rhsp};
    }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool colon() ध्रुव { प्रत्यागम m_colon; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class Ast_रिक्तानि_Queue final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
सार्वजनिक:
    explicit Ast_रिक्तानि_Queue(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_रिक्तानि_Queue(fl) {}
    ASTGEN_MEMBERS_Ast_रिक्तानि_Queue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { प्रत्यागम "{}"; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
};
class AstEnumItemRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen ptr := m_itemp : Optional[AstEnumItem]  // [AfterLink] Pointer to item
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of enum (for param relink)
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    bool m_अन्तर्भवति_Gen_खण्डम् = false;  // Contains gen block reference
सार्वजनिक:
    AstEnumItemRef(File_पङ्क्तिः* fl, AstEnumItem* itemp, Ast_ग्रन्थिः_Module* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_name{itemp->name()}
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_AstEnumItemRef;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    ध्रुव वर्ण* broken() ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम itemp() ? itemp()->name() : m_name; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstEnumItemRef* ध्रुव sp = VN_DBG_AS(samep, EnumItemRef);
        प्रत्यागम itemp() == sp->itemp() && dotted() == sp->dotted()
               && अन्तर्भवति_Gen_खण्डम्() == sp->अन्तर्भवति_Gen_खण्डम्();
    }
    AstEnumItem* itemp() ध्रुव VL_MT_STABLE { प्रत्यागम m_itemp; }
    शून्य itemp(AstEnumItem* nodep) { m_itemp = nodep; }
    string dotted() ध्रुव { प्रत्यागम m_dotted; }
    शून्य dotted(ध्रुव string& dotted) { m_dotted = dotted; }
    bool अन्तर्भवति_Gen_खण्डम्() ध्रुव { प्रत्यागम m_अन्तर्भवति_Gen_खण्डम्; }
    शून्य अन्तर्भवति_Gen_खण्डम्(ध्रुव bool flag) { m_अन्तर्भवति_Gen_खण्डम् = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    Ast_ग्रन्थिः_Module* classOrPackagep() ध्रुव { प्रत्यागम m_classOrPackagep; }
    शून्य classOrPackagep(Ast_ग्रन्थिः_Module* nodep) { m_classOrPackagep = nodep; }
};
class Ast_व्यञ्जकम्_Stmt final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Perform a statement, often assignment inside an expression node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNodeExpr
गुप्त:
    bool m_has_परिणाम = true;  // Returns result via resultp()

सार्वजनिक:
    Ast_व्यञ्जकम्_Stmt(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp, Ast_ग्रन्थिः_व्यञ्जकम्* resultp)
        : ASTGEN_SUPER_व्यञ्जकम्_Stmt(fl) {
        सङ्कलय_Stmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_Ast_व्यञ्जकम्_Stmt;
    // METHODS
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool is_विशुद्धम्() override {
        यदि (Ast_ग्रन्थिः::afterCommentp(stmtsp())) प्रत्यागम false;
        प्रत्यागम resultp()->is_विशुद्धम्();
    }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    bool has_परिणाम() ध्रुव { प्रत्यागम m_has_परिणाम; }
    शून्य has_परिणाम(bool flag) { m_has_परिणाम = flag; }
};
class AstFError final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := filep : AstNodeExpr
    // @astgen op2 := strp : AstNodeExpr
सार्वजनिक:
    AstFError(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* filep, Ast_ग्रन्थिः_व्यञ्जकम्* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFError;
    string emitVerilog() override { प्रत्यागम "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 64; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstFOpen final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op2 := filenamep : AstNodeExpr
    // @astgen op3 := modep : AstNodeExpr
सार्वजनिक:
    AstFOpen(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* filenamep, Ast_ग्रन्थिः_व्यञ्जकम्* modep)
        : ASTGEN_SUPER_FOpen(fl) {
        this->filenamep(filenamep);
        this->modep(modep);
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpen;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    string verilogKwd() ध्रुव override { प्रत्यागम "$fopen"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstFOpenMcd final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op2 := filenamep : AstNodeExpr
सार्वजनिक:
    AstFOpenMcd(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* filenamep)
        : ASTGEN_SUPER_FOpenMcd(fl) {
        this->filenamep(filenamep);
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFOpenMcd;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    string verilogKwd() ध्रुव override { प्रत्यागम "$fopen"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstFRead final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := memp : AstNodeExpr // VarRef for result
    // @astgen op2 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNodeExpr] // Offset
    // @astgen op4 := countp : Optional[AstNodeExpr] // Size
सार्वजनिक:
    AstFRead(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* memp, Ast_ग्रन्थिः_व्यञ्जकम्* filep, Ast_ग्रन्थिः_व्यञ्जकम्* startp,
             Ast_ग्रन्थिः_व्यञ्जकम्* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
        dtype_समुच्चयः_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRead;
    string verilogKwd() ध्रुव override { प्रत्यागम "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { प्रत्यागम true; }  // SPECIAL: makes output
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstFRewind final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := filep : Optional[AstNodeExpr]
सार्वजनिक:
    AstFRewind(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFRewind;
    string verilogKwd() ध्रुव override { प्रत्यागम "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstFScanF final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
    string m_text;

सार्वजनिक:
    AstFScanF(File_पङ्क्तिः* fl, ध्रुव string& text, Ast_ग्रन्थिः_व्यञ्जकम्* filep, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        सङ्कलय_Exprsp(exprsp);
        this->filep(filep);
        dtype_समुच्चयः_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFScanF;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_text; }
    string verilogKwd() ध्रुव override { प्रत्यागम "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { प्रत्यागम true; }  // SPECIAL: makes output
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम text() == VN_DBG_AS(samep, FScanF)->text();
    }
    string text() ध्रुव { प्रत्यागम m_text; }  // * = Text to display
    शून्य text(ध्रुव string& text) { m_text = text; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstFSeek final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNodeExpr]
    // @astgen op3 := operation : Optional[AstNodeExpr]
सार्वजनिक:
    AstFSeek(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* filep, Ast_ग्रन्थिः_व्यञ्जकम्* विस्थापनम्, Ast_ग्रन्थिः_व्यञ्जकम्* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->विस्थापनम्(विस्थापनम्);
        this->operation(operation);
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFSeek;
    string verilogKwd() ध्रुव override { प्रत्यागम "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { प्रत्यागम true; }  // SPECIAL: makes output
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstFTell final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
सार्वजनिक:
    AstFTell(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFTell;
    string verilogKwd() ध्रुव override { प्रत्यागम "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstFalling final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $falling_gclk
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    AstFalling(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_Falling(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstFalling;
    string emitVerilog() override { प्रत्यागम "$falling_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstFell final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $fell
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
सार्वजनिक:
    AstFell(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_Sen_वृक्षः* sentreep)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFell;
    string emitVerilog() override { प्रत्यागम "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstFuture final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $future_gclk
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
सार्वजनिक:
    AstFuture(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_Sen_वृक्षः* sentreep)
        : ASTGEN_SUPER_Future(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstFuture;
    string emitVerilog() override { प्रत्यागम "$future_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstGatePin final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNodeExpr // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
सार्वजनिक:
    AstGatePin(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_AstGatePin;
    string emitVerilog() override { प्रत्यागम "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
};
class Ast_Get_Initial_Random_बीजम् final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $get_initial_random_seed()
सार्वजनिक:
    explicit Ast_Get_Initial_Random_बीजम्(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_Get_Initial_Random_बीजम्(fl) {
        dtype_समुच्चयः_Int();
    }
    ASTGEN_MEMBERS_Ast_Get_Initial_Random_बीजम्;
    string emitVerilog() override { प्रत्यागम "$get_initial_random_seed()"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम true; }
    bool is_विशुद्धम्() override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstImplication final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Implication |-> |=> (IEEE 1800-2023 16.12.7) and followed-by #-# #=#
    // (IEEE 1800-2023 16.12.9). Antecedent-miss is vacuous-pass for implication
    // and non-vacuous-fail for followed-by, hence the separate flag.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
    // @astgen op3 := sentreep : Optional[AstSenTree]

गुप्त:
    ध्रुव bool m_isOverlapped;  // True if overlapped form (|-> / #-#)
    ध्रुव bool m_isFollowedBy;  // True if followed-by (#-# / #=#) rather than implication

सार्वजनिक:
    AstImplication(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, bool isOverlapped,
                   bool isFollowedBy = false)
        : ASTGEN_SUPER_Implication(fl)
        , m_isOverlapped{isOverlapped}
        , m_isFollowedBy{isFollowedBy} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstImplication;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम m_isFollowedBy || !m_isOverlapped; }
    bool isOverlapped() ध्रुव { प्रत्यागम m_isOverlapped; }
    bool isFollowedBy() ध्रुव { प्रत्यागम m_isFollowedBy; }
};
class AstInside final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := itemsp : List[AstNodeExpr]
सार्वजनिक:
    AstInside(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        सङ्कलय_Itemsp(itemsp);
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstInside;
    string emitVerilog() override { प्रत्यागम "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }  // NA
};
class AstInsideRange final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr
सार्वजनिक:
    AstInsideRange(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstInsideRange;
    string emitVerilog() override { प्रत्यागम "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    Ast_ग्रन्थिः_व्यञ्जकम्* newAndFromInside(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp);
};
class AstLambdaArgRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
    string m_name;  // Name of variable
    bool m_सूचकांक;  // Index, not value

सार्वजनिक:
    AstLambdaArgRef(File_पङ्क्तिः* fl, ध्रुव string& name, bool सूचकांक)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_सूचकांक{सूचकांक} {}
    ASTGEN_MEMBERS_AstLambdaArgRef;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    string emitVerilog() override { प्रत्यागम name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Var name
    शून्य name(ध्रुव string& name) override { m_name = name; }
    bool सूचकांक() ध्रुव { प्रत्यागम m_सूचकांक; }
    bool is_व्यञ्जकम्_Coverage_Eligible() ध्रुव override { प्रत्यागम false; }
};
class Ast_मेलनम्_Masked final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // This is a non-source construct, created internally to represent
    // some case statements. It is a '(mask & _) == bits' matching loop
    // where {mask, bits} pairs are packed into a single wide 'matchp',
    // and the result is the index of the first matching entry.
    // See VL_DECODER_* runtime functions.
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := matchp : AstVarRef
सार्वजनिक:
    अन्तःस्थ Ast_मेलनम्_Masked(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, AstVarRef* matchp);
    ASTGEN_MEMBERS_Ast_मेलनम्_Masked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { प्रत्यागम "VL_MATCHMASKED_%lq(%lw, %li, %ri)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    स्थिर uint32_t fold(ध्रुव V3_सङ्ख्या& lhs, AstVar* मेलनम्_Varp);
};
class AstMatches final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // "matches" operator: "expr matches pattern"
    // @astgen op1 := lhsp : AstNodeExpr  // Expression to match
    // @astgen op2 := patternp : AstNode  // Pattern to match against
सार्वजनिक:
    AstMatches(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः* patternp)
        : ASTGEN_SUPER_Matches(fl) {
        this->lhsp(lhsp);
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_AstMatches;
    string emitVerilog() override { प्रत्यागम "%l matches %r"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstMemberSel final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := fromp : AstNodeExpr
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Post link, variable in class that is selecting
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    VAccess m_access;  // Read or write, as in AstNodeVarRef
सार्वजनिक:
    AstMemberSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, VFlagChildDType, ध्रुव string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, AstVar* varp);
    ASTGEN_MEMBERS_AstMemberSel;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
    शून्य name(ध्रुव string& name) override { m_name = name; }
    VAccess access() ध्रुव { प्रत्यागम m_access; }
    शून्य access(ध्रुव VAccess& flag) { m_access = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    AstVar* varp() ध्रुव { प्रत्यागम m_varp; }
    शून्य varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // New as shallow copy
    // @astgen op1 := rhsp : AstNodeExpr
सार्वजनिक:
    AstNewCopy(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewCopy;
    string emitVerilog() override { प्रत्यागम "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
};
class AstNewDynamic final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // New for dynamic array
    // @astgen op1 := sizep : AstNodeExpr
    // @astgen op2 := rhsp : Optional[AstNodeExpr]
सार्वजनिक:
    AstNewDynamic(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* sizep, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstNewDynamic;
    string emitVerilog() override { प्रत्यागम "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
};
class AstPExpr final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Property expression
    // @astgen op1 := bodyp : AstBegin
    // @astgen op2 := finalp : Optional[AstNodeStmt]
सार्वजनिक:
    explicit AstPExpr(File_पङ्क्तिः* fl, AstBegin* bodyp, Ast_ग्रन्थिः_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(nullptr);
        this->dtypep(dtypep);
    }
    explicit AstPExpr(File_पङ्क्तिः* fl, AstBegin* bodyp, Ast_ग्रन्थिः_Stmt* finalp, Ast_ग्रन्थिः_DType* dtypep)
        : ASTGEN_SUPER_PExpr(fl) {
        this->bodyp(bodyp);
        this->finalp(finalp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstPExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
};
class AstParseHolder final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A reference to something soon to replace, used in a select at parse time
    // that needs conversion to pull the upper lvalue later
सार्वजनिक:
    explicit AstParseHolder(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_ParseHolder(fl) {}
    ASTGEN_MEMBERS_AstParseHolder;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstParseRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A reference to a variable, function or task
    // We don't know which at parse time due to bison constraints
    // The link stages will replace this with AstVarRef, or AstTaskRef, etc.
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
    // @astgen op2 := ftaskrefp : Optional[AstNodeFTaskRef]

    string m_name;  // Name of the variable/function/task

सार्वजनिक:
    AstParseRef(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nullptr,
                Ast_ग्रन्थिः_FTask_Ref* ftaskrefp = nullptr)
        : ASTGEN_SUPER_ParseRef(fl)
        , m_name{name} {
        this->lhsp(lhsp);
        this->ftaskrefp(ftaskrefp);
    }
    ASTGEN_MEMBERS_AstParseRef;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Var name
    शून्य name(ध्रुव string& name) override { m_name = name; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstParseRef* ध्रुव asamep = VN_DBG_AS(samep, ParseRef);
        प्रत्यागम m_name == asamep->m_name;
    }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstPast final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
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
    AstPast(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* ticksp = nullptr,
            Ast_Sen_वृक्षः* sentreep = nullptr, bool propertyTiming = false)
        : ASTGEN_SUPER_Past(fl)
        , m_propertyTiming{propertyTiming} {
        this->exprp(exprp);
        this->ticksp(ticksp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstPast;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम m_propertyTiming == VN_DBG_AS(samep, Past)->m_propertyTiming;
    }
    bool propertyTiming() ध्रुव { प्रत्यागम m_propertyTiming; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstPatMember final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
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
    AstPatMember(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhssp, Ast_ग्रन्थिः* keyp, Ast_ग्रन्थिः_व्यञ्जकम्* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        सङ्कलय_Lhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_AstPatMember;
    string emitVerilog() override { प्रत्यागम lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 2; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    bool isConcat() ध्रुव { प्रत्यागम m_isConcat; }
    शून्य isConcat(bool flag) { m_isConcat = flag; }
    bool isDefault() ध्रुव { प्रत्यागम m_isDefault; }
    शून्य isDefault(bool flag) { m_isDefault = flag; }
    Ast_ग्रन्थिः_व्यञ्जकम्* कुञ्चिका_Exprp() ध्रुव { प्रत्यागम varrefp() ? varrefp() : VN_CAST(keyp(), ग्रन्थिः_व्यञ्जकम्); }
};
class Ast_आदर्शः final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
सार्वजनिक:
    Ast_आदर्शः(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* itemsp)
        : ASTGEN_SUPER_आदर्शः(fl) {
        सङ्कलय_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_आदर्शः;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    Ast_ग्रन्थिः_DType* getChildDTypep() ध्रुव override { प्रत्यागम childDTypep(); }
    Ast_ग्रन्थिः_DType* subDTypep() ध्रुव VL_MT_STABLE { प्रत्यागम dtypep() ? dtypep() : childDTypep(); }
};
class Ast_आदर्शः_तारकम् final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Pattern wildcard: ".*"
सार्वजनिक:
    explicit Ast_आदर्शः_तारकम्(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_आदर्शः_तारकम्(fl) {}
    ASTGEN_MEMBERS_Ast_आदर्शः_तारकम्;
    string emitVerilog() override { प्रत्यागम ".*"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class Ast_आदर्शः_Var final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Pattern variable binding: ".variable"
    string m_name;  // Variable name
सार्वजनिक:
    Ast_आदर्शः_Var(File_पङ्क्तिः* fl, ध्रुव string& name)
        : ASTGEN_SUPER_आदर्शः_Var(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_आदर्शः_Var;
    string emitVerilog() override { प्रत्यागम ".%k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम m_name == VN_DBG_AS(samep, आदर्शः_Var)->m_name;
    }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
};
class AstPropAlways final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // always[m:n] / s_always[m:n] (IEEE 1800-2023 16.12.11)
    // @astgen op1 := propp : AstNodeExpr
    // @astgen op2 := loBoundp : AstNodeExpr
    // @astgen op3 := hiBoundp : AstNodeExpr
    ध्रुव bool m_isStrong = false;  // s_always
सार्वजनिक:
    AstPropAlways(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* propp, Ast_ग्रन्थिः_व्यञ्जकम्* loBoundp, Ast_ग्रन्थिः_व्यञ्जकम्* hiBoundp,
                  bool isStrong)
        : ASTGEN_SUPER_PropAlways(fl)
        , m_isStrong{isStrong} {
        this->propp(propp);
        this->loBoundp(loBoundp);
        this->hiBoundp(hiBoundp);
    }
    ASTGEN_MEMBERS_AstPropAlways;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { V3ERROR_NA_RETURN(0); }
    bool isStrong() ध्रुव { प्रत्यागम m_isStrong; }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstRand final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNodeExpr]
    ध्रुव bool m_urandom;  // $urandom vs $random
सार्वजनिक:
    class Reset {};
    AstRand(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    class SRandomU32 {};
    AstRand(File_पङ्क्तिः* fl, SRandomU32)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{true} {
        dtype_समुच्चयः_UInt32();
    }
    ASTGEN_MEMBERS_AstRand;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override {
        प्रत्यागम seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        यदि (seedp()) {
            यदि (urandom()) प्रत्यागम "VL_URANDOM_SEEDED_%nq%lq(%li)";
            प्रत्यागम "VL_RANDOM_SEEDED_%nq%lq(%li)";
        }
        यदि (isWide()) प्रत्यागम "VL_RANDOM_%nq(%nw, %P)";
        प्रत्यागम "VL_RANDOM_%nq()";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम !seedp(); }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool combinable(ध्रुव AstRand* samep) ध्रुव {
        प्रत्यागम !seedp() && !samep->seedp() && urandom() == samep->urandom();
    }
    bool urandom() ध्रुव { प्रत्यागम m_urandom; }
};
class AstRandRNG final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Random used in a class using VlRNG
    // Return a random number, based upon width()
सार्वजनिक:
    AstRandRNG(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_RandRNG(fl) {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstRandRNG;
    string emitVerilog() override { प्रत्यागम "%f$rngrandom()"; }
    string emitC() override {
        प्रत्यागम isWide() ? "VL_RANDOM_RNG_%nq(__Vm_rng, %nw, %P)"  //
                        : "VL_RANDOM_RNG_%nq(__Vm_rng)";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstRising final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $rising_gclk
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    AstRising(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_Rising(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstRising;
    string emitVerilog() override { प्रत्यागम "$rising_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstRose final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $rose
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
सार्वजनिक:
    AstRose(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_Sen_वृक्षः* sentreep)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstRose;
    string emitVerilog() override { प्रत्यागम "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstSClocked final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Sequence expression with an explicit leading clocking event
    // IEEE 1800-2023 16.7: sequence_expr ::= clocking_event sequence_expr
    // The clocking event is hoisted to the enclosing assertion clock by V3AssertNfa.
    // @astgen op1 := sensesp : AstSenItem
    // @astgen op2 := exprp : AstNodeExpr
सार्वजनिक:
    AstSClocked(File_पङ्क्तिः* fl, AstSenItem* sensesp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_SClocked(fl) {
        this->sensesp(sensesp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSClocked;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(false); }
};
class AstSConsRep final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
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
    ध्रुव bool m_unbounded = false;  // True when the upper bound is $
सार्वजनिक:
    // Exact [*N]
    AstSConsRep(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* countp)
        : ASTGEN_SUPER_SConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [*N:M] or unbounded [*N:$]/[+]/[*]
    AstSConsRep(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* countp, Ast_ग्रन्थिः_व्यञ्जकम्* maxCountp,
                bool unbounded)
        : ASTGEN_SUPER_SConsRep(fl)
        , m_unbounded{unbounded} {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSConsRep;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { V3ERROR_NA_RETURN(0); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {  // LCOV_EXCL_LINE
        प्रत्यागम m_unbounded == VN_DBG_AS(samep, SConsRep)->m_unbounded;  // LCOV_EXCL_LINE
    }
    bool unbounded() ध्रुव { प्रत्यागम m_unbounded; }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstSEventually final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // s_eventually
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    explicit AstSEventually(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_SEventually(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSEventually;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
};
class AstSExpr final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Sequence expression
    // @astgen op1 := preExprp: Optional[AstNodeExpr]
    // @astgen op2 := delayp : AstNodeStmt<AstDelay|AstBegin>
    // @astgen op3 := exprp : AstNodeExpr
सार्वजनिक:
    explicit AstSExpr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* preExprp, Ast_ग्रन्थिः_Stmt* delayp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(preExprp);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    explicit AstSExpr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_Stmt* delayp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_SExpr(fl) {
        this->preExprp(nullptr);
        this->delayp(delayp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSExpr;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstSFormatArg final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Information for formatting each argument to AstSFormat,
    // used to pass to (potentially) runtime decoding of format arguments
    // PARENT: SFormatF (or next list of expressions)
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := namep : Optional[AstNodeExpr] // Runtime enum name lookup
    VFormatAttr m_formatAttr;  // How to format expression

सार्वजनिक:
    AstSFormatArg(File_पङ्क्तिः* fl, VFormatAttr formatAttr, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_SFormatArg(fl)
        , m_formatAttr{formatAttr} {
        dtypeFrom(exprp);
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSFormatArg;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम formatAttr() == VN_DBG_AS(samep, SFormatArg)->formatAttr();
    }
    string verilogKwd() ध्रुव override { प्रत्यागम "$sformatarg"; }
    string emitVerilog() override { प्रत्यागम "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!VN_IS(backp(), SFormatF) && firstAbovep());  // In list under SFormatF
        BROKEN_RTN(formatAttr().isEnum() != static_cast<bool>(namep()));
        प्रत्यागम nullptr;
    }
    VFormatAttr formatAttr() ध्रुव { प्रत्यागम m_formatAttr; }
    शून्य formatAttr(ध्रुव VFormatAttr& formatAttr) { m_formatAttr = formatAttr; }
    स्थिर VFormatAttr formatAttrDefauled(ध्रुव AstSFormatArg* nodep, ध्रुव Ast_ग्रन्थिः_DType* dtypep);
};
class AstSFormatF final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Convert format to string, generally under an AstDisplay or AstSFormat
    // Also used as "real" function for /*verilator sformat*/ functions
    // exprsp() once past parsing may be AstSFormatArgs
    // @astgen op1 := exprsp : List[AstNodeExpr]
    // @astgen op2 := scopeNamep : Optional[AstScopeName]
    string m_text;
    bool m_hidden;  // Under display, etc
    bool m_व्यञ्जकम्_Format
        = false;  // Runtime Node* format, false = text() format code, false = possibly r
    bool m_optionalFormat
        = false;  // With exprFormat, first argument is either format or format-implied
    ध्रुव वर्ण m_missingArgChar;  // Format code when argument without format, 'h'/'o'/'b'
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    class व्यञ्जकम्_Format {};
    AstSFormatF(File_पङ्क्तिः* fl, ध्रुव string& text, bool hidden, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp,
                वर्ण missingArgChar = 'd')
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{text}
        , m_hidden{hidden}
        , m_missingArgChar{missingArgChar} {
        dtype_समुच्चयः_String();
        सङ्कलय_Exprsp(exprsp);
    }
    AstSFormatF(File_पङ्क्तिः* fl, व्यञ्जकम्_Format, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp, वर्ण missingArgChar = 'd',
                bool hidden = true)
        : ASTGEN_SUPER_SFormatF(fl)
        , m_text{""}
        , m_hidden{hidden}
        , m_व्यञ्जकम्_Format{true}
        , m_missingArgChar{missingArgChar} {
        dtype_समुच्चयः_String();
        सङ्कलय_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSFormatF;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_text; }
    शून्य name(ध्रुव string& name) override { m_text = name; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम text() == VN_DBG_AS(samep, SFormatF)->text();
    }
    string verilogKwd() ध्रुव override { प्रत्यागम "$sformatf"; }
    string text() ध्रुव { प्रत्यागम m_text; }  // * = Text to display
    शून्य text(ध्रुव string& text) { m_text = text; }
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(text() != "" && व्यञ्जकम्_Format_2());  // Expr format means no literal format
        प्रत्यागम nullptr;
    }
    bool formatScopeTracking() ध्रुव {  // Track scopeNamep();  Ok if false positive
        यदि (व्यञ्जकम्_Format_2() || name().find("%m") != string::npos || name().find("%M") != string::npos)
            प्रत्यागम true;
        प्रति (ध्रुव Ast_ग्रन्थिः* exprp = this->exprsp(); exprp; exprp = exprp->nextp()) {
            यदि (ध्रुव AstConst* ध्रुव fmtp = VN_CAST(exprp, Const)) {
                यदि (fmtp->अङ्कः().isFromString()) {
                    ध्रुव string str = fmtp->अङ्कः().गन्तव्यम्_String();
                    यदि (str.find("%m") != string::npos || str.find("%M") != string::npos)
                        प्रत्यागम true;
                }
            }
        }
        प्रत्यागम false;
    }
    bool hidden() ध्रुव { प्रत्यागम m_hidden; }
    शून्य hidden(bool flag) { m_hidden = flag; }
    bool व्यञ्जकम्_Format_2() ध्रुव { प्रत्यागम m_व्यञ्जकम्_Format; }
    शून्य व्यञ्जकम्_Format_2(bool flag) { m_व्यञ्जकम्_Format = flag; }
    bool optionalFormat() ध्रुव { प्रत्यागम m_optionalFormat; }
    शून्य optionalFormat(bool flag) { m_optionalFormat = flag; }
    वर्ण missingArgChar() ध्रुव { प्रत्यागम m_missingArgChar; }
    VTimescale timeunit() ध्रुव { प्रत्यागम m_timeunit; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstSGotoRep final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
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
    AstSGotoRep(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* countp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [->M:N]
    AstSGotoRep(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* countp, Ast_ग्रन्थिः_व्यञ्जकम्* maxCountp)
        : ASTGEN_SUPER_SGotoRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSGotoRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstSNonConsRep final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
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
    AstSNonConsRep(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* countp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
    }
    // Range [=M:N]
    AstSNonConsRep(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* countp, Ast_ग्रन्थिः_व्यञ्जकम्* maxCountp)
        : ASTGEN_SUPER_SNonConsRep(fl) {
        this->exprp(exprp);
        this->countp(countp);
        this->maxCountp(maxCountp);
    }
    ASTGEN_MEMBERS_AstSNonConsRep;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstSScanF final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := exprsp : List[AstNodeExpr] // VarRefs for results
    // @astgen op2 := fromp : AstNodeExpr
    string m_text;
    VTimescale m_timeunit;  // Parent module time unit

सार्वजनिक:
    AstSScanF(File_पङ्क्तिः* fl, ध्रुव string& text, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        सङ्कलय_Exprsp(exprsp);
        this->fromp(fromp);
        dtype_समुच्चयः_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSScanF;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_text; }
    string verilogKwd() ध्रुव override { प्रत्यागम "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() override { प्रत्यागम true; }  // SPECIAL: makes output
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstSScanF* ध्रुव asamep = VN_DBG_AS(samep, SScanF);
        प्रत्यागम text() == asamep->text() && timeunit() == asamep->timeunit();
    }
    string text() ध्रुव { प्रत्यागम m_text; }  // * = Text to display
    VTimescale timeunit() ध्रुव { प्रत्यागम m_timeunit; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstSampled final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode<AstNodeExpr|AstPropSpec>
    bool m_आन्तरिकम् : 1;  // Internally created, not from a source $sampled
सार्वजनिक:
    AstSampled(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* exprp, Ast_ग्रन्थिः_DType* dtypep, bool आन्तरिकम्_2 = false)
        : ASTGEN_SUPER_Sampled(fl)
        , m_आन्तरिकम्{आन्तरिकम्_2} {
        this->exprp(exprp);
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstSampled;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { प्रत्यागम "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम m_आन्तरिकम् == VN_DBG_AS(samep, Sampled)->m_आन्तरिकम्;
    }
    bool आन्तरिकम्_2() ध्रुव { प्रत्यागम m_आन्तरिकम्; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstScopeName final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // For display %m and DPI context imports
    // Parents:  AstSFormatF, AstNodeFTaskRef, AstNodeFTask, AstScope
    std::string m_scopeAttr;
    std::string m_scopeEntr;  // Scope path for the DPI import/export context name
    bool m_dpiExport = false;  // Is for dpiExport
    bool m_selfRelative = false;  // scopeAttr is the pretty path below the running instance
    ध्रुव bool m_forFormat;  // Is for a format %m
    स्थिर std::string scopeNameFormatter(ध्रुव std::string& text);
    स्थिर std::string scopePrettyNameFormatter(ध्रुव std::string& text);

सार्वजनिक:
    class ForFormat {};
    AstScopeName(File_पङ्क्तिः* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtype_समुच्चयः_UInt64();
    }
    ASTGEN_MEMBERS_AstScopeName;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstScopeName* ध्रुव sp = VN_DBG_AS(samep, ScopeName);
        प्रत्यागम m_scopeAttr == sp->m_scopeAttr  //
               && m_scopeEntr == sp->m_scopeEntr  //
               && m_dpiExport == sp->m_dpiExport  //
               && m_selfRelative == sp->m_selfRelative  //
               && m_forFormat == sp->m_forFormat;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    // ACCESSORS
    ध्रुव std::string& scopeAttr() ध्रुव { प्रत्यागम m_scopeAttr; }
    शून्य scopeAttr(ध्रुव std::string& val) { m_scopeAttr = val; }
    ध्रुव std::string& scopeEntr() ध्रुव { प्रत्यागम m_scopeEntr; }
    शून्य scopeEntr(ध्रुव std::string& val) { m_scopeEntr = val; }
    bool dpiExport() ध्रुव { प्रत्यागम m_dpiExport; }
    शून्य dpiExport(bool flag) { m_dpiExport = flag; }
    bool selfRelative() ध्रुव { प्रत्यागम m_selfRelative; }
    शून्य selfRelative(bool flag) { m_selfRelative = flag; }
    bool forFormat() ध्रुव { प्रत्यागम m_forFormat; }
    // Name for __Vscopep variable including children
    string scopeSymName() ध्रुव { प्रत्यागम scopeNameFormatter(m_scopeAttr); }
    // Name for DPI import scope
    string scopeDpiName() ध्रुव { प्रत्यागम scopeNameFormatter(m_scopeEntr); }
    // Name for __Vscopep variable including children
    string scopePrettySymName() ध्रुव { प्रत्यागम scopePrettyNameFormatter(m_scopeAttr); }
    // Name for __Vscopep variable including children
    string scopePrettyDpiName() ध्रुव { प्रत्यागम scopePrettyNameFormatter(m_scopeEntr); }
};
class AstStable final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $stable
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := sentreep : Optional[AstSenTree]
सार्वजनिक:
    AstStable(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_Sen_वृक्षः* sentreep)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstStable;
    string emitVerilog() override { प्रत्यागम "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class Ast_राशिः_Trace_F final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // $stacktrace used as function
सार्वजनिक:
    explicit Ast_राशिः_Trace_F(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_राशिः_Trace_F(fl) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_Ast_राशिः_Trace_F;
    string verilogKwd() ध्रुव override { प्रत्यागम "$stacktrace"; }
    string emitVerilog() override { प्रत्यागम verilogKwd(); }
    string emitC() override { प्रत्यागम "VL_STACKTRACE_N()"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstSteady final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $steady_gclk
    // @astgen op1 := exprp : AstNodeExpr
सार्वजनिक:
    AstSteady(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_Steady(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstSteady;
    string emitVerilog() override { प्रत्यागम "$steady_gclk(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstStructSel final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Unpacked struct/union member access
    // Parents: math|stmt
    // Children: varref, math
    // @astgen op1 := fromp : AstNodeExpr
    string m_name;  // Name of the member
सार्वजनिक:
    AstStructSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, ध्रुव string& name)
        : ASTGEN_SUPER_StructSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstStructSel;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override {
        // Not a union
        प्रत्यागम VN_IS(fromp()->dtypep()->skipRefp(), StructDType);
    }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstStructSel* ध्रुव sp = VN_DBG_AS(samep, StructSel);
        प्रत्यागम m_name == sp->m_name;
    }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
};
class AstSysIgnore final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // @astgen op1 := exprsp : List[AstNodeExpr] // Expressions to output (???)
सार्वजनिक:
    AstSysIgnore(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp)
        : ASTGEN_SUPER_SysIgnore(fl) {
        सङ्कलय_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstSysIgnore;
    string verilogKwd() ध्रुव override { प्रत्यागम "$ignored"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }  // Though deleted before opt
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }  // Though deleted before opt
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // Though deleted before opt
    bool isOutputter() override { प्रत्यागम true; }  // Though deleted before opt
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstSystemF final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // $system used as function
    // @astgen op1 := lhsp : AstNodeExpr
सार्वजनिक:
    AstSystemF(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
        dtype_समुच्चयः_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstSystemF;
    string verilogKwd() ध्रुव override { प्रत्यागम "$system"; }
    string emitVerilog() override { प्रत्यागम verilogKwd(); }
    string emitC() override { प्रत्यागम "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isOutputter() override { प्रत्यागम true; }
    bool isUnlikely() ध्रुव override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class Ast_Tagged_व्यञ्जकम् final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Tagged union expression: "tagged MemberName [expr]"
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Optional value expression
    string m_name;  // Member identifier name
सार्वजनिक:
    Ast_Tagged_व्यञ्जकम्(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_Tagged_व्यञ्जकम्(fl)
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_व्यञ्जकम्;
    string emitVerilog() override { प्रत्यागम "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम m_name == VN_DBG_AS(samep, Tagged_व्यञ्जकम्)->m_name;
    }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
};
class Ast_Tagged_आदर्शः final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Tagged pattern for matches: "tagged MemberName [pattern]"
    // @astgen op1 := patternp : Optional[AstNode]  // Optional nested pattern
    string m_name;  // Member identifier name
सार्वजनिक:
    Ast_Tagged_आदर्शः(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* patternp)
        : ASTGEN_SUPER_Tagged_आदर्शः(fl)
        , m_name{name} {
        this->patternp(patternp);
    }
    ASTGEN_MEMBERS_Ast_Tagged_आदर्शः;
    string emitVerilog() override { प्रत्यागम "tagged %k"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम m_name == VN_DBG_AS(samep, Tagged_आदर्शः)->m_name;
    }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }
};
class Ast_Test_Plus_तर्काः final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
सार्वजनिक:
    Ast_Test_Plus_तर्काः(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* searchp)
        : ASTGEN_SUPER_Test_Plus_तर्काः(fl) {
        this->searchp(searchp);
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_Test_Plus_तर्काः;
    string verilogKwd() ध्रुव override { प्रत्यागम "$test$plusargs"; }
    string emitVerilog() override { प्रत्यागम verilogKwd(); }
    string emitC() override { प्रत्यागम "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    // but isPure() true
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstThisRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Reference to 'this'.
    // @astgen op1 := childDTypep : Optional[AstClassRefDType] // dtype of the node
सार्वजनिक:
    AstThisRef(File_पङ्क्तिः* fl, VFlagChildDType, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        childDTypep(dtypep);
    }
    AstThisRef(File_पङ्क्तिः* fl, AstClassRefDType* dtypep)
        : ASTGEN_SUPER_ThisRef(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstThisRef;
    string emitC() override { प्रत्यागम "this"; }
    string emitVerilog() override { प्रत्यागम "this"; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    Ast_ग्रन्थिः_DType* getChildDTypep() ध्रुव override { प्रत्यागम childDTypep(); }
};
class AstTimePrecision final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Verilog $timeprecision
सार्वजनिक:
    explicit AstTimePrecision(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_TimePrecision(fl) {
        dtype_समुच्चयः_Integer2_State();
    }
    ASTGEN_MEMBERS_AstTimePrecision;
    string emitVerilog() override { प्रत्यागम "$timeprecision"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstTimeUnit final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    VTimescale m_timeunit;  // Parent module time unit
    // Verilog $timeunit
सार्वजनिक:
    explicit AstTimeUnit(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_TimeUnit(fl) {
        dtype_समुच्चयः_Integer2_State();
    }
    ASTGEN_MEMBERS_AstTimeUnit;
    string emitVerilog() override { प्रत्यागम "$timeunit"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    VTimescale timeunit() ध्रुव { प्रत्यागम m_timeunit; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }
};
class AstTriggered final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // True if any of the triggers at 'm_indices' are set in 'trigRefp'.
    // Explicit form of the trigger check, so the triggers tested can be recovered directly.
    // @astgen op1 := trigRefp : AstVarRef  // Reference to the trigger vector
    ध्रुव std::सदिशम्<uint32_t> m_indices;  // Indices of the triggers tested
सार्वजनिक:
    AstTriggered(File_पङ्क्तिः* fl, AstVarScope* trigVscp, ध्रुव std::सदिशम्<uint32_t>& indices);
    ASTGEN_MEMBERS_AstTriggered;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम m_indices == VN_DBG_AS(samep, Triggered)->m_indices;
    }
    ध्रुव std::सदिशम्<uint32_t>& indices() ध्रुव { प्रत्यागम m_indices; }
};
class AstUnbounded final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as int
सार्वजनिक:
    explicit AstUnbounded(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtype_समुच्चयः_Int();
    }
    ASTGEN_MEMBERS_AstUnbounded;
    string emitVerilog() override { प्रत्यागम "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
};
class AstUnlinkedRef final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // As-of-yet unlinkable Ref
    // @astgen op1 := refp : AstNode
    // @astgen op2 := cellrefp : AstNode

    string m_name;  // Var name // TODO: There is no way to access this, fix or remove
सार्वजनिक:
    AstUnlinkedRef(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* refp, ध्रुव string& name, Ast_ग्रन्थिः* cellrefp)
        : ASTGEN_SUPER_UnlinkedRef(fl)
        , m_name{name} {
        this->refp(refp);
        this->cellrefp(cellrefp);
    }
    ASTGEN_MEMBERS_AstUnlinkedRef;

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstUntil final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // The until property expression
    // @astgen op1 := lhsp : AstNodeExpr
    // @astgen op2 := rhsp : AstNodeExpr

    ध्रुव bool m_strong;  // 's_' preffix
    ध्रुव bool m_overlapping;  // '_with` suffix
सार्वजनिक:
    AstUntil(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, bool strong, bool overlapping)
        : ASTGEN_SUPER_Until(fl)
        , m_strong{strong}
        , m_overlapping{overlapping} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstUntil;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    string verilogKwd() ध्रुव override {
        प्रत्यागम (isStrong() ? "s_" : string()) + "until" + (isOverlapping() ? "_with" : "");
    }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
    bool isStrong() ध्रुव { प्रत्यागम m_strong; }
    bool isOverlapping() ध्रुव { प्रत्यागम m_overlapping; }
};
class Ast_मानम्_Plus_तर्काः final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNodeExpr]
    // @astgen op2 := outp : AstNodeExpr // VarRef for result
सार्वजनिक:
    Ast_मानम्_Plus_तर्काः(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* searchp, Ast_ग्रन्थिः_व्यञ्जकम्* outp)
        : ASTGEN_SUPER_मानम्_Plus_तर्काः(fl) {
        this->searchp(searchp);
        this->outp(outp);
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_Ast_मानम्_Plus_तर्काः;
    string verilogKwd() ध्रुव override { प्रत्यागम "$value$plusargs"; }
    string emitVerilog() override { प्रत्यागम "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम !outp(); }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstWithParse final : सार्वजनिक Ast_ग्रन्थिः_व्यञ्जकम् {
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
    AstWithParse(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* funcrefp, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp,
                 Ast_ग्रन्थिः* constraintsp = nullptr)
        : ASTGEN_SUPER_WithParse(fl) {
        this->funcrefp(funcrefp);
        सङ्कलय_Exprsp(exprsp);
        सङ्कलय_Constraintsp(constraintsp);
    }
    ASTGEN_MEMBERS_AstWithParse;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    bool restricted() ध्रुव { प्रत्यागम m_restricted; }
    शून्य restricted(bool flag) { m_restricted = flag; }

    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(true); }
};

// === AstNodeBiop ===
class AstBufIf1 final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
सार्वजनिक:
    AstBufIf1(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstBufIf1;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstCastDynamic final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
    // @astgen alias op1 := fromp
    // @astgen alias op2 := top
सार्वजनिक:
    AstCastDynamic(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Unsized(32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
                           VSigning::SIGNED);  // Spec says int return
    }
    ASTGEN_MEMBERS_AstCastDynamic;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { प्रत्यागम "%f$cast(%r, %l)"; }
    string emitC() override { प्रत्यागम "VL_DYNAMIC_CAST(%r, %l)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 20; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
};
class AstCompareNN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Verilog str.compare() and str.icompare()
    ध्रुव bool m_ignoreCase;  // True for str.icompare()
सार्वजनिक:
    AstCompareNN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtype_समुच्चयः_Int();
    }
    ASTGEN_MEMBERS_AstCompareNN;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        प्रत्यागम m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        प्रत्यागम m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool ignoreCase() ध्रुव { प्रत्यागम m_ignoreCase; }
};
class AstConcat final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // If you're looking for {#{}}, see AstReplicate
    // @astgen makeDfgVertex
सार्वजनिक:
    AstConcat(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        यदि (lhsp->dtypep() && rhsp->dtypep()) {
            dtype_समुच्चयः_Bit_Or_Logic_Sized(lhsp->dtypep()->विस्तारः() + rhsp->dtypep()->विस्तारः(),
                                    VSigning::UNSIGNED, isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_AstConcat;
    string emitVerilog() override { प्रत्यागम "%f{%l, %k%r}"; }
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opConcat(lhs, rhs);
    }
    string emitC() override { प्रत्यागम "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(concat %l %r)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 2; }
};
class AstConcatN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // String concatenate
सार्वजनिक:
    AstConcatN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_AstConcatN;
    string emitVerilog() override { प्रत्यागम "%f{%l, %k%r}"; }
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opConcatN(lhs, rhs);
    }
    string emitC() override { प्रत्यागम "VL_CONCATN_NNN(%li, %ri)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
    bool stringFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstDiv final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstDiv(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDiv;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opDiv(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f/ %r)"; }
    string emitC() override { प्रत्यागम "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvudiv %l %r)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_DIV; }
};
class AstDivD final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstDivD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_AstDivD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opDivD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "/"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL_DIV; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstDivS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstDivS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstDivS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opDivS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f/ %r)"; }
    string emitC() override { प्रत्यागम "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvsdiv %l %r)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_DIV; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstEqWild final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Note wildcard operator rhs differs from lhs
    // @astgen makeDfgVertex
सार्वजनिक:
    AstEqWild(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstEqWild;
    // Return AstEqWild/AstEqD
    स्थिर Ast_ग्रन्थिः_Biop* newTyped(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp);
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f==? %r)"; }
    string emitC() override {
        यदि (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            प्रत्यागम "VL_EQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override {
        यदि (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            प्रत्यागम "==";
        }
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstFGetS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen alias op1 := strgp
    // @astgen alias op2 := filep
सार्वजनिक:
    AstFGetS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Integer2_State();  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%f$fgets(%l,%r)"; }
    string emitC() override {
        प्रत्यागम strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 64; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
};
class AstFUngetC final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstFUngetC(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFUngetC;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        प्रत्यागम "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 64; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ग्रन्थिः* filep() ध्रुव { प्रत्यागम lhsp(); }
    Ast_ग्रन्थिः* charp() ध्रुव { प्रत्यागम rhsp(); }
};
class AstGetcN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Verilog string.getc()
सार्वजनिक:
    AstGetcN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGetcN(lhs, rhs);
    }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम "getc"; }
    string emitVerilog() override { प्रत्यागम "%k(%l.getc(%r))"; }
    string emitC() override { प्रत्यागम "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstGetcRefN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
सार्वजनिक:
    AstGetcRefN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Sized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_AstGetcRefN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstGt final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstGt(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGt;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGt(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f> %r)"; }
    string emitC() override { प्रत्यागम "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (bvugt %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम ">"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstGtD final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstGtD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstGtD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGtD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम ">"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstGtN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstGtN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstGtN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGtN(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम ">"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
    bool stringFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstGtS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstGtS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGtS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGtS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f> %r)"; }
    string emitC() override { प्रत्यागम "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (bvsgt %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstGte final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstGte(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGte;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGte(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f>= %r)"; }
    string emitC() override { प्रत्यागम "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (bvuge %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम ">="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstGteD final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstGteD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstGteD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGteD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम ">="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstGteN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstGteN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstGteN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGteN(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम ">="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
    bool stringFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstGteS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstGteS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstGteS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opGteS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f>= %r)"; }
    string emitC() override { प्रत्यागम "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (bvsge %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstLogAnd final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLogAnd(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogAnd;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f&& %r)"; }
    string emitC() override { प्रत्यागम "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvand %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "&&"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() + INSTR_गणनम्_BRANCH; }
};
class AstLogIf final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLogIf(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogIf;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f-> %r)"; }
    string emitC() override { प्रत्यागम "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (=> (__Vbool %l) (__Vbool %r)))"; }
    string emitSimpleOperator() override { प्रत्यागम "->"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() + INSTR_गणनम्_BRANCH; }
};
class AstLogOr final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLogOr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogOr;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f|| %r)"; }
    string emitC() override { प्रत्यागम "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvor %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "||"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() + INSTR_गणनम्_BRANCH; }
};
class AstLt final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLt(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLt;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLt(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f< %r)"; }
    string emitC() override { प्रत्यागम "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (bvult %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम "<"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstLtD final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstLtD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstLtD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLtD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "<"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstLtN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstLtN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstLtN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLtN(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "<"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
    bool stringFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstLtS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLtS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLtS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLtS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f< %r)"; }
    string emitC() override { प्रत्यागम "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (bvslt %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstLte final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLte(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLte;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLte(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f<= %r)"; }
    string emitC() override { प्रत्यागम "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (bvule %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम "<="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstLteD final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstLteD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstLteD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLteD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "<="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstLteN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstLteN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstLteN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLteN(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "<="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
    bool stringFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstLteS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLteS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLteS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLteS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f<= %r)"; }
    string emitC() override { प्रत्यागम "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (bvsle %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstModDiv final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstModDiv(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDiv;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f%% %r)"; }
    string emitC() override { प्रत्यागम "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvurem %l %r)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_DIV; }
};
class AstModDivS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstModDivS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstModDivS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f%% %r)"; }
    string emitC() override { प्रत्यागम "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvsmod %l %r)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_DIV; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstNeqWild final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNeqWild(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstNeqWild;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f!=? %r)"; }
    string emitC() override {
        यदि (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            प्रत्यागम "VL_NEQ_%lq(%lW, %P, %li, %ri)";
        }
    }
    string emitSimpleOperator() override {
        यदि (v3Global.opt.fourstate()) {
            V3ERROR_NA_RETURN("");
        } अन्यथा {
            प्रत्यागम "!=";
        }
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstPow final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstPow(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPow;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opPow(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f** %r)"; }
    string emitC() override { प्रत्यागम "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_MUL * 10; }
};
class AstPowD final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstPowD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_AstPowD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opPowD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f** %r)"; }
    string emitC() override { प्रत्यागम "pow(%li,%ri)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL_DIV * 5; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstPowSS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstPowSS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f** %r)"; }
    string emitC() override { प्रत्यागम "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_MUL * 10; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstPowSU final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstPowSU(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowSU;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f** %r)"; }
    string emitC() override { प्रत्यागम "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_MUL * 10; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstPowUS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstPowUS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstPowUS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f** %r)"; }
    string emitC() override { प्रत्यागम "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_MUL * 10; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstReplicate final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
सार्वजनिक:
    AstReplicate(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        यदि (lhsp) {
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(rhsp, Const)) {
                यदि (constp->अङ्कः().isFourState()
                    || (constp->dtypep()->isSigned() && constp->अङ्कः().isNegative())) {
                    dtype_समुच्चयः_Bit_Or_Logic_Sized(lhsp->विस्तारः(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());  // V3Width warns
                } अन्यथा {
                    dtype_समुच्चयः_Bit_Or_Logic_Sized(lhsp->विस्तारः() * constp->गन्तव्यम्_SInt(), VSigning::UNSIGNED,
                                            lhsp->isFourstateOrNoDtype());
                }
            }
        }
    }
    AstReplicate(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, uint32_t rep_गणनम्)
        : AstReplicate{fl, lhsp, new AstConst{fl, rep_गणनम्}} {}
    ASTGEN_MEMBERS_AstReplicate;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opRepl(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%f{%r{%k%l}}"; }
    string emitC() override { प्रत्यागम "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override {
        प्रत्यागम "((_ repeat " + cvt_गन्तव्यम्_Str(विस्तारः() / lhsp()->विस्तारः()) + ") %l)";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 2; }
};
class AstReplicateN final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // String replicate
सार्वजनिक:
    AstReplicateN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_AstReplicateN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opReplN(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%f{%r{%k%l}}"; }
    string emitC() override { प्रत्यागम "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 2; }
    bool stringFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstSAnd final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Sequence 'and' (IEEE 1800-2023 16.9.5): both operand sequences must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogAnd.
    ध्रुव bool m_propertyControl;  // Parser-generated property if/case branch conjunction
सार्वजनिक:
    AstSAnd(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, bool propertyControl = false)
        : ASTGEN_SUPER_SAnd(fl, lhsp, rhsp)
        , m_propertyControl{propertyControl} {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstSAnd;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %fand %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() + INSTR_गणनम्_BRANCH; }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {  // LCOV_EXCL_LINE
        प्रत्यागम m_propertyControl == VN_DBG_AS(samep, SAnd)->m_propertyControl;  // LCOV_EXCL_LINE
    }
    bool propertyControl() ध्रुव { प्रत्यागम m_propertyControl; }
};
class AstSIntersect final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Sequence 'intersect' (IEEE 1800-2023 16.9.6): both operands match with equal length.
    // AND with a length restriction. For boolean operands, lowered to AstLogAnd.
सार्वजनिक:
    AstSIntersect(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_SIntersect(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstSIntersect;
    // LCOV_EXCL_START  // Lowered before these are ever called
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %fintersect %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() + INSTR_गणनम्_BRANCH; }
    // LCOV_EXCL_STOP
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstSOr final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Sequence 'or' (IEEE 1800-2023 16.9.7): at least one operand sequence must match.
    // Operates on match sets, not values. For boolean operands, lowered to AstLogOr.
सार्वजनिक:
    AstSOr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_SOr(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstSOr;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogOr(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %for %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() + INSTR_गणनम्_BRANCH; }
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstSThroughout final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // expr throughout seq (IEEE 1800-2023 16.9.9)
सार्वजनिक:
    AstSThroughout(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_SThroughout(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstSThroughout;
    // LCOV_EXCL_START // Lowered in V3AssertProp before these are called
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %fthroughout %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    // LCOV_EXCL_STOP
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstSWithin final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // seq1 within seq2 (IEEE 1800-2023 16.9.10)
सार्वजनिक:
    AstSWithin(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_SWithin(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstSWithin;
    // LCOV_EXCL_START  // Lowered in V3AssertNfa before these are called
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %fwithin %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    // LCOV_EXCL_STOP
    bool is_Multi_आवृत्तिः_Sva() ध्रुव override { प्रत्यागम true; }
};
class AstSel final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // *Resolved* (tyep checked) multiple bit range extraction. Always const width
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    पूर्णाङ्क m_decl_El_विस्तारः;  // If a packed array, the number of bits per element
    // Selection width (!= this->width() from V3Clean onwards, != this->widthMin() in V3Width)
    पूर्णाङ्क m_विस्तारः_Const;

सार्वजनिक:
    AstSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* lsbp, पूर्णाङ्क bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp)
        , m_decl_El_विस्तारः{1}
        , m_विस्तारः_Const{bitwidth} {
        dtype_समुच्चयः_Bit_Or_Logic_Sized(bitwidth, VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    AstSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, पूर्णाङ्क lsb, पूर्णाङ्क bitwidth, पूर्णाङ्क bitwidthMin = 0)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb))  // Need () constructor
        , m_decl_El_विस्तारः{1}
        , m_विस्तारः_Const{bitwidth} {
        dtype_समुच्चयः_Bit_Or_Logic_Unsized(bitwidth, bitwidthMin ? bitwidthMin : bitwidth,
                                  VSigning::UNSIGNED, fromp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstSel;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& from, ध्रुव V3_सङ्ख्या& bit) override {
        बहिर्गमनम्.opSel(from, bit.गन्तव्यम्_UInt() + विस्तारः_Const() - 1, bit.गन्तव्यम्_UInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        प्रत्यागम विस्तारः_Const() == 1 ? "VL_BITSEL_%nq%lq%rqI(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rqI(%nw, %lw, %P, %li, %ri, %nw)"
                                 : "VL_SEL_%nq%lq%rqI(%lw, %P, %li, %ri, %nw)";
    }
    string emitSMT() ध्रुव override { प्रत्यागम "((_ extract %t %r) %l)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* otherp) ध्रुव override {
        प्रत्यागम विस्तारः_Const() == VN_DBG_AS(otherp, Sel)->विस्तारः_Const();
    }
    पूर्णाङ्क instr_गणनम्() ध्रुव override {
        प्रत्यागम विस्तारः_Instrs() * (VN_CAST(lsbp(), Const) ? 3 : 10) + 1;
    }
    पूर्णाङ्क विस्तारः_Const() ध्रुव { प्रत्यागम m_विस्तारः_Const; }
    शून्य विस्तारः_Const(पूर्णाङ्क मानम्) { m_विस्तारः_Const = मानम्; }
    पूर्णाङ्क lsbConst() ध्रुव { प्रत्यागम VN_AS(lsbp(), Const)->गन्तव्यम्_SInt(); }
    पूर्णाङ्क msbConst() ध्रुव { प्रत्यागम lsbConst() + विस्तारः_Const() - 1; }
    VNumRange& declRange() VL_MT_STABLE { प्रत्यागम m_declRange; }
    ध्रुव VNumRange& declRange() ध्रुव VL_MT_STABLE { प्रत्यागम m_declRange; }
    शून्य declRange(ध्रुव VNumRange& flag) { m_declRange = flag; }
    पूर्णाङ्क decl_El_विस्तारः() ध्रुव { प्रत्यागम m_decl_El_विस्तारः; }
    शून्य decl_El_विस्तारः(पूर्णाङ्क flag) { m_decl_El_विस्तारः = flag; }
};
class Ast_सरणम्_L final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    Ast_सरणम्_L(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_सरणम्_L(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = lhsp->dtypep()) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(dtypep->विस्तारः(), dtypep->विस्तारः_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_सरणम्_L;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_सरणम्_L(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f<< %r)"; }
    string emitC() override { प्रत्यागम "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvshl %l %r)"; }
    string emitSimpleOperator() override {
        प्रत्यागम (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class Ast_सरणम्_LOvr final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Like ShiftL but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftL
सार्वजनिक:
    Ast_सरणम्_LOvr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_सरणम्_LOvr(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(dtypep->विस्तारः(), dtypep->विस्तारः_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_सरणम्_LOvr;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_सरणम्_L(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f<< %r)"; }
    string emitC() override { प्रत्यागम "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class Ast_सरणम्_R final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    Ast_सरणम्_R(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_सरणम्_R(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = lhsp->dtypep()) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(dtypep->विस्तारः(), dtypep->विस्तारः_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_सरणम्_R;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_सरणम्_R(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f>> %r)"; }
    string emitC() override { प्रत्यागम "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvlshr %l %r)"; }
    string emitSimpleOperator() override {
        प्रत्यागम (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    // LHS size might be > output size, so don't want to force size
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class Ast_सरणम्_ROvr final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Like ShiftR but checks for an over shift and returns zeros
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftR
सार्वजनिक:
    Ast_सरणम्_ROvr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_सरणम्_ROvr(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(setwidth, 0,
                                      lhsp->dtypep() ? lhsp->dtypep()->numeric()
                                                     : VSigning{VSigning::UNSIGNED},
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(dtypep->विस्तारः(), dtypep->विस्तारः_Min(), dtypep->numeric(),
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_सरणम्_ROvr;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_सरणम्_R(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f>> %r)"; }
    string emitC() override { प्रत्यागम "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    // LHS size might be > output size, so don't want to force size
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class Ast_सरणम्_RS final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
    // @astgen makeDfgVertex
सार्वजनिक:
    Ast_सरणम्_RS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_सरणम्_RS(fl, lhsp, rhsp) {
        यदि (setwidth) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = lhsp->dtypep()) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(dtypep->विस्तारः(), dtypep->विस्तारः_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_सरणम्_RS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_सरणम्_RS(lhs, rhs, lhsp()->विस्तारः_Min_V());
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f>>> %r)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvashr %l %r)"; }
    string emitC() override { प्रत्यागम "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class Ast_सरणम्_RSOvr final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // Shift right with sign extension, >>> operator, checks for an over shift and returns zeros
    // Output data type's width determines which bit is used for sign extension
    // TODO: All Verilog shifts should start as these and later get demoted to AstShiftRSOvr
सार्वजनिक:
    Ast_सरणम्_RSOvr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, पूर्णाङ्क setwidth = 0)
        : ASTGEN_SUPER_सरणम्_RSOvr(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        यदि (setwidth) {
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(setwidth, 0, VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        } अन्यथा {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = lhsp->dtypep();
            UASSERT_OBJ(dtypep, this, "ShiftOvr should be created with lhs which has dtype");
            dtype_समुच्चयः_Bit_Or_Logic_Unsized(dtypep->विस्तारः(), dtypep->विस्तारः_Min(), VSigning::SIGNED,
                                      isFourstateOrNoDtype(lhsp, rhsp));
        }
    }
    ASTGEN_MEMBERS_Ast_सरणम्_RSOvr;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_सरणम्_RS(lhs, rhs, lhsp()->विस्तारः_Min_V());
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f>>> %r)"; }
    string emitC() override { प्रत्यागम "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstSub final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstSub(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstSub;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opSub(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f- %r)"; }
    string emitC() override { प्रत्यागम "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvsub %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "-"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
};
class AstSubD final : सार्वजनिक Ast_ग्रन्थिः_Biop {
सार्वजनिक:
    AstSubD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_AstSubD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opSubD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "-"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstURandomRange final : सार्वजनिक Ast_ग्रन्थिः_Biop {
    // $urandom_range
सार्वजनिक:
    AstURandomRange(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtype_समुच्चयः_UInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_AstURandomRange;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%f$urandom_range(%l, %r)"; }
    string emitC() override { प्रत्यागम "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstEq(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstEq;
    // Return AstEq/AstEqD
    स्थिर Ast_ग्रन्थिः_Biop* newTyped(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp);
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opEq(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f== %r)"; }
    string emitC() override { प्रत्यागम "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (= %l %r))"; }
    string emitSimpleOperator() override { प्रत्यागम "=="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstEqCase final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstEqCase(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstEqCase;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f=== %r)"; }
    string emitC() override { प्रत्यागम "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { प्रत्यागम "=="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstEqD final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
सार्वजनिक:
    AstEqD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstEqD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opEqD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "=="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstEqN final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
सार्वजनिक:
    AstEqN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstEqN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opEqN(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "=="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
    bool stringFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstEqT final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
    // Equal (==) for data types
सार्वजनिक:
    AstEqT(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_EqT(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstEqT;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "=="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
};
class AstLogEq final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstLogEq(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstLogEq;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f<-> %r)"; }
    string emitC() override { प्रत्यागम "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvxnor %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "<->"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() + INSTR_गणनम्_BRANCH; }
};
class AstNeq final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNeq(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(isFourstateOrNoDtype(lhsp, rhsp));
    }
    ASTGEN_MEMBERS_AstNeq;
    स्थिर Ast_ग्रन्थिः_Biop* newTyped(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp);
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opNeq(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f!= %r)"; }
    string emitC() override { प्रत्यागम "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { प्रत्यागम "!="; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (not (= %l %r)))"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstNeqCase final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNeqCase(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstNeqCase;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f!== %r)"; }
    string emitC() override { प्रत्यागम "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { प्रत्यागम "!="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
};
class AstNeqD final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
सार्वजनिक:
    AstNeqD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstNeqD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "!="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstNeqN final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
सार्वजनिक:
    AstNeqN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstNeqN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "!="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
    bool stringFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstNeqT final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com {
    // Not-equal (!=) for data types
सार्वजनिक:
    AstNeqT(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_NeqT(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstNeqT;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "!="; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_STR; }
};

// === AstNodeBiComAsv ===
class Ast_सङ्कलय final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    Ast_सङ्कलय(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_सङ्कलय(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_सङ्कलय;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_सङ्कलय(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f+ %r)"; }
    string emitC() override { प्रत्यागम "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvadd %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "+"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
};
class Ast_सङ्कलय_D final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com_Asv {
सार्वजनिक:
    Ast_सङ्कलय_D(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_सङ्कलय_D(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_Ast_सङ्कलय_D;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_सङ्कलय_D(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "+"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstAnd final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstAnd(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAnd;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opAnd(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f& %r)"; }
    string emitC() override { प्रत्यागम "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvand %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "&"; }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    ध्रुव वर्ण* विस्तारः_Mismatch() ध्रुव override VL_MT_STABLE;
};
class AstMul final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstMul(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMul;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opMul(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f* %r)"; }
    string emitC() override { प्रत्यागम "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvmul %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "*"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_MUL; }
};
class AstMulD final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com_Asv {
सार्वजनिक:
    AstMulD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_AstMulD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opMulD(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "*"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstMulS final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstMulS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstMulS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opMulS(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f* %r)"; }
    string emitC() override { प्रत्यागम "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvmul %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * INSTR_गणनम्_INT_MUL; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstOr final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstOr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstOr;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opOr(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f| %r)"; }
    string emitC() override { प्रत्यागम "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvor %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "|"; }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    ध्रुव वर्ण* विस्तारः_Mismatch() ध्रुव override VL_MT_STABLE;
};
class AstXor final : सार्वजनिक Ast_ग्रन्थिः_Bi_Com_Asv {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstXor(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstXor;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.opXor(lhs, rhs);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f^ %r)"; }
    string emitC() override { प्रत्यागम "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvxor %l %r)"; }
    string emitSimpleOperator() override { प्रत्यागम "^"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }  // Lclean && Rclean
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    ध्रुव वर्ण* विस्तारः_Mismatch() ध्रुव override VL_MT_STABLE;
};

// === AstNodeDistBiop ===
class Ast_Dist_Chi_वर्गफलम् final : सार्वजनिक Ast_ग्रन्थिः_Dist_Biop {
सार्वजनिक:
    Ast_Dist_Chi_वर्गफलम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Dist_Chi_वर्गफलम्(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_Chi_वर्गफलम्;
    string emitVerilog() override { प्रत्यागम "%f$dist_chi_square(%l, %r)"; }
    string emitC() override { प्रत्यागम "VL_DIST_CHI_SQUARE(%li, %ri)"; }
};
class Ast_Dist_घातीयम् final : सार्वजनिक Ast_ग्रन्थिः_Dist_Biop {
सार्वजनिक:
    Ast_Dist_घातीयम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Dist_घातीयम्(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_घातीयम्;
    string emitVerilog() override { प्रत्यागम "%f$dist_exponential(%l, %r)"; }
    string emitC() override { प्रत्यागम "VL_DIST_EXPONENTIAL(%li, %ri)"; }
};
class Ast_Dist_पोयसन final : सार्वजनिक Ast_ग्रन्थिः_Dist_Biop {
सार्वजनिक:
    Ast_Dist_पोयसन(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Dist_पोयसन(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_Dist_पोयसन;
    string emitVerilog() override { प्रत्यागम "%f$dist_poisson(%l, %r)"; }
    string emitC() override { प्रत्यागम "VL_DIST_POISSON(%li, %ri)"; }
};
class AstDistT final : सार्वजनिक Ast_ग्रन्थिः_Dist_Biop {
सार्वजनिक:
    AstDistT(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_DistT(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstDistT;
    string emitVerilog() override { प्रत्यागम "%f$dist_t(%l, %r)"; }
    string emitC() override { प्रत्यागम "VL_DIST_T(%li, %ri)"; }
};

// === AstNodeSel ===
class AstArraySel final : सार्वजनिक Ast_ग्रन्थिः_Sel {
    // @astgen makeDfgVertex
    शून्य init(ध्रुव Ast_ग्रन्थिः* fromp) {
        यदि (fromp && VN_IS(fromp->dtypep()->skipRefp(), ग्रन्थिः_Array_DType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), ग्रन्थिः_Array_DType)->subDTypep());
        }
    }

सार्वजनिक:
    AstArraySel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, पूर्णाङ्क bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {  // Need () constructor
        init(fromp);
    }
    ASTGEN_MEMBERS_AstArraySel;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    string emitSMT() ध्रुव override { प्रत्यागम "(select %l %r)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
};
class AstAssocSel final : सार्वजनिक Ast_ग्रन्थिः_Sel {
    शून्य init(ध्रुव Ast_ग्रन्थिः* fromp) {
        यदि (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

सार्वजनिक:
    AstAssocSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstAssocSel;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%k(%l%f[%r])"; }
    string emitC() override { प्रत्यागम "%li%k[%ri]"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(select %l %r)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isGateOptimizable() ध्रुव override {
        प्रत्यागम !isLValue();  // AssocSel creates on miss
    }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम !isLValue(); }  // AssocSel creates on miss
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
};
class AstWildcardSel final : सार्वजनिक Ast_ग्रन्थिः_Sel {
    शून्य init(ध्रुव Ast_ग्रन्थिः* fromp) {
        यदि (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

सार्वजनिक:
    AstWildcardSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AstWildcardSel;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%k(%l%f[%r])"; }
    string emitC() override { प्रत्यागम "%li%k[%ri]"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
};
class AstWordSel final : सार्वजनिक Ast_ग्रन्थिः_Sel {
    // Select a single word from a multi-word wide value
सार्वजनिक:
    AstWordSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtype_समुच्चयः_UInt32();  // Always used on VlWide arrays so returns EData size
    }
    ASTGEN_MEMBERS_AstWordSel;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&, ध्रुव V3_सङ्ख्या&) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%k(%l%f[%r])"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Special cased
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};

// === AstNodeStream ===
class Ast_प्रवाहः_L final : सार्वजनिक Ast_ग्रन्थिः_प्रवाहः {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
सार्वजनिक:
    Ast_प्रवाहः_L(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_प्रवाहः_L(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_प्रवाहः_L;
    string emitVerilog() override { प्रत्यागम "%f{ << %r %k{%l} }"; }
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_प्रवाहः_L(lhs, rhs);
    }
    string emitC() override { प्रत्यागम "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 2; }
};
class Ast_प्रवाहः_R final : सार्वजनिक Ast_ग्रन्थिः_प्रवाहः {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
    // @astgen makeDfgVertex
सार्वजनिक:
    Ast_प्रवाहः_R(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_प्रवाहः_R(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_प्रवाहः_R;
    string emitVerilog() override { प्रत्यागम "%f{ >> %r %k{%l} }"; }
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.op_नियोजय(lhs);
    }
    string emitC() override { प्रत्यागम isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 2; }
};

// === AstNodeSystemBiopD ===
class AstAtan2D final : सार्वजनिक Ast_ग्रन्थिः_System_Biop_D {
सार्वजनिक:
    AstAtan2D(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAtan2D;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::atan2(lhs.गन्तव्यम्_Double(), rhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$atan2(%l,%r)"; }
    string emitC() override { प्रत्यागम "atan2(%li,%ri)"; }
};
class AstHypotD final : सार्वजनिक Ast_ग्रन्थिः_System_Biop_D {
सार्वजनिक:
    AstHypotD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstHypotD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::hypot(lhs.गन्तव्यम्_Double(), rhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$hypot(%l,%r)"; }
    string emitC() override { प्रत्यागम "hypot(%li,%ri)"; }
};

// === AstNodeCCall ===
class AstCCall final : सार्वजनिक Ast_ग्रन्थिः_CCall {
    // C++ function call
    VSelfPointerText m_selfPointer
        = VSelfPointerText{VSelfPointerText::रिक्तानि_2()};  // Output code object
                                                        // pointer (e.g.: 'this')
सार्वजनिक:
    AstCCall(File_पङ्क्तिः* fl, AstCFunc* funcp, Ast_ग्रन्थिः_व्यञ्जकम्* argsp = nullptr)
        : ASTGEN_SUPER_CCall(fl, funcp, argsp) {
        UASSERT_OBJ(funcp, this, "CCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCCall;

    ध्रुव VSelfPointerText& selfPointer() ध्रुव { प्रत्यागम m_selfPointer; }
    शून्य selfPointer(ध्रुव VSelfPointerText& selfPointer) { m_selfPointer = selfPointer; }
    string selfPointerProtect(bool useSelfForThis) ध्रुव {
        प्रत्यागम selfPointer().protect(useSelfForThis, protect());
    }
    bool maybe_Pointed_गन्तव्यम्() ध्रुव override VL_MT_SAFE { प्रत्यागम true; }
};
class AstCMethodCall final : सार्वजनिक Ast_ग्रन्थिः_CCall {
    // C++ method call
    // @astgen op1 := fromp : AstNodeExpr
सार्वजनिक:
    AstCMethodCall(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, AstCFunc* funcp, Ast_ग्रन्थिः_व्यञ्जकम्* argsp = nullptr)
        : ASTGEN_SUPER_CMethodCall(fl, funcp, argsp) {
        this->fromp(fromp);
        UASSERT_OBJ(funcp, this, "CMethodCall created calling null function");
    }
    ASTGEN_MEMBERS_AstCMethodCall;
};
class AstCNew final : सार्वजनिक Ast_ग्रन्थिः_CCall {
    // C++ new() call
सार्वजनिक:
    AstCNew(File_पङ्क्तिः* fl, AstCFunc* funcp, Ast_ग्रन्थिः_व्यञ्जकम्* argsp = nullptr)
        : ASTGEN_SUPER_CNew(fl, funcp, argsp) {}
    ASTGEN_MEMBERS_AstCNew;
};

// === AstNodeCons ===
class AstConsArray final : सार्वजनिक Ast_ग्रन्थिः_Cons {
    // Construct an unpacked array value, '{value, ...}, '{index: value, default: value}
    // If default is specified, the items may be sparse, and not provide each element.
    // Indices are C++ array style, with lo() at index 0
    // @astgen op1 := defaultp : Optional[AstNodeExpr] // Default, if sparse
    // @astgen op2 := itemsp : private List[AstConsArrayItem] // In increasing index order
    // dist-ast-dump-suppress  // Dumped by the itemsp list
    std::map<uint64_t, AstConsArrayItem*> m_itemps;  // The item at each index given

सार्वजनिक:
    AstConsArray(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_DType* newDTypep, Ast_ग्रन्थिः_व्यञ्जकम्* defaultp = nullptr)
        : ASTGEN_SUPER_ConsArray(fl) {
        dtypep(newDTypep);
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsArray;
    ध्रुव वर्ण* broken() ध्रुव override;
    शून्य cloneRelink() override;
    अन्तःस्थ AstConsArrayItem* itemsp() ध्रुव VL_MT_STABLE;
    // The value at the index, or the default if not given
    Ast_ग्रन्थिः_व्यञ्जकम्* valuep(uint64_t सूचकांक) ध्रुव;
    // Add the value of the element at the index, which must not be given yet
    शून्य valuep(uint64_t सूचकांक, Ast_ग्रन्थिः_व्यञ्जकम्* newp);
    // Add the elements of the array 'arrayp', starting at the index
    शून्य splice_मानानि(uint64_t सूचकांक, Ast_ग्रन्थिः_व्यञ्जकम्* arrayp);
};
class AstConsAssoc final : सार्वजनिक Ast_ग्रन्थिः_Cons {
    // Construct an associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsAssocItem]
सार्वजनिक:
    explicit AstConsAssoc(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* defaultp = nullptr)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsAssoc;
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), AssocArrayDType));
        प्रत्यागम nullptr;
    }
};
class AstConsDynArray final : सार्वजनिक Ast_ग्रन्थिः_Cons {
    // Construct a dynamic array value, '{value, ...}, {value, array, ...},
    // or with 'sizep', new[size]('{index: value, default: value})
    // @astgen op1 := sizep : Optional[AstNodeExpr]
    // @astgen op2 := defaultp : Optional[AstNodeExpr] // Only with sizep
    // @astgen op3 := itemsp : List[AstConsDynArrayItem]
सार्वजनिक:
    explicit AstConsDynArray(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_ConsDynArray(fl) {}
    ASTGEN_MEMBERS_AstConsDynArray;
    ध्रुव वर्ण* broken() ध्रुव override;
};
class AstConsQueue final : सार्वजनिक Ast_ग्रन्थिः_Cons {
    // Construct a queue value, '{value, ...}, {value, array, ...}
    // @astgen op1 := itemsp : List[AstConsQueueItem]
सार्वजनिक:
    explicit AstConsQueue(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_ConsQueue(fl) {}
    ASTGEN_MEMBERS_AstConsQueue;
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), QueueDType));
        प्रत्यागम nullptr;
    }
};
class AstConsStruct final : सार्वजनिक Ast_ग्रन्थिः_Cons {
    // Construct an unpacked struct value, '{member: value, ...}
    // @astgen op1 := itemsp : List[AstConsStructItem] // A value for each member, in order
सार्वजनिक:
    AstConsStruct(File_पङ्क्तिः* fl, AstStructDType* dtypep)
        : ASTGEN_SUPER_ConsStruct(fl) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstConsStruct;
    ध्रुव वर्ण* broken() ध्रुव override;
    // The value of the member called 'name'
    Ast_ग्रन्थिः_व्यञ्जकम्* memberValuep(ध्रुव string& name) ध्रुव;
};
class AstConsWildcard final : सार्वजनिक Ast_ग्रन्थिः_Cons {
    // Construct a wildcard associative array value, '{key: value, default: value}
    // @astgen op1 := defaultp : Optional[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstConsWildcardItem]
सार्वजनिक:
    explicit AstConsWildcard(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* defaultp = nullptr)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_AstConsWildcard;
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!VN_IS(dtypep()->skipRefp(), WildcardArrayDType));
        प्रत्यागम nullptr;
    }
};

// === AstNodeFTaskRef ===
class AstFuncRef final : सार्वजनिक Ast_ग्रन्थिः_FTask_Ref {
    // A reference to a function
सार्वजनिक:
    अन्तःस्थ AstFuncRef(File_पङ्क्तिः* fl, AstFunc* taskp, AstArg* argsp = nullptr);
    AstFuncRef(File_पङ्क्तिः* fl, ध्रुव string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_FuncRef(fl, name, argsp) {}
    ASTGEN_MEMBERS_AstFuncRef;
};
class Ast_विधानम्_Call final : सार्वजनिक Ast_ग्रन्थिः_FTask_Ref {
    // A reference to a member task (or function)
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    // @astgen op1 := fromp : AstNodeExpr
    //
सार्वजनिक:
    Ast_विधानम्_Call(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, VFlagChildDType, ध्रुव string& name,
                  AstArg* argsp = nullptr)
        : ASTGEN_SUPER_विधानम्_Call(fl, name, argsp) {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_विधानम्_Call(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, ध्रुव string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_विधानम्_Call(fl, name, argsp) {
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_Ast_विधानम्_Call;
};
class AstNew final : सार्वजनिक Ast_ग्रन्थिः_FTask_Ref {
    // New as constructor
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    bool m_isImplicit = false;  // Implicitly generated from extends args
    bool m_isScoped = false;  // Had :: scope when parsed
सार्वजनिक:
    AstNew(File_पङ्क्तिः* fl, AstArg* argsp = nullptr, bool isScoped = false)
        : ASTGEN_SUPER_New(fl, "new", argsp)
        , m_isScoped{isScoped} {}
    ASTGEN_MEMBERS_AstNew;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool isImplicit() ध्रुव { प्रत्यागम m_isImplicit; }
    शून्य isImplicit(bool flag) { m_isImplicit = flag; }
    bool isScoped() ध्रुव { प्रत्यागम m_isScoped; }
    शून्य isScoped(bool flag) { m_isScoped = flag; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }
};
class AstTaskRef final : सार्वजनिक Ast_ग्रन्थिः_FTask_Ref {
    // A reference to a task
सार्वजनिक:
    अन्तःस्थ AstTaskRef(File_पङ्क्तिः* fl, AstTask* taskp, AstArg* argsp = nullptr);
    AstTaskRef(File_पङ्क्तिः* fl, ध्रुव string& name, AstArg* argsp = nullptr)
        : ASTGEN_SUPER_TaskRef(fl, name, argsp) {
        dtype_समुच्चयः_Void();
    }
    ASTGEN_MEMBERS_AstTaskRef;
};

// === AstNodePreSel ===
class AstSelBit final : सार्वजनिक Ast_ग्रन्थिः_Pre_Sel {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // Gets replaced during link with AstArraySel or AstSel
    // @astgen alias op2 := bitp
गुप्त:
    VAccess m_access;  // Left hand side assignment
सार्वजनिक:
    AstSelBit(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* bitp)
        : ASTGEN_SUPER_SelBit(fl, fromp, bitp, nullptr) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSelBit;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    VAccess access() ध्रुव { प्रत्यागम m_access; }
    शून्य access(ध्रुव VAccess& flag) { m_access = flag; }
};
class AstSelExtract final : सार्वजनिक Ast_ग्रन्थिः_Pre_Sel {
    // Range extraction, gets replaced with AstSel
    // @astgen alias op2 := leftp
    // @astgen alias op3 := rightp
सार्वजनिक:
    AstSelExtract(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* msbp, Ast_ग्रन्थिः_व्यञ्जकम्* lsbp)
        : ASTGEN_SUPER_SelExtract(fl, fromp, msbp, lsbp) {}
    ASTGEN_MEMBERS_AstSelExtract;
};
class AstSelMinus final : सार्वजनिक Ast_ग्रन्थिः_Pre_Sel {
    // -: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
सार्वजनिक:
    AstSelMinus(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* bitp, Ast_ग्रन्थिः_व्यञ्जकम्* widthp)
        : ASTGEN_SUPER_SelMinus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelMinus;
};
class AstSelPlus final : सार्वजनिक Ast_ग्रन्थिः_Pre_Sel {
    // +: range extraction, perhaps with non-constant selection
    // Gets replaced during link with AstSel
    // @astgen alias op2 := bitp
    // @astgen alias op3 := widthp
सार्वजनिक:
    AstSelPlus(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* bitp, Ast_ग्रन्थिः_व्यञ्जकम्* widthp)
        : ASTGEN_SUPER_SelPlus(fl, fromp, bitp, widthp) {}
    ASTGEN_MEMBERS_AstSelPlus;
};

// === AstNodeQuadop ===
class Ast_गणनम्_द्वयङ्काः final : सार्वजनिक Ast_ग्रन्थिः_Quadop {
    // Number of bits set in vector
सार्वजनिक:
    Ast_गणनम्_द्वयङ्काः(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* ctrl1p)
        : ASTGEN_SUPER_गणनम्_द्वयङ्काः(fl, exprp, ctrl1p, ctrl1p->clone_वृक्षः_विशुद्धम्(false),
                                 ctrl1p->clone_वृक्षः_विशुद्धम्(false)) {
        dtype_समुच्चयः_Int();
    }
    Ast_गणनम्_द्वयङ्काः(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* ctrl1p, Ast_ग्रन्थिः_व्यञ्जकम्* ctrl2p)
        : ASTGEN_SUPER_गणनम्_द्वयङ्काः(fl, exprp, ctrl1p, ctrl2p, ctrl2p->clone_वृक्षः_विशुद्धम्(false)) {
        dtype_समुच्चयः_Int();
    }
    Ast_गणनम्_द्वयङ्काः(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* ctrl1p, Ast_ग्रन्थिः_व्यञ्जकम्* ctrl2p,
                 Ast_ग्रन्थिः_व्यञ्जकम्* ctrl3p)
        : ASTGEN_SUPER_गणनम्_द्वयङ्काः(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {
        dtype_समुच्चयः_Int();
    }
    ASTGEN_MEMBERS_Ast_गणनम्_द्वयङ्काः;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& व्यञ्जकम्, ध्रुव V3_सङ्ख्या& ctrl1,
                       ध्रुव V3_सङ्ख्या& ctrl2, ध्रुव V3_सङ्ख्या& ctrl3) override {
        बहिर्गमनम्.op_गणनम्_द्वयङ्काः(व्यञ्जकम्, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { प्रत्यागम "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool cleanThs() ध्रुव override { प्रत्यागम true; }
    bool cleanFhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Ths() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Fhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 16; }
};

// === AstNodeTermop ===
class AstInferredDisable final : सार्वजनिक Ast_ग्रन्थिः_Termop {
सार्वजनिक:
    explicit AstInferredDisable(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_InferredDisable(fl) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstInferredDisable;
    string emitVerilog() override { प्रत्यागम "%f$inferred_disable"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstTime final : सार्वजनिक Ast_ग्रन्थिः_Termop {
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    AstTime(File_पङ्क्तिः* fl, ध्रुव VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtype_समुच्चयः_UInt64();
    }
    ASTGEN_MEMBERS_AstTime;
    string emitVerilog() override { प्रत्यागम "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_TIME; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    VTimescale timeunit() ध्रुव { प्रत्यागम m_timeunit; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }
};
class AstTimeD final : सार्वजनिक Ast_ग्रन्थिः_Termop {
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    AstTimeD(File_पङ्क्तिः* fl, ध्रुव VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_AstTimeD;
    string emitVerilog() override { प्रत्यागम "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool isGateOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_TIME; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    VTimescale timeunit() ध्रुव { प्रत्यागम m_timeunit; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }
};

// === AstNodeTriop ===

class AstCond final : सार्वजनिक Ast_ग्रन्थिः_Triop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
    // @astgen makeDfgVertex
सार्वजनिक:
    AstCond(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः_व्यञ्जकम्* thenp, Ast_ग्रन्थिः_व्यञ्जकम्* elsep);
    ASTGEN_MEMBERS_AstCond;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs,
                       ध्रुव V3_सङ्ख्या& ths) override {
        बहिर्गमनम्.op_नियोजय(lhs.isNeqZero() ? rhs : ths);
    }
    string emitVerilog() override { प्रत्यागम "%k(%l %f? %r %k: %t)"; }
    string emitC() override { प्रत्यागम "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(ite (__Vbool %l) %r %t)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }  // clean if e1 & e2 clean
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम false; }
    bool cleanThs() ध्रुव override { प्रत्यागम false; }  // Propagates up
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Ths() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_BRANCH; }
};
class AstPutcN final : सार्वजनिक Ast_ग्रन्थिः_Triop {
    // Verilog string.putc()
सार्वजनिक:
    AstPutcN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः_व्यञ्जकम्* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_AstPutcN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs,
                       ध्रुव V3_सङ्ख्या& ths) override {
        बहिर्गमनम्.opPutcN(lhs, rhs, ths);
    }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम "putc"; }
    string emitVerilog() override { प्रत्यागम "%k(%l.putc(%r,%t))"; }
    string emitC() override { प्रत्यागम "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool cleanThs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Ths() ध्रुव override { प्रत्यागम false; }
};
class Ast_खण्डिका_Sel final : सार्वजनिक Ast_ग्रन्थिः_Triop {
    // Multiple array element extraction
    // @astgen alias op1 := fromp
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
सार्वजनिक:
    Ast_खण्डिका_Sel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, ध्रुव VNumRange& declRange)
        : ASTGEN_SUPER_खण्डिका_Sel(fl, fromp,
                                new AstConst(fl, declRange.lo()),  // Need () constructor
                                new AstConst(fl, declRange.elements()))  // Need () constructor
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_Ast_खण्डिका_Sel;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& from, ध्रुव V3_सङ्ख्या& lo,
                       ध्रुव V3_सङ्ख्या& विस्तारः) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool cleanThs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Ths() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() VL_MT_STABLE { प्रत्यागम m_declRange; }
    ध्रुव VNumRange& declRange() ध्रुव VL_MT_STABLE { प्रत्यागम m_declRange; }
};
class AstSubstrN final : सार्वजनिक Ast_ग्रन्थिः_Triop {
    // Verilog string.substr()
सार्वजनिक:
    AstSubstrN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः_व्यञ्जकम्* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_AstSubstrN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs,
                       ध्रुव V3_सङ्ख्या& ths) override {
        बहिर्गमनम्.opSubstrN(lhs, rhs, ths);
    }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम "substr"; }
    string emitVerilog() override { प्रत्यागम "%k(%l.substr(%r,%t))"; }
    string emitC() override { प्रत्यागम "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { प्रत्यागम ""; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool cleanRhs() ध्रुव override { प्रत्यागम true; }
    bool cleanThs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Rhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Ths() ध्रुव override { प्रत्यागम false; }
};

// === AstNodeDistTriop ===
class AstDistErlang final : सार्वजनिक Ast_ग्रन्थिः_Dist_Triop {
सार्वजनिक:
    AstDistErlang(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः_व्यञ्जकम्* thsp)
        : ASTGEN_SUPER_DistErlang(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_AstDistErlang;
    string emitVerilog() override { प्रत्यागम "%f$dist_erlang(%l, %r, %t)"; }
    string emitC() override { प्रत्यागम "VL_DIST_ERLANG(%li, %ri, %ti)"; }
};
class Ast_Dist_प्रसामान्यम् final : सार्वजनिक Ast_ग्रन्थिः_Dist_Triop {
सार्वजनिक:
    Ast_Dist_प्रसामान्यम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः_व्यञ्जकम्* thsp)
        : ASTGEN_SUPER_Dist_प्रसामान्यम्(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_प्रसामान्यम्;
    string emitVerilog() override { प्रत्यागम "%f$dist_normal(%l, %r, %t)"; }
    string emitC() override { प्रत्यागम "VL_DIST_NORMAL(%li, %ri, %ti)"; }
};
class Ast_Dist_एकरूपम् final : सार्वजनिक Ast_ग्रन्थिः_Dist_Triop {
सार्वजनिक:
    Ast_Dist_एकरूपम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, Ast_ग्रन्थिः_व्यञ्जकम्* thsp)
        : ASTGEN_SUPER_Dist_एकरूपम्(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_Ast_Dist_एकरूपम्;
    string emitVerilog() override { प्रत्यागम "%f$dist_uniform(%l, %r, %t)"; }
    string emitC() override { प्रत्यागम "VL_DIST_UNIFORM(%li, %ri, %ti)"; }
};

// === AstNodeUniop ===
class AstAtoN final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
सार्वजनिक:
    गणना FmtType : पूर्णाङ्क { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

गुप्त:
    // dist-ast-dump-suppress  // Part of name()
    ध्रुव FmtType m_fmtType;  // Operation type
सार्वजनिक:
    AstAtoN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, FmtType fmtType)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmtType{fmtType} {
        fmtType == ATOREAL ? dtype_समुच्चयः_Double() : dtype_समुच्चयः_Integer();
    }
    ASTGEN_MEMBERS_AstAtoN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opAtoN(lhs, m_fmtType); }
    string name() ध्रुव override VL_MT_STABLE {
        विकल्प (m_fmtType) {
        पक्ष ATOI: प्रत्यागम "atoi";
        पक्ष ATOHEX: प्रत्यागम "atohex";
        पक्ष ATOOCT: प्रत्यागम "atooct";
        पक्ष ATOBIN: प्रत्यागम "atobin";
        पक्ष ATOREAL: प्रत्यागम "atoreal";
        }
        V3ERROR_NA_RETURN("");
    }
    string emitVerilog() override { प्रत्यागम "%l." + name() + "()"; }
    string emitC() override {
        विकल्प (m_fmtType) {
        पक्ष ATOI: प्रत्यागम "VL_ATOI_N(%li, 10)";
        पक्ष ATOHEX: प्रत्यागम "VL_ATOI_N(%li, 16)";
        पक्ष ATOOCT: प्रत्यागम "VL_ATOI_N(%li, 8)";
        पक्ष ATOBIN: प्रत्यागम "VL_ATOI_N(%li, 2)";
        पक्ष ATOREAL: प्रत्यागम "VL_ATOREAL_N(%li)";
        }
        V3ERROR_NA_RETURN("");
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    FmtType fmtType() ध्रुव { प्रत्यागम m_fmtType; }
};
class Ast_द्वयङ्काः_गन्तव्यम्_Real_D final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
सार्वजनिक:
    Ast_द्वयङ्काः_गन्तव्यम्_Real_D(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_द्वयङ्काः_गन्तव्यम्_Real_D(fl, lhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_Ast_द्वयङ्काः_गन्तव्यम्_Real_D;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_द्वयङ्काः_गन्तव्यम्_Real_D(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$bitstoreal(%l)"; }
    string emitC() override { प्रत्यागम "VL_CVT_D_Q(%li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }  // Eliminated before matters
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstCCast final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Cast to C-based data type
    पूर्णाङ्क m_आकार;

सार्वजनिक:
    AstCCast(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, पूर्णाङ्क setwidth, पूर्णाङ्क minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_आकार = setwidth;
        यदि (setwidth) {
            यदि (minwidth == -1) minwidth = setwidth;
            dtype_समुच्चयः_Bit_Unsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    // cppcheck-suppress constParameterPointer
    // cppcheck-suppress constParameterCallback
    AstCCast(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_आकार = विस्तारः();
    }
    ASTGEN_MEMBERS_AstCCast;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_नियोजय(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$_CAST(%l)"; }
    string emitC() override { प्रत्यागम "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }  // Special cased in V3Cast
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        प्रत्यागम आकार() == VN_DBG_AS(samep, CCast)->आकार();
    }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    //
    पूर्णाङ्क आकार() ध्रुव { प्रत्यागम m_आकार; }
};
class AstCLog2 final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
सार्वजनिक:
    AstCLog2(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtype_समुच्चयः_Integer2_State();
    }
    ASTGEN_MEMBERS_AstCLog2;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opCLog2(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$clog2(%l)"; }
    string emitC() override { प्रत्यागम "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 16; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstCastWrap final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // A cast which has been expanded and the LHSP does all the lifting
    // This remains until V3Width final commit pass to suppress ENUMVALUE warnings
सार्वजनिक:
    AstCastWrap(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_CastWrap(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCastWrap;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_नियोजय(lhs); }
    string emitVerilog() override { प्रत्यागम "(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
};
class Ast_गणनम्_Ones final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Number of bits set in vector
    // @astgen makeDfgVertex
सार्वजनिक:
    Ast_गणनम्_Ones(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_गणनम्_Ones(fl, lhsp) {
        dtype_समुच्चयः_Int();
    }
    ASTGEN_MEMBERS_Ast_गणनम्_Ones;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_गणनम्_Ones(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$countones(%l)"; }
    string emitC() override { प्रत्यागम "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 16; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstCvtPackString final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Convert to Verilator Packed String (aka verilog "string")
सार्वजनिक:
    AstCvtPackString(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_AstCvtPackString;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opCvtPackString(lhs); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { प्रत्यागम true; }
};
class AstExtend final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
सार्वजनिक:
    AstExtend(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, पूर्णाङ्क विस्तारः, पूर्णाङ्क विस्तारः_Min = 0)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtype_समुच्चयः_Bit_Or_Logic_Unsized(विस्तारः, विस्तारः_Min ? विस्तारः_Min : विस्तारः, VSigning::UNSIGNED,
                                  lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtend;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_नियोजय(lhs); }
    string emitVerilog() override { प्रत्यागम "%l"; }
    string emitC() override { प्रत्यागम "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ध्रुव override {
        प्रत्यागम "((_ zero_extend " + cvt_गन्तव्यम्_Str(विस्तारः() - lhsp()->विस्तारः()) + ") %l)";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override {
        प्रत्यागम false;  // Because the EXTEND operator self-casts
    }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
};
class AstExtendS final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
    // @astgen makeDfgVertex
सार्वजनिक:
    AstExtendS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, पूर्णाङ्क विस्तारः)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtype_समुच्चयः_Bit_Or_Logic_Sized(विस्तारः, VSigning::UNSIGNED, lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstExtendS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.opExtendS(lhs, lhsp()->विस्तारः_Min_V());
    }
    string emitVerilog() override { प्रत्यागम "%l"; }
    string emitC() override { प्रत्यागम "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ध्रुव override {
        प्रत्यागम "((_ sign_extend " + cvt_गन्तव्यम्_Str(विस्तारः() - lhsp()->विस्तारः()) + ") %l)";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override {
        प्रत्यागम false;  // Because the EXTEND operator self-casts
    }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool signedFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstFEof final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
सार्वजनिक:
    AstFEof(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 1,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFEof;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%f$feof(%l)"; }
    string emitC() override { प्रत्यागम "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 16; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
    Ast_ग्रन्थिः* filep() ध्रुव { प्रत्यागम lhsp(); }
};
class AstFGetC final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
सार्वजनिक:
    AstFGetC(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {
        dtype_समुच्चयः_Logic2_State_Unsized(
            32, v3Global.विस्तारः_Min_Usage() == VWidthMinUsage::MATCHES_विस्तारः ? 32 : 8,
            VSigning::SIGNED);  // Spec says integer return
    }
    ASTGEN_MEMBERS_AstFGetC;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { प्रत्यागम "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 64; }
    bool isPredictOptimizable() ध्रुव override { प्रत्यागम false; }
    bool is_विशुद्धम्() override { प्रत्यागम false; }  // SPECIAL: $display has 'visual' ordering
    Ast_ग्रन्थिः* filep() ध्रुव { प्रत्यागम lhsp(); }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstISToRD final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // $itor where lhs is signed
सार्वजनिक:
    AstISToRD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_AstISToRD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opISToRD(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$itor($signed(%l))"; }
    string emitC() override { प्रत्यागम "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emit_Check_Max_शब्दाः() override { प्रत्यागम true; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstIToRD final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // $itor where lhs is unsigned
सार्वजनिक:
    AstIToRD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_AstIToRD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opIToRD(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$itor(%l)"; }
    string emitC() override { प्रत्यागम "VL_ITOR_D_%lq(%lw, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstIsUnbounded final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // True if is unbounded ($)
सार्वजनिक:
    AstIsUnbounded(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstIsUnbounded;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या&) override {
        // Any constant isn't unbounded
        बहिर्गमनम्.समुच्चयः_Zero();
    }
    string emitVerilog() override { प्रत्यागम "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstIsUnknown final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // True if any unknown bits
सार्वजनिक:
    AstIsUnknown(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstIsUnknown;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opIsUnknown(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstLenN final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Length of a string
सार्वजनिक:
    AstLenN(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtype_समुच्चयः_Int();
    }
    ASTGEN_MEMBERS_AstLenN;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opLenN(lhs); }
    string emitVerilog() override { प्रत्यागम "%f(%l)"; }
    string emitC() override { प्रत्यागम "VL_LEN_IN(%li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
};
class AstLogNot final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // @astgen makeDfgVertex
गुप्त:
    ध्रुव bool m_fromProperty;  // True if from property 'not' keyword (IEEE 1800-2023 16.12.3),
                                // false for boolean '!' (IEEE 1800-2023 11.4.7)
सार्वजनिक:
    AstLogNot(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, bool fromProperty = false)
        : ASTGEN_SUPER_LogNot(fl, lhsp)
        , m_fromProperty{fromProperty} {
        dtype_समुच्चयः_Bit_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstLogNot;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opLogNot(lhs); }
    string emitVerilog() override { प्रत्यागम "%f(! %l)"; }
    string emitC() override { प्रत्यागम "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(__Vbv (not (__Vbool %l)))"; }
    string emitSimpleOperator() override { प्रत्यागम "!"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool fromProperty() ध्रुव { प्रत्यागम m_fromProperty; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class Ast_Most_समुच्चयः_Bit_P1 final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Most-significant set bit plus one (bit-width); 0 if value is zero
सार्वजनिक:
    Ast_Most_समुच्चयः_Bit_P1(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Most_समुच्चयः_Bit_P1(fl, lhsp) {
        dtype_समुच्चयः_Integer2_State();
    }
    ASTGEN_MEMBERS_Ast_Most_समुच्चयः_Bit_P1;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_Most_समुच्चयः_Bit_P1(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$mostsetbitp1(%l)"; }
    string emitC() override { प्रत्यागम "VL_MOSTSETBITP1_%lq(%lW, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 16; }
};
class AstNToI final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // String to any-size integral
सार्वजनिक:
    AstNToI(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_DType* dtypep = nullptr)
        : ASTGEN_SUPER_NToI(fl, lhsp) {
        this->dtypep(dtypep);
    }
    ASTGEN_MEMBERS_AstNToI;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opNToI(lhs); }
    string emitVerilog() override { प्रत्यागम "'(%l)"; }
    string emitC() override { प्रत्यागम "VL_NTOI_%nq(%nw, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
};
class AstNegate final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNegate(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNegate;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opNegate(lhs); }
    string emitVerilog() override { प्रत्यागम "%f(- %l)"; }
    string emitC() override { प्रत्यागम "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvneg %l)"; }
    string emitSimpleOperator() override { प्रत्यागम "-"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
};
class AstNegateD final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
सार्वजनिक:
    AstNegateD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtype_समुच्चयः_Double();
    }
    ASTGEN_MEMBERS_AstNegateD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opNegateD(lhs); }
    string emitVerilog() override { प्रत्यागम "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { प्रत्यागम "-"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool doubleFlavor() ध्रुव override { प्रत्यागम true; }
};
class AstNot final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstNot(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNot;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opNot(lhs); }
    string emitVerilog() override { प्रत्यागम "%f(~ %l)"; }
    string emitC() override { प्रत्यागम "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSMT() ध्रुव override { प्रत्यागम "(bvnot %l)"; }
    string emitSimpleOperator() override { प्रत्यागम "~"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }
    ध्रुव वर्ण* विस्तारः_Mismatch() ध्रुव override VL_MT_STABLE;
};
class AstNullCheck final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Return LHS after checking that LHS is non-null
सार्वजनिक:
    AstNullCheck(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstNullCheck;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 1; }  // Rarely executes
    string emitVerilog() override { प्रत्यागम "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override { प्रत्यागम fileline() == samep->fileline(); }
};
class AstOneHot final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // True if only single bit set in vector
    // @astgen makeDfgVertex
सार्वजनिक:
    AstOneHot(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstOneHot;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opOneHot(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$onehot(%l)"; }
    string emitC() override { प्रत्यागम "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 4; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstOneHot0 final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // True if only single bit, or no bits set in vector
    // @astgen makeDfgVertex
सार्वजनिक:
    AstOneHot0(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtype_समुच्चयः_Bit();
    }
    ASTGEN_MEMBERS_AstOneHot0;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opOneHot0(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$onehot0(%l)"; }
    string emitC() override { प्रत्यागम "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs() * 3; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstPostDec final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Post-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
सार्वजनिक:
    AstPostDec(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_PostDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostDec;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { प्रत्यागम "%k(%l--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool आकार_Matters_Lhs() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstPostInc final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Post-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
सार्वजनिक:
    AstPostInc(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_PostInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPostInc;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { प्रत्यागम "%k(%l++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool आकार_Matters_Lhs() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstPreDec final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Pre-decrement/subtract
    // Children: lhsp: tree with AstVarRef to be decremented
सार्वजनिक:
    AstPreDec(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_PreDec(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreDec;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { प्रत्यागम "%k(--%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool आकार_Matters_Lhs() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstPreInc final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Pre-increment/add
    // Children: lhsp: tree with AstVarRef to be incremented
सार्वजनिक:
    AstPreInc(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_PreInc(fl, lhsp) {}
    ASTGEN_MEMBERS_AstPreInc;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { प्रत्यागम "%k(++%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() ध्रुव override { V3ERROR_NA_RETURN(false); }
    bool आकार_Matters_Lhs() ध्रुव override { V3ERROR_NA_RETURN(true); }
};
class AstRToIRoundS final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
सार्वजनिक:
    AstRToIRoundS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtype_समुच्चयः_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIRoundS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opRToIRoundS(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        प्रत्यागम isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstRToIS final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // $rtoi(lhs)
सार्वजनिक:
    AstRToIS(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtype_समुच्चयः_Integer2_State();
    }
    ASTGEN_MEMBERS_AstRToIS;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opRToIS(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$rtoi(%l)"; }
    string emitC() override { प्रत्यागम "VL_RTOI_I_D(%li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }  // Eliminated before matters
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class Ast_Real_गन्तव्यम्_द्वयङ्काः final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
सार्वजनिक:
    Ast_Real_गन्तव्यम्_द्वयङ्काः(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Real_गन्तव्यम्_द्वयङ्काः(fl, lhsp) {
        dtype_समुच्चयः_UInt64();
    }
    ASTGEN_MEMBERS_Ast_Real_गन्तव्यम्_द्वयङ्काः;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_Real_गन्तव्यम्_द्वयङ्काः(lhs); }
    string emitVerilog() override { प्रत्यागम "%f$realtobits(%l)"; }
    string emitC() override { प्रत्यागम "VL_CVT_Q_D(%li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }  // Eliminated before matters
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम INSTR_गणनम्_DBL; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstRedAnd final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstRedAnd(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedAnd;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opRedAnd(lhs); }
    string emitVerilog() override { प्रत्यागम "%f(& %l)"; }
    string emitC() override { प्रत्यागम "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
};
class AstRedOr final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstRedOr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedOr;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opRedOr(lhs); }
    string emitVerilog() override { प्रत्यागम "%f(| %l)"; }
    string emitC() override { प्रत्यागम "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
};
class AstRedXor final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // @astgen makeDfgVertex
सार्वजनिक:
    AstRedXor(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtype_समुच्चयः_Bit_Or_Logic(lhsp->isFourstateOrNoDtype());
    }
    ASTGEN_MEMBERS_AstRedXor;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.opRedXor(lhs); }
    string emitVerilog() override { प्रत्यागम "%f(^ %l)"; }
    string emitC() override { प्रत्यागम "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override {
        ध्रुव पूर्णाङ्क w = lhsp()->विस्तारः();
        प्रत्यागम (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 1 + V3_सङ्ख्या::log2b(विस्तारः()); }
};
class AstResizeLValue final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Resize a LValue into a wider/narrower entity at function argument boundry
    // Width and signness is implied from nodep->width()
सार्वजनिक:
    AstResizeLValue(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_ResizeLValue(fl, lhsp) {}
    ASTGEN_MEMBERS_AstResizeLValue;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%l"; }
    string emitC() final override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
};
class AstSigned final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // $signed(lhs)
सार्वजनिक:
    AstSigned(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstSigned;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.op_नियोजय(lhs);
        बहिर्गमनम्.isSigned(false);
    }
    string emitVerilog() override { प्रत्यागम "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }  // Eliminated before matters
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};
class AstTimeImport final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
सार्वजनिक:
    AstTimeImport(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTimeImport;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    VTimescale timeunit() ध्रुव { प्रत्यागम m_timeunit; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }
};
class Ast_गन्तव्यम्_अधस्तनम्_N final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // string.tolower()
सार्वजनिक:
    Ast_गन्तव्यम्_अधस्तनम्_N(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_गन्तव्यम्_अधस्तनम्_N(fl, lhsp) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_Ast_गन्तव्यम्_अधस्तनम्_N;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_गन्तव्यम्_अधस्तनम्_N(lhs); }
    string emitVerilog() override { प्रत्यागम "%l.tolower()"; }
    string emitC() override { प्रत्यागम "VL_TOLOWER_NN(%li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
};
class Ast_गन्तव्यम्_String_N final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
सार्वजनिक:
    Ast_गन्तव्यम्_String_N(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_गन्तव्यम्_String_N(fl, lhsp) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_Ast_गन्तव्यम्_String_N;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { प्रत्यागम "$sformatf(\"%p\", %l)"; }
    string emitC() override {
        प्रत्यागम isWide() ? "VL_TO_STRING_W(%nw, %li)" : "VL_TO_STRING_DEREF(%li)";
    }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
};
class Ast_गन्तव्यम्_ऊर्ध्वस्थम्_N final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // string.toupper()
सार्वजनिक:
    Ast_गन्तव्यम्_ऊर्ध्वस्थम्_N(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_गन्तव्यम्_ऊर्ध्वस्थम्_N(fl, lhsp) {
        dtype_समुच्चयः_String();
    }
    ASTGEN_MEMBERS_Ast_गन्तव्यम्_ऊर्ध्वस्थम्_N;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override { बहिर्गमनम्.op_गन्तव्यम्_ऊर्ध्वस्थम्_N(lhs); }
    string emitVerilog() override { प्रत्यागम "%l.toupper()"; }
    string emitC() override { प्रत्यागम "VL_TOUPPER_NN(%li)"; }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    bool cleanLhs() ध्रुव override { प्रत्यागम true; }
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम false; }
};
class AstUnsigned final : सार्वजनिक Ast_ग्रन्थिः_Uniop {
    // $unsigned(lhs)
सार्वजनिक:
    AstUnsigned(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_AstUnsigned;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.op_नियोजय(lhs);
        बहिर्गमनम्.isSigned(false);
    }
    string emitVerilog() override { प्रत्यागम "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम false; }
    bool cleanLhs() ध्रुव override { प्रत्यागम false; }  // Eliminated before matters
    bool आकार_Matters_Lhs() ध्रुव override { प्रत्यागम true; }  // Eliminated before matters
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम 0; }
    bool isSystemFunc() ध्रुव override { प्रत्यागम true; }
};

// === AstNodeSystemUniopD ===
class AstAcosD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstAcosD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcosD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::acos(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$acos(%l)"; }
    string emitC() override { प्रत्यागम "acos(%li)"; }
};
class AstAcoshD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstAcoshD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAcoshD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::acosh(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$acosh(%l)"; }
    string emitC() override { प्रत्यागम "acosh(%li)"; }
};
class AstAsinD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstAsinD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::asin(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$asin(%l)"; }
    string emitC() override { प्रत्यागम "asin(%li)"; }
};
class AstAsinhD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstAsinhD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAsinhD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::asinh(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$asinh(%l)"; }
    string emitC() override { प्रत्यागम "asinh(%li)"; }
};
class AstAtanD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstAtanD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::atan(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$atan(%l)"; }
    string emitC() override { प्रत्यागम "atan(%li)"; }
};
class AstAtanhD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstAtanhD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstAtanhD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::atanh(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$atanh(%l)"; }
    string emitC() override { प्रत्यागम "atanh(%li)"; }
};
class AstCeilD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstCeilD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCeilD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::ceil(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$ceil(%l)"; }
    string emitC() override { प्रत्यागम "ceil(%li)"; }
};
class AstCosD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstCosD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCosD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::कोटिज्या(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$cos(%l)"; }
    string emitC() override { प्रत्यागम "cos(%li)"; }
};
class AstCoshD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstCoshD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstCoshD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::cosh(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$cosh(%l)"; }
    string emitC() override { प्रत्यागम "cosh(%li)"; }
};
class AstExpD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstExpD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstExpD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::घातांकः(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$exp(%l)"; }
    string emitC() override { प्रत्यागम "exp(%li)"; }
};
class AstFloorD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstFloorD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstFloorD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::floor(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$floor(%l)"; }
    string emitC() override { प्रत्यागम "floor(%li)"; }
};
class AstLog10D final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstLog10D(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLog10D;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::log10(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$log10(%l)"; }
    string emitC() override { प्रत्यागम "log10(%li)"; }
};
class AstLogD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstLogD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstLogD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::लघुगणकः(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$ln(%l)"; }
    string emitC() override { प्रत्यागम "log(%li)"; }
};
class AstSinD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstSinD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::ज्या(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$sin(%l)"; }
    string emitC() override { प्रत्यागम "sin(%li)"; }
};
class AstSinhD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstSinhD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSinhD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::sinh(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$sinh(%l)"; }
    string emitC() override { प्रत्यागम "sinh(%li)"; }
};
class AstSqrtD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstSqrtD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstSqrtD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::वर्गमूलम्(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$sqrt(%l)"; }
    string emitC() override { प्रत्यागम "sqrt(%li)"; }
};
class AstTanD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstTanD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::tan(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$tan(%l)"; }
    string emitC() override { प्रत्यागम "tan(%li)"; }
};
class AstTanhD final : सार्वजनिक Ast_ग्रन्थिः_System_Uniop_D {
सार्वजनिक:
    AstTanhD(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AstTanhD;
    शून्य सङ्ख्या_सञ्चालय(V3_सङ्ख्या& बहिर्गमनम्, ध्रुव V3_सङ्ख्या& lhs) override {
        बहिर्गमनम्.समुच्चयः_Double(std::tanh(lhs.गन्तव्यम्_Double()));
    }
    string emitVerilog() override { प्रत्यागम "%f$tanh(%l)"; }
    string emitC() override { प्रत्यागम "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : सार्वजनिक Ast_ग्रन्थिः_Var_Ref {
    // A reference to a variable (lvalue or rvalue)
    std::string m_name;  // Name of varref, though name() favors varp()->namep()
                         // if non-null pointer (e.g.: 'this')
सार्वजनिक:
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    अन्तःस्थ AstVarRef(File_पङ्क्तिः* fl, AstVar* varp, ध्रुव VAccess& access);
    अन्तःस्थ AstVarRef(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_Module* classOrPackagep, AstVar* varp,
                     ध्रुव VAccess& access);
    // This form only allowed post-link (see above)
    अन्तःस्थ AstVarRef(File_पङ्क्तिः* fl, AstVarScope* varscp, ध्रुव VAccess& access);
    ASTGEN_MEMBERS_AstVarRef;
    std::string nameThis() ध्रुव { प्रत्यागम m_name; }
    अन्तःस्थ string name() ध्रुव override;  // * = Var name
    शून्य name(ध्रुव string& name) override { m_name = name; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    ध्रुव वर्ण* broken() ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override;
    अन्तःस्थ bool same_ग्रन्थिः(ध्रुव AstVarRef* samep) ध्रुव;
    अन्तःस्थ bool sameNoLvalue(ध्रुव AstVarRef* samep) ध्रुव;
    पूर्णाङ्क instr_गणनम्() ध्रुव override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
};
class AstVarXRef final : सार्वजनिक Ast_ग्रन्थिः_Var_Ref {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
    string m_name;
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
    bool m_अन्तर्भवति_Gen_खण्डम् = false;  // Contains gen block reference
    bool m_readOnlyModport = false;  // Linked via an input-only modport, until V3LinkLValue
सार्वजनिक:
    AstVarXRef(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& dotted, ध्रुव VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, nullptr, access)
        , m_name{name}
        , m_dotted{dotted} {}
    अन्तःस्थ AstVarXRef(File_पङ्क्तिः* fl, AstVar* varp, ध्रुव string& dotted, ध्रुव VAccess& access);
    ASTGEN_MEMBERS_AstVarXRef;
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_name; }  // * = Var name
    शून्य name(ध्रुव std::string& name) override { m_name = name; }  // * = Var name
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string dotted() ध्रुव { प्रत्यागम m_dotted; }
    शून्य dotted(ध्रुव string& dotted) { m_dotted = dotted; }
    string inlinedDots() ध्रुव { प्रत्यागम m_inlinedDots; }
    शून्य inlinedDots(ध्रुव string& flag) { m_inlinedDots = flag; }
    bool अन्तर्भवति_Gen_खण्डम्() ध्रुव { प्रत्यागम m_अन्तर्भवति_Gen_खण्डम्; }
    शून्य अन्तर्भवति_Gen_खण्डम्(ध्रुव bool flag) { m_अन्तर्भवति_Gen_खण्डम् = flag; }
    bool readOnlyModport() ध्रुव { प्रत्यागम m_readOnlyModport; }
    शून्य readOnlyModport(ध्रुव bool flag) { m_readOnlyModport = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool clean_बहिर्गमनम्() ध्रुव override { प्रत्यागम true; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { प्रत्यागम विस्तारः_Instrs(); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstVarXRef* asamep = VN_DBG_AS(samep, VarXRef);
        प्रत्यागम (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#यदि_अन्तः  // Guard
