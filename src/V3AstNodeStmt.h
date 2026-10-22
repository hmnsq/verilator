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

#నిర్వచితం_కాకపోతే VERILATOR_V3ASTNODESTMT_H_
#నిర్వచించు VERILATOR_V3ASTNODESTMT_H_

#నిర్వచితం_కాకపోతే VERILATOR_V3AST_H_
#దోషం "Use V3Ast.h as the include"
#అన్తర్భావ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#నిర్వచించు VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#అయితే_ముగింపు

// === Abstract base node types (AstNode*) =====================================

class Ast_కణుపు_Stmt VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // Procedural statement
రక్షిత:
    Ast_కణుపు_Stmt(VNType t, File_పంక్తి* fl)
        : Ast_కణుపు{t, fl} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Stmt;
    // METHODS
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
};
class Ast_కణుపు_కేటాయించు VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Stmt {
    // Iteration is in order, and we want rhsp to be visited first (which is the execution order)
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
    // @astgen op3 := timingControlp : Optional[AstNode]
రక్షిత:
    Ast_కణుపు_కేటాయించు(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp,
                  Ast_కణుపు* timingControlp = nullptr)
        : Ast_కణుపు_Stmt{t, fl} {
        this->rhsp(rhsp);
        this->lhsp(lhsp);
        this->timingControlp(timingControlp);
        dtypeFrom(lhsp);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_కేటాయించు;
    // Clone single node, just get same type back.
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    మాయా Ast_కణుపు_కేటాయించు* cloneType(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) = 0;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    మాయా bool cleanRhs() స్థిరరాశి { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    string verilogKwd() స్థిరరాశి override { తిరిగి "="; }
    bool isTimingControl() స్థిరరాశి override { తిరిగి timingControlp(); }
};
class Ast_కణుపు_ఖండం VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Stmt {
    // A Begin/fork block
    // Parents: statement
    // @astgen op1 := declsp : List[AstNode] // Declarations inside block
    // @astgen op2 := stmtsp : List[AstNode] // Sequential statements inside block
    string m_name;  // Name of block
    bool m_unnamed;  // Originally unnamed (name change does not affect this)
రక్షిత:
    Ast_కణుపు_ఖండం(VNType t, File_పంక్తి* fl, స్థిరరాశి string& name)
        : Ast_కణుపు_Stmt{t, fl}
        , m_name{name}
        , m_unnamed{name == ""} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_ఖండం;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Block name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    bool unnamed() స్థిరరాశి { తిరిగి m_unnamed; }
};
class Ast_కణుపు_Cover_Or_Assert VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Stmt {
    // Cover or Assert
    // Parents:  {statement list}
    // @astgen op1 := propp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
    // op3 used by some sub-types only
    // @astgen op4 := passsp: List[AstNode] // Statements when propp is passing/truthly
    string m_name;  // Name to report
    స్థిరరాశి VAssertType m_userType;  // Assertion/cover type for user enable/disable
    స్థిరరాశి VAssertDirectiveType m_directive;  // Assertion directive type
    bool m_senFromAlways = false;  // Sensitivity list copied from upper always
    bool m_తక్షణ = false;  // Immediate assert (may differ from userType being immediate)

బహిరంగ:
    Ast_కణుపు_Cover_Or_Assert(VNType t, File_పంక్తి* fl, Ast_కణుపు* propp, Ast_కణుపు* passsp,
                         VAssertType userType, VAssertDirectiveType directive,
                         స్థిరరాశి string& name = "")
        : Ast_కణుపు_Stmt{t, fl}
        , m_name{name}
        , m_userType{userType}
        , m_directive{directive} {
        this->propp(propp);
        కూడు_Passsp(passsp);
        m_తక్షణ = m_userType.కలిగియుంది_Any(VAssertType::SIMPLE_తక్షణ
                                             | VAssertType::OBSERVED_DEFERRED_తక్షణ
                                             | VAssertType::FINAL_DEFERRED_తక్షణ)
                      || m_userType == VAssertType::అంతర్గత;
    }
    ASTGEN_MEMBERS_Ast_కణుపు_Cover_Or_Assert;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి samep->name() == name(); }
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string type() { తిరిగి ""; }
    VAssertType userType() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_userType; }
    VAssertDirectiveType directive() స్థిరరాశి { తిరిగి m_directive; }
    bool తక్షణ() స్థిరరాశి { తిరిగి m_తక్షణ; }
    శూన్య తక్షణ(bool flag) { m_తక్షణ = flag; }
    bool senFromAlways() స్థిరరాశి VL_MT_STABLE { తిరిగి m_senFromAlways; }
    శూన్య senFromAlways(bool flag) { m_senFromAlways = flag; }
};
class Ast_కణుపు_Foreach VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op1 := headerp : AstForeachHeader
    // @astgen op2 := bodyp : List[AstNode]
బహిరంగ:
    Ast_కణుపు_Foreach(VNType t, File_పంక్తి* fl, Ast_Foreach_శీర్షిక* headerp, Ast_కణుపు* bodyp)
        : Ast_కణుపు_Stmt(t, fl) {
        this->headerp(headerp);
        కూడు_Bodyp(bodyp);
    }
    ASTGEN_MEMBERS_Ast_కణుపు_Foreach;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_కణుపు_If VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
    VBranchPred m_branchPred;  // Branch prediction as taken/untaken?
    bool m_isBoundsCheck;  // True if this if node is for assertion/bounds checking
రక్షిత:
    Ast_కణుపు_If(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* condp, Ast_కణుపు* thensp, Ast_కణుపు* elsesp)
        : Ast_కణుపు_Stmt{t, fl} {
        this->condp(condp);
        కూడు_Thensp(thensp);
        కూడు_Elsesp(elsesp);
        isBoundsCheck(false);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_If;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isGateDedupable() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    శూన్య branchPred(VBranchPred flag) { m_branchPred = flag; }
    VBranchPred branchPred() స్థిరరాశి { తిరిగి m_branchPred; }
    శూన్య isBoundsCheck(bool flag) { m_isBoundsCheck = flag; }
    bool isBoundsCheck() స్థిరరాశి { తిరిగి m_isBoundsCheck; }
};
class Ast_కణుపు_Read_Write_Mem VL_NOT_FINAL : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op1 := filenamep : AstNodeExpr
    // @astgen op2 := memp : AstNodeExpr
    // @astgen op3 := lsbp : Optional[AstNodeExpr]
    // @astgen op4 := msbp : Optional[AstNodeExpr]

    స్థిరరాశి bool m_isHex;  // readmemh, not readmemb
బహిరంగ:
    Ast_కణుపు_Read_Write_Mem(VNType t, File_పంక్తి* fl, bool hex, Ast_కణుపు_సమాసం* filenamep,
                        Ast_కణుపు_సమాసం* memp, Ast_కణుపు_సమాసం* lsbp, Ast_కణుపు_సమాసం* msbp)
        : Ast_కణుపు_Stmt{t, fl}
        , m_isHex{hex} {
        this->filenamep(filenamep);
        this->memp(memp);
        this->lsbp(lsbp);
        this->msbp(msbp);
    }
    ASTGEN_MEMBERS_Ast_కణుపు_Read_Write_Mem;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి isHex() == VN_DBG_AS(samep, కణుపు_Read_Write_Mem)->isHex();
    }
    bool isHex() స్థిరరాశి { తిరిగి m_isHex; }
    మాయా స్థిరరాశి అక్షరం* cFuncPrefixp() స్థిరరాశి = 0;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Case_అంశం final : బహిరంగ Ast_కణుపు {
    // Single item of AstCase/AstRandCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := stmtsp : List[AstNode]
బహిరంగ:
    Ast_Case_అంశం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* condsp, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_Case_అంశం(fl) {
        కూడు_Condsp(condsp);
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Case_అంశం;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs() + INSTR_లెక్క_BRANCH; }
    bool isDefault() స్థిరరాశి { తిరిగి condsp() == nullptr; }
};

class Ast_Foreach_శీర్షిక final : బహిరంగ Ast_కణుపు {
    // Variable reference + index enumeration "ref [id, id, id]" for a foreach statement
    // @astgen op1 := fromp : AstNodeExpr
    // @astgen op2 := elementsp : List[AstNode<AstNodeExpr|AstVar|AstEmpty>]
    //             AstNodeExpr/AstEmpty during parsing (only AstParseRef/AstEmpty is well formed)
    //             then AstVar/AstEmpty after LinkDot
బహిరంగ:
    Ast_Foreach_శీర్షిక(File_పంక్తి* fl, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు* elementsp)
        : ASTGEN_SUPER_Foreach_శీర్షిక(fl) {
        this->fromp(fromp);
        కూడు_Elementsp(elementsp);
    }
    ASTGEN_MEMBERS_Ast_Foreach_శీర్షిక;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి false; }
};

// === AstNodeStmt ===
class AstAssertCtl final : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op1 := controlTypep : AstNodeExpr
    // @astgen op2 := assertTypesp : Optional[AstNodeExpr]
    // @astgen op3 := directiveTypesp : Optional[AstNodeExpr]
    // Type of assertcontrol task; either known from parser or from evaluated
    // controlTypep expression.
    VAssertCtlType m_ctlType;  // $assert keyword type (control_type)
    VAssertType m_assertTypes;  // Types of assertions affected
    VAssertDirectiveType m_directiveTypes;  // Types of directives affected

