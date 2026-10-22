// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode sub-types representing statements
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
// This files contains all 'AstNode' sub-types that are statements.
//
//*************************************************************************

#ifndef VERILATOR_V3ASTNODESTMT_H_
#define VERILATOR_V3ASTNODESTMT_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#অন্তর্ভাব "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_গ্রন্থি_Stmt VL_NOT_FINAL : সার্বজনিক Ast_গ্রন্থি {
    // Procedural statement
রক্ষিত:
    Ast_গ্রন্থি_Stmt(VNType t, File_সারি* fl)
        : Ast_গ্রন্থি{t, fl} {}

সার্বজনিক:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Stmt;
    // METHODS
    শূন্য dump(std::ostream& str = std::cout) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুব override;
};
class Ast_গ্রন্থি_Assign VL_NOT_FINAL : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
রক্ষিত:
    Ast_গ্রন্থি_Assign(VNType t, File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp,
                  Ast_গ্রন্থি* timingControlp = nullptr)
        : Ast_গ্রন্থি_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

সার্বজনিক:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Assign;
    // Clone single node, just get same type back.
    শূন্য dump(std::ostream& str) ধ্রুব override;
    আভাস Ast_গ্রন্থি_Assign* cloneType(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp) = 0;
    bool hasDType() ধ্রুব override VL_MT_SAFE { প্রত্যাগম true; }
    আভাস bool cleanRhs() ধ্রুব { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম widthInstrs(); }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি*) ধ্রুব override { প্রত্যাগম true; }
    string verilogKwd() ধ্রুব override { প্রত্যাগম "="; }
    bool isTimingControl() ধ্রুব override { প্রত্যাগম timingControlp(); }
};
class Ast_গ্রন্থি_Block VL_NOT_FINAL : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
রক্ষিত:
    Ast_গ্রন্থি_Block(VNType t, File_সারি* fl, ধ্রুব string& name)
        : Ast_গ্রন্থি_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

সার্বজনিক:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Block;
    bool maybePointedTo() ধ্রুব override VL_MT_SAFE { প্রত্যাগম true; }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }  // * = Block name
    শূন্য name(ধ্রুব string& name) override { m_name = name; }
    bool unnamed() ধ্রুব { প্রত্যাগম m_unnamed; }
};
class Ast_গ্রন্থি_Cover_Or_Assert VL_NOT_FINAL : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    ধ্রুব VAssertType m_userType;  // Assertion/cover type for user enable/disable
    ধ্রুব VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_immediate = false;  // Immediate assert (may differ from userType being immediate)

সার্বজনিক:
    Ast_গ্রন্থি_Cover_Or_Assert(VNType t, File_সারি* fl, Ast_গ্রন্থি* propp, Ast_গ্রন্থি* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         ধ্রুব string& name = "")
        : Ast_গ্রন্থি_Stmt{t, fl}
        , m_name{name}
        , m_userType{userType}
        , m_directive{directive} {
        this->propp(propp);
        addPasssp(passsp);
        m_immediate = m_userType.containsAny(VAssertType::SIMPLE_IMMEDIATE
                                             | VAssertType::OBSERVED_DEFERRED_IMMEDIATE
                                             | VAssertType::FINAL_DEFERRED_IMMEDIATE)
                      || m_userType == VAssertType::INTERNAL;
    }
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Cover_Or_Assert;
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }  // * = Var name
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override { প্রত্যাগম samep->name() == name(); }
    শূন্য name(ধ্রুব string& name) override { m_name = name; }
    শূন্য dump(std::ostream& str = std::cout) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুব override;
    string type() { প্রত্যাগম ""; }
    VAssertType userType() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_userType; }
    VAssertDirectiveType directive() ধ্রুব { প্রত্যাগম m_directive; }
    bool immediate() ধ্রুব { প্রত্যাগম m_immediate; }
    শূন্য immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_senFromAlways; }
    শূন্য senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_গ্রন্থি_Foreach VL_NOT_FINAL : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
সার্বজনিক:
    Ast_গ্রন্থি_Foreach(VNType t, File_সারি* fl, AstForeachHeader* headerp, Ast_গ্রন্থি* bodyp)
        : Ast_গ্রন্থি_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Foreach;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class Ast_গ্রন্থি_If VL_NOT_FINAL : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
রক্ষিত:
    Ast_গ্রন্থি_If(VNType t, File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি* thensp, Ast_গ্রন্থি* elsesp)
        : Ast_গ্রন্থি_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

সার্বজনিক:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_If;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isGateDedupable() ধ্রুব override { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
    শূন্য branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() ধ্রুব { প্রত্যাগম m_branchPred; }
    শূন্য isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() ধ্রুব { প্রত্যাগম m_isBoundsCheck; }
};
class Ast_গ্রন্থি_Read_Write_Mem VL_NOT_FINAL : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    ধ্রুব bool m_isHex;  // readmemh, not readmemb
সার্বজনিক:
    Ast_গ্রন্থি_Read_Write_Mem(VNType t, File_সারি* fl, bool hex, Ast_গ্রন্থি_রাশিমালা* filenamep,
                        Ast_গ্রন্থি_রাশিমালা* memp, Ast_গ্রন্থি_রাশিমালা* lsbp, Ast_গ্রন্থি_রাশিমালা* msbp)
        : Ast_গ্রন্থি_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Read_Write_Mem;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override {
        প্রত্যাগম isHex() == VN_DBG_AS(samep, গ্রন্থি_Read_Write_Mem)->isHex();
    }
    bool isHex() ধ্রুব { প্রত্যাগম m_isHex; }
    আভাস ধ্রুব বর্ণ* cFuncPrefixp() ধ্রুব = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : সার্বজনিক Ast_গ্রন্থি {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
সার্বজনিক:
    AstCaseItem(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* condsp, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম widthInstrs() + INSTR_গুনতি_BRANCH; }
    bool isDefault() ধ্রুব { প্রত্যাগম condsp() == nullptr; }
};

