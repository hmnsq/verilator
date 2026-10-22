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
#അന്തര്ഭാവ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_കണ്ണി_Stmt VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // Procedural statement
സംരക്ഷിത:
    Ast_കണ്ണി_Stmt(VNType t, File_വരി* fl)
        : Ast_കണ്ണി{t, fl} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Stmt;
    // METHODS
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
};
class Ast_കണ്ണി_Assign VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
സംരക്ഷിത:
    Ast_കണ്ണി_Assign(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp,
                  Ast_കണ്ണി* timingControlp = nullptr)
        : Ast_കണ്ണി_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Assign;
    // Clone single node, just get same type back.
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    മായാ Ast_കണ്ണി_Assign* cloneType(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) = 0;
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    മായാ bool cleanRhs() സ്ഥിരം { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    string verilogKwd() സ്ഥിരം override { തിരികെ "="; }
    bool isTimingControl() സ്ഥിരം override { തിരികെ timingControlp(); }
};
class Ast_കണ്ണി_Block VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
സംരക്ഷിത:
    Ast_കണ്ണി_Block(VNType t, File_വരി* fl, സ്ഥിരം string& name)
        : Ast_കണ്ണി_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Block;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Block name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    bool unnamed() സ്ഥിരം { തിരികെ m_unnamed; }
};
class Ast_കണ്ണി_Cover_Or_Assert VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    സ്ഥിരം VAssertType m_userType;  // Assertion/cover type for user enable/disable
    സ്ഥിരം VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_immediate = false;  // Immediate assert (may differ from userType being immediate)

പൊതു:
    Ast_കണ്ണി_Cover_Or_Assert(VNType t, File_വരി* fl, Ast_കണ്ണി* propp, Ast_കണ്ണി* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         സ്ഥിരം string& name = "")
        : Ast_കണ്ണി_Stmt{t, fl}
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
    ASTGEN_MEMBERS_Ast_കണ്ണി_Cover_Or_Assert;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ samep->name() == name(); }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string type() { തിരികെ ""; }
    VAssertType userType() സ്ഥിരം VL_MT_SAFE { തിരികെ m_userType; }
    VAssertDirectiveType directive() സ്ഥിരം { തിരികെ m_directive; }
    bool immediate() സ്ഥിരം { തിരികെ m_immediate; }
    ശൂന്യം immediate(bool flag) { m_immediate = flag; }
    bool senFromAlways() സ്ഥിരം VL_MT_STABLE { തിരികെ m_senFromAlways; }
    ശൂന്യം senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_കണ്ണി_Foreach VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