బహిరంగ:
    AstAssertCtl(File_పంక్తి* fl, VAssertCtlType ctlType, uint32_t assertType, uint32_t directiveType,
                 Ast_కణుపు_సమాసం* levelp = nullptr, Ast_కణుపు_సమాసం* itemsp = nullptr);
    AstAssertCtl(File_పంక్తి* fl, Ast_కణుపు_సమాసం* controlTypep, Ast_కణుపు_సమాసం* assertTypesp = nullptr,
                 Ast_కణుపు_సమాసం* directiveTypep = nullptr, Ast_కణుపు_సమాసం* levelp = nullptr,
                 Ast_కణుపు_సమాసం* itemsp = nullptr);
    ASTGEN_MEMBERS_AstAssertCtl;
    string verilogKwd() స్థిరరాశి override { తిరిగి m_ctlType.ascii(); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    VAssertCtlType ctlType() స్థిరరాశి { తిరిగి m_ctlType; }
    శూన్య ctlType(int32_t type) { m_ctlType = VAssertCtlType{type}; }
    VAssertType assertTypes() స్థిరరాశి { తిరిగి m_assertTypes; }
    శూన్య assertTypes(VAssertType types) { m_assertTypes = types; }
    VAssertDirectiveType directiveTypes() స్థిరరాశి { తిరిగి m_directiveTypes; }
    శూన్య directiveTypes(VAssertDirectiveType types) { m_directiveTypes = types; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
};
class AstBreak final : బహిరంగ Ast_కణుపు_Stmt {
బహిరంగ:
    explicit AstBreak(File_పంక్తి* fl)
        : ASTGEN_SUPER_Break(fl) {}
    ASTGEN_MEMBERS_AstBreak;
    string verilogKwd() స్థిరరాశి override { తిరిగి "break"; }
    bool isBrancher() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCAwait final : బహిరంగ Ast_కణుపు_Stmt {
    // C++'s co_await. While this is an expression in C++, in Verilator it is only used
    // with void result types and always appears in statement position. There must never
    // be a suspendable process that returns a value, so modeling as a statement instead.
    // @astgen op1 := exprp : AstNodeExpr
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sentree related to this await
బహిరంగ:
    AstCAwait(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_Sen_వృక్షం* sentreep = nullptr)
        : ASTGEN_SUPER_CAwait(fl)
        , m_sentreep{sentreep} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCAwait;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isTimingControl() స్థిరరాశి override { తిరిగి true; }
    Ast_Sen_వృక్షం* sentreep() స్థిరరాశి { తిరిగి m_sentreep; }
    శూన్య clearSentreep() { m_sentreep = nullptr; }
};
class AstCLocalScope final : బహిరంగ Ast_కణుపు_Stmt {
    // Pack statements into an unnamed scope when generating C++
    // @astgen op1 := stmtsp : List[AstNode]
బహిరంగ:
    AstCLocalScope(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_CLocalScope(fl) {
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstCLocalScope;
};
class AstCReturn final : బహిరంగ Ast_కణుపు_Stmt {
    // C++ return from a function
    // @astgen op1 := lhsp : AstNodeExpr
బహిరంగ:
    AstCReturn(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_CReturn(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstCReturn;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి వెడల్పు_Instrs(); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstCStmt final : బహిరంగ Ast_కణుపు_Stmt {
    // C statement emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode<AstNodeStmt|AstNodeExpr|AstText>]
    స్థిరరాశి VCStmtType m_stmtType;  // Special statement (instead of comparing name())

    స్థిర AstCStmt* profExecSection(File_పంక్తి* flp, స్థిరరాశి std::string& section, bool నెట్టు) {
        // Compute the label
        std::string label;
        ఒకవేళ (v3Global.opt.hier_బిడ్డ()) {
            label += v3Global.opt.శిఖరం_Module() + ":";
        } లేకపోతే ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            label += v3Global.opt.libCreate() + ":";
        }
        label += section;
        // The profiler statement
        std::string pStmt = "VL_EXEC_TRACE_ADD_RECORD(vlSymsp)";
        pStmt += నెట్టు ? ".sectionPush" : ".sectionPop";
        pStmt += "(";
        ఒకవేళ (నెట్టు) pStmt += "\"" + label + "\"";
        pStmt += ");";
        // The debug statement
        std::string dStmt = "VL_DEBUG_IF(VL_DBG_MSGF(\"+    --prof-exec ";
        dStmt += నెట్టు ? "sectionPush " : "sectionPop ";
        dStmt += label;
        dStmt += "\\n\"););";
        // Concatenate
        తిరిగి new AstCStmt{flp, నెట్టు ? pStmt + "\n" + dStmt : dStmt + "\n" + pStmt};
    }

బహిరంగ:
    explicit AstCStmt(File_పంక్తి* fl, స్థిరరాశి std::string& పాఠ్యం = "",
                      స్థిరరాశి VCStmtType& stmtType = VCStmtType::NONE)
        : ASTGEN_SUPER_CStmt(fl)
        , m_stmtType{stmtType} {
        ఒకవేళ (!పాఠ్యం.ఖాళీలు()) కూడు(పాఠ్యం);
    }
    ASTGEN_MEMBERS_AstCStmt;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    VCStmtType stmtType() స్థిరరాశి { తిరిగి m_stmtType; }
    // Add some text, or a node to this statement
    శూన్య కూడు(స్థిరరాశి std::string& పాఠ్యం) { కూడు_Nodesp(new Ast_పాఠ్యం{fileline(), పాఠ్యం}); }
    శూన్య కూడు(Ast_కణుపు* nodep) { కూడు_Nodesp(nodep); }
    // Static factory methods for specific cases
    స్థిర AstCStmt* prof_Exec_Section_నెట్టు(File_పంక్తి* flp, స్థిరరాశి std::string& section) {
        తిరిగి profExecSection(flp, section, true);
    }
    స్థిర AstCStmt* prof_Exec_Section_తీయి(File_పంక్తి* flp, స్థిరరాశి std::string& section) {
        తిరిగి profExecSection(flp, section, false);
    }
};
class AstCStmtUser final : బహిరంగ Ast_కణుపు_Stmt {
    // User '$c' statement, also used for handling some AstSystemCSection.
    // Same as AstCStmt, with text tracking disabled.
    //
    // Note this cannot be modelled as AstStmtExpr(AstCExprUser) because the
    // latter would have an extra semicolon emitted, which might be undesirable.
    //
    // Use AstCStmt instead, unless the text is from user input.
    //
    // @astgen op1 := nodesp : List[AstNode<AstNodeExpr|AstText>]
    స్థిరరాశి bool m_fromDollarC;  // Is from source '$c', emit decoration
బహిరంగ:
    explicit AstCStmtUser(File_పంక్తి* fl, bool fromDollarC = false)
        : ASTGEN_SUPER_CStmtUser(fl)
        , m_fromDollarC{fromDollarC} {}
    ASTGEN_MEMBERS_AstCStmtUser;
    // METHODS
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    bool fromDollarC() స్థిరరాశి { తిరిగి m_fromDollarC; }
    // Add some text, or a node to this statement
    శూన్య కూడు(స్థిరరాశి std::string& పాఠ్యం) { కూడు_Nodesp(new Ast_పాఠ్యం{fileline(), పాఠ్యం}); }
    శూన్య కూడు(Ast_కణుపు* nodep) { కూడు_Nodesp(nodep); }
};
class AstCase final : బహిరంగ Ast_కణుపు_Stmt {
    // Case statement
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstCaseItem]
    // @astgen op3 := notParallelp : List[AstNode] // assertion code for non-full case's
    VCaseType m_caseType;  // 0=case, 1=casex, 2=casez
    bool m_నిండిన_Pragma = false;  // Synthesis full_case
    bool m_parallelPragma = false;  // Synthesis parallel_case
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ప్రాధాన్యత_Pragma = false;  // priority case
బహిరంగ:
    AstCase(File_పంక్తి* fl, VCaseType caseType, Ast_కణుపు_సమాసం* exprp, Ast_Case_అంశం* itemsp)
        : ASTGEN_SUPER_Case(fl)
        , m_caseType{caseType} {
        this->exprp(exprp);
        కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCase;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    string verilogKwd() స్థిరరాశి override { తిరిగి casez() ? "casez" : casex() ? "casex" : "case"; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_caseType == VN_DBG_AS(samep, Case)->m_caseType;
    }
    bool casex() స్థిరరాశి { తిరిగి m_caseType == VCaseType::CT_CASEX; }
    bool casez() స్థిరరాశి { తిరిగి m_caseType == VCaseType::CT_CASEZ; }
    bool caseInside() స్థిరరాశి { తిరిగి m_caseType == VCaseType::CT_CASEINSIDE; }
    bool caseMatches() స్థిరరాశి { తిరిగి m_caseType == VCaseType::CT_CASEMATCHES; }
    bool caseSimple() స్థిరరాశి { తిరిగి m_caseType == VCaseType::CT_CASE; }
    శూన్య case_Inside_సమితి() { m_caseType = VCaseType::CT_CASEINSIDE; }
    శూన్య case_Matches_సమితి() { m_caseType = VCaseType::CT_CASEMATCHES; }
    bool నిండిన_Pragma() స్థిరరాశి { తిరిగి m_నిండిన_Pragma; }
    శూన్య నిండిన_Pragma(bool flag) { m_నిండిన_Pragma = flag; }
    bool parallelPragma() స్థిరరాశి { తిరిగి m_parallelPragma; }
    శూన్య parallelPragma(bool flag) { m_parallelPragma = flag; }
    bool uniquePragma() స్థిరరాశి { తిరిగి m_uniquePragma; }
    శూన్య uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() స్థిరరాశి { తిరిగి m_unique0Pragma; }
    శూన్య unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ప్రాధాన్యత_Pragma() స్థిరరాశి { తిరిగి m_ప్రాధాన్యత_Pragma; }
    శూన్య ప్రాధాన్యత_Pragma(bool flag) { m_ప్రాధాన్యత_Pragma = flag; }
    string pragmaString() స్థిరరాశి;
};
class AstComment final : బహిరంగ Ast_కణుపు_Stmt {
    // Some comment to put into the output stream
    స్థిరరాశి string m_name;  // Text of comment
    స్థిరరాశి bool m_showAt;  // Show "at <fileline>"
బహిరంగ:
    AstComment(File_పంక్తి* fl, స్థిరరాశి string& name, bool showAt = false)
        : ASTGEN_SUPER_Comment(fl)
        , m_name{name}
        , m_showAt{showAt} {}
    ASTGEN_MEMBERS_AstComment;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Text
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి true; }  // Ignore name in comments
    మాయా bool showAt() స్థిరరాశి { తిరిగి m_showAt; }
};
class AstConstraintBefore final : బహిరంగ Ast_కణుపు_Stmt {
    // Constraint solve before item
    // @astgen op1 := lhssp : List[AstNodeExpr]
    // @astgen op2 := rhssp : List[AstNodeExpr]
బహిరంగ:
    AstConstraintBefore(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhssp, Ast_కణుపు_సమాసం* rhssp)
        : ASTGEN_SUPER_ConstraintBefore(fl) {
        కూడు_Lhssp(lhssp);
        కూడు_Rhssp(rhssp);
    }
    ASTGEN_MEMBERS_AstConstraintBefore;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_Constraint_సమాసం final : బహిరంగ Ast_కణుపు_Stmt {
    // Constraint expression
    // @astgen op1 := exprp : AstNodeExpr
    bool m_isDisableSoft = false;  // Disable soft constraint expression
    bool m_isSoft = false;  // Soft constraint expression
బహిరంగ:
    Ast_Constraint_సమాసం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_Constraint_సమాసం(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Constraint_సమాసం;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isDisableSoft() స్థిరరాశి { తిరిగి m_isDisableSoft; }
    శూన్య isDisableSoft(bool flag) { m_isDisableSoft = flag; }
    bool isSoft() స్థిరరాశి { తిరిగి m_isSoft; }
    శూన్య isSoft(bool flag) { m_isSoft = flag; }
};
class AstConstraintUnique final : బహిరంగ Ast_కణుపు_Stmt {
    // Constraint unique statement
    // @astgen op1 := rangesp : List[AstNode]
బహిరంగ:
    AstConstraintUnique(File_పంక్తి* fl, Ast_కణుపు* rangesp)
        : ASTGEN_SUPER_ConstraintUnique(fl) {
        కూడు_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstConstraintUnique;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstContinue final : బహిరంగ Ast_కణుపు_Stmt {
బహిరంగ:
    explicit AstContinue(File_పంక్తి* fl)
        : ASTGEN_SUPER_Continue(fl) {}
    ASTGEN_MEMBERS_AstContinue;
    string verilogKwd() స్థిరరాశి override { తిరిగి "continue"; }
    bool isBrancher() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstCoverInc final : బహిరంగ Ast_కణుపు_Stmt {
    // Coverage analysis point; increment coverage count
    // @astgen op1 := toggleExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // @astgen op2 := toggleCovExprp : Optional[AstNodeExpr]  // [After V3Clock]
    // These are expressions to which the node corresponds. Used only in toggle coverage
    //
    // @astgen ptr := m_declp : AstNodeCoverDecl  // [After V3CoverageJoin] Declaration

    bool m_isDuplicate = false;  // Duplicate AstCoverInc associated with one AstNodeCoverDecl

బహిరంగ:
    AstCoverInc(File_పంక్తి* fl, Ast_కణుపు_Cover_Decl* declp, స్థిరరాశి bool isDuplicate = false)
        : ASTGEN_SUPER_CoverInc(fl)
        , m_isDuplicate{isDuplicate}
        , m_declp{declp} {}
    ASTGEN_MEMBERS_AstCoverInc;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 1 + 2 * INSTR_లెక్క_LD; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstCoverInc* స్థిరరాశి otherp = VN_DBG_AS(samep, CoverInc);
        తిరిగి declp() == otherp->declp() && isDuplicate() == otherp->isDuplicate();
    }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool is_శుద్ధ() override { తిరిగి false; }
    Ast_కణుపు_Cover_Decl* declp() స్థిరరాశి { తిరిగి m_declp; }  // Where defined
    శూన్య declp(Ast_కణుపు_Cover_Decl* nodep) { m_declp = nodep; }
    bool isDuplicate() స్థిరరాశి { తిరిగి m_isDuplicate; }
};
class AstDeassign final : బహిరంగ Ast_కణుపు_Stmt {
    // Procedural 'deassign' statement
    // @astgen op1 := lhsp : AstNodeExpr
బహిరంగ:
    AstDeassign(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Deassign(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDeassign;
};
class AstDelay final : బహిరంగ Ast_కణుపు_Stmt {
    // Delay statement
    // @astgen op1 := lhsp : AstNodeExpr // Delay value (or min for range)
    // @astgen op2 := stmtsp : List[AstNode] // Statements under delay
    // @astgen op3 := rhsp : Optional[AstNodeExpr] // Max bound for cycle range or fall delay
    // @astgen op4 := throughoutp : Optional[AstNodeExpr] // Throughout condition (IEEE 16.9.9)
    VTimescale m_timeunit;  // Delay's time unit
    స్థిరరాశి bool m_is_చక్రం_Delay;  // True if it is a cycle delay
బహిరంగ:
    AstDelay(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, bool is_చక్రం_Delay)
        : ASTGEN_SUPER_Delay(fl)
        , m_is_చక్రం_Delay{is_చక్రం_Delay} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstDelay;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isTimingControl() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    bool is_చక్రం_Delay() స్థిరరాశి { తిరిగి m_is_చక్రం_Delay; }
    bool isRangeDelay() స్థిరరాశి { తిరిగి m_is_చక్రం_Delay && rhsp(); }
    bool isUnbounded() స్థిరరాశి { తిరిగి isRangeDelay() && VN_IS(rhsp(), Unbounded); }
    శూన్య fallDelay(Ast_కణుపు_సమాసం* స్థిరరాశి fallDelayp) { rhsp(fallDelayp); }
    Ast_కణుపు_సమాసం* fallDelay() స్థిరరాశి { తిరిగి m_is_చక్రం_Delay ? nullptr : rhsp(); }
};
class AstDisable final : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op1 := targetRefp : Optional[AstNodeExpr]  // Reference to link in V3LinkDot
    // @astgen ptr := m_targetp : Optional[AstNode]  // Task or block after V3LinkDot
బహిరంగ:
    AstDisable(File_పంక్తి* fl, Ast_కణుపు_సమాసం* లక్ష్యం_Refp)
        : ASTGEN_SUPER_Disable(fl) {
        this->లక్ష్యం_Refp(లక్ష్యం_Refp);
    }
    ASTGEN_MEMBERS_AstDisable;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య targetp(Ast_కణుపు* nodep) { m_targetp = nodep; }
    Ast_కణుపు* targetp() స్థిరరాశి { తిరిగి m_targetp; }
    bool isBrancher() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstDisableFork final : బహిరంగ Ast_కణుపు_Stmt {
    // A "disable fork" statement
బహిరంగ:
    explicit AstDisableFork(File_పంక్తి* fl)
        : ASTGEN_SUPER_DisableFork(fl) {}
    ASTGEN_MEMBERS_AstDisableFork;
    string verilogKwd() స్థిరరాశి override { తిరిగి "disable fork"; }
};
class AstDisplay final : బహిరంగ Ast_కణుపు_Stmt {
    // Parents: stmtlist
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := filep : Optional[AstNodeExpr] // file (must resolve to a VarRef)
    VDisplayType m_displayType;

బహిరంగ:
    AstDisplay(File_పంక్తి* fl, VDisplayType dispType, స్థిరరాశి string& పాఠ్యం, Ast_కణుపు_సమాసం* filep,
               Ast_కణుపు_సమాసం* exprsp, అక్షరం missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        fmtp(new AstSFormatF{fl, పాఠ్యం, true, exprsp, missingArgChar});
        this->filep(filep);
    }
    AstDisplay(File_పంక్తి* fl, VDisplayType dispType, Ast_కణుపు_సమాసం* filep, Ast_కణుపు_సమాసం* exprsp,
               అక్షరం missingArgChar = 'd')
        : ASTGEN_SUPER_Display(fl)
        , m_displayType{dispType} {
        AstSFormatF* స్థిరరాశి newp
            = new AstSFormatF{fl, AstSFormatF::సమాసం_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(true);
        fmtp(newp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstDisplay;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!fmtp());
        తిరిగి nullptr;
    }
    string verilogKwd() స్థిరరాశి override {
        తిరిగి (filep() ? "$f"s + string{displayType().ascii()}
                        : "$"s + string{displayType().ascii()});
    }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: $display makes output
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి displayType() == VN_DBG_AS(samep, Display)->displayType();
    }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    VDisplayType displayType() స్థిరరాశి { తిరిగి m_displayType; }
    శూన్య displayType(VDisplayType type) { m_displayType = type; }
    // * = Add a newline for $display
    bool కూడు_Newline() స్థిరరాశి { తిరిగి displayType().కూడు_Newline(); }
};
class AstDumpCtl final : బహిరంగ Ast_కణుపు_Stmt {
    // $dumpon etc
    // Parents: expr
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression based on type of statement
    స్థిరరాశి VDumpCtlType m_ctlType;  // Type of operation
బహిరంగ:
    AstDumpCtl(File_పంక్తి* fl, VDumpCtlType ctlType, Ast_కణుపు_సమాసం* exprp = nullptr)
        : ASTGEN_SUPER_DumpCtl(fl)
        , m_ctlType{ctlType} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstDumpCtl;
    string verilogKwd() స్థిరరాశి override { తిరిగి ctlType().ascii(); }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    మాయా bool clean_బహిర్గతం() స్థిరరాశి { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    VDumpCtlType ctlType() స్థిరరాశి { తిరిగి m_ctlType; }
};
class AstEventControl final : బహిరంగ Ast_కణుపు_Stmt {
    // Parents: stmtlist
    // @astgen op1 := sentreep : Optional[AstSenTree]
    // @astgen op2 := stmtsp : List[AstNode]
బహిరంగ:
    AstEventControl(File_పంక్తి* fl, Ast_Sen_వృక్షం* sentreep, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_EventControl(fl) {
        this->sentreep(sentreep);
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstEventControl;
    string verilogKwd() స్థిరరాశి override { తిరిగి "@(%l) %r"; }
    bool isTimingControl() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
};
class Ast_Exec_లేఖాచిత్రం final : బహిరంగ Ast_కణుపు_Stmt {
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
    V3_లేఖాచిత్రం* స్థిరరాశి m_depGraphp;  // contains ExecMTask vertices
    స్థిరరాశి string m_name;  // Name of this AstExecGraph (for uniqueness at code generation)

బహిరంగ:
    Ast_Exec_లేఖాచిత్రం(File_పంక్తి* fl, స్థిరరాశి string& name, AstVarScope* trigVscp) VL_MT_DISABLED;
    ~Ast_Exec_లేఖాచిత్రం() override;
    ASTGEN_MEMBERS_Ast_Exec_లేఖాచిత్రం;
    శూన్య cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    V3_లేఖాచిత్రం* depGraphp() { తిరిగి m_depGraphp; }
    స్థిరరాశి V3_లేఖాచిత్రం* depGraphp() స్థిరరాశి { తిరిగి m_depGraphp; }
    AstCFunc* runfuncp() స్థిరరాశి { తిరిగి m_runfuncp; }
    శూన్య runfuncp(AstCFunc* funcp) { m_runfuncp = funcp; }
};
class AstFClose final : బహిరంగ Ast_కణుపు_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNodeExpr // file (must be a VarRef)
బహిరంగ:
    AstFClose(File_పంక్తి* fl, Ast_కణుపు_సమాసం* filep)
        : ASTGEN_SUPER_FClose(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFClose;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$fclose"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstFFlush final : బహిరంగ Ast_కణుపు_Stmt {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNodeExpr] // file (must be a VarRef)
బహిరంగ:
    AstFFlush(File_పంక్తి* fl, Ast_కణుపు_సమాసం* filep)
        : ASTGEN_SUPER_FFlush(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_AstFFlush;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$fflush"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_ముగింపు final : బహిరంగ Ast_కణుపు_Stmt {
బహిరంగ:
    explicit Ast_ముగింపు(File_పంక్తి* fl)
        : ASTGEN_SUPER_ముగింపు(fl) {}
    ASTGEN_MEMBERS_Ast_ముగింపు;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: $display makes output
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }  // Rarely executes
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి fileline() == samep->fileline(); }
};
class Ast_ముగింపు_Fork final : బహిరంగ Ast_కణుపు_Stmt {
    // $finish in fork
బహిరంగ:
    explicit Ast_ముగింపు_Fork(File_పంక్తి* fl)
        : ASTGEN_SUPER_ముగింపు_Fork(fl) {}
    ASTGEN_MEMBERS_Ast_ముగింపు_Fork;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: $display makes output
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }  // Rarely executes
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి fileline() == samep->fileline(); }
};
class AstFireEvent final : బహిరంగ Ast_కణుపు_Stmt {
    // '-> _' and '->> _' event trigger statements
    // @astgen op1 := operandp : AstNodeExpr
    స్థిరరాశి bool m_delayed;  // Delayed (->>) vs non-delayed (->)
బహిరంగ:
    AstFireEvent(File_పంక్తి* fl, Ast_కణుపు_సమాసం* operandp, bool delayed)
        : ASTGEN_SUPER_FireEvent(fl)
        , m_delayed{delayed} {
        this->operandp(operandp);
    }
    ASTGEN_MEMBERS_AstFireEvent;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isDelayed() స్థిరరాశి { తిరిగి m_delayed; }
};
class AstInitialAutomaticStmt final : బహిరంగ Ast_కణుపు_Stmt {
    // Automatic variable initialization in a statement position
    // Used during early stages to record an initial initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialAutomatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
బహిరంగ:
    AstInitialAutomaticStmt(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_InitialAutomaticStmt(fl) {
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialAutomaticStmt;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool is_శుద్ధ() override { తిరిగి true; }
};
class AstInitialStaticStmt final : బహిరంగ Ast_కణుపు_Stmt {
    // Static variable initialization in a statement position
    // Used during early stages to record a static initialization of a variable
    // Moves later to an appropriate constructor, or AstInitialStatic, or
    // AstCFunc normal statement
    // Children: {statement list usually only with assignments}
    // @astgen op1 := stmtsp : List[AstNode]
బహిరంగ:
    AstInitialStaticStmt(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_InitialStaticStmt(fl) {
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstInitialStaticStmt;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool is_శుద్ధ() override { తిరిగి true; }
};
class Ast_Jump_ఖండం final : బహిరంగ Ast_కణుపు_Stmt {
    // Block of code that might contain AstJumpGo statements as children,
    // which when exectued branch to right after the referenced AstJumpBlock.
    // AstJumpBlocks can nest, and an AstJumpGo can reference any of the
    // enclosing AstJumpBlocks (can break out of mulitple levels).
    // Parents:  {statement list}
    // Children: {statement list, with JumpGo below}
    // @astgen op1 := stmtsp : List[AstNode]
    VIsCached m_purity;  // Pure state
బహిరంగ:
    // After construction must call ->labelp to associate with appropriate label
    Ast_Jump_ఖండం(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_Jump_ఖండం(fl) {
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_Ast_Jump_ఖండం;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool is_శుద్ధ() override;

అంతరంగ:
    bool getPurityRecurse() స్థిరరాశి;
};
class AstJumpGo final : బహిరంగ Ast_కణుపు_Stmt {
    // Branch to right after the referenced encloding AstJumpBlock
    // Parents:  statement, including the referenced AstJumpBlock
    // Children: none
    //
    // @astgen ptr := m_blockp : AstJumpBlock  // The AstJumpBlock we are branching after
బహిరంగ:
    AstJumpGo(File_పంక్తి* fl, Ast_Jump_ఖండం* blockp)
        : ASTGEN_SUPER_JumpGo(fl)
        , m_blockp{blockp} {}
    ASTGEN_MEMBERS_AstJumpGo;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి blockp() == VN_DBG_AS(samep, JumpGo)->blockp();
    }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isBrancher() స్థిరరాశి override {
        తిరిగి true;  // SPECIAL: We don't process code after breaks
    }
    Ast_Jump_ఖండం* blockp() స్థిరరాశి { తిరిగి m_blockp; }
};
class AstLoop final : బహిరంగ Ast_కణుపు_Stmt {
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
బహిరంగ:
    explicit AstLoop(File_పంక్తి* fl, Ast_కణుపు* stmtsp = nullptr)
        : ASTGEN_SUPER_Loop(fl) {
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstLoop;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* thatp) స్థిరరాశి override {
        తిరిగి m_unroll == VN_DBG_AS(thatp, Loop)->m_unroll;
    }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    // ACCESSORS
    VOptionBool unroll() స్థిరరాశి { తిరిగి m_unroll; }
    శూన్య unroll(స్థిరరాశి VOptionBool flag) { m_unroll = flag; }
};
class AstLoopTest final : బహిరంగ Ast_కణుపు_Stmt {
    // The condition test inside an AstLoop. If the condition is true,
    // execution continues after this AstLoopTest statement. If the condition
    // is false, control is transfered to after the corresponding AstLoop.
    // In other words: AstLoopTest is like a conditional 'break' statement,
    // which breaks out of the loop if the condition is false.
    // @astgen op1 := condp : AstNodeExpr  // The loop condition
    // @astgen ptr := m_loopp : AstLoop  // The corresponding AstLoop
బహిరంగ:
    AstLoopTest(File_పంక్తి* fl, AstLoop* loopp, Ast_కణుపు_సమాసం* condp)
        : ASTGEN_SUPER_LoopTest(fl)
        , m_loopp{loopp} {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstLoopTest;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isBrancher() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }
    // ACCESSORS
    AstLoop* loopp() స్థిరరాశి { తిరిగి m_loopp; }
};
class AstMonitorOff final : బహిరంగ Ast_కణుపు_Stmt {
    స్థిరరాశి bool m_off;  // Monitor off.  Using 0=on allows faster init and comparison

బహిరంగ:
    AstMonitorOff(File_పంక్తి* fl, bool off)
        : ASTGEN_SUPER_MonitorOff(fl)
        , m_off{off} {}
    ASTGEN_MEMBERS_AstMonitorOff;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string verilogKwd() స్థిరరాశి override { తిరిగి m_off ? "$monitoroff" : "$monitoron"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }  // Though deleted before opt
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }  // Though deleted before opt
    bool is_శుద్ధ() override { తిరిగి false; }  // Though deleted before opt
    bool isOutputter() override { తిరిగి true; }  // Though deleted before opt
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_off == VN_DBG_AS(samep, MonitorOff)->m_off;
    }
    bool off() స్థిరరాశి { తిరిగి m_off; }
};
class AstPExprClause final : బహిరంగ Ast_కణుపు_Stmt {
    స్థిరరాశి bool m_pass;  // Will be replaced by passing assertion clause, false for
                        // assertion failure clause
    స్థిరరాశి bool m_vacuous;  // Pass is vacuous

బహిరంగ:
    ASTGEN_MEMBERS_AstPExprClause;
    explicit AstPExprClause(File_పంక్తి* fl, bool pass = true, bool vacuous = false)
        : ASTGEN_SUPER_PExprClause(fl)
        , m_pass{pass}
        , m_vacuous{vacuous} {}
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool pass() స్థిరరాశి { తిరిగి m_pass; }
    bool vacuous() స్థిరరాశి { తిరిగి m_vacuous; }
};
class Ast_Print_సమయం_Scale final : బహిరంగ Ast_కణుపు_Stmt {
    // Parents: stmtlist
    string m_name;  // Parent module name
    VTimescale m_timeunit;  // Parent module time unit
బహిరంగ:
    explicit Ast_Print_సమయం_Scale(File_పంక్తి* fl)
        : ASTGEN_SUPER_Print_సమయం_Scale(fl) {}
    ASTGEN_MEMBERS_Ast_Print_సమయం_Scale;
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$printtimescale"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
};
class AstRSBreak final : బహిరంగ Ast_కణుపు_Stmt {
    // randsequence break
బహిరంగ:
    explicit AstRSBreak(File_పంక్తి* fl)
        : ASTGEN_SUPER_RSBreak(fl) {}
    ASTGEN_MEMBERS_AstRSBreak;
    string verilogKwd() స్థిరరాశి override { తిరిగి "break"; }
    bool isBrancher() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSProd final : బహిరంగ Ast_కణుపు_Stmt {
    // randomsequence production, under a AstRandSequence
    // @astgen op1 := fvarp : Optional[AstVar]
    // @astgen op2 := portsp : List[AstNode]
    // @astgen op3 := rulesp : List[AstRSRule]
    string m_name;  // Name of block, or "" to use first production
బహిరంగ:
    AstRSProd(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* portsp, AstRSRule* rulesp)
        : ASTGEN_SUPER_RSProd(fl)
        , m_name{name} {
        కూడు_Portsp(portsp);
        కూడు_Rulesp(rulesp);
    }
    ASTGEN_MEMBERS_AstRSProd;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
};
class Ast_RSProd_అంశం final : బహిరంగ Ast_కణుపు_Stmt {
    // randomsquence production item
    // @astgen op1 := argsp : List[AstArg]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to production
    string m_name;  // Name of block, or "" to use first production
బహిరంగ:
    Ast_RSProd_అంశం(File_పంక్తి* fl, స్థిరరాశి string& name, AstArg* argsp)
        : ASTGEN_SUPER_RSProd_అంశం(fl)
        , m_name{name} {
        కూడు_Argsp(argsp);
    }
    ASTGEN_MEMBERS_Ast_RSProd_అంశం;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    AstRSProd* prodp() స్థిరరాశి { తిరిగి m_prodp; }
    శూన్య prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRSProdList final : బహిరంగ Ast_కణుపు_Stmt {
    // randomsquence production list
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodsp : List[AstNode]
    bool m_randJoin = false;  // Is rand join'ed
బహిరంగ:
    AstRSProdList(File_పంక్తి* fl, Ast_కణుపు_సమాసం* weightp, Ast_కణుపు* prodsp)
        : ASTGEN_SUPER_RSProdList(fl) {
        this->weightp(weightp);
        కూడు_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRSProdList;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    bool randJoin() స్థిరరాశి { తిరిగి m_randJoin; }
    శూన్య randJoin(bool flag) { m_randJoin = flag; }
};
class AstRSReturn final : బహిరంగ Ast_కణుపు_Stmt {
    // randsequence return
బహిరంగ:
    explicit AstRSReturn(File_పంక్తి* fl)
        : ASTGEN_SUPER_RSReturn(fl) {}
    ASTGEN_MEMBERS_AstRSReturn;
    string verilogKwd() స్థిరరాశి override { తిరిగి "return"; }
    bool isBrancher() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstRSRule final : బహిరంగ Ast_కణుపు_Stmt {
    // randomsquence rule
    // @astgen op1 := weightp : Optional[AstNodeExpr]
    // @astgen op2 := prodlistsp : List[AstRSProdList]
    // @astgen op3 := weightStmtsp : List[AstNode]
బహిరంగ:
    AstRSRule(File_పంక్తి* fl, Ast_కణుపు_సమాసం* weightp, AstRSProdList* prodlistsp, Ast_కణుపు* బరువు_Stmtsp)
        : ASTGEN_SUPER_RSRule(fl) {
        this->weightp(weightp);
        కూడు_Prodlistsp(prodlistsp);
        కూడు_బరువు_Stmtsp(బరువు_Stmtsp);
    }
    ASTGEN_MEMBERS_AstRSRule;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
};
class AstRandCase final : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op2 := itemsp : List[AstCaseItem]
బహిరంగ:
    AstRandCase(File_పంక్తి* fl, Ast_Case_అంశం* itemsp)
        : ASTGEN_SUPER_RandCase(fl) {
        కూడు_Itemsp(itemsp);
        v3Global.useRandomizeMethods(true);
    }
    ASTGEN_MEMBERS_AstRandCase;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
};
class AstRandSequence final : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op2 := prodsp : List[AstRSProd]
    // @astgen ptr := m_prodp : Optional[AstRSProd]  // Pointer to start production (if any)
    string m_name;  // Created unique name
    string m_ప్రారంభం;  // Name of start production, or "" to use first production
బహిరంగ:
    AstRandSequence(File_పంక్తి* fl, స్థిరరాశి string& ప్రారంభం, AstRSProd* prodsp)
        : ASTGEN_SUPER_RandSequence(fl)
        , m_ప్రారంభం{ప్రారంభం} {
        కూడు_Prodsp(prodsp);
    }
    ASTGEN_MEMBERS_AstRandSequence;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Block name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string ప్రారంభం() స్థిరరాశి VL_MT_STABLE { తిరిగి m_ప్రారంభం; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    AstRSProd* prodp() స్థిరరాశి { తిరిగి m_prodp; }
    శూన్య prodp(AstRSProd* nodep) { m_prodp = nodep; }
};
class AstRelease final : బహిరంగ Ast_కణుపు_Stmt {
    // Procedural 'release' statement
    // @astgen op1 := lhsp : AstNodeExpr
బహిరంగ:
    AstRelease(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_Release(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRelease;
};
class AstRepeat final : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op1 := countp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
బహిరంగ:
    AstRepeat(File_పంక్తి* fl, Ast_కణుపు_సమాసం* countp, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_Repeat(fl) {
        this->countp(countp);
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstRepeat;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }  // Not relevant - converted to FOR
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_BRANCH; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstReturn final : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op1 := lhsp : Optional[AstNodeExpr]
బహిరంగ:
    explicit AstReturn(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp = nullptr)
        : ASTGEN_SUPER_Return(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstReturn;
    string verilogKwd() స్థిరరాశి override { తిరిగి "return"; }
    bool isBrancher() స్థిరరాశి override { V3ERROR_NA_RETURN(true); }  // Node removed early
};
class AstSFormat final : బహిరంగ Ast_కణుపు_Stmt {
    // Parents: statement container
    // @astgen op1 := fmtp : AstSFormatF
    // @astgen op2 := lhsp : AstNodeExpr
బహిరంగ:
    AstSFormat(File_పంక్తి* fl, bool optionalFormat, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* exprsp,
               అక్షరం missingArgChar = 'd')
        : ASTGEN_SUPER_SFormat(fl) {
        AstSFormatF* స్థిరరాశి newp
            = new AstSFormatF{fl, AstSFormatF::సమాసం_Format{}, exprsp, missingArgChar};
        newp->optionalFormat(optionalFormat);
        fmtp(newp);
        this->lhsp(lhsp);
    }
    AstSFormat(File_పంక్తి* fl, AstSFormatF* fmtp, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_SFormat(fl) {
        this->fmtp(fmtp);
        this->lhsp(lhsp);
        fmtp->hidden(true);
    }
    ASTGEN_MEMBERS_AstSFormat;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!fmtp());
        తిరిగి nullptr;
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "$sformat"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి true; }
    bool is_శుద్ధ() override { తిరిగి true; }
    bool isOutputter() override { తిరిగి false; }
    మాయా bool clean_బహిర్గతం() స్థిరరాశి { తిరిగి false; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstSetuphold final : బహిరంగ Ast_కణుపు_Stmt {
    // Verilog $setuphold
    // @astgen op1 := refevp : AstNodeExpr
    // @astgen op2 := dataevp : AstNodeExpr
    // @astgen op3 := delrefp : Optional[AstNodeExpr]
    // @astgen op4 := deldatap : Optional[AstNodeExpr]
బహిరంగ:
    AstSetuphold(File_పంక్తి* fl, Ast_కణుపు_సమాసం* refevp, Ast_కణుపు_సమాసం* dataevp,
                 Ast_కణుపు_సమాసం* delrefp = nullptr, Ast_కణుపు_సమాసం* deldatap = nullptr)
        : ASTGEN_SUPER_Setuphold(fl) {
        this->refevp(refevp);
        this->dataevp(dataevp);
        this->delrefp(delrefp);
        this->deldatap(deldatap);
    }
    ASTGEN_MEMBERS_AstSetuphold;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_దొంతర_Trace_T final : బహిరంగ Ast_కణుపు_Stmt {
    // $stacktrace used as task
బహిరంగ:
    explicit Ast_దొంతర_Trace_T(File_పంక్తి* fl)
        : ASTGEN_SUPER_దొంతర_Trace_T(fl) {}
    ASTGEN_MEMBERS_Ast_దొంతర_Trace_T;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$stacktrace"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_Stmt_సమాసం final : బహిరంగ Ast_కణుపు_Stmt {
    // Expression in statement position
    // @astgen op1 := exprp : AstNodeExpr
బహిరంగ:
    Ast_Stmt_సమాసం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_Stmt_సమాసం(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Ast_Stmt_సమాసం;
    // cppcheck-suppress uselessOverride
    bool is_శుద్ధ() override { తిరిగి exprp()->is_శుద్ధ(); }
};
class AstStmtPragma final : బహిరంగ Ast_కణుపు_Stmt {
    // Pragma in statement position
    // @astgen op1 := pragp : AstPragma
బహిరంగ:
    AstStmtPragma(File_పంక్తి* fl, AstPragma* pragp)
        : ASTGEN_SUPER_StmtPragma(fl) {
        this->pragp(pragp);
    }
    ASTGEN_MEMBERS_AstStmtPragma;
};
class AstStop final : బహిరంగ Ast_కణుపు_Stmt {
    స్థిరరాశి bool m_isFatal;  // $fatal not $stop
బహిరంగ:
    AstStop(File_పంక్తి* fl, bool isFatal)
        : ASTGEN_SUPER_Stop(fl)
        , m_isFatal{isFatal} {}
    ASTGEN_MEMBERS_AstStop;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: $display makes output
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 0; }  // Rarely executes
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి fileline() == samep->fileline(); }
    string emitVerilog() స్థిరరాశి { తిరిగి m_isFatal ? "$fatal" : "$stop"; }
    bool isFatal() స్థిరరాశి { తిరిగి m_isFatal; }
};
class AstSystemT final : బహిరంగ Ast_కణుపు_Stmt {
    // $system used as task
    // @astgen op1 := lhsp : AstNodeExpr
బహిరంగ:
    AstSystemT(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp)
        : ASTGEN_SUPER_SystemT(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstSystemT;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$system"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_సమయం_Format final : బహిరంగ Ast_కణుపు_Stmt {
    // Parents: stmtlist
    // @astgen op1 := unitsp : Optional[AstNodeExpr]
    // @astgen op2 := precisionp : Optional[AstNodeExpr]
    // @astgen op3 := suffixp : Optional[AstNodeExpr]
    // @astgen op4 := widthp : Optional[AstNodeExpr]
బహిరంగ:
    Ast_సమయం_Format(File_పంక్తి* fl, Ast_కణుపు_సమాసం* unitsp, Ast_కణుపు_సమాసం* precisionp, Ast_కణుపు_సమాసం* suffixp,
                  Ast_కణుపు_సమాసం* widthp)
        : ASTGEN_SUPER_సమయం_Format(fl) {
        this->unitsp(unitsp);
        this->precisionp(precisionp);
        this->suffixp(suffixp);
        this->widthp(widthp);
    }
    ASTGEN_MEMBERS_Ast_సమయం_Format;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$timeformat"; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
};
class AstTraceDecl final : బహిరంగ Ast_కణుపు_Stmt {
    // Trace point declaration
    // Separate from AstTraceInc; as a declaration can't be deleted
    // Parents:  {statement list}
    // Expression being traced - Moved to AstTraceInc by V3Trace
    // @astgen op1 := valuep : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_dtypeCallp: Optional[AstCCall] // Type init function call
    // @astgen ptr := m_dtypeDeclp: Optional[AstTraceDecl] // CCall TraceDecl which replaces this
    uint32_t m_సంకేతం{std::numeric_limits<uint32_t>::max()};  // Trace identifier code
    uint32_t m_fidx{0};  // Trace function index
    స్థిరరాశి string m_showname;  // Name of variable
    స్థిరరాశి VNumRange m_బిట్టు_Range;  // Property of var the trace details
    స్థిరరాశి VNumRange m_arrayRange;  // Property of var the trace details
    స్థిరరాశి VVarType m_varType;  // Type of variable (for localparam vs. param)
    స్థిరరాశి VDirection m_declDirection;  // Declared direction input/output etc
    స్థిరరాశి bool m_లోపల_Dtype_Func;  // Trace decl inside type init function
    // dist-ast-dump-suppress  // Not stable and of low value
    పూర్ణాంకం m_సంకేతం_Inc{0};  // Code increment for type
బహిరంగ:
    AstTraceDecl(File_పంక్తి* fl, స్థిరరాశి string& showname,
                 AstVar* varp,  // For input/output state etc
                 Ast_కణుపు_సమాసం* valuep, స్థిరరాశి VNumRange& బిట్టు_Range, స్థిరరాశి VNumRange& arrayRange,
                 AstCCall* స్థిరరాశి dtypeCallp, స్థిరరాశి bool లోపల_Dtype_Func)
        : ASTGEN_SUPER_TraceDecl(fl)
        , m_showname{showname}
        , m_బిట్టు_Range{బిట్టు_Range}
        , m_arrayRange{arrayRange}
        , m_varType{varp->varType()}
        , m_declDirection{varp->declDirection()}
        , m_లోపల_Dtype_Func{లోపల_Dtype_Func} {
        dtypeFrom(valuep);
        this->valuep(valuep);
        this->dtypeCallp(dtypeCallp);
    }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 100; }  // Large...
    ASTGEN_MEMBERS_AstTraceDecl;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_showname; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి true; }
    string showname() స్థిరరాశి { తిరిగి m_showname; }  // * = Var name
    // Details on what we're tracing
    uint32_t సంకేతం_2() స్థిరరాశి { తిరిగి m_సంకేతం; }
    శూన్య సంకేతం_2(uint32_t సంకేతం_2) { m_సంకేతం = సంకేతం_2; }
    bool సంకేతం_Assigned() స్థిరరాశి { తిరిగి m_సంకేతం != std::numeric_limits<uint32_t>::max(); }
    uint32_t fidx() స్థిరరాశి { తిరిగి m_fidx; }
    శూన్య fidx(uint32_t fidx) { m_fidx = fidx; }
    శూన్య సంకేతం_Inc(uint32_t సంకేతం_Inc) { m_సంకేతం_Inc = సంకేతం_Inc; }
    uint32_t సంకేతం_Inc() స్థిరరాశి {
        ఒకవేళ (m_సంకేతం_Inc) { తిరిగి m_సంకేతం_Inc; }
        తిరిగి (m_arrayRange.ranged() ? m_arrayRange.elements() : 1)
               * valuep()->dtypep()->వెడల్పు_పదాలు()
               * (VL_EDATASIZE / 32);  // A code is always 32-bits
    }
    స్థిరరాశి VNumRange& బిట్టు_Range() స్థిరరాశి { తిరిగి m_బిట్టు_Range; }
    స్థిరరాశి VNumRange& arrayRange() స్థిరరాశి { తిరిగి m_arrayRange; }
    VVarType varType() స్థిరరాశి { తిరిగి m_varType; }
    VDirection declDirection() స్థిరరాశి { తిరిగి m_declDirection; }
    AstCCall* dtypeCallp() స్థిరరాశి { తిరిగి m_dtypeCallp; }
    శూన్య dtypeCallp(AstCCall* స్థిరరాశి callp) { m_dtypeCallp = callp; }
    AstTraceDecl* dtypeDeclp() స్థిరరాశి { తిరిగి m_dtypeDeclp; }
    శూన్య dtypeDeclp(AstTraceDecl* స్థిరరాశి declp) { m_dtypeDeclp = declp; }
    bool లోపల_Dtype_Func() స్థిరరాశి { తిరిగి m_లోపల_Dtype_Func; }
};
class AstTraceInc final : బహిరంగ Ast_కణుపు_Stmt {
    // Trace point dump
    // @astgen op1 := valuep : AstNodeExpr // Expression being traced (from decl)
    //
    // @astgen ptr := m_declp : AstTraceDecl  // Pointer to declaration
    స్థిరరాశి uint32_t m_పునాది_సంకేతం;  // Trace code base value in function containing this AstTraceInc
    స్థిరరాశి VTraceType m_traceType;  // Is this a const/full/incremental dump

బహిరంగ:
    AstTraceInc(File_పంక్తి* fl, AstTraceDecl* declp, VTraceType traceType, uint32_t పునాది_సంకేతం = 0)
        : ASTGEN_SUPER_TraceInc(fl)
        , m_పునాది_సంకేతం{పునాది_సంకేతం}
        , m_traceType{traceType}
        , m_declp{declp} {
        dtypeFrom(declp);
        // Note: A clone is necessary (instead of using declp()->valuep()),
        // for insertion of local temporaries in V3Premit
        valuep(declp->valuep()->clone_వృక్షం(true));
    }
    ASTGEN_MEMBERS_AstTraceInc;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 10 + 2 * INSTR_లెక్క_LD; }
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి declp() == VN_DBG_AS(samep, TraceInc)->declp();
    }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
    bool is_శుద్ధ() override { తిరిగి false; }
    AstTraceDecl* declp() స్థిరరాశి { తిరిగి m_declp; }
    VTraceType traceType() స్థిరరాశి { తిరిగి m_traceType; }
    uint32_t పునాది_సంకేతం() స్థిరరాశి { తిరిగి m_పునాది_సంకేతం; }
};
class Ast_Trace_తీయి_ఉపసర్గ final : బహిరంగ Ast_కణుపు_Stmt {
బహిరంగ:
    explicit Ast_Trace_తీయి_ఉపసర్గ(File_పంక్తి* fl)
        : ASTGEN_SUPER_Trace_తీయి_ఉపసర్గ(fl) {}
    ASTGEN_MEMBERS_Ast_Trace_తీయి_ఉపసర్గ;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి false; }
};
class Ast_Trace_నెట్టు_ఉపసర్గ final : బహిరంగ Ast_కణుపు_Stmt {
    స్థిరరాశి string m_ఉపసర్గ;  // Prefix to add to signal names
    స్థిరరాశి VTrace_ఉపసర్గ_Type m_ఉపసర్గ_Type;  // Type of prefix being pushed
    స్థిరరాశి పూర్ణాంకం m_ఎడమ;  // Array left index, or struct/union member count
    స్థిరరాశి పూర్ణాంకం m_కుడి;  // Array right index
    స్థిరరాశి bool m_quoted_ఉపసర్గ;  // Quote prefix name
బహిరంగ:
    Ast_Trace_నెట్టు_ఉపసర్గ(File_పంక్తి* fl, స్థిరరాశి string& ఉపసర్గ, VTrace_ఉపసర్గ_Type ఉపసర్గ_Type,
                       పూర్ణాంకం ఎడమ_2 = 0, పూర్ణాంకం కుడి_2 = 0, bool quoted_ఉపసర్గ = true)
        : ASTGEN_SUPER_Trace_నెట్టు_ఉపసర్గ(fl)
        , m_ఉపసర్గ{ఉపసర్గ}
        , m_ఉపసర్గ_Type{ఉపసర్గ_Type}
        , m_ఎడమ{ఎడమ_2}
        , m_కుడి{కుడి_2}
        , m_quoted_ఉపసర్గ{quoted_ఉపసర్గ} {}
    ASTGEN_MEMBERS_Ast_Trace_నెట్టు_ఉపసర్గ;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override { తిరిగి false; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string ఉపసర్గ() స్థిరరాశి { తిరిగి m_ఉపసర్గ; }
    VTrace_ఉపసర్గ_Type ఉపసర్గ_Type() స్థిరరాశి { తిరిగి m_ఉపసర్గ_Type; }
    పూర్ణాంకం ఎడమ_2() స్థిరరాశి { తిరిగి m_ఎడమ; }
    పూర్ణాంకం కుడి_2() స్థిరరాశి { తిరిగి m_కుడి; }
    bool quoted_ఉపసర్గ() స్థిరరాశి { తిరిగి m_quoted_ఉపసర్గ; }
};
class AstWait final : బహిరంగ Ast_కణుపు_Stmt {
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := stmtsp : List[AstNode]
బహిరంగ:
    AstWait(File_పంక్తి* fl, Ast_కణుపు_సమాసం* condp, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_Wait(fl) {
        this->condp(condp);
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstWait;
    bool isTimingControl() స్థిరరాశి override { తిరిగి true; }
};
class AstWaitFork final : బహిరంగ Ast_కణుపు_Stmt {
    // A "wait fork" statement
బహిరంగ:
    explicit AstWaitFork(File_పంక్తి* fl)
        : ASTGEN_SUPER_WaitFork(fl) {}
    ASTGEN_MEMBERS_AstWaitFork;
    bool isTimingControl() స్థిరరాశి override { తిరిగి true; }
    string verilogKwd() స్థిరరాశి override { తిరిగి "wait fork"; }
};

// === AstNodeAssign ===
class Ast_కేటాయించు final : బహిరంగ Ast_కణుపు_కేటాయించు {
బహిరంగ:
    Ast_కేటాయించు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp,
              Ast_కణుపు* timingControlp = nullptr)
        : ASTGEN_SUPER_కేటాయించు(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_కేటాయించు;
    Ast_కణుపు_కేటాయించు* cloneType(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) override {
        Ast_కణుపు* స్థిరరాశి controlp = timingControlp() ? timingControlp()->clone_వృక్షం(false) : nullptr;
        తిరిగి new Ast_కేటాయించు{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_కేటాయించు_Compound final : బహిరంగ Ast_కణుపు_కేటాయించు {
    // Compound assignments (+=, -=, *=, ...)
బహిరంగ:
    గణన operation : uint8_t {
        కూడు_2,
        And,
        Div,
        ModDiv,
        Mul,
        Or,
        జరుపు_L,
        జరుపు_R,
        జరుపు_RS,
        Sub,
        Xor,
    };

అంతరంగ:
    operation m_operation;

బహిరంగ:
    Ast_కేటాయించు_Compound(Ast_కేటాయించు_Compound::operation operation, File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp,
                      Ast_కణుపు_సమాసం* rhsp, Ast_కణుపు* timingControlp = nullptr)
        : ASTGEN_SUPER_కేటాయించు_Compound(fl, lhsp, rhsp, timingControlp) {
        this->m_operation = operation;
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_కేటాయించు_Compound;
    Ast_కణుపు_కేటాయించు* cloneType(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) override {
        Ast_కణుపు* స్థిరరాశి controlp = timingControlp() ? timingControlp()->clone_వృక్షం(false) : nullptr;
        తిరిగి new Ast_కేటాయించు_Compound{operation(), fileline(), lhsp, rhsp, controlp};
    }
    operation operation() { తిరిగి m_operation; }
};
class Ast_కేటాయించు_Cont final : బహిరంగ Ast_కణుపు_కేటాయించు {
    // Continuous procedural 'assign'.  See AstAssignW for non-procedural version.
బహిరంగ:
    Ast_కేటాయించు_Cont(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp,
                  Ast_కణుపు* timingControlp = nullptr)
        : ASTGEN_SUPER_కేటాయించు_Cont(fl, lhsp, rhsp, timingControlp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Ast_కేటాయించు_Cont;
    Ast_కణుపు_కేటాయించు* cloneType(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) override {
        Ast_కణుపు* స్థిరరాశి controlp = timingControlp() ? timingControlp()->clone_వృక్షం(false) : nullptr;
        తిరిగి new Ast_కేటాయించు_Cont{fileline(), lhsp, rhsp, controlp};
    }
};
class Ast_కేటాయించు_Dly final : బహిరంగ Ast_కణుపు_కేటాయించు {
బహిరంగ:
    Ast_కేటాయించు_Dly(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp,
                 Ast_కణుపు* timingControlp = nullptr)
        : ASTGEN_SUPER_కేటాయించు_Dly(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_కేటాయించు_Dly;
    Ast_కణుపు_కేటాయించు* cloneType(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) override {
        Ast_కణుపు* స్థిరరాశి controlp = timingControlp() ? timingControlp()->clone_వృక్షం(false) : nullptr;
        తిరిగి new Ast_కేటాయించు_Dly{fileline(), lhsp, rhsp, controlp};
    }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    string verilogKwd() స్థిరరాశి override { తిరిగి "<="; }
};
class Ast_కేటాయించు_Force final : బహిరంగ Ast_కణుపు_కేటాయించు {
    // Procedural 'force' statement
బహిరంగ:
    Ast_కేటాయించు_Force(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_కేటాయించు_Force(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Ast_కేటాయించు_Force;
    Ast_కణుపు_కేటాయించు* cloneType(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) override {
        తిరిగి new Ast_కేటాయించు_Force{fileline(), lhsp, rhsp};
    }
};
class Ast_కేటాయించు_W final : బహిరంగ Ast_కణుపు_కేటాయించు {
    // Like assign, but wire/assign's in verilog, the only setting of the specified variable
    // @astgen op4 := strengthSpecp : Optional[AstStrengthSpec]
బహిరంగ:
    Ast_కేటాయించు_W(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp,
               Ast_కణుపు* timingControlp = nullptr)
        : ASTGEN_SUPER_కేటాయించు_W(fl, lhsp, rhsp, timingControlp) {}
    ASTGEN_MEMBERS_Ast_కేటాయించు_W;
    Ast_కణుపు_కేటాయించు* cloneType(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp) override {
        Ast_కణుపు* స్థిరరాశి controlp = timingControlp() ? timingControlp()->clone_వృక్షం(false) : nullptr;
        తిరిగి new Ast_కేటాయించు_W{fileline(), lhsp, rhsp, controlp};
    }
    bool isTimingControl() స్థిరరాశి override {
        తిరిగి timingControlp() || lhsp()->exists([](స్థిరరాశి Ast_కణుపు_Var_Ref* refp) {
            తిరిగి refp->access().isWriteOrRW() && refp->varp()->delayp();
        });
    }
    AstDelay* getLhsNetDelay() స్థిరరాశి;
};

// === AstNodeBlock ===
class AstBegin final : బహిరంగ Ast_కణుపు_ఖండం {
    // A 'begin'/'end' named block.
    bool m_needProcess : 1;  // Uses VlProcess
    స్థిరరాశి bool m_implied : 1;  // Not inserted by user
బహిరంగ:
    AstBegin(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* stmtsp, bool implied)
        : ASTGEN_SUPER_Begin(fl, name)
        , m_needProcess{false}
        , m_implied{implied} {
        కూడు_Stmtsp(stmtsp);
    }
    ASTGEN_MEMBERS_AstBegin;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    శూన్య సమితి_Need_Process() { m_needProcess = true; }
    bool needProcess() స్థిరరాశి { తిరిగి m_needProcess; }
    bool implied() స్థిరరాశి { తిరిగి m_implied; }
};
class AstFork final : బహిరంగ Ast_కణుపు_ఖండం {
    // A 'fork'/'join*' named block. Note that this is a strict superset of
    // AstBegin, and it does contain AstNodeBlock::stmtsp(), which are
    // statements that execute sequentially before the parallel statements are
    // spawned. This is necessary to implement things like local variable
    // initializers properly. The parallel statements inside the fork must all
    // be AstBegin, as lowering stages will introduce additional statements to
    // be executed sequentially within each fork branch.
    //
    // @astgen op3 := forksp : List[AstBegin]
    స్థిరరాశి VJoinType m_joinType;  // Join keyword type
    bool m_తక్షణ_ప్రారంభం = false;  // Fork starts before its parent blocks or exits

బహిరంగ:
    AstFork(File_పంక్తి* fl, VJoinType joinType, స్థిరరాశి string& name = "")
        : ASTGEN_SUPER_Fork(fl, name)
        , m_joinType{joinType} {}
    ASTGEN_MEMBERS_AstFork;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstFork* స్థిరరాశి asamep = VN_DBG_AS(samep, Fork);
        తిరిగి joinType() == asamep->joinType() && తక్షణ_ప్రారంభం() == asamep->తక్షణ_ప్రారంభం();
    }
    bool isTimingControl() స్థిరరాశి override { తిరిగి !joinType().joinNone(); }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    VJoinType joinType() స్థిరరాశి { తిరిగి m_joinType; }
    bool తక్షణ_ప్రారంభం() స్థిరరాశి { తిరిగి m_తక్షణ_ప్రారంభం; }
    శూన్య తక్షణ_ప్రారంభం(bool flag) { m_తక్షణ_ప్రారంభం = flag; }
};

// === AstNodeCoverOrAssert ===
class AstAssert final : బహిరంగ Ast_కణుపు_Cover_Or_Assert {
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey

బహిరంగ:
    ASTGEN_MEMBERS_AstAssert;
    AstAssert(File_పంక్తి* fl, Ast_కణుపు* propp, Ast_కణుపు* passsp, Ast_కణుపు* failsp, VAssertType type,
              VAssertDirectiveType directive, స్థిరరాశి string& name = "")
        : ASTGEN_SUPER_Assert(fl, propp, passsp, type, directive, name) {
        కూడు_Failsp(failsp);
    }
    string verilogKwd() స్థిరరాశి override {
        తిరిగి directive() == VAssertDirectiveType::ASSERT ? "assert" : "assume";
    }
};
class AstAssertIntrinsic final : బహిరంగ Ast_కణుపు_Cover_Or_Assert {
    // A $cast or other compiler inserted assert, that must run even without --assert option
    // @astgen op3 := failsp: List[AstNode] // Statements when propp is failing/falsey
బహిరంగ:
    ASTGEN_MEMBERS_AstAssertIntrinsic;
    AstAssertIntrinsic(File_పంక్తి* fl, Ast_కణుపు* propp, Ast_కణుపు* passsp, Ast_కణుపు* failsp,
                       స్థిరరాశి string& name = "")
        // Intrinsic asserts are always enabled thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_AssertIntrinsic(fl, propp, passsp, VAssertType::అంతర్గత,
                                       VAssertDirectiveType::INTRINSIC, name) {
        కూడు_Failsp(failsp);
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "assert"; }
};
class AstCover final : బహిరంగ Ast_కణుపు_Cover_Or_Assert {
    // @astgen op3 := coverincsp: List[AstNode] // Coverage node
    bool m_isCoverSeq = false;  // 'cover sequence' (IEEE 1800-2023 16.14.3): fires per
                                // end-of-match, not per property success
    bool m_isSeqEvent = false;  // Synthesized for a sequence used as an event control
                                // (IEEE 1800-2023 9.4.2.4)
బహిరంగ:
    ASTGEN_MEMBERS_AstCover;
    AstCover(File_పంక్తి* fl, Ast_కణుపు* propp, Ast_కణుపు* stmtsp, VAssertType type,
             స్థిరరాశి string& name = "")
        : ASTGEN_SUPER_Cover(fl, propp, stmtsp, type, VAssertDirectiveType::COVER, name) {}
    string verilogKwd() స్థిరరాశి override { తిరిగి "cover"; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isCoverSeq() స్థిరరాశి { తిరిగి m_isCoverSeq; }
    శూన్య isCoverSeq(bool flag) { m_isCoverSeq = flag; }
    bool isSeqEvent() స్థిరరాశి { తిరిగి m_isSeqEvent; }
    శూన్య isSeqEvent(bool flag) { m_isSeqEvent = flag; }
};
class AstRestrict final : బహిరంగ Ast_కణుపు_Cover_Or_Assert {
బహిరంగ:
    ASTGEN_MEMBERS_AstRestrict;
    AstRestrict(File_పంక్తి* fl, Ast_కణుపు* propp)
        // Intrinsic asserts are always ignored thus 'type' field is set to INTERNAL.
        : ASTGEN_SUPER_Restrict(fl, propp, nullptr, VAssertType::అంతర్గత,
                                VAssertDirectiveType::RESTRICT) {}
    string verilogKwd() స్థిరరాశి override { తిరిగి "restrict"; }
};

// === AstNodeForeach ===
class AstConstraintForeach final : బహిరంగ Ast_కణుపు_Foreach {
    // Constraint foreach statement
    bool m_soft;  // is soft foreach, non-standard extension
బహిరంగ:
    AstConstraintForeach(File_పంక్తి* fl, Ast_Foreach_శీర్షిక* headerp, Ast_కణుపు* bodyp,
                         bool soft = false)
        : ASTGEN_SUPER_ConstraintForeach(fl, headerp, bodyp)
        , m_soft{soft} {}
    ASTGEN_MEMBERS_AstConstraintForeach;
    bool isSoft() స్థిరరాశి { తిరిగి m_soft; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class AstForeach final : బహిరంగ Ast_కణుపు_Foreach {
బహిరంగ:
    AstForeach(File_పంక్తి* fl, Ast_Foreach_శీర్షిక* headerp, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_Foreach(fl, headerp, stmtsp) {}
    ASTGEN_MEMBERS_AstForeach;
};

// === AstNodeIf ===
class AstConstraintIf final : బహిరంగ Ast_కణుపు_If {
బహిరంగ:
    AstConstraintIf(File_పంక్తి* fl, Ast_కణుపు_సమాసం* condp, Ast_కణుపు* thensp, Ast_కణుపు* elsesp)
        : ASTGEN_SUPER_ConstraintIf(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstConstraintIf;
};

class AstIf final : బహిరంగ Ast_కణుపు_If {
    bool m_uniquePragma = false;  // unique case
    bool m_unique0Pragma = false;  // unique0 case
    bool m_ప్రాధాన్యత_Pragma = false;  // priority case
బహిరంగ:
    AstIf(File_పంక్తి* fl, Ast_కణుపు_సమాసం* condp, Ast_కణుపు* thensp = nullptr, Ast_కణుపు* elsesp = nullptr)
        : ASTGEN_SUPER_If(fl, condp, thensp, elsesp) {}
    ASTGEN_MEMBERS_AstIf;
    bool uniquePragma() స్థిరరాశి { తిరిగి m_uniquePragma; }
    శూన్య uniquePragma(bool flag) { m_uniquePragma = flag; }
    bool unique0Pragma() స్థిరరాశి { తిరిగి m_unique0Pragma; }
    శూన్య unique0Pragma(bool flag) { m_unique0Pragma = flag; }
    bool ప్రాధాన్యత_Pragma() స్థిరరాశి { తిరిగి m_ప్రాధాన్యత_Pragma; }
    శూన్య ప్రాధాన్యత_Pragma(bool flag) { m_ప్రాధాన్యత_Pragma = flag; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};

// === AstNodeReadWriteMem ===
class AstReadMem final : బహిరంగ Ast_కణుపు_Read_Write_Mem {
బహిరంగ:
    AstReadMem(File_పంక్తి* fl, bool hex, Ast_కణుపు_సమాసం* filenamep, Ast_కణుపు_సమాసం* memp,
               Ast_కణుపు_సమాసం* lsbp, Ast_కణుపు_సమాసం* msbp)
        : ASTGEN_SUPER_ReadMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstReadMem;
    string verilogKwd() స్థిరరాశి override { తిరిగి (isHex() ? "$readmemh" : "$readmemb"); }
    స్థిరరాశి అక్షరం* cFuncPrefixp() స్థిరరాశి override { తిరిగి "VL_READMEM_"; }
};
class AstWriteMem final : బహిరంగ Ast_కణుపు_Read_Write_Mem {
బహిరంగ:
    AstWriteMem(File_పంక్తి* fl, bool hex, Ast_కణుపు_సమాసం* filenamep, Ast_కణుపు_సమాసం* memp,
                Ast_కణుపు_సమాసం* lsbp, Ast_కణుపు_సమాసం* msbp)
        : ASTGEN_SUPER_WriteMem(fl, hex, filenamep, memp, lsbp, msbp) {}
    ASTGEN_MEMBERS_AstWriteMem;
    string verilogKwd() స్థిరరాశి override { తిరిగి (isHex() ? "$writememh" : "$writememb"); }
    స్థిరరాశి అక్షరం* cFuncPrefixp() స్థిరరాశి override { తిరిగి "VL_WRITEMEM_"; }
};

#అయితే_ముగింపు  // Guard