class AstForeachHeader final : সার্বজনিক Ast_গ্রন্থি {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
সার্বজনিক:
    AstForeachHeader(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* fromp, Ast_গ্রন্থি* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
    bool maybePointedTo() ধ্রুব override VL_MT_SAFE { প্রত্যাগম false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

সার্বজনিক:
    AstAssertCtl(File_সারি* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_গ্রন্থি_রাশিমালা* levelp = nullptr, Ast_গ্রন্থি_রাশিমালা* itemsp = nullptr);
    AstAssertCtl(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* controlTypep, Ast_গ্রন্থি_রাশিমালা* assertTypesp = nullptr,
                 Ast_গ্রন্থি_রাশিমালা* directiveTypep = nullptr, Ast_গ্রন্থি_রাশিমালা* levelp = nullptr,
                 Ast_গ্রন্থি_রাশিমালা* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() ধ্রুব override { প্রত্যাগম m_ctlType.ascii(); }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    VAssertCtlType ctlType() ধ্রুব { প্রত্যাগম m_ctlType; }
    শূন্য ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() ধ্রুব { প্রত্যাগম m_assertTypes; }
    শূন্য assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() ধ্রুব { প্রত্যাগম m_directiveTypes; }
    শূন্য directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    শূন্য dump(std::ostream& str = std::cout) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুব override;
};
class AstBreak final : সার্বজনিক Ast_গ্রন্থি_Stmt {
সার্বজনিক:
    explicit AstBreak(File_সারি* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "break"; }
    bool isBrancher() ধ্রুব override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
সার্বজনিক:
    AstCAwait(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp, Ast_Sen_বৃক্ষ* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isTimingControl() ধ্রুব override { প্রত্যাগম true; }
    Ast_Sen_বৃক্ষ* sentreep() ধ্রুব { প্রত্যাগম m_sentreep; }
    শূন্য clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
সার্বজনিক:
    AstCLocalScope(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
সার্বজনিক:
    AstCReturn(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম widthInstrs(); }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class AstCStmt final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    ধ্রুব VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    স্থির AstCStmt* profExecSection(File_সারি* flp, ধ্রুব std::string& section, bool ঠেলো) {
        // Compute the label
        std::string label;
        যদি (v3Global.opt.hierChild()) {
            label += v3Global.opt.শীর্ষ_Module() + ":";
        } অন্যথা যদি (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += ঠেলো ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        যদি (ঠেলো) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += ঠেলো ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        প্রত্যাগম new AstCStmt{flp, ঠেলো ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

সার্বজনিক:
    explicit AstCStmt(File_সারি* fl, ধ্রুব std::string& text = "",
                      ধ্রুব VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        যদি (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি*) ধ্রুব override { প্রত্যাগম true; }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    VCStmtType stmtType() ধ্রুব { প্রত্যাগম m_stmtType; }
    // Add some text, or a node to this statement
    শূন্য add(ধ্রুব std::string& text) { addNodesp(new AstText{fileline(), text}); }
    শূন্য add(Ast_গ্রন্থি* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    স্থির AstCStmt* prof_Exec_Section_ঠেলো(File_সারি* flp, ধ্রুব std::string& section) {
        প্রত্যাগম profExecSection(flp, section, true);
    }
    স্থির AstCStmt* prof_Exec_Section_বের_করো(File_সারি* flp, ধ্রুব std::string& section) {
        প্রত্যাগম profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    ধ্রুব bool m_fromDollarC;  // Is from source '$c', emit decoration
সার্বজনিক:
    explicit AstCStmtUser(File_সারি* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি*) ধ্রুব override { প্রত্যাগম true; }
    bool fromDollarC() ধ্রুব { প্রত্যাগম m_fromDollarC; }
    // Add some text, or a node to this statement
    শূন্য add(ধ্রুব std::string& text) { addNodesp(new AstText{fileline(), text}); }
    শূন্য add(Ast_গ্রন্থি* nodep) { addNodesp(nodep); }
};
class AstCase final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Case statement
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstCaseItem]
    // @astgen op3 := notParallelp : List[AstNode] // assertion code for non-full case's
    VCaseType m_caseType;  // 0=case, 1=casex, 2=casez
    bool m_fullPragma = false;  // Synthesis full_case
    bool m_parallelPragma = false;  // Synthesis parallel_case
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
সার্বজনিক:
    AstCase(File_সারি* fl, VCaseType caseType, Ast_গ্রন্থি_রাশিমালা* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    string verilogKwd() ধ্রুব override { প্রত্যাগম casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override {
        প্রত্যাগম m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() ধ্রুব { প্রত্যাগম m_caseType == VCaseType::CT_CASEX; }
    bool casez() ধ্রুব { প্রত্যাগম m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() ধ্রুব { প্রত্যাগম m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() ধ্রুব { প্রত্যাগম m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() ধ্রুব { প্রত্যাগম m_caseType == VCaseType::CT_CASE; }
    শূন্য caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    শূন্য caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() ধ্রুব { প্রত্যাগম m_fullPragma; }
    শূন্য fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() ধ্রুব { প্রত্যাগম m_parallelPragma; }
    শূন্য parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() ধ্রুব { প্রত্যাগম m_uniquePragma; }
    শূন্য uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ধ্রুব { প্রত্যাগম m_unique0Pragma; }
    শূন্য unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ধ্রুব { প্রত্যাগম m_priorityPragma; }
    শূন্য priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() ধ্রুব;
};
class AstComment final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Some comment to put into the output stream
    ধ্রুব string m_name;  // Text of comment
    ধ্রুব bool m_showAt;  // Show "at <fileline>"
সার্বজনিক:
    AstComment(File_সারি* fl, ধ্রুব string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }  // * = Text
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override { প্রত্যাগম true; }  // Ignore name in comments
    আভাস bool showAt() ধ্রুব { প্রত্যাগম m_showAt; }
};
class AstConstraintBefore final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
সার্বজনিক:
    AstConstraintBefore(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhssp, Ast_গ্রন্থি_রাশিমালা* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class Ast_Constraint_রাশিমালা final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
সার্বজনিক:
    Ast_Constraint_রাশিমালা(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp)
        : ASTGEN_SUPER_Constraint_রাশিমালা(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_রাশিমালা;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
    bool isDisableSoft() ধ্রুব { প্রত্যাগম m_isDisableSoft; }
    শূন্য isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() ধ্রুব { প্রত্যাগম m_isSoft; }
    শূন্য isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
সার্বজনিক:
    AstConstraintUnique(File_সারি* fl, Ast_গ্রন্থি* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class AstContinue final : সার্বজনিক Ast_গ্রন্থি_Stmt {
সার্বজনিক:
    explicit AstContinue(File_সারি* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "continue"; }
    bool isBrancher() ধ্রুব override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

সার্বজনিক:
    AstCoverInc(File_সারি* fl, Ast_গ্রন্থি_Cover_Decl* declp, ধ্রুব bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 1 + 2 * INSTR_গুনতি_LD; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override {
        ধ্রুব AstCoverInc* ধ্রুব otherp = VN_DBG_AS(samep, CoverInc);
        প্রত্যাগম declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isPure() override { প্রত্যাগম false; }
    Ast_গ্রন্থি_Cover_Decl* declp() ধ্রুব { প্রত্যাগম m_declp; }  // Where defined
    শূন্য declp(Ast_গ্রন্থি_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() ধ্রুব { প্রত্যাগম m_isDuplicate; }
};
class AstDeassign final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
সার্বজনিক:
    AstDeassign(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    ধ্রুব bool m_isCycleDelay;  // True if it is a cycle delay
সার্বজনিক:
    AstDelay(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isTimingControl() ধ্রুব override { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
    শূন্য timeunit(ধ্রুব VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ধ্রুব { প্রত্যাগম m_timeunit; }
    bool isCycleDelay() ধ্রুব { প্রত্যাগম m_isCycleDelay; }
    bool isRangeDelay() ধ্রুব { প্রত্যাগম m_isCycleDelay && rhsp(); }
    bool isUnbounded() ধ্রুব { প্রত্যাগম isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    শূন্য fallDelay(Ast_গ্রন্থি_রাশিমালা* ধ্রুব fallDelayp) { rhsp(fallDelayp); }
    Ast_গ্রন্থি_রাশিমালা* fallDelay() ধ্রুব { প্রত্যাগম m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
সার্বজনিক:
    AstDisable(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    ধ্রুব বর্ণ* broken() ধ্রুব override;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য targetp(Ast_গ্রন্থি* nodep) { m_targetp = nodep; }
    Ast_গ্রন্থি* targetp() ধ্রুব { প্রত্যাগম m_targetp; }
    bool isBrancher() ধ্রুব override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // A "disable fork" statement
সার্বজনিক:
    explicit AstDisableFork(File_সারি* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "disable fork"; }
};
class AstDisplay final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

সার্বজনিক:
    AstDisplay(File_সারি* fl, VDisplayType dispType, ধ্রুব string& text, Ast_গ্রন্থি_রাশিমালা* filep,
               Ast_গ্রন্থি_রাশিমালা* exprsp, বর্ণ missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_সারি* fl, VDisplayType dispType, Ast_গ্রন্থি_রাশিমালা* filep, Ast_গ্রন্থি_রাশিমালা* exprsp,
               বর্ণ missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* ধ্রুব newp
            = new AstSFormatF{fl, AstSFormatF::রাশিমালা_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    ধ্রুব বর্ণ* broken() ধ্রুব override {
        BROKEN_RTN(!fmtp());
        প্রত্যাগম nullptr;
    }
    string verilogKwd() ধ্রুব override {
        প্রত্যাগম (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { প্রত্যাগম true; }  // SPECIAL: $display makes output
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override {
        প্রত্যাগম displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_PLI; }
    VDisplayType displayType() ধ্রুব { প্রত্যাগম m_displayType; }
    শূন্য displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() ধ্রুব { প্রত্যাগম displayType().addNewline(); }
};
class AstDumpCtl final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    ধ্রুব VDumpCtlType m_ctlType;  // Type of operation
সার্বজনিক:
    AstDumpCtl(File_সারি* fl, VDumpCtlType ctlType, Ast_গ্রন্থি_রাশিমালা* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() ধ্রুব override { প্রত্যাগম ctlType().ascii(); }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    আভাস bool cleanOut() ধ্রুব { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    VDumpCtlType ctlType() ধ্রুব { প্রত্যাগম m_ctlType; }
};
class AstEventControl final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
সার্বজনিক:
    AstEventControl(File_সারি* fl, Ast_Sen_বৃক্ষ* sentreep, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "@(%l) %r"; }
    bool isTimingControl() ধ্রুব override { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 0; }
};
class AstExecGraph final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // For parallel execution, this node contains a dependency graph. Each
    // vertex in the graph is an ExecMTask, which includes a function that
    // holds the sequential body of the mtask.
    //
    // The location where AstExecGraph appears as a procedural statement is
    // where the parallel graph will be executed. Execution proceeds after
    // the AstExecGraph when all threads have joined.
    //
    // The calls to the MTask functions in 'callsp' are in a topological
    // order, so they represent a valid sequential execution of the graph for
    // code analysis purposes. The emitted code passes a description of the
    // graph to the run-time library, which executes the graph.
    // @astgen op1 := callsp : List[AstCCall]  // Calls to MTask bodies
    // Trigger masks zipped with callsp, populated by V3ExecGraph::implement
    // @astgen op2 := maskps : List[AstVarRef]
    // @astgen op3 := triggerp : AstVarRef  // Trigger vector this graph is sensitive to
    // Static cost of the logic each trigger guards, populated by V3ExecGraph::implement
    // @astgen op4 := triggerCostsp : Optional[AstVarRef]
    // @astgen ptr := m_runfuncp : Optional[AstCFunc]  // Statically scheduled dispatch function
    V3Graph* ধ্রুব m_depGraphp;  // contains ExecMTask vertices
    ধ্রুব string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

সার্বজনিক:
    AstExecGraph(File_সারি* fl, ধ্রুব string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    শূন্য cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    ধ্রুব বর্ণ* broken() ধ্রুব override;
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }
    V3Graph* depGraphp() { প্রত্যাগম m_depGraphp; }
    ধ্রুব V3Graph* depGraphp() ধ্রুব { প্রত্যাগম m_depGraphp; }
    AstCFunc* runfuncp() ধ্রুব { প্রত্যাগম m_runfuncp; }
    শূন্য runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
সার্বজনিক:
    AstFClose(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "$fclose"; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class AstFFlush final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
সার্বজনিক:
    AstFFlush(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "$fflush"; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class AstFinish final : সার্বজনিক Ast_গ্রন্থি_Stmt {
সার্বজনিক:
    explicit AstFinish(File_সারি* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { প্রত্যাগম true; }  // SPECIAL: $display makes output
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 0; }  // Rarely executes
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override { প্রত্যাগম fileline() == samep->fileline(); }
};
class AstFinishFork final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // $finish in fork
সার্বজনিক:
    explicit AstFinishFork(File_সারি* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { প্রত্যাগম true; }  // SPECIAL: $display makes output
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 0; }  // Rarely executes
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override { প্রত্যাগম fileline() == samep->fileline(); }
};
class AstFireEvent final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    ধ্রুব bool m_delayed;  // Delayed (->>) vs non-delayed (->)
সার্বজনিক:
    AstFireEvent(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isDelayed() ধ্রুব { প্রত্যাগম m_delayed; }
};
class AstInitialAutomaticStmt final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
সার্বজনিক:
    AstInitialAutomaticStmt(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 0; }
    bool isPure() override { প্রত্যাগম true; }
};
class AstInitialStaticStmt final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
সার্বজনিক:
    AstInitialStaticStmt(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 0; }
    bool isPure() override { প্রত্যাগম true; }
};
class AstJumpBlock final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
সার্বজনিক:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 0; }
    bool maybePointedTo() ধ্রুব override VL_MT_SAFE { প্রত্যাগম true; }
    bool isPure() override;

গুপ্ত:
    bool getPurityRecurse() ধ্রুব;
};
class AstJumpGo final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
সার্বজনিক:
    AstJumpGo(File_সারি* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    ধ্রুব বর্ণ* broken() ধ্রুব override;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override {
        প্রত্যাগম blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isBrancher() ধ্রুব override {
        প্রত্যাগম true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() ধ্রুব { প্রত্যাগম m_blockp; }
};
class AstLoop final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // An inifinite loop, used to model all source level procedural loops.
    // Executes as:
    //  while (true) {
    //      stmtsp;
    //      // <- 'continue' inside 'stmtsp goes here
    //      contsp;
    //  }
    // 'contsp' is moved into 'stmtsp' in LinkJump when 'continue' statements are resovled.
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := contsp : List[AstNode] // Empty after LinkJump
    VOptionBool m_unroll;  // Full, none, or default unrolling
সার্বজনিক:
    explicit AstLoop(File_সারি* fl, Ast_গ্রন্থি* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* thatp) ধ্রুব override {
        প্রত্যাগম m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    bool maybePointedTo() ধ্রুব override VL_MT_SAFE { প্রত্যাগম true; }
    // ACCESSORS
    VOptionBool unroll() ধ্রুব { প্রত্যাগম m_unroll; }
    শূন্য unroll(ধ্রুব VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
সার্বজনিক:
    AstLoopTest(File_সারি* fl, AstLoop* loopp, Ast_গ্রন্থি_রাশিমালা* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    ধ্রুব বর্ণ* broken() ধ্রুব override;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি*) ধ্রুব override { প্রত্যাগম true; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isBrancher() ধ্রুব override { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 0; }
    // ACCESSORS
    AstLoop* loopp() ধ্রুব { প্রত্যাগম m_loopp; }
};
class AstMonitorOff final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    ধ্রুব bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

সার্বজনিক:
    AstMonitorOff(File_সারি* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    string verilogKwd() ধ্রুব override { প্রত্যাগম m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }  // Though deleted before opt
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }  // Though deleted before opt
    bool isPure() override { প্রত্যাগম false; }  // Though deleted before opt
    bool isOutputter() override { প্রত্যাগম true; }  // Though deleted before opt
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_PLI; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override {
        প্রত্যাগম m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() ধ্রুব { প্রত্যাগম m_off; }
};
class AstPExprClause final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    ধ্রুব bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    ধ্রুব bool m_vacuous;  // Pass is vacuous

সার্বজনিক:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_সারি* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool pass() ধ্রুব { প্রত্যাগম m_pass; }
    bool vacuous() ধ্রুব { প্রত্যাগম m_vacuous; }
};
class AstPrintTimeScale final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
সার্বজনিক:
    explicit AstPrintTimeScale(File_সারি* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    শূন্য name(ধ্রুব string& name) override { m_name = name; }
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }  // * = Var name
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "$printtimescale"; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_PLI; }
    শূন্য timeunit(ধ্রুব VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() ধ্রুব { প্রত্যাগম m_timeunit; }
};
class AstRSBreak final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // randsequence break
সার্বজনিক:
    explicit AstRSBreak(File_সারি* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "break"; }
    bool isBrancher() ধ্রুব override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
সার্বজনিক:
    AstRSProd(File_সারি* fl, ধ্রুব string& name, Ast_গ্রন্থি* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() ধ্রুব override VL_MT_SAFE { প্রত্যাগম true; }
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
};
class AstRSProdItem final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
সার্বজনিক:
    AstRSProdItem(File_সারি* fl, ধ্রুব string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    AstRSProd* prodp() ধ্রুব { প্রত্যাগম m_prodp; }
    শূন্য prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
সার্বজনিক:
    AstRSProdList(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* weightp, Ast_গ্রন্থি* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    bool randJoin() ধ্রুব { প্রত্যাগম m_randJoin; }
    শূন্য randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // randsequence return
সার্বজনিক:
    explicit AstRSReturn(File_সারি* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "return"; }
    bool isBrancher() ধ্রুব override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
সার্বজনিক:
    AstRSRule(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* weightp, AstRSProdList* prodlistsp, Ast_গ্রন্থি* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
};
class AstRandCase final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
সার্বজনিক:
    AstRandCase(File_সারি* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
};
class AstRandSequence final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
সার্বজনিক:
    AstRandSequence(File_সারি* fl, ধ্রুব string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }  // * = Block name
    শূন্য name(ধ্রুব string& name) override { m_name = name; }
    string start() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_start; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    AstRSProd* prodp() ধ্রুব { প্রত্যাগম m_prodp; }
    শূন্য prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
সার্বজনিক:
    AstRelease(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
সার্বজনিক:
    AstRepeat(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* countp, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }  // Not relevant - converted to FOR
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_BRANCH; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class AstReturn final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
সার্বজনিক:
    explicit AstReturn(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "return"; }
    bool isBrancher() ধ্রুব override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
সার্বজনিক:
    AstSFormat(File_সারি* fl, bool optionalFormat, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* exprsp,
               বর্ণ missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* ধ্রুব newp
            = new AstSFormatF{fl, AstSFormatF::রাশিমালা_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_সারি* fl, AstSFormatF* fmtp, Ast_গ্রন্থি_রাশিমালা* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    ধ্রুব বর্ণ* broken() ধ্রুব override {
        BROKEN_RTN(!fmtp());
        প্রত্যাগম nullptr;
    }
    string verilogKwd() ধ্রুব override { প্রত্যাগম "$sformat"; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম true; }
    bool isPure() override { প্রত্যাগম true; }
    bool isOutputter() override { প্রত্যাগম false; }
    আভাস bool cleanOut() ধ্রুব { প্রত্যাগম false; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_PLI; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class AstSetuphold final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
সার্বজনিক:
    AstSetuphold(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* refevp, Ast_গ্রন্থি_রাশিমালা* dataevp,
                 Ast_গ্রন্থি_রাশিমালা* delrefp = nullptr, Ast_গ্রন্থি_রাশিমালা* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class Ast_স্তূপ_Trace_T final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // $stacktrace used as task
সার্বজনিক:
    explicit Ast_স্তূপ_Trace_T(File_সারি* fl)
        : ASTGEN_SUPER_স্তূপ_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_স্তূপ_Trace_T;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "$stacktrace"; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class Ast_Stmt_রাশিমালা final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
সার্বজনিক:
    Ast_Stmt_রাশিমালা(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp)
        : ASTGEN_SUPER_Stmt_রাশিমালা(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_রাশিমালা;
    // cppcheck-suppress uselessOverride
    bool isPure() override { প্রত্যাগম exprp()->isPure(); }
};
class AstStmtPragma final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
সার্বজনিক:
    AstStmtPragma(File_সারি* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    ধ্রুব bool m_isFatal;  // $fatal not $stop
সার্বজনিক:
    AstStop(File_সারি* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { প্রত্যাগম true; }  // SPECIAL: $display makes output
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 0; }  // Rarely executes
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override { প্রত্যাগম fileline() == samep->fileline(); }
    string emitVerilog() ধ্রুব { প্রত্যাগম m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() ধ্রুব { প্রত্যাগম m_isFatal; }
};
class AstSystemT final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
সার্বজনিক:
    AstSystemT(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "$system"; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isUnlikely() ধ্রুব override { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* /*samep*/) ধ্রুব override { প্রত্যাগম true; }
};
class AstTimeFormat final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
সার্বজনিক:
    AstTimeFormat(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* unitsp, Ast_গ্রন্থি_রাশিমালা* precisionp, Ast_গ্রন্থি_রাশিমালা* suffixp,
                  Ast_গ্রন্থি_রাশিমালা* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() ধ্রুব override { প্রত্যাগম "$timeformat"; }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPure() override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম INSTR_গুনতি_PLI; }
};
class AstTraceDecl final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Trace point declaration
    // Separate from AstTraceInc; as a declaration can't be deleted
    // Parents:  {statement list}
    // Expression being traced - Moved to AstTraceInc by V3Trace
    // @astgen op1 := valuep : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_dtypeCallp: Optional[AstCCall] // Type init function call
    // @astgen ptr := m_dtypeDeclp: Optional[AstTraceDecl] // CCall TraceDecl which replaces this
    uint32_t m_code{std::numeric_limits<uint32_t>::max()};  // Trace identifier code
    uint32_t m_fidx{0};  // Trace function index
    ধ্রুব string m_showname;  // Name of variable
    ধ্রুব VNumRange m_bitRange;  // Property of var the trace details
    ধ্রুব VNumRange m_arrayRange;  // Property of var the trace details
    ধ্রুব VVarType m_varType;  // Type of variable (for localparam vs. param)
    ধ্রুব VDirection m_declDirection;  // Declared direction input/output etc
    ধ্রুব bool m_inDtypeFunc;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    পূর্ণাঙ্ক m_codeInc{0};  // Code increment for type
সার্বজনিক:
    AstTraceDecl(File_সারি* fl, ধ্রুব string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_গ্রন্থি_রাশিমালা* valuep, ধ্রুব VNumRange& bitRange, ধ্রুব VNumRange& arrayRange,
                 AstCCall* ধ্রুব dtypeCallp, ধ্রুব bool inDtypeFunc)
        : ASTGEN_SUPER_TraceDecl(fl)
        , m_showname{showname}
        , m_bitRange{bitRange}
        , m_arrayRange{arrayRange}
        , m_varType{varp->varType()}
        , m_declDirection{varp->declDirection()}
        , m_inDtypeFunc{inDtypeFunc} {
        dtypeFrom(valuep);
        this->valuep(valuep);
        this->dtypeCallp(dtypeCallp);
    }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_showname; }
    bool maybePointedTo() ধ্রুব override VL_MT_SAFE { প্রত্যাগম true; }
    bool hasDType() ধ্রুব override VL_MT_SAFE { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override { প্রত্যাগম true; }
    string showname() ধ্রুব { প্রত্যাগম m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() ধ্রুব { প্রত্যাগম m_code; }
    শূন্য code(uint32_t code) { m_code = code; }
    bool codeAssigned() ধ্রুব { প্রত্যাগম m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() ধ্রুব { প্রত্যাগম m_fidx; }
    শূন্য fidx(uint32_t fidx) { m_fidx = fidx; }
    শূন্য codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() ধ্রুব {
        যদি (m_codeInc) { প্রত্যাগম m_codeInc; }
        প্রত্যাগম (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_শব্দগুলি()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    ধ্রুব VNumRange& bitRange() ধ্রুব { প্রত্যাগম m_bitRange; }
    ধ্রুব VNumRange& arrayRange() ধ্রুব { প্রত্যাগম m_arrayRange; }
    VVarType varType() ধ্রুব { প্রত্যাগম m_varType; }
    VDirection declDirection() ধ্রুব { প্রত্যাগম m_declDirection; }
    AstCCall* dtypeCallp() ধ্রুব { প্রত্যাগম m_dtypeCallp; }
    শূন্য dtypeCallp(AstCCall* ধ্রুব callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() ধ্রুব { প্রত্যাগম m_dtypeDeclp; }
    শূন্য dtypeDeclp(AstTraceDecl* ধ্রুব declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() ধ্রুব { প্রত্যাগম m_inDtypeFunc; }
};
class AstTraceInc final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    ধ্রুব uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    ধ্রুব VTraceType m_traceType;  // Is this a const/full/incremental dump

সার্বজনিক:
    AstTraceInc(File_সারি* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_বৃক্ষ(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুব override { প্রত্যাগম 10 + 2 * INSTR_গুনতি_LD; }
    bool hasDType() ধ্রুব override VL_MT_SAFE { প্রত্যাগম true; }
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override {
        প্রত্যাগম declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isPredictOptimizable() ধ্রুব override { প্রত্যাগম false; }
    bool isOutputter() override { প্রত্যাগম true; }
    bool isPure() override { প্রত্যাগম false; }
    AstTraceDecl* declp() ধ্রুব { প্রত্যাগম m_declp; }
    VTraceType traceType() ধ্রুব { প্রত্যাগম m_traceType; }
    uint32_t baseCode() ধ্রুব { প্রত্যাগম m_baseCode; }
};
class Ast_Trace_বের_করো_Prefix final : সার্বজনিক Ast_গ্রন্থি_Stmt {
সার্বজনিক:
    explicit Ast_Trace_বের_করো_Prefix(File_সারি* fl)
        : ASTGEN_SUPER_Trace_বের_করো_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_বের_করো_Prefix;
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override { প্রত্যাগম false; }
};
class Ast_Trace_ঠেলো_Prefix final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    ধ্রুব string m_prefix;  // Prefix to add to signal names
    ধ্রুব VTracePrefixType m_prefixType;  // Type of prefix being pushed
    ধ্রুব পূর্ণাঙ্ক m_বাম;  // Array left index, or struct/union member count
    ধ্রুব পূর্ণাঙ্ক m_ডান;  // Array right index
    ধ্রুব bool m_quotedPrefix;  // Quote prefix name
সার্বজনিক:
    Ast_Trace_ঠেলো_Prefix(File_সারি* fl, ধ্রুব string& prefix, VTracePrefixType prefixType,
                       পূর্ণাঙ্ক বাম = 0, পূর্ণাঙ্ক ডান = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_ঠেলো_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_বাম{বাম}
        , m_ডান{ডান}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_ঠেলো_Prefix;
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override { প্রত্যাগম false; }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    string prefix() ধ্রুব { প্রত্যাগম m_prefix; }
    VTracePrefixType prefixType() ধ্রুব { প্রত্যাগম m_prefixType; }
    পূর্ণাঙ্ক বাম() ধ্রুব { প্রত্যাগম m_বাম; }
    পূর্ণাঙ্ক ডান() ধ্রুব { প্রত্যাগম m_ডান; }
    bool quotedPrefix() ধ্রুব { প্রত্যাগম m_quotedPrefix; }
};
class AstWait final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
সার্বজনিক:
    AstWait(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() ধ্রুব override { প্রত্যাগম true; }
};
class AstWaitFork final : সার্বজনিক Ast_গ্রন্থি_Stmt {
    // A "wait fork" statement
সার্বজনিক:
    explicit AstWaitFork(File_সারি* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() ধ্রুব override { প্রত্যাগম true; }
    string verilogKwd() ধ্রুব override { প্রত্যাগম "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : সার্বজনিক Ast_গ্রন্থি_Assign {
সার্বজনিক:
    AstAssign(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp,
              Ast_গ্রন্থি* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_গ্রন্থি_Assign* cloneType(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp) override {
        Ast_গ্রন্থি* ধ্রুব controlp = timingControlp() ? timingControlp()->clone_বৃক্ষ(false) : nullptr;
        প্রত্যাগম new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : সার্বজনিক Ast_গ্রন্থি_Assign {
    // Compound assignments (+=, -=, *=, ...)
সার্বজনিক:
    গণনা operation : uint8_t {
        Add,
        And,
        Div,
        ModDiv,
        Mul,
        Or,
        ShiftL,
        ShiftR,
        ShiftRS,
        Sub,
        Xor,
    };

গুপ্ত:
    operation m_operation;

সার্বজনিক:
    AstAssignCompound(AstAssignCompound::operation operation, File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp,
                      Ast_গ্রন্থি_রাশিমালা* rhsp, Ast_গ্রন্থি* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_গ্রন্থি_Assign* cloneType(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp) override {
        Ast_গ্রন্থি* ধ্রুব controlp = timingControlp() ? timingControlp()->clone_বৃক্ষ(false) : nullptr;
        প্রত্যাগম new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { প্রত্যাগম m_operation; }
};
class AstAssignCont final : সার্বজনিক Ast_গ্রন্থি_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
সার্বজনিক:
    AstAssignCont(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp,
                  Ast_গ্রন্থি* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_গ্রন্থি_Assign* cloneType(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp) override {
        Ast_গ্রন্থি* ধ্রুব controlp = timingControlp() ? timingControlp()->clone_বৃক্ষ(false) : nullptr;
        প্রত্যাগম new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : সার্বজনিক Ast_গ্রন্থি_Assign {
সার্বজনিক:
    AstAssignDly(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp,
                 Ast_গ্রন্থি* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_গ্রন্থি_Assign* cloneType(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp) override {
        Ast_গ্রন্থি* ধ্রুব controlp = timingControlp() ? timingControlp()->clone_বৃক্ষ(false) : nullptr;
        প্রত্যাগম new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() ধ্রুব override { প্রত্যাগম false; }
    string verilogKwd() ধ্রুব override { প্রত্যাগম "<="; }
};
class AstAssignForce final : সার্বজনিক Ast_গ্রন্থি_Assign {
    // Procedural 'force' statement
সার্বজনিক:
    AstAssignForce(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_গ্রন্থি_Assign* cloneType(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp) override {
        প্রত্যাগম new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : সার্বজনিক Ast_গ্রন্থি_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
সার্বজনিক:
    AstAssignW(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp,
               Ast_গ্রন্থি* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_গ্রন্থি_Assign* cloneType(Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp) override {
        Ast_গ্রন্থি* ধ্রুব controlp = timingControlp() ? timingControlp()->clone_বৃক্ষ(false) : nullptr;
        প্রত্যাগম new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() ধ্রুব override {
        প্রত্যাগম timingControlp() || lhsp()->exists([](ধ্রুব Ast_গ্রন্থি_Var_Ref* refp) {
            প্রত্যাগম refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() ধ্রুব;
};

// === AstNodeBlock ===
class AstBegin final : সার্বজনিক Ast_গ্রন্থি_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    ধ্রুব bool m_implied : 1;  // Not inserted by user
সার্বজনিক:
    AstBegin(File_সারি* fl, ধ্রুব string& name, Ast_গ্রন্থি* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    শূন্য setNeedProcess() { m_needProcess = true; }
    bool needProcess() ধ্রুব { প্রত্যাগম m_needProcess; }
    bool implied() ধ্রুব { প্রত্যাগম m_implied; }
};
class AstFork final : সার্বজনিক Ast_গ্রন্থি_Block {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    ধ্রুব VJoinType m_joinType;  // Join keyword type
    bool m_immediateStart = false;  // Fork starts before its parent blocks or exits

সার্বজনিক:
    AstFork(File_সারি* fl, VJoinType joinType, ধ্রুব string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_গ্রন্থি(ধ্রুব Ast_গ্রন্থি* samep) ধ্রুব override {
        ধ্রুব AstFork* ধ্রুব asamep = VN_DBG_AS(samep, Fork);
        প্রত্যাগম joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() ধ্রুব override { প্রত্যাগম !joinType().joinNone(); }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    VJoinType joinType() ধ্রুব { প্রত্যাগম m_joinType; }
    bool immediateStart() ধ্রুব { প্রত্যাগম m_immediateStart; }
    শূন্য immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : সার্বজনিক Ast_গ্রন্থি_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

সার্বজনিক:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_সারি* fl, Ast_গ্রন্থি* propp, Ast_গ্রন্থি* passsp, Ast_গ্রন্থি* failsp, VAssertType type,
              VAssertDirectiveType directive, ধ্রুব string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ধ্রুব override {
        প্রত্যাগম directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : সার্বজনিক Ast_গ্রন্থি_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
সার্বজনিক:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_সারি* fl, Ast_গ্রন্থি* propp, Ast_গ্রন্থি* passsp, Ast_গ্রন্থি* failsp,
                       ধ্রুব string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() ধ্রুব override { প্রত্যাগম "assert"; }
};
class AstCover final : সার্বজনিক Ast_গ্রন্থি_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
সার্বজনিক:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_সারি* fl, Ast_গ্রন্থি* propp, Ast_গ্রন্থি* stmtsp, VAssertType type,
             ধ্রুব string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() ধ্রুব override { প্রত্যাগম "cover"; }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
    bool isCoverSeq() ধ্রুব { প্রত্যাগম m_isCoverSeq; }
    শূন্য isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() ধ্রুব { প্রত্যাগম m_isSeqEvent; }
    শূন্য isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : সার্বজনিক Ast_গ্রন্থি_Cover_Or_Assert {
সার্বজনিক:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_সারি* fl, Ast_গ্রন্থি* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() ধ্রুব override { প্রত্যাগম "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : সার্বজনিক Ast_গ্রন্থি_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
সার্বজনিক:
    AstConstraintForeach(File_সারি* fl, AstForeachHeader* headerp, Ast_গ্রন্থি* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() ধ্রুব { প্রত্যাগম m_soft; }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
};
class AstForeach final : সার্বজনিক Ast_গ্রন্থি_Foreach {
সার্বজনিক:
    AstForeach(File_সারি* fl, AstForeachHeader* headerp, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : সার্বজনিক Ast_গ্রন্থি_If {
সার্বজনিক:
    AstConstraintIf(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি* thensp, Ast_গ্রন্থি* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : সার্বজনিক Ast_গ্রন্থি_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
সার্বজনিক:
    AstIf(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি* thensp = nullptr, Ast_গ্রন্থি* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() ধ্রুব { প্রত্যাগম m_uniquePragma; }
    শূন্য uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() ধ্রুব { প্রত্যাগম m_unique0Pragma; }
    শূন্য unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() ধ্রুব { প্রত্যাগম m_priorityPragma; }
    শূন্য priorityPragma(bool flag) { m_priorityPragma = flag; }
    শূন্য dump(std::ostream& str) ধ্রুব override;
    শূন্য dumpJson(std::ostream& str) ধ্রুব override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : সার্বজনিক Ast_গ্রন্থি_Read_Write_Mem {
সার্বজনিক:
    AstReadMem(File_সারি* fl, bool hex, Ast_গ্রন্থি_রাশিমালা* filenamep, Ast_গ্রন্থি_রাশিমালা* memp,
               Ast_গ্রন্থি_রাশিমালা* lsbp, Ast_গ্রন্থি_রাশিমালা* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() ধ্রুব override { প্রত্যাগম (isHex() ? "$readmemh" : "$readmemb"); }
    ধ্রুব বর্ণ* cFuncPrefixp() ধ্রুব override { প্রত্যাগম "VL_READMEM_"; }
};
class AstWriteMem final : সার্বজনিক Ast_গ্রন্থি_Read_Write_Mem {
সার্বজনিক:
    AstWriteMem(File_সারি* fl, bool hex, Ast_গ্রন্থি_রাশিমালা* filenamep, Ast_গ্রন্থি_রাশিমালা* memp,
                Ast_গ্রন্থি_রাশিমালা* lsbp, Ast_গ্রন্থি_রাশিমালা* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() ধ্রুব override { প্রত্যাগম (isHex() ? "$writememh" : "$writememb"); }
    ধ্রুব বর্ণ* cFuncPrefixp() ধ্রুব override { প্রত্যাগম "VL_WRITEMEM_"; }
};

#endif  // Guard