പൊതു:
    Ast_കണ്ണി_Foreach(VNType t, File_വരി* fl, AstForeachHeader* headerp, Ast_കണ്ണി* bodyp)
        : Ast_കണ്ണി_Stmt(t, fl) {
        this->headerp(headerp);
        addBodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_കണ്ണി_Foreach;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class Ast_കണ്ണി_If VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
സംരക്ഷിത:
    Ast_കണ്ണി_If(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി* thensp, Ast_കണ്ണി* elsesp)
        : Ast_കണ്ണി_Stmt{t, fl} {
        this->condp(condp);
        addThensp(thensp);
        addElsesp(elsesp);
        isBoundsCheck(false);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_If;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isGateDedupable() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() സ്ഥിരം { തിരികെ m_branchPred; }
    ശൂന്യം isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() സ്ഥിരം { തിരികെ m_isBoundsCheck; }
};
class Ast_കണ്ണി_Read_Write_Mem VL_NOT_FINAL : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    സ്ഥിരം bool m_isHex;  // readmemh, not readmemb
പൊതു:
    Ast_കണ്ണി_Read_Write_Mem(VNType t, File_വരി* fl, bool hex, Ast_കണ്ണി_വ്യഞ്ജകം* filenamep,
                        Ast_കണ്ണി_വ്യഞ്ജകം* memp, Ast_കണ്ണി_വ്യഞ്ജകം* lsbp, Ast_കണ്ണി_വ്യഞ്ജകം* msbp)
        : Ast_കണ്ണി_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_കണ്ണി_Read_Write_Mem;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ isHex() == VN_DBG_AS(samep, കണ്ണി_Read_Write_Mem)->isHex();
    }
    bool isHex() സ്ഥിരം { തിരികെ m_isHex; }
    മായാ സ്ഥിരം അക്ഷരം* cFuncPrefixp() സ്ഥിരം = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstCaseItem final : പൊതു Ast_കണ്ണി {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
പൊതു:
    AstCaseItem(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condsp, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_CaseItem(fl) {
        addCondsp(condsp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCaseItem;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs() + INSTR_എണ്ണം_BRANCH; }
    bool isDefault() സ്ഥിരം { തിരികെ condsp() == nullptr; }
};

class AstForeachHeader final : പൊതു Ast_കണ്ണി {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
പൊതു:
    AstForeachHeader(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി* elementsp)
        : ASTGEN_SUPER_ForeachHeader(fl) {
        this->fromp(fromp);
        addElementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstForeachHeader;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

പൊതു:
    AstAssertCtl(File_വരി* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_കണ്ണി_വ്യഞ്ജകം* levelp = nullptr, Ast_കണ്ണി_വ്യഞ്ജകം* itemsp = nullptr);
    AstAssertCtl(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* controlTypep, Ast_കണ്ണി_വ്യഞ്ജകം* assertTypesp = nullptr,
                 Ast_കണ്ണി_വ്യഞ്ജകം* directiveTypep = nullptr, Ast_കണ്ണി_വ്യഞ്ജകം* levelp = nullptr,
                 Ast_കണ്ണി_വ്യഞ്ജകം* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() സ്ഥിരം override { തിരികെ m_ctlType.ascii(); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    VAssertCtlType ctlType() സ്ഥിരം { തിരികെ m_ctlType; }
    ശൂന്യം ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() സ്ഥിരം { തിരികെ m_assertTypes; }
    ശൂന്യം assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() സ്ഥിരം { തിരികെ m_directiveTypes; }
    ശൂന്യം directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
};
class AstBreak final : പൊതു Ast_കണ്ണി_Stmt {
പൊതു:
    explicit AstBreak(File_വരി* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() സ്ഥിരം override { തിരികെ "break"; }
    bool isBrancher() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : പൊതു Ast_കണ്ണി_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
പൊതു:
    AstCAwait(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_Sen_വൃക്ഷം* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isTimingControl() സ്ഥിരം override { തിരികെ true; }
    Ast_Sen_വൃക്ഷം* sentreep() സ്ഥിരം { തിരികെ m_sentreep; }
    ശൂന്യം clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : പൊതു Ast_കണ്ണി_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
പൊതു:
    AstCLocalScope(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : പൊതു Ast_കണ്ണി_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
പൊതു:
    AstCReturn(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ widthInstrs(); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstCStmt final : പൊതു Ast_കണ്ണി_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    സ്ഥിരം VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    സ്ഥിതം AstCStmt* profExecSection(File_വരി* flp, സ്ഥിരം std::string& section, bool തള്ളുക) {
        // Compute the label
        std::string label;
        എങ്കിൽ (v3Global.opt.hierChild()) {
            label += v3Global.opt.മുകൾ_Module() + ":";
        } അല്ലെങ്കിൽ എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += തള്ളുക ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        എങ്കിൽ (തള്ളുക) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += തള്ളുക ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        തിരികെ new AstCStmt{flp, തള്ളുക ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

പൊതു:
    explicit AstCStmt(File_വരി* fl, സ്ഥിരം std::string& text = "",
                      സ്ഥിരം VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        എങ്കിൽ (!text.empty()) add(text);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    VCStmtType stmtType() സ്ഥിരം { തിരികെ m_stmtType; }
    // Add some text, or a node to this statement
    ശൂന്യം add(സ്ഥിരം std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ശൂന്യം add(Ast_കണ്ണി* nodep) { addNodesp(nodep); }
    // Static factory methods for specific cases
    സ്ഥിതം AstCStmt* prof_Exec_Section_തള്ളുക(File_വരി* flp, സ്ഥിരം std::string& section) {
        തിരികെ profExecSection(flp, section, true);
    }
    സ്ഥിതം AstCStmt* prof_Exec_Section_എടുക്കുക(File_വരി* flp, സ്ഥിരം std::string& section) {
        തിരികെ profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : പൊതു Ast_കണ്ണി_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    സ്ഥിരം bool m_fromDollarC;  // Is from source '$c', emit decoration
പൊതു:
    explicit AstCStmtUser(File_വരി* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    bool fromDollarC() സ്ഥിരം { തിരികെ m_fromDollarC; }
    // Add some text, or a node to this statement
    ശൂന്യം add(സ്ഥിരം std::string& text) { addNodesp(new AstText{fileline(), text}); }
    ശൂന്യം add(Ast_കണ്ണി* nodep) { addNodesp(nodep); }
};
class AstCase final : പൊതു Ast_കണ്ണി_Stmt {
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
പൊതു:
    AstCase(File_വരി* fl, VCaseType caseType, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstCaseItem* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    string verilogKwd() സ്ഥിരം override { തിരികെ casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() സ്ഥിരം { തിരികെ m_caseType == VCaseType::CT_CASEX; }
    bool casez() സ്ഥിരം { തിരികെ m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() സ്ഥിരം { തിരികെ m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() സ്ഥിരം { തിരികെ m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() സ്ഥിരം { തിരികെ m_caseType == VCaseType::CT_CASE; }
    ശൂന്യം caseInsideSet() { m_caseType = VCaseType::CT_CASEINSIDE; }
    ശൂന്യം caseMatchesSet() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool fullPragma() സ്ഥിരം { തിരികെ m_fullPragma; }
    ശൂന്യം fullPragma(bool flag) { m_fullPragma = flag; }
    bool parallelPragma() സ്ഥിരം { തിരികെ m_parallelPragma; }
    ശൂന്യം parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() സ്ഥിരം { തിരികെ m_uniquePragma; }
    ശൂന്യം uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() സ്ഥിരം { തിരികെ m_unique0Pragma; }
    ശൂന്യം unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() സ്ഥിരം { തിരികെ m_priorityPragma; }
    ശൂന്യം priorityPragma(bool flag) { m_priorityPragma = flag; }
    string pragmaString() സ്ഥിരം;
};
class AstComment final : പൊതു Ast_കണ്ണി_Stmt {
    // Some comment to put into the output stream
    സ്ഥിരം string m_name;  // Text of comment
    സ്ഥിരം bool m_showAt;  // Show "at <fileline>"
പൊതു:
    AstComment(File_വരി* fl, സ്ഥിരം string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Text
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ true; }  // Ignore name in comments
    മായാ bool showAt() സ്ഥിരം { തിരികെ m_showAt; }
};
class AstConstraintBefore final : പൊതു Ast_കണ്ണി_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
പൊതു:
    AstConstraintBefore(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhssp, Ast_കണ്ണി_വ്യഞ്ജകം* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        addLhssp(lhssp);
        addRhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class Ast_Constraint_വ്യഞ്ജകം final : പൊതു Ast_കണ്ണി_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
പൊതു:
    Ast_Constraint_വ്യഞ്ജകം(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_Constraint_വ്യഞ്ജകം(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_വ്യഞ്ജകം;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isDisableSoft() സ്ഥിരം { തിരികെ m_isDisableSoft; }
    ശൂന്യം isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() സ്ഥിരം { തിരികെ m_isSoft; }
    ശൂന്യം isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : പൊതു Ast_കണ്ണി_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
പൊതു:
    AstConstraintUnique(File_വരി* fl, Ast_കണ്ണി* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstContinue final : പൊതു Ast_കണ്ണി_Stmt {
പൊതു:
    explicit AstContinue(File_വരി* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() സ്ഥിരം override { തിരികെ "continue"; }
    bool isBrancher() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : പൊതു Ast_കണ്ണി_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

പൊതു:
    AstCoverInc(File_വരി* fl, Ast_കണ്ണി_Cover_Decl* declp, സ്ഥിരം bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 1 + 2 * INSTR_എണ്ണം_LD; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstCoverInc* സ്ഥിരം otherp = VN_DBG_AS(samep, CoverInc);
        തിരികെ declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isPure() override { തിരികെ false; }
    Ast_കണ്ണി_Cover_Decl* declp() സ്ഥിരം { തിരികെ m_declp; }  // Where defined
    ശൂന്യം declp(Ast_കണ്ണി_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() സ്ഥിരം { തിരികെ m_isDuplicate; }
};
class AstDeassign final : പൊതു Ast_കണ്ണി_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
പൊതു:
    AstDeassign(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : പൊതു Ast_കണ്ണി_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    സ്ഥിരം bool m_isCycleDelay;  // True if it is a cycle delay
പൊതു:
    AstDelay(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, bool isCycleDelay)
        : ASTGEN_SUPER_Delay(fl)
        , m_isCycleDelay{isCycleDelay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isTimingControl() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    bool isCycleDelay() സ്ഥിരം { തിരികെ m_isCycleDelay; }
    bool isRangeDelay() സ്ഥിരം { തിരികെ m_isCycleDelay && rhsp(); }
    bool isUnbounded() സ്ഥിരം { തിരികെ isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    ശൂന്യം fallDelay(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fallDelayp) { rhsp(fallDelayp); }
    Ast_കണ്ണി_വ്യഞ്ജകം* fallDelay() സ്ഥിരം { തിരികെ m_isCycleDelay ? nullptr : rhsp(); }
};
class AstDisable final : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
പൊതു:
    AstDisable(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* targetRefp)
        : ASTGEN_SUPER_Disable(fl) {
        this->targetRefp(targetRefp);
    }
    ASTGEN_MEMBERS_AstDisable;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം targetp(Ast_കണ്ണി* nodep) { m_targetp = nodep; }
    Ast_കണ്ണി* targetp() സ്ഥിരം { തിരികെ m_targetp; }
    bool isBrancher() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : പൊതു Ast_കണ്ണി_Stmt {
    // A "disable fork" statement
പൊതു:
    explicit AstDisableFork(File_വരി* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() സ്ഥിരം override { തിരികെ "disable fork"; }
};
class AstDisplay final : പൊതു Ast_കണ്ണി_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

പൊതു:
    AstDisplay(File_വരി* fl, VDisplayType dispType, സ്ഥിരം string& text, Ast_കണ്ണി_വ്യഞ്ജകം* filep,
               Ast_കണ്ണി_വ്യഞ്ജകം* exprsp, അക്ഷരം missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, text, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_വരി* fl, VDisplayType dispType, Ast_കണ്ണി_വ്യഞ്ജകം* filep, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp,
               അക്ഷരം missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* സ്ഥിരം newp
            = new AstSFormatF{fl, AstSFormatF::വ്യഞ്ജകം_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!fmtp());
        തിരികെ nullptr;
    }
    string verilogKwd() സ്ഥിരം override {
        തിരികെ (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: $display makes output
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    VDisplayType displayType() സ്ഥിരം { തിരികെ m_displayType; }
    ശൂന്യം displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool addNewline() സ്ഥിരം { തിരികെ displayType().addNewline(); }
};
class AstDumpCtl final : പൊതു Ast_കണ്ണി_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    സ്ഥിരം VDumpCtlType m_ctlType;  // Type of operation
പൊതു:
    AstDumpCtl(File_വരി* fl, VDumpCtlType ctlType, Ast_കണ്ണി_വ്യഞ്ജകം* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() സ്ഥിരം override { തിരികെ ctlType().ascii(); }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    മായാ bool cleanOut() സ്ഥിരം { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    VDumpCtlType ctlType() സ്ഥിരം { തിരികെ m_ctlType; }
};
class AstEventControl final : പൊതു Ast_കണ്ണി_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
പൊതു:
    AstEventControl(File_വരി* fl, Ast_Sen_വൃക്ഷം* sentreep, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() സ്ഥിരം override { തിരികെ "@(%l) %r"; }
    bool isTimingControl() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
};
class AstExecGraph final : പൊതു Ast_കണ്ണി_Stmt {
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
    V3Graph* സ്ഥിരം m_depGraphp;  // contains ExecMTask vertices
    സ്ഥിരം string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

പൊതു:
    AstExecGraph(File_വരി* fl, സ്ഥിരം string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~AstExecGraph() override;
    ASTGEN_MEMBERS_AstExecGraph;
    ശൂന്യം cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    V3Graph* depGraphp() { തിരികെ m_depGraphp; }
    സ്ഥിരം V3Graph* depGraphp() സ്ഥിരം { തിരികെ m_depGraphp; }
    AstCFunc* runfuncp() സ്ഥിരം { തിരികെ m_runfuncp; }
    ശൂന്യം runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : പൊതു Ast_കണ്ണി_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
പൊതു:
    AstFClose(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$fclose"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstFFlush final : പൊതു Ast_കണ്ണി_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
പൊതു:
    AstFFlush(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$fflush"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstFinish final : പൊതു Ast_കണ്ണി_Stmt {
പൊതു:
    explicit AstFinish(File_വരി* fl)
        : ASTGEN_SUPER_Finish(fl) {}
    ASTGEN_MEMBERS_AstFinish;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: $display makes output
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }  // Rarely executes
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ fileline() == samep->fileline(); }
};
class AstFinishFork final : പൊതു Ast_കണ്ണി_Stmt {
    // $finish in fork
പൊതു:
    explicit AstFinishFork(File_വരി* fl)
        : ASTGEN_SUPER_FinishFork(fl) {}
    ASTGEN_MEMBERS_AstFinishFork;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: $display makes output
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }  // Rarely executes
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ fileline() == samep->fileline(); }
};
class AstFireEvent final : പൊതു Ast_കണ്ണി_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    സ്ഥിരം bool m_delayed;  // Delayed (->>) vs non-delayed (->)
പൊതു:
    AstFireEvent(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isDelayed() സ്ഥിരം { തിരികെ m_delayed; }
};
class AstInitialAutomaticStmt final : പൊതു Ast_കണ്ണി_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
പൊതു:
    AstInitialAutomaticStmt(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool isPure() override { തിരികെ true; }
};
class AstInitialStaticStmt final : പൊതു Ast_കണ്ണി_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
പൊതു:
    AstInitialStaticStmt(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool isPure() override { തിരികെ true; }
};
class AstJumpBlock final : പൊതു Ast_കണ്ണി_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
പൊതു:
    // After construction must call ->labelp to associate with appropriate label
    AstJumpBlock(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_JumpBlock(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstJumpBlock;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    bool isPure() override;

സ്വകാര്യ:
    bool getPurityRecurse() സ്ഥിരം;
};
class AstJumpGo final : പൊതു Ast_കണ്ണി_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
പൊതു:
    AstJumpGo(File_വരി* fl, AstJumpBlock* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isBrancher() സ്ഥിരം override {
        തിരികെ true;  // SPECIAL: We don't process code after breaks
    }
    AstJumpBlock* blockp() സ്ഥിരം { തിരികെ m_blockp; }
};
class AstLoop final : പൊതു Ast_കണ്ണി_Stmt {
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
പൊതു:
    explicit AstLoop(File_വരി* fl, Ast_കണ്ണി* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* thatp) സ്ഥിരം override {
        തിരികെ m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    // ACCESSORS
    VOptionBool unroll() സ്ഥിരം { തിരികെ m_unroll; }
    ശൂന്യം unroll(സ്ഥിരം VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : പൊതു Ast_കണ്ണി_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
പൊതു:
    AstLoopTest(File_വരി* fl, AstLoop* loopp, Ast_കണ്ണി_വ്യഞ്ജകം* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isBrancher() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }
    // ACCESSORS
    AstLoop* loopp() സ്ഥിരം { തിരികെ m_loopp; }
};
class AstMonitorOff final : പൊതു Ast_കണ്ണി_Stmt {
    സ്ഥിരം bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

പൊതു:
    AstMonitorOff(File_വരി* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string verilogKwd() സ്ഥിരം override { തിരികെ m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }  // Though deleted before opt
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }  // Though deleted before opt
    bool isPure() override { തിരികെ false; }  // Though deleted before opt
    bool isOutputter() override { തിരികെ true; }  // Though deleted before opt
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() സ്ഥിരം { തിരികെ m_off; }
};
class AstPExprClause final : പൊതു Ast_കണ്ണി_Stmt {
    സ്ഥിരം bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    സ്ഥിരം bool m_vacuous;  // Pass is vacuous

പൊതു:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_വരി* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool pass() സ്ഥിരം { തിരികെ m_pass; }
    bool vacuous() സ്ഥിരം { തിരികെ m_vacuous; }
};
class AstPrintTimeScale final : പൊതു Ast_കണ്ണി_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
പൊതു:
    explicit AstPrintTimeScale(File_വരി* fl)
        : ASTGEN_SUPER_PrintTimeScale(fl) {}
    ASTGEN_MEMBERS_AstPrintTimeScale;
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$printtimescale"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
};
class AstRSBreak final : പൊതു Ast_കണ്ണി_Stmt {
    // randsequence break
പൊതു:
    explicit AstRSBreak(File_വരി* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() സ്ഥിരം override { തിരികെ "break"; }
    bool isBrancher() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : പൊതു Ast_കണ്ണി_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
പൊതു:
    AstRSProd(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        addPortsp(portsp);
        addRulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
};
class AstRSProdItem final : പൊതു Ast_കണ്ണി_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
പൊതു:
    AstRSProdItem(File_വരി* fl, സ്ഥിരം string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProdItem(fl)
        , m_name{name} {
        addArgsp(argsp);
    }
    ASTGEN_MEMBERS_AstRSProdItem;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    AstRSProd* prodp() സ്ഥിരം { തിരികെ m_prodp; }
    ശൂന്യം prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : പൊതു Ast_കണ്ണി_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
പൊതു:
    AstRSProdList(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* weightp, Ast_കണ്ണി* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    bool randJoin() സ്ഥിരം { തിരികെ m_randJoin; }
    ശൂന്യം randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : പൊതു Ast_കണ്ണി_Stmt {
    // randsequence return
പൊതു:
    explicit AstRSReturn(File_വരി* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() സ്ഥിരം override { തിരികെ "return"; }
    bool isBrancher() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : പൊതു Ast_കണ്ണി_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
പൊതു:
    AstRSRule(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* weightp, AstRSProdList* prodlistsp, Ast_കണ്ണി* weightStmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        addProdlistsp(prodlistsp);
        addWeightStmtsp(weightStmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
};
class AstRandCase final : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
പൊതു:
    AstRandCase(File_വരി* fl, AstCaseItem* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        addItemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
};
class AstRandSequence final : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_start;  // Name of start production, or "" to use first production
പൊതു:
    AstRandSequence(File_വരി* fl, സ്ഥിരം string& start, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_start{start} {
        addProdsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Block name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string start() സ്ഥിരം VL_MT_STABLE { തിരികെ m_start; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    AstRSProd* prodp() സ്ഥിരം { തിരികെ m_prodp; }
    ശൂന്യം prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : പൊതു Ast_കണ്ണി_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
പൊതു:
    AstRelease(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
പൊതു:
    AstRepeat(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* countp, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }  // Not relevant - converted to FOR
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_BRANCH; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstReturn final : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
പൊതു:
    explicit AstReturn(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() സ്ഥിരം override { തിരികെ "return"; }
    bool isBrancher() സ്ഥിരം override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : പൊതു Ast_കണ്ണി_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
പൊതു:
    AstSFormat(File_വരി* fl, bool optionalFormat, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp,
               അക്ഷരം missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* സ്ഥിരം newp
            = new AstSFormatF{fl, AstSFormatF::വ്യഞ്ജകം_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_വരി* fl, AstSFormatF* fmtp, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!fmtp());
        തിരികെ nullptr;
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "$sformat"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ true; }
    bool isPure() override { തിരികെ true; }
    bool isOutputter() override { തിരികെ false; }
    മായാ bool cleanOut() സ്ഥിരം { തിരികെ false; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstSetuphold final : പൊതു Ast_കണ്ണി_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
പൊതു:
    AstSetuphold(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* refevp, Ast_കണ്ണി_വ്യഞ്ജകം* dataevp,
                 Ast_കണ്ണി_വ്യഞ്ജകം* delrefp = nullptr, Ast_കണ്ണി_വ്യഞ്ജകം* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class Ast_അട്ടി_Trace_T final : പൊതു Ast_കണ്ണി_Stmt {
    // $stacktrace used as task
പൊതു:
    explicit Ast_അട്ടി_Trace_T(File_വരി* fl)
        : ASTGEN_SUPER_അട്ടി_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_അട്ടി_Trace_T;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$stacktrace"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class Ast_Stmt_വ്യഞ്ജകം final : പൊതു Ast_കണ്ണി_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
പൊതു:
    Ast_Stmt_വ്യഞ്ജകം(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_Stmt_വ്യഞ്ജകം(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_വ്യഞ്ജകം;
    // cppcheck-suppress uselessOverride
    bool isPure() override { തിരികെ exprp()->isPure(); }
};
class AstStmtPragma final : പൊതു Ast_കണ്ണി_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
പൊതു:
    AstStmtPragma(File_വരി* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : പൊതു Ast_കണ്ണി_Stmt {
    സ്ഥിരം bool m_isFatal;  // $fatal not $stop
പൊതു:
    AstStop(File_വരി* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: $display makes output
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 0; }  // Rarely executes
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ fileline() == samep->fileline(); }
    string emitVerilog() സ്ഥിരം { തിരികെ m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() സ്ഥിരം { തിരികെ m_isFatal; }
};
class AstSystemT final : പൊതു Ast_കണ്ണി_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
പൊതു:
    AstSystemT(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$system"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstTimeFormat final : പൊതു Ast_കണ്ണി_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
പൊതു:
    AstTimeFormat(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* unitsp, Ast_കണ്ണി_വ്യഞ്ജകം* precisionp, Ast_കണ്ണി_വ്യഞ്ജകം* suffixp,
                  Ast_കണ്ണി_വ്യഞ്ജകം* widthp)
        : ASTGEN_SUPER_TimeFormat(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_AstTimeFormat;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$timeformat"; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
};
class AstTraceDecl final : പൊതു Ast_കണ്ണി_Stmt {
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
    സ്ഥിരം string m_showname;  // Name of variable
    സ്ഥിരം VNumRange m_bitRange;  // Property of var the trace details
    സ്ഥിരം VNumRange m_arrayRange;  // Property of var the trace details
    സ്ഥിരം VVarType m_varType;  // Type of variable (for localparam vs. param)
    സ്ഥിരം VDirection m_declDirection;  // Declared direction input/output etc
    സ്ഥിരം bool m_inDtypeFunc;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    പൂർണ്ണസംഖ്യ m_codeInc{0};  // Code increment for type
പൊതു:
    AstTraceDecl(File_വരി* fl, സ്ഥിരം string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_കണ്ണി_വ്യഞ്ജകം* valuep, സ്ഥിരം VNumRange& bitRange, സ്ഥിരം VNumRange& arrayRange,
                 AstCCall* സ്ഥിരം dtypeCallp, സ്ഥിരം bool inDtypeFunc)
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
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_showname; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ true; }
    string showname() സ്ഥിരം { തിരികെ m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t code() സ്ഥിരം { തിരികെ m_code; }
    ശൂന്യം code(uint32_t code) { m_code = code; }
    bool codeAssigned() സ്ഥിരം { തിരികെ m_code != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() സ്ഥിരം { തിരികെ m_fidx; }
    ശൂന്യം fidx(uint32_t fidx) { m_fidx = fidx; }
    ശൂന്യം codeInc(uint32_t codeInc) { m_codeInc = codeInc; }
    uint32_t codeInc() സ്ഥിരം {
        എങ്കിൽ (m_codeInc) { തിരികെ m_codeInc; }
        തിരികെ (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->width_വാക്കുകൾ()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    സ്ഥിരം VNumRange& bitRange() സ്ഥിരം { തിരികെ m_bitRange; }
    സ്ഥിരം VNumRange& arrayRange() സ്ഥിരം { തിരികെ m_arrayRange; }
    VVarType varType() സ്ഥിരം { തിരികെ m_varType; }
    VDirection declDirection() സ്ഥിരം { തിരികെ m_declDirection; }
    AstCCall* dtypeCallp() സ്ഥിരം { തിരികെ m_dtypeCallp; }
    ശൂന്യം dtypeCallp(AstCCall* സ്ഥിരം callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() സ്ഥിരം { തിരികെ m_dtypeDeclp; }
    ശൂന്യം dtypeDeclp(AstTraceDecl* സ്ഥിരം declp) { m_dtypeDeclp = declp; }
    bool inDtypeFunc() സ്ഥിരം { തിരികെ m_inDtypeFunc; }
};
class AstTraceInc final : പൊതു Ast_കണ്ണി_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    സ്ഥിരം uint32_t m_baseCode;  // Trace code base value in function containing this AstTraceInc
    സ്ഥിരം VTraceType m_traceType;  // Is this a const/full/incremental dump

പൊതു:
    AstTraceInc(File_വരി* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t baseCode = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_baseCode{baseCode}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_വൃക്ഷം(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 10 + 2 * INSTR_എണ്ണം_LD; }
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
    bool isPure() override { തിരികെ false; }
    AstTraceDecl* declp() സ്ഥിരം { തിരികെ m_declp; }
    VTraceType traceType() സ്ഥിരം { തിരികെ m_traceType; }
    uint32_t baseCode() സ്ഥിരം { തിരികെ m_baseCode; }
};
class Ast_Trace_എടുക്കുക_Prefix final : പൊതു Ast_കണ്ണി_Stmt {
പൊതു:
    explicit Ast_Trace_എടുക്കുക_Prefix(File_വരി* fl)
        : ASTGEN_SUPER_Trace_എടുക്കുക_Prefix(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_എടുക്കുക_Prefix;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ false; }
};
class Ast_Trace_തള്ളുക_Prefix final : പൊതു Ast_കണ്ണി_Stmt {
    സ്ഥിരം string m_prefix;  // Prefix to add to signal names
    സ്ഥിരം VTracePrefixType m_prefixType;  // Type of prefix being pushed
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_ഇടത്;  // Array left index, or struct/union member count
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_വലത്;  // Array right index
    സ്ഥിരം bool m_quotedPrefix;  // Quote prefix name
പൊതു:
    Ast_Trace_തള്ളുക_Prefix(File_വരി* fl, സ്ഥിരം string& prefix, VTracePrefixType prefixType,
                       പൂർണ്ണസംഖ്യ ഇടത് = 0, പൂർണ്ണസംഖ്യ വലത് = 0, bool quotedPrefix = true)
        : ASTGEN_SUPER_Trace_തള്ളുക_Prefix(fl)
        , m_prefix{prefix}
        , m_prefixType{prefixType}
        , m_ഇടത്{ഇടത്}
        , m_വലത്{വലത്}
        , m_quotedPrefix{quotedPrefix} {}
    ASTGEN_MEMBERS_Ast_Trace_തള്ളുക_Prefix;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override { തിരികെ false; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string prefix() സ്ഥിരം { തിരികെ m_prefix; }
    VTracePrefixType prefixType() സ്ഥിരം { തിരികെ m_prefixType; }
    പൂർണ്ണസംഖ്യ ഇടത്() സ്ഥിരം { തിരികെ m_ഇടത്; }
    പൂർണ്ണസംഖ്യ വലത്() സ്ഥിരം { തിരികെ m_വലത്; }
    bool quotedPrefix() സ്ഥിരം { തിരികെ m_quotedPrefix; }
};
class AstWait final : പൊതു Ast_കണ്ണി_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
പൊതു:
    AstWait(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() സ്ഥിരം override { തിരികെ true; }
};
class AstWaitFork final : പൊതു Ast_കണ്ണി_Stmt {
    // A "wait fork" statement
പൊതു:
    explicit AstWaitFork(File_വരി* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() സ്ഥിരം override { തിരികെ true; }
    string verilogKwd() സ്ഥിരം override { തിരികെ "wait fork"; }
};

// === AstNodeAssign ===
class AstAssign final : പൊതു Ast_കണ്ണി_Assign {
പൊതു:
    AstAssign(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp,
              Ast_കണ്ണി* timingControlp = nullptr)
        : ASTGEN_SUPER_Assign(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssign;
    Ast_കണ്ണി_Assign* cloneType(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) override {
        Ast_കണ്ണി* സ്ഥിരം controlp = timingControlp() ? timingControlp()->clone_വൃക്ഷം(false) : nullptr;
        തിരികെ new AstAssign{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignCompound final : പൊതു Ast_കണ്ണി_Assign {
    // Compound assignments (+=, -=, *=, ...)
പൊതു:
    എണ്ണൽ operation : uint8_t {
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

സ്വകാര്യ:
    operation m_operation;

പൊതു:
    AstAssignCompound(AstAssignCompound::operation operation, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp,
                      Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, Ast_കണ്ണി* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCompound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCompound;
    Ast_കണ്ണി_Assign* cloneType(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) override {
        Ast_കണ്ണി* സ്ഥിരം controlp = timingControlp() ? timingControlp()->clone_വൃക്ഷം(false) : nullptr;
        തിരികെ new AstAssignCompound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { തിരികെ m_operation; }
};
class AstAssignCont final : പൊതു Ast_കണ്ണി_Assign {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
പൊതു:
    AstAssignCont(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp,
                  Ast_കണ്ണി* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignCont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_AstAssignCont;
    Ast_കണ്ണി_Assign* cloneType(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) override {
        Ast_കണ്ണി* സ്ഥിരം controlp = timingControlp() ? timingControlp()->clone_വൃക്ഷം(false) : nullptr;
        തിരികെ new AstAssignCont{fileline(), lhsp, rhsp, controlp};
    }
};
class AstAssignDly final : പൊതു Ast_കണ്ണി_Assign {
പൊതു:
    AstAssignDly(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp,
                 Ast_കണ്ണി* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignDly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignDly;
    Ast_കണ്ണി_Assign* cloneType(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) override {
        Ast_കണ്ണി* സ്ഥിരം controlp = timingControlp() ? timingControlp()->clone_വൃക്ഷം(false) : nullptr;
        തിരികെ new AstAssignDly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    string verilogKwd() സ്ഥിരം override { തിരികെ "<="; }
};
class AstAssignForce final : പൊതു Ast_കണ്ണി_Assign {
    // Procedural 'force' statement
പൊതു:
    AstAssignForce(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_AssignForce(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_AstAssignForce;
    Ast_കണ്ണി_Assign* cloneType(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) override {
        തിരികെ new AstAssignForce{fileline(), lhsp, rhsp};
    }
};
class AstAssignW final : പൊതു Ast_കണ്ണി_Assign {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
പൊതു:
    AstAssignW(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp,
               Ast_കണ്ണി* timingControlp = nullptr)
        : ASTGEN_SUPER_AssignW(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_AstAssignW;
    Ast_കണ്ണി_Assign* cloneType(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) override {
        Ast_കണ്ണി* സ്ഥിരം controlp = timingControlp() ? timingControlp()->clone_വൃക്ഷം(false) : nullptr;
        തിരികെ new AstAssignW{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() സ്ഥിരം override {
        തിരികെ timingControlp() || lhsp()->exists([](സ്ഥിരം Ast_കണ്ണി_Var_Ref* refp) {
            തിരികെ refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() സ്ഥിരം;
};

// === AstNodeBlock ===
class AstBegin final : പൊതു Ast_കണ്ണി_Block {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    സ്ഥിരം bool m_implied : 1;  // Not inserted by user
പൊതു:
    AstBegin(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        addStmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം setNeedProcess() { m_needProcess = true; }
    bool needProcess() സ്ഥിരം { തിരികെ m_needProcess; }
    bool implied() സ്ഥിരം { തിരികെ m_implied; }
};
class AstFork final : പൊതു Ast_കണ്ണി_Block {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    സ്ഥിരം VJoinType m_joinType;  // Join keyword type
    bool m_immediateStart = false;  // Fork starts before its parent blocks or exits

പൊതു:
    AstFork(File_വരി* fl, VJoinType joinType, സ്ഥിരം string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstFork* സ്ഥിരം asamep = VN_DBG_AS(samep, Fork);
        തിരികെ joinType() == asamep->joinType() && immediateStart() == asamep->immediateStart();
    }
    bool isTimingControl() സ്ഥിരം override { തിരികെ !joinType().joinNone(); }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    VJoinType joinType() സ്ഥിരം { തിരികെ m_joinType; }
    bool immediateStart() സ്ഥിരം { തിരികെ m_immediateStart; }
    ശൂന്യം immediateStart(bool flag) { m_immediateStart = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : പൊതു Ast_കണ്ണി_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

പൊതു:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_വരി* fl, Ast_കണ്ണി* propp, Ast_കണ്ണി* passsp, Ast_കണ്ണി* failsp, VAssertType type,
              VAssertDirectiveType directive, സ്ഥിരം string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        addFailsp(failsp);
    }
    string verilogKwd() സ്ഥിരം override {
        തിരികെ directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : പൊതു Ast_കണ്ണി_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
പൊതു:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_വരി* fl, Ast_കണ്ണി* propp, Ast_കണ്ണി* passsp, Ast_കണ്ണി* failsp,
                       സ്ഥിരം string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::INTERNAL,
                                       VAssertDirectiveType::INTRINSIC, name) {
        addFailsp(failsp);
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "assert"; }
};
class AstCover final : പൊതു Ast_കണ്ണി_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
പൊതു:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_വരി* fl, Ast_കണ്ണി* propp, Ast_കണ്ണി* stmtsp, VAssertType type,
             സ്ഥിരം string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() സ്ഥിരം override { തിരികെ "cover"; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isCoverSeq() സ്ഥിരം { തിരികെ m_isCoverSeq; }
    ശൂന്യം isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() സ്ഥിരം { തിരികെ m_isSeqEvent; }
    ശൂന്യം isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : പൊതു Ast_കണ്ണി_Cover_Or_Assert {
പൊതു:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_വരി* fl, Ast_കണ്ണി* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::INTERNAL,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() സ്ഥിരം override { തിരികെ "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : പൊതു Ast_കണ്ണി_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
പൊതു:
    AstConstraintForeach(File_വരി* fl, AstForeachHeader* headerp, Ast_കണ്ണി* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() സ്ഥിരം { തിരികെ m_soft; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstForeach final : പൊതു Ast_കണ്ണി_Foreach {
പൊതു:
    AstForeach(File_വരി* fl, AstForeachHeader* headerp, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : പൊതു Ast_കണ്ണി_If {
പൊതു:
    AstConstraintIf(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി* thensp, Ast_കണ്ണി* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : പൊതു Ast_കണ്ണി_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_priorityPragma = false;  // priority case
പൊതു:
    AstIf(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി* thensp = nullptr, Ast_കണ്ണി* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() സ്ഥിരം { തിരികെ m_uniquePragma; }
    ശൂന്യം uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() സ്ഥിരം { തിരികെ m_unique0Pragma; }
    ശൂന്യം unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool priorityPragma() സ്ഥിരം { തിരികെ m_priorityPragma; }
    ശൂന്യം priorityPragma(bool flag) { m_priorityPragma = flag; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : പൊതു Ast_കണ്ണി_Read_Write_Mem {
പൊതു:
    AstReadMem(File_വരി* fl, bool hex, Ast_കണ്ണി_വ്യഞ്ജകം* filenamep, Ast_കണ്ണി_വ്യഞ്ജകം* memp,
               Ast_കണ്ണി_വ്യഞ്ജകം* lsbp, Ast_കണ്ണി_വ്യഞ്ജകം* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() സ്ഥിരം override { തിരികെ (isHex() ? "$readmemh" : "$readmemb"); }
    സ്ഥിരം അക്ഷരം* cFuncPrefixp() സ്ഥിരം override { തിരികെ "VL_READMEM_"; }
};
class AstWriteMem final : പൊതു Ast_കണ്ണി_Read_Write_Mem {
പൊതു:
    AstWriteMem(File_വരി* fl, bool hex, Ast_കണ്ണി_വ്യഞ്ജകം* filenamep, Ast_കണ്ണി_വ്യഞ്ജകം* memp,
                Ast_കണ്ണി_വ്യഞ്ജകം* lsbp, Ast_കണ്ണി_വ്യഞ്ജകം* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() സ്ഥിരം override { തിരികെ (isHex() ? "$writememh" : "$writememb"); }
    സ്ഥിരം അക്ഷരം* cFuncPrefixp() സ്ഥിരം override { തിരികെ "VL_WRITEMEM_"; }
};

#endif  // Guard
