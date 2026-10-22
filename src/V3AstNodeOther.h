// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode sub-types representing other constructs
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
// This files contains all 'AstNode' sub-types that relate to other constructs
// not covered by the more specific V3AstNode*.h files.
//
//*************************************************************************

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ASTNODES_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ASTNODES_H_

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3AST_H_
#ದೋಷ "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ವ್ಯಾಖ್ಯಾನಿಸು VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_ಗಂಟು_Cons_ಅಂಶ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    ಮಿತ್ರ class Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ;  // For m_knownConstantTree
    mutable bool m_ತಿಳಿದ_Constant_ವೃಕ್ಷ = false;  // Known to be a tree of constants

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Cons_ಅಂಶ(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : Ast_ಗಂಟು{t, fl} {
        this->valuep(valuep);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Cons_ಅಂಶ;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class Ast_ಗಂಟು_Cover_Decl VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_ಪುಟ;  // Coverage point's page tag
    string m_ಪಠ್ಯ;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    ಪೂರ್ಣಾಂಕ m_bin_ಅಂಕೆ = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    ಪೂರ್ಣಾಂಕ m_local_Bin_ಅಂಕೆ = 0;  // Per-module coverage bin offset
    bool m_ಪ್ರತಿ_Instance = false;  // Don't clone during inlining, already per instance
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Cover_Decl(VNType t, File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪುಟ, ಸ್ಥಿರಾಂಕ string& comment)
        : Ast_ಗಂಟು(t, fl)
        , m_ಪುಟ{ಪುಟ}
        , m_ಪಠ್ಯ{comment} {}
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Cover_Decl;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        ಆದರೆ (m_ದತ್ತಾಂಶ_Declp
            && (m_ದತ್ತಾಂಶ_Declp == this || m_ದತ್ತಾಂಶ_Declp->m_ದತ್ತಾಂಶ_Declp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvt_ಗಮ್ಯ_Hex(this));
        }
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1 + 2 * INSTR_ಎಣಿಕೆ_LD; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಪೂರ್ಣಾಂಕ bin_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bin_ಅಂಕೆ; }
    ಶೂನ್ಯ bin_ಅಂಕೆ(ಪೂರ್ಣಾಂಕ flag) { m_bin_ಅಂಕೆ = flag; }
    ಪೂರ್ಣಾಂಕ local_Bin_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_local_Bin_ಅಂಕೆ; }
    ಶೂನ್ಯ local_Bin_ಅಂಕೆ(ಪೂರ್ಣಾಂಕ flag) { m_local_Bin_ಅಂಕೆ = flag; }
    bool ಪ್ರತಿ_Instance() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪ್ರತಿ_Instance; }
    ಶೂನ್ಯ ಪ್ರತಿ_Instance(bool flag) { m_ಪ್ರತಿ_Instance = flag; }
    ವಾಸ್ತವಿಕ ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ = 0;
    ಸ್ಥಿರಾಂಕ string& comment() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }  // text to insert in code
    ಸ್ಥಿರಾಂಕ string& ಪುಟ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪುಟ; }
    ಸ್ಥಿರಾಂಕ string& hier() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hier; }
    ಶೂನ್ಯ hier(ಸ್ಥಿರಾಂಕ string& flag) { m_hier = flag; }
    ಶೂನ್ಯ comment(ಸ್ಥಿರಾಂಕ string& flag) { m_ಪಠ್ಯ = flag; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Cover_Decl* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ಗಂಟು_Cover_Decl);
        ಹಿಂತಿರುಗಿ (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && ಪುಟ() == asamep->ಪುಟ()
                && ಪ್ರತಿ_Instance() == asamep->ಪ್ರತಿ_Instance());
    }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    ಶೂನ್ಯ ದತ್ತಾಂಶ_Declp(Ast_ಗಂಟು_Cover_Decl* nodep) { m_ದತ್ತಾಂಶ_Declp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_ಗಂಟು_Cover_Decl* ದತ್ತಾಂಶ_Decl_Nullp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ_Declp; }
    Ast_ಗಂಟು_Cover_Decl* ದತ್ತಾಂಶ_Decl_Thisp() { ಹಿಂತಿರುಗಿ ದತ್ತಾಂಶ_Decl_Nullp() ? ದತ್ತಾಂಶ_Decl_Nullp() : this; }
};
class Ast_ಗಂಟು_FTask VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Output variable in functions, nullptr in tasks
    // @astgen op1 := fvarp : Optional[AstNode]
    // Class/package scope
    // @astgen op2 := classOrPackagep : Optional[AstNode]
    // Statements/Ports/Vars
    // @astgen op3 := stmtsp : List[AstNode]
    // Scope name
    // @astgen op4 := scopeNamep : Optional[AstScopeName]
    string m_name;  // Name of task
    string m_cname;  // Name of task if DPI import
    // dist-ast-dump-suppress  // Not dumped due to verbosity
    string m_dpiCDecl;  // Custom DPI-C function declaration
    string m_iface_ಪೋರ್ಟ್_Name;  // Interface port name for out-of-block definition (IEEE 25.8)
    uint64_t m_dpi_Open_ಪೋಷಕ = 0;  // DPI import open array, if !=0, how many callees
    bool m_task_ಬಹಿರಂಗ : 1;  // Public task
    bool m_class_ವಿಧಾನ : 1;  // Class method
    bool m_didProto : 1;  // Did prototype processing
    bool m_prototype : 1;  // Just a prototype
    bool m_dpiExport : 1;  // DPI exported
    bool m_dpiImport : 1;  // DPI imported
    bool m_dpiContext : 1;  // DPI import context
    bool m_dpi_Open_ಮಗು : 1;  // DPI import open array child wrapper
    bool m_dpiTask : 1;  // DPI import task (vs. void function)
    bool m_isConstructor : 1;  // Class constructor
    bool m_isExternProto : 1;  // Extern prototype
    bool m_isExternDef : 1;  // Extern definition
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_dpi_ಶುದ್ಧ : 1;  // DPI import pure (vs. virtual pure)
    bool m_keepAlive : 1;  // Disable dead function elimination
    bool m_needsSyms : 1;  // Requires vlSymsp argument
    bool m_ಶುದ್ಧ_Virtual : 1;  // Pure virtual
    bool m_recursive : 1;  // Recursive or part of recursion
    bool m_static : 1;  // Static method in class
    bool m_underGenerate : 1;  // Under generate (for warning)
    bool m_verilogFunction : 1;  // Declared by user as function (versus internal-made)
    bool m_verilogTask : 1;  // Declared by user as task (versus internal-made)
    bool m_virtual : 1;  // Virtual method in class
    bool m_needProcess : 1;  // Needs access to VlProcess of the caller
    bool m_isCovergroupSample : 1;  // Covergroup sample() method
    VBaseOverride m_ತಳ_Override;  // BaseOverride (inital/final/extends)
    VLifetime m_lifetime;  // Default lifetime of local vars
    VIsCached m_purity;  // Pure state

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_FTask(VNType t, File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* stmtsp)
        : Ast_ಗಂಟು{t, fl}
        , m_name{name}
        , m_task_ಬಹಿರಂಗ{false}
        , m_class_ವಿಧಾನ{false}
        , m_didProto{false}
        , m_prototype{false}
        , m_dpiExport{false}
        , m_dpiImport{false}
        , m_dpiContext{false}
        , m_dpi_Open_ಮಗು{false}
        , m_dpiTask{false}
        , m_isConstructor{false}
        , m_isExternProto{false}
        , m_isExternDef{false}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_dpi_ಶುದ್ಧ{false}
        , m_keepAlive{false}
        , m_needsSyms{true}
        , m_ಶುದ್ಧ_Virtual{false}
        , m_recursive{false}
        , m_static{false}
        , m_underGenerate{false}
        , m_verilogFunction{false}
        , m_verilogTask{false}
        , m_virtual{false}
        , m_needProcess{false}
        , m_isCovergroupSample{false} {
        ಕೂಡಿಸು_Stmtsp(stmtsp);
        cname(name);  // Might be overridden by dpi import/export
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_FTask;
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_FTask* cloneType(ಸ್ಥಿರಾಂಕ string& name) = 0;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ !((m_dpiExport || m_dpiImport) && !m_dpi_ಶುದ್ಧ);
    }
    // {AstFunc only} op1 = Range output variable
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string cname() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cname; }
    ಶೂನ್ಯ cname(ಸ್ಥಿರಾಂಕ string& cname) { m_cname = cname; }
    bool isFunction() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ fvarp() != nullptr; }
    // MORE ACCESSORS
    ಶೂನ್ಯ dpi_Open_ಪೋಷಕ_Inc() { ++m_dpi_Open_ಪೋಷಕ; }
    ಶೂನ್ಯ dpi_Open_ಪೋಷಕ_Clear() { m_dpi_Open_ಪೋಷಕ = 0; }
    uint64_t dpi_Open_ಪೋಷಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpi_Open_ಪೋಷಕ; }
    bool task_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_task_ಬಹಿರಂಗ; }
    ಶೂನ್ಯ task_ಬಹಿರಂಗ(bool flag) { m_task_ಬಹಿರಂಗ = flag; }
    bool class_ವಿಧಾನ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_class_ವಿಧಾನ; }
    ಶೂನ್ಯ class_ವಿಧಾನ(bool flag) { m_class_ವಿಧಾನ = flag; }
    bool didProto() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_didProto; }
    ಶೂನ್ಯ didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isExternProto; }
    ಶೂನ್ಯ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isExternDef; }
    ಶೂನ್ಯ isExternDef(bool flag) { m_isExternDef = flag; }
    ಸ್ಥಿರಾಂಕ string& iface_ಪೋರ್ಟ್_Name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_iface_ಪೋರ್ಟ್_Name; }
    ಶೂನ್ಯ iface_ಪೋರ್ಟ್_Name(ಸ್ಥಿರಾಂಕ string& name) { m_iface_ಪೋರ್ಟ್_Name = name; }
    bool prototype() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_prototype; }
    ಶೂನ್ಯ prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiExport; }
    ಶೂನ್ಯ dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiImport; }
    ಶೂನ್ಯ dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiContext; }
    ಶೂನ್ಯ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpi_Open_ಮಗು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpi_Open_ಮಗು; }
    ಶೂನ್ಯ dpi_Open_ಮಗು(bool flag) { m_dpi_Open_ಮಗು = flag; }
    bool dpiTask() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiTask; }
    ಶೂನ್ಯ dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_dpiCDecl.ಖಾಲಿಗಳು(); }
    ಸ್ಥಿರಾಂಕ string& dpiCDecl() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiCDecl; }
    ಶೂನ್ಯ dpiCDecl(ಸ್ಥಿರಾಂಕ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isConstructor; }
    ಶೂನ್ಯ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isHideLocal; }
    ಶೂನ್ಯ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isHideProtected; }
    ಶೂನ್ಯ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpi_ಶುದ್ಧ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpi_ಶುದ್ಧ; }
    ಶೂನ್ಯ dpi_ಶುದ್ಧ(bool flag) { m_dpi_ಶುದ್ಧ = flag; }
    bool keepAlive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_keepAlive; }
    ಶೂನ್ಯ keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needsSyms; }
    ಶೂನ್ಯ needsSyms(bool flag) { m_needsSyms = flag; }
    bool ಶುದ್ಧ_Virtual() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಶುದ್ಧ_Virtual; }
    ಶೂನ್ಯ ಶುದ್ಧ_Virtual(bool flag) { m_ಶುದ್ಧ_Virtual = flag; }
    bool recursive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_recursive; }
    ಶೂನ್ಯ recursive(bool flag) { m_recursive = flag; }
    bool isStatic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_static; }
    ಶೂನ್ಯ isStatic(bool flag) { m_static = flag; }
    bool underGenerate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_underGenerate; }
    ಶೂನ್ಯ underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_verilogFunction; }
    ಶೂನ್ಯ verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_verilogTask; }
    ಶೂನ್ಯ verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_virtual; }
    ಶೂನ್ಯ isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needProcess; }
    ಶೂನ್ಯ ಗಣ_Need_Process() { m_needProcess = true; }
    bool isCovergroupSample() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isCovergroupSample; }
    ಶೂನ್ಯ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    ಶೂನ್ಯ ತಳ_Override(ಸ್ಥಿರಾಂಕ VBaseOverride& flag) { m_ತಳ_Override = flag; }
    VBaseOverride ತಳ_Override() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಳ_Override; }
    ಶೂನ್ಯ lifetime(ಸ್ಥಿರಾಂಕ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lifetime; }
    bool is_ಶುದ್ಧ() override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ propagateAttrFrom(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        class_ವಿಧಾನ(fromp->class_ವಿಧಾನ());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

ಖಾಸಗಿ:
    bool getPurityRecurse() ಸ್ಥಿರಾಂಕ;
};
class Ast_ಗಂಟು_File VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_File(VNType t, File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : Ast_ಗಂಟು{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ಗಂಟು_File;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಗಂಟು_Func_Cov_ಅಂಶ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Base class for functional coverage items (coverpoints, crosses)
ಸಂರಕ್ಷಿತ:
    string m_name;  // Item name

ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Func_Cov_ಅಂಶ(VNType t, File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : Ast_ಗಂಟು{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Func_Cov_ಅಂಶ;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಗಂಟು_Gen VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Generate construct
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Gen(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Gen;
};
class Ast_ಗಂಟು_Module VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    ಸ್ಥಿರಾಂಕ string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    ಪೂರ್ಣಾಂಕ m_ಆಳ = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    ಪೂರ್ಣಾಂಕ m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
    VLifetime m_lifetime;  // Lifetime
    VTimescale m_timeunit;  // Global time unit
    VOptionBool m_unconnectedDrive;  // State of `unconnected_drive
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

    bool m_mod_ಬಹಿರಂಗ : 1;  // Module has public references
    bool m_modTrace : 1;  // Tracing this module
    bool m_ಒಳ_Library : 1;  // From a library, no error if not used, never top level
    bool m_ctorVarReset : 1;  // Ctor needs to call ctor_var_reset
    bool m_dead : 1;  // LinkDot believes is dead; will remove in Dead visitors
    bool m_hasGParam : 1;  // Has global parameter (for link)
    bool m_hasParameterList : 1;  // Has #() for parameter declaration
    bool m_hier_ಖಂಡ : 1;  // Hierarchical Block marked by HIER_BLOCK pragma
    bool m_hierParams : 1;  // Block containing params for parameterized hier blocks
    bool m_recursive : 1;  // Recursive module
    bool m_recursiveClone : 1;  // If recursive, what module it clones, otherwise nullptr
    bool m_parameterizedTemplate : 1;  // True when at least one specialized clone exists;
                                       // set by V3Param::deepCloneModule. Suppresses
                                       // width/type errors on the unresolved template.
    bool m_verilatorLib : 1;  // Module is a stub for a Verilator produced --lib-create
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Module(VNType t, File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname)
        : Ast_ಗಂಟು{t, fl}
        , m_name{name}
        , m_origName{name}
        , m_libname{libname}
        , m_mod_ಬಹಿರಂಗ{false}
        , m_modTrace{false}
        , m_ಒಳ_Library{false}
        , m_ctorVarReset{false}
        , m_dead{false}
        , m_hasGParam{false}
        , m_hasParameterList{false}
        , m_hier_ಖಂಡ{false}
        , m_hierParams{false}
        , m_recursive{false}
        , m_recursiveClone{false}
        , m_parameterizedTemplate{false}
        , m_verilatorLib{false} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Module;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ವಾಸ್ತವಿಕ bool timescaleMatters() ಸ್ಥಿರಾಂಕ = 0;
    ಸಾಲೊಳಗೆ bool isDollarUnit() ಸ್ಥಿರಾಂಕ;  // Is the $unit package
    ಸಾಲೊಳಗೆ bool isConstPool() ಸ್ಥಿರಾಂಕ;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(ಸ್ಥಿರಾಂಕ string& outer) ಸ್ಥಿರಾಂಕ;
    // ACCESSORS
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string origName() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_origName; }
    string someInstanceName() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_someInstanceName; }
    ಶೂನ್ಯ someInstanceName(ಸ್ಥಿರಾಂಕ string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_defaultDisablep; }
    ಶೂನ್ಯ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool ಒಳ_Library() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಳ_Library; }
    ಶೂನ್ಯ ಒಳ_Library(bool flag) { m_ಒಳ_Library = flag; }
    ಶೂನ್ಯ ಆಳ(ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) { m_ಆಳ = ಮೌಲ್ಯ; }
    ಪೂರ್ಣಾಂಕ ಆಳ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಆಳ; }
    ಶೂನ್ಯ level(ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) { m_level = ಮೌಲ್ಯ; }
    ಪೂರ್ಣಾಂಕ level() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_level; }
    string libname() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libname; }
    bool is_ತುದಿ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ level() == 1; }
    bool mod_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_mod_ಬಹಿರಂಗ; }
    ಶೂನ್ಯ mod_ಬಹಿರಂಗ(bool flag) { m_mod_ಬಹಿರಂಗ = flag; }
    bool modTrace() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modTrace; }
    ಶೂನ್ಯ modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ctorVarReset; }
    ಶೂನ್ಯ ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dead; }
    ಶೂನ್ಯ dead(bool flag) { m_dead = flag; }
    bool hasGParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasGParam; }
    ಶೂನ್ಯ hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasParameterList; }
    ಶೂನ್ಯ hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hier_ಖಂಡ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hier_ಖಂಡ; }
    ಶೂನ್ಯ hier_ಖಂಡ(bool flag) { m_hier_ಖಂಡ = flag; }
    bool hierParams() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hierParams; }
    ಶೂನ್ಯ hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_recursive; }
    ಶೂನ್ಯ recursive(bool flag) { m_recursive = flag; }
    ಶೂನ್ಯ recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_recursiveClone; }
    bool parameterizedTemplate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_parameterizedTemplate; }
    ಶೂನ್ಯ parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    ಶೂನ್ಯ verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_verilatorLib; }
    VLifetime lifetime() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lifetime; }
    ಶೂನ್ಯ lifetime(ಸ್ಥಿರಾಂಕ VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unconnectedDrive; }
    ಶೂನ್ಯ unconnectedDrive(ಸ್ಥಿರಾಂಕ VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_ಗಂಟು_Procedure VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Procedure(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : Ast_ಗಂಟು{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        ಕೂಡಿಸು_Stmtsp(stmtsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Procedure;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isJustOneBodyStmt() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_suspendable; }
    ಶೂನ್ಯ ಗಣ_Suspendable() { m_suspendable = true; }
    bool needProcess() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needProcess; }
    ಶೂನ್ಯ ಗಣ_Need_Process() { m_needProcess = true; }
};
class Ast_ಗಂಟು_Range VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A range, sized or unsized
ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Range(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Range;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_ಸಕ್ರಿಯ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

ಸಾರ್ವಜನಿಕ:
    Ast_ಸಕ್ರಿಯ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_Sen_ವೃಕ್ಷ* sentreep)
        : ASTGEN_SUPER_ಸಕ್ರಿಯ(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_Ast_ಸಕ್ರಿಯ;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    // Statements are broken into pieces, as some must come before others.
    ಶೂನ್ಯ sentreep(Ast_Sen_ವೃಕ್ಷ* nodep) { m_sentreep = nodep; }
    Ast_Sen_ವೃಕ್ಷ* sentreep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sentreep; }
    // METHODS
    ಸಾಲೊಳಗೆ bool hasClocked() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool hasCombo() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool hasInitial() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool hasStatic() ಸ್ಥಿರಾಂಕ;
};
class AstAlias final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
ಸಾರ್ವಜನಿಕ:
    AstAlias(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstAliasScope(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
ಸಾರ್ವಜನಿಕ:
    AstBind(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        ಕೂಡಿಸು_Cellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Bind Target name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
};
class AstCFunc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // C++ function
    // Parents:  MODULE/SCOPE
    // If adding node accessors, see below emptyBody
    // @astgen op1 := argsp : List[AstVar]  // Argument (and return value) variables
    // @astgen op2 := varsp : List[AstVar]  // Local variables
    // @astgen op3 := stmtsp : List[AstNode]
    // @astgen op4 := scopeNamep : Optional[AstScopeName]  // Scoping context for DPI export
    //
    // @astgen ptr := m_scopep : Optional[AstScope]  // Scope that function is under
    string m_name;
    string m_cname;  // C name, for dpiExports
    string m_rtnType;  // void, bool, or other return type
    string m_argTypes;  // Argument types
    string m_ifdef;  // #ifdef symbol around this function
    string m_dpiCDecl;  // Custom DPI-C function declaration
    VBoolOrUnknown m_isConst;  // Function is declared const (*this not changed)
    bool m_isStatic : 1;  // Function is static (no need for a 'this' pointer)
    bool m_isTrace : 1;  // Function is related to tracing
    bool m_dontCombine : 1;  // V3Combine shouldn't compare this func tree, it's special
    bool m_decl_ಖಾಸಗಿ : 1;  // Declare it private
    bool m_keep_If_ಖಾಲಿಗಳು : 1;  // Keep declaration and definition separate, even if empty
    bool m_slow : 1;  // Slow routine, called once or just at init time
    bool m_func_ಬಹಿರಂಗ : 1;  // From user public task/function
    bool m_isConstructor : 1;  // Is C class constructor
    bool m_isDestructor : 1;  // Is C class destructor
    bool m_is_ವಿಧಾನ : 1;  // Is inside a class definition
    bool m_isLoose : 1;  // Semantically this is a method, but is implemented as a function with
                         // an explicitly passed 'self' pointer as the first argument.  This can
                         // be slightly faster due to __restrict, and we do not declare in header
                         // so adding/removing loose functions doesn't recompile everything.
    bool m_isVirtual : 1;  // Virtual function
    bool m_ನಮೂದು_ಬಿಂದು : 1;  // User may call into this top level function
    bool m_dpi_ಶುದ್ಧ : 1;  // Pure DPI function
    bool m_dpiContext : 1;  // Declared as 'context' DPI import/export function
    bool m_dpiExportDispatcher : 1;  // This is the DPI export entry point (i.e.: called by user)
    bool m_dpiExportImpl : 1;  // DPI export implementation (called from DPI dispatcher via lookup)
    bool m_dpiImportPrototype : 1;  // This is the DPI import prototype (i.e.: provided by user)
    bool m_dpiImportWrapper : 1;  // Wrapper for invoking DPI import prototype from generated code
    bool m_needProcess : 1;  // Needs access to VlProcess of the caller
    bool m_recursive : 1;  // Recursive or part of recursion
    bool m_unlikely : 1;  // Unlikely to get called (though still optimize unlike slow())
    bool m_noLife : 1;  // Disable V3Life on this function - has multiple calls, and reads Syms
                        // state
    bool m_isCovergroupSample : 1;  // Automatic covergroup sample() function
    bool m_voidSelf : 1;  // Loose method taking its 'self' pointer as 'void* vlVoidSelf'
    ಪೂರ್ಣಾಂಕ m_ವೆಚ್ಚ;  // Function call cost
ಸಾರ್ವಜನಿಕ:
    AstCFunc(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstScope* scopep, ಸ್ಥಿರಾಂಕ string& rtnType = "")
        : ASTGEN_SUPER_CFunc(fl) {
        m_isConst = VBoolOrUnknown::BU_UNKNOWN;  // Unknown until analyzed
        m_scopep = scopep;
        m_name = name;
        m_rtnType = rtnType;
        m_isStatic = false;
        m_isTrace = false;
        m_dontCombine = false;
        m_decl_ಖಾಸಗಿ = false;
        m_keep_If_ಖಾಲಿಗಳು = false;
        m_slow = false;
        m_func_ಬಹಿರಂಗ = false;
        m_isConstructor = false;
        m_isDestructor = false;
        m_is_ವಿಧಾನ = true;
        m_isLoose = false;
        m_isVirtual = false;
        m_needProcess = false;
        m_ನಮೂದು_ಬಿಂದು = false;
        m_dpi_ಶುದ್ಧ = false;
        m_dpiContext = false;
        m_dpiExportDispatcher = false;
        m_dpiExportImpl = false;
        m_dpiImportPrototype = false;
        m_dpiImportWrapper = false;
        m_recursive = false;
        m_unlikely = false;
        m_noLife = false;
        m_isCovergroupSample = false;
        m_voidSelf = false;
        m_ವೆಚ್ಚ = v3Global.opt.instr_ಎಣಿಕೆ_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstCFunc* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, CFunc);
        ಹಿಂತಿರುಗಿ ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ವೆಚ್ಚ; }
    VBoolOrUnknown isConst() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isConst; }
    ಶೂನ್ಯ isConst(bool flag) { m_isConst.ಗಣ_True_Or_False(flag); }
    ಶೂನ್ಯ isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isStatic; }
    ಶೂನ್ಯ isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_isTrace; }
    ಶೂನ್ಯ isTrace(bool flag) { m_isTrace = flag; }
    ಶೂನ್ಯ cname(ಸ್ಥಿರಾಂಕ string& name) { m_cname = name; }
    string cname() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cname; }
    AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopep; }
    ಶೂನ್ಯ scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ((m_rtnType == "") ? "void" : m_rtnType); }
    ಶೂನ್ಯ rtnType(ಸ್ಥಿರಾಂಕ string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dontCombine || isTrace() || ನಮೂದು_ಬಿಂದು(); }
    ಶೂನ್ಯ dontCombine(bool flag) { m_dontCombine = flag; }
    bool decl_ಖಾಸಗಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_decl_ಖಾಸಗಿ; }
    ಶೂನ್ಯ decl_ಖಾಸಗಿ(bool flag) { m_decl_ಖಾಸಗಿ = flag; }
    bool keep_If_ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_keep_If_ಖಾಲಿಗಳು; }
    ಶೂನ್ಯ keep_If_ಖಾಲಿಗಳು(bool flag) { m_keep_If_ಖಾಲಿಗಳು = flag; }
    bool slow() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_slow; }
    ಶೂನ್ಯ slow(bool flag) { m_slow = flag; }
    bool func_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_func_ಬಹಿರಂಗ; }
    ಶೂನ್ಯ func_ಬಹಿರಂಗ(bool flag) { m_func_ಬಹಿರಂಗ = flag; }
    ಶೂನ್ಯ argTypes(ಸ್ಥಿರಾಂಕ string& str) { m_argTypes = str; }
    string argTypes() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_argTypes; }
    ಶೂನ್ಯ ifdef(ಸ್ಥಿರಾಂಕ string& str) { m_ifdef = str; }
    string ifdef() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ifdef; }
    bool isConstructor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isConstructor; }
    ಶೂನ್ಯ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isDestructor; }
    ಶೂನ್ಯ isDestructor(bool flag) { m_isDestructor = flag; }
    bool is_ವಿಧಾನ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_ವಿಧಾನ; }
    ಶೂನ್ಯ is_ವಿಧಾನ(bool flag) { m_is_ವಿಧಾನ = flag; }
    bool isLoose() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isLoose; }
    ಶೂನ್ಯ isLoose(bool flag) { m_isLoose = flag; }
    bool is_Proper_ವಿಧಾನ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ is_ವಿಧಾನ() && !isLoose(); }
    bool isVirtual() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isVirtual; }
    ಶೂನ್ಯ isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needProcess; }
    ಶೂನ್ಯ ಗಣ_Need_Process() { m_needProcess = true; }
    bool ನಮೂದು_ಬಿಂದು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ನಮೂದು_ಬಿಂದು; }
    ಶೂನ್ಯ ನಮೂದು_ಬಿಂದು(bool flag) { m_ನಮೂದು_ಬಿಂದು = flag; }
    bool dpi_ಶುದ್ಧ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpi_ಶುದ್ಧ; }
    ಶೂನ್ಯ dpi_ಶುದ್ಧ(bool flag) { m_dpi_ಶುದ್ಧ = flag; }
    bool dpiContext() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiContext; }
    ಶೂನ್ಯ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_dpiExportDispatcher; }
    ಶೂನ್ಯ dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiExportImpl; }
    ಶೂನ್ಯ dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_dpiImportPrototype; }
    ಶೂನ್ಯ dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiImportWrapper; }
    ಶೂನ್ಯ dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_dpiCDecl.ಖಾಲಿಗಳು(); }
    ಸ್ಥಿರಾಂಕ string& dpiCDecl() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiCDecl; }
    ಶೂನ್ಯ dpiCDecl(ಸ್ಥಿರಾಂಕ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rtnType == "VlCoroutine"; }
    ಶೂನ್ಯ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_recursive; }
    ಶೂನ್ಯ unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_unlikely; }  // Note virtual override
    ಶೂನ್ಯ noLife(bool flag) { m_noLife = flag; }
    bool noLife() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noLife; }
    bool isCovergroupSample() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isCovergroupSample; }
    ಶೂನ್ಯ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_voidSelf; }
    ಶೂನ್ಯ voidSelf(bool flag) { m_voidSelf = flag; }
    ಶೂನ್ಯ ವೆಚ್ಚ(ಪೂರ್ಣಾಂಕ ವೆಚ್ಚ) { m_ವೆಚ್ಚ = ವೆಚ್ಚ; }
    // Special methods
    bool ಖಾಲಿಗಳು_Body() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ !keep_If_ಖಾಲಿಗಳು() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ಸ್ಥಿರಾಂಕ string m_name;
    ಸ್ಥಿರಾಂಕ VUseType m_useType;  // What sort of use this is

ಸಾರ್ವಜನಿಕ:
    AstCUse(File_ಸಾಲು* fl, VUseType useType, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    VUseType useType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_useType; }
};
class AstCell final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A instantiation cell or interface call (don't know which until link)
    // @astgen op1 := pinsp : List[AstPin] // List of port assignments
    // @astgen op2 := paramsp : List[AstPin] // List of parameter assignments
    // Pre V3Param: Range(s) for arrayed instances.
    // From V3Param to V3Width: ranges of the arrayed cell this element cell was expanded from
    // After V3Width: nullptr
    // @astgen op3 := rangep : List[AstRange]
    // @astgen op4 := intfRefsp : List[AstIntfRef] // List of interface references, for tracing/VPI
    //
    // @astgen ptr := m_modp : Optional[AstNodeModule]  // [AfterLink] Pointer to module instanced
    File_ಸಾಲು* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    ಪೂರ್ಣಾಂಕ m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
ಸಾರ್ವಜನಿಕ:
    AstCell(File_ಸಾಲು* fl, File_ಸಾಲು* mfl, ಸ್ಥಿರಾಂಕ string& instName, ಸ್ಥಿರಾಂಕ string& modName,
            AstPin* pinsp, AstPin* paramsp, AstRange* rangep)
        : ASTGEN_SUPER_Cell(fl)
        , m_modNameFileline{mfl}
        , m_name{instName}
        , m_origName{instName}
        , m_modName{modName}
        , m_hasIfaceVar{false}
        , m_recursive{false}
        , m_trace{true} {
        ಕೂಡಿಸು_Pinsp(pinsp);
        ಕೂಡಿಸು_Paramsp(paramsp);
        ಕೂಡಿಸು_Rangep(rangep);
    }
    ASTGEN_MEMBERS_AstCell;
    // No cloneRelink, we presume cloneee's want the same module linkages
    ಶೂನ್ಯ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    // ACCESSORS
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Cell name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string origName() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_origName; }  // * = Original name
    ಶೂನ್ಯ origName(ಸ್ಥಿರಾಂಕ string& name) { m_origName = name; }
    string modName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modName; }  // * = Instance name
    ಶೂನ್ಯ modName(ಸ್ಥಿರಾಂಕ string& name) { m_modName = name; }
    File_ಸಾಲು* modNameFileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modNameFileline; }
    Ast_ಗಂಟು_Module* modp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modp; }  // [AfterLink] = Pointer to module instantiated
    ಶೂನ್ಯ modp(Ast_ಗಂಟು_Module* nodep) { m_modp = nodep; }
    ಪೂರ್ಣಾಂಕ arrayIdx() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_arrayIdx; }
    ಶೂನ್ಯ arrayIdx(ಪೂರ್ಣಾಂಕ idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasIfaceVar; }
    ಶೂನ್ಯ hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    ಶೂನ್ಯ trace(bool flag) { m_trace = flag; }
    bool isTrace() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_trace; }
    ಶೂನ್ಯ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_recursive; }
};
class AstCellInline final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ಸ್ಥಿರಾಂಕ std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ಸ್ಥಿರಾಂಕ std::string m_origModName;  // __BEGIN__ or __FORK__
ಸಾರ್ವಜನಿಕ:
    AstCellInline(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    // ACCESSORS
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    std::string origModName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_origModName; }
};
class AstCellInlineScope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ಸ್ಥಿರಾಂಕ std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
ಸಾರ್ವಜನಿಕ:
    AstCellInlineScope(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    // ACCESSORS
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ std::string& name) override { m_name = name; }
    std::string origModName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_origModName; }
};
class Ast_Cg_Option_ನಿಯೋಜಿಸು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ಸ್ಥಿರಾಂಕ VCoverOptionType m_optType;  // Option type
    ಸ್ಥಿರಾಂಕ string m_name;  // Original option name (for diagnostics on unknown options)
    ಸ್ಥಿರಾಂಕ bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    Ast_Cg_Option_ನಿಯೋಜಿಸು(File_ಸಾಲು* fl, bool typeOption, VCoverOptionType optType,
                      ಸ್ಥಿರಾಂಕ string& rawName, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Cg_Option_ನಿಯೋಜಿಸು(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Cg_Option_ನಿಯೋಜಿಸು;
    // ACCESSORS
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    VCoverOptionType optType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_optType; }
    bool typeOption() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_typeOption; }
};
class AstClassExtends final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ಸ್ಥಿರಾಂಕ bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

ಸಾರ್ವಜನಿಕ:
    AstClassExtends(File_ಸಾಲು* fl, Ast_ಗಂಟು* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ಸ್ಥಿರಾಂಕ;
    AstClass* classp() ಸ್ಥಿರಾಂಕ;  // Like above, but throws error if nulll
    bool isImplements() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isImplements; }
    ಶೂನ್ಯ parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_parameterized; }
};
class AstClocking final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ಸ್ಥಿರಾಂಕ bool m_isGlobal;  // True if global clocking

ಸಾರ್ವಜನಿಕ:
    AstClocking(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& name, Ast_Sen_ವೃಕ್ಷ* sentreep, Ast_ಗಂಟು* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool isDefault() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isDefault; }
    bool isGlobal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isGlobal; }
    AstVar* ensureEventp(bool ಮಗು_DType = false);
    ಶೂನ್ಯ makeDefault() { m_isDefault = true; }
};
class Ast_Clocking_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

ಸಾರ್ವಜನಿಕ:
    Ast_Clocking_ಅಂಶ(File_ಸಾಲು* fl, VDirection direction, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* skewp, Ast_ಗಂಟು* clockingDeclp)
        : ASTGEN_SUPER_Clocking_ಅಂಶ(fl) {
        m_direction = direction;
        this->skewp(skewp);
        ಆದರೆ (Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ clkAssignp = VN_CAST(clockingDeclp, ನಿಯೋಜಿಸು)) {
            this->assignp(clkAssignp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            exprp(VN_AS(clockingDeclp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
        }
    }
    ASTGEN_MEMBERS_Ast_Clocking_ಅಂಶ;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    VDirection direction() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_direction; }
    Ast_Clocking_ಅಂಶ* outputp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_outputp; }
    ಶೂನ್ಯ outputp(Ast_Clocking_ಅಂಶ* outputp) { m_outputp = outputp; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
};
class AstConfig final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

ಸಾರ್ವಜನಿಕ:
    AstConfig(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& libname, ಸ್ಥಿರಾಂಕ std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_libname + "." + m_configname; }
    std::string libname() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_libname; }
    std::string configname() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_configname; }
};
class AstConfigCell final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

ಸಾರ್ವಜನಿಕ:
    AstConfigCell(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& libname, ಸ್ಥಿರಾಂಕ std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_libname + "." + m_cellname; }
    std::string libname() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_libname; }
    std::string cellname() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_cellname; }
};
class Ast_Config_ನಿಯಮ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ಸ್ಥಿರಾಂಕ bool m_isCell;  // Declared as "cell" versus "instance"

ಸಾರ್ವಜನಿಕ:
    Ast_Config_ನಿಯಮ(File_ಸಾಲು* fl, Ast_ಗಂಟು* cellp, Ast_ಗಂಟು* usep, bool isCell)
        : ASTGEN_SUPER_Config_ನಿಯಮ(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        ಕೂಡಿಸು_Usep(usep);
    }
    ASTGEN_MEMBERS_Ast_Config_ನಿಯಮ;
    bool isCell() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_isCell; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class AstConfigUse final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ಸ್ಥಿರಾಂಕ bool m_isConfig;  // ":config"; Config, not module/primitive name

ಸಾರ್ವಜನಿಕ:
    AstConfigUse(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& libname, ಸ್ಥಿರಾಂಕ std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        ಕೂಡಿಸು_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_libname + "." + m_cellname; }
    std::string libname() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_libname; }
    std::string cellname() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_cellname; }
    bool isConfig() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_isConfig; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class AstConstraint final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Constraint
    // @astgen op1 := itemsp : List[AstNode]
    // @astgen op2 := classOrPackagep : Optional[AstNode]
    string m_name;  // Name of constraint
    VBaseOverride m_ತಳ_Override;  // BaseOverride (inital/final/extends)
    bool m_isExternDef = false;  // Extern prototype definition
    bool m_isExternExplicit = false;  // Explicit prototype declaration (has extern)
    bool m_isExternProto = false;  // Prototype declaration (implicit or explicit)
    bool m_is_Kwd_ಶುದ್ಧ = false;  // Pure constraint
    bool m_isStatic = false;  // Static constraint
ಸಾರ್ವಜನಿಕ:
    AstConstraint(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Scope name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ ತಳ_Override(ಸ್ಥಿರಾಂಕ VBaseOverride& flag) { m_ತಳ_Override = flag; }
    VBaseOverride ತಳ_Override() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಳ_Override; }
    bool isExternDef() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isExternDef; }
    ಶೂನ್ಯ isExternDef(bool flag) { m_isExternDef = flag; }
    ಶೂನ್ಯ isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isExternExplicit; }
    ಶೂನ್ಯ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isExternProto; }
    ಶೂನ್ಯ is_Kwd_ಶುದ್ಧ(bool flag) { m_is_Kwd_ಶುದ್ಧ = flag; }
    bool is_Kwd_ಶುದ್ಧ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_Kwd_ಶುದ್ಧ; }
    ಶೂನ್ಯ isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isStatic; }
};
class AstCoverBin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    ಸ್ಥಿರಾಂಕ string m_name;  // Base name of the bin
    ಸ್ಥಿರಾಂಕ VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

ಸಾರ್ವಜನಿಕ:
    AstCoverBin(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp = nullptr)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{isIllegal ? VCoverBinsType::BINS_ILLEGAL
                               : (isIgnore ? VCoverBinsType::BINS_IGNORE
                                           : (isWildcard ? VCoverBinsType::BINS_WILDCARD
                                                         : VCoverBinsType::BINS_USER))}
        , m_isWildcard{isWildcard} {
        ಕೂಡಿಸು_Rangesp(rangesp);
        this->iffp(iffp);
    }
    // Constructor for automatic bins
    AstCoverBin(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_Cover_Trans_ಗಣ* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        ಕೂಡಿಸು_Transp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    VCoverBinsType binsType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_binsType; }
    bool isWildcard() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isWildcard; }
    bool isArray() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isArray; }
    ಶೂನ್ಯ isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ಸ್ಥಿರಾಂಕ bool m_isNegated;  // Complement the selection within the cross product

ಸಾರ್ವಜನಿಕ:
    AstCoverBinsof(File_ಸಾಲು* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_ಗಂಟು* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        ಕೂಡಿಸು_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    bool isNegated() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isNegated; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {  // LCOV_EXCL_START
        ಸ್ಥಿರಾಂಕ AstCoverBinsof* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, CoverBinsof);
        ಹಿಂತಿರುಗಿ m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ಸ್ಥಿರಾಂಕ string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ಸ್ಥಿರಾಂಕ VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

ಸಾರ್ವಜನಿಕ:
    AstCoverCrossBin(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* selectp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override;
    VCoverBinsType binsType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_binsType; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {  // LCOV_EXCL_START
        ಸ್ಥಿರಾಂಕ AstCoverCrossBin* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, CoverCrossBin);
        ಹಿಂತಿರುಗಿ m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // The complete bin-tuple space of the enclosing cross
    ಸ್ಥಿರಾಂಕ string m_name;  // Referenced cross name

ಸಾರ್ವಜನಿಕ:
    AstCoverCrossRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {  // LCOV_EXCL_START
        ಹಿಂತಿರುಗಿ m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ಸ್ಥಿರಾಂಕ bool m_isOr;  // Union (||), rather than intersection (&&)

ಸಾರ್ವಜನಿಕ:
    AstCoverCrossSelect(File_ಸಾಲು* fl, Ast_ಗಂಟು* lhsp, Ast_ಗಂಟು* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool isOr() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isOr; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ isOr() ? "||" : "&&"; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {  // LCOV_EXCL_START
        ಹಿಂತಿರುಗಿ m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
ಸಾರ್ವಜನಿಕ:
    AstCoverCrossWith(File_ಸಾಲು* fl, Ast_ಗಂಟು* selectp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filterp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    ಸ್ಥಿರಾಂಕ VCoverOptionType m_optType;  // Option being assigned
    ಸ್ಥಿರಾಂಕ bool m_typeOption;  // type_option (vs option)

ಸಾರ್ವಜನಿಕ:
    AstCoverOption(File_ಸಾಲು* fl, bool typeOption, VCoverOptionType optType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    VCoverOptionType optType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_optType; }
    bool typeOption() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_typeOption; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {  // LCOV_EXCL_START
        ಸ್ಥಿರಾಂಕ AstCoverOption* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, CoverOption);
        ಹಿಂತಿರುಗಿ m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstCoverToggle(File_ಸಾಲು* fl, AstCoverInc* incp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* origp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 3 + INSTR_ಎಣಿಕೆ_BRANCH + INSTR_ಎಣಿಕೆ_LD; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isOutputter() override {
        ಹಿಂತಿರುಗಿ false;  // Though the AstCoverInc under this is an outputter
    }
};
class Ast_Cover_Trans_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ಸ್ಥಿರಾಂಕ VTransRepType m_repType;

ಸಾರ್ವಜನಿಕ:
    Ast_Cover_Trans_ಅಂಶ(File_ಸಾಲು* fl, Ast_ಗಂಟು* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_Cover_Trans_ಅಂಶ(fl)
        , m_repType{repType} {
        ಕೂಡಿಸು_Valuesp(valuesp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_ಅಂಶ;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class Ast_Cover_Trans_ಗಣ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
ಸಾರ್ವಜನಿಕ:
    Ast_Cover_Trans_ಗಣ(File_ಸಾಲು* fl, Ast_Cover_Trans_ಅಂಶ* itemsp)
        : ASTGEN_SUPER_Cover_Trans_ಗಣ(fl) {
        ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_ಗಣ;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class AstCoverWith final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
ಸಾರ್ವಜನಿಕ:
    AstCoverWith(File_ಸಾಲು* fl, Ast_ಗಂಟು* subp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        ಕೂಡಿಸು_Subp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

ಸಾರ್ವಜನಿಕ:
    AstCovergroup(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_ಗಂಟು* membersp, Ast_Sen_ವೃಕ್ಷ* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        ಕೂಡಿಸು_Argsp(argsp);
        ಕೂಡಿಸು_Sample_Argsp(sampleArgsp);
        ಕೂಡಿಸು_Membersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstCoverpointRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ಸ್ಥಿರಾಂಕ string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

ಸಾರ್ವಜನಿಕ:
    AstCoverpointRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
};
class AstDefParam final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
ಸಾರ್ವಜನಿಕ:
    AstDefParam(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pathp, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstDefaultClocking final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    std::string m_name;  // Clocking block name

ಸಾರ್ವಜನಿಕ:
    AstDefaultClocking(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
};
class AstDefaultDisable final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := condp : AstNodeExpr

ಸಾರ್ವಜನಿಕ:
    AstDefaultDisable(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
ಸಾರ್ವಜನಿಕ:
    AstDpiExport(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& vname, ಸ್ಥಿರಾಂಕ string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string cname() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cname; }
    ಶೂನ್ಯ cname(ಸ್ಥಿರಾಂಕ string& cname) { m_cname = cname; }
};
class AstElabDisplay final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

ಸಾರ್ವಜನಿಕ:
    ಸಾಲೊಳಗೆ AstElabDisplay(File_ಸಾಲು* fl, VDisplayType dispType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!fmtp());
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ INSTR_ಎಣಿಕೆ_PLI; }
    VDisplayType displayType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_displayType; }
    ಶೂನ್ಯ displayType(VDisplayType type) { m_displayType = type; }
};
class Ast_ಖಾಲಿಗಳು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents something missing, e.g. a missing argument in FOREACH
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಖಾಲಿಗಳು(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ಖಾಲಿಗಳು(fl) {}
    ASTGEN_MEMBERS_Ast_ಖಾಲಿಗಳು;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_Gen_Case_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    Ast_Gen_Case_ಅಂಶ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condsp, Ast_ಗಂಟು* itemsp)
        : ASTGEN_SUPER_Gen_Case_ಅಂಶ(fl) {
        ಕೂಡಿಸು_Condsp(condsp);
        ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_Case_ಅಂಶ;
    bool isDefault() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ condsp() == nullptr; }
};
class AstImplicit final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstImplicit(File_ಸಾಲು* fl, Ast_ಗಂಟು* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        ಕೂಡಿಸು_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_ಪೋಷಕ_Name;  // Hierarchical path of the declaring module, before inlining
    string m_ತಳ_Name;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
ಸಾರ್ವಜನಿಕ:
    AstIntfRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& ಪೋಷಕ_Name, ಸ್ಥಿರಾಂಕ string& ತಳ_Name,
               ಸ್ಥಿರಾಂಕ string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_ಪೋಷಕ_Name{ಪೋಷಕ_Name}
        , m_ತಳ_Name{ತಳ_Name}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    string ಪೋಷಕ_Name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪೋಷಕ_Name; }
    string ತಳ_Name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಳ_Name; }
    string modportName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modportName; }
};
class AstLibrary final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

ಸಾರ್ವಜನಿಕ:
    AstLibrary(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filesp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        ಕೂಡಿಸು_Filesp(filesp);
        ಕೂಡಿಸು_Incdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
};
class AstModport final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
ಸಾರ್ವಜನಿಕ:
    AstModport(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        ಕೂಡಿಸು_Varsp(varsp);
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "modport"; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
ಸಾರ್ವಜನಿಕ:
    AstModportClockingRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    AstClocking* clockingp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_clockingp;
    }  // [After Link] Pointer to clocking block
    ಶೂನ್ಯ clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
ಸಾರ್ವಜನಿಕ:
    AstModportFTaskRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool isImport() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_export; }
    bool isExport() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_export; }
    Ast_ಗಂಟು_FTask* ftaskp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ftaskp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ ftaskp(Ast_ಗಂಟು_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    ಸ್ಥಿರಾಂಕ VDirection m_direction;  // Direction of the variable (in/out)
ಸಾರ್ವಜನಿಕ:
    AstModportVarRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    VDirection direction() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_direction; }
    AstVar* varp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_varp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // All modules are under this single top node.
    // Parents:   none
    // Children:  MODULEs & CFILEs
    // @astgen op1 := modulesp : List[AstNodeModule]
    // @astgen op2 := filesp : List[AstNodeFile]
    // @astgen op3 := miscsp : List[AstNode]
    //
    // @astgen ptr := m_typeTablep : AstTypeTable  // Reference to type table, for faster lookup
    // @astgen ptr := m_constPoolPkgp : AstPackage  // Constant pool package
    // @astgen ptr := m_dollarUnitPkgp : Optional[AstPackage]  // $unit
    // @astgen ptr := m_stdPackagep : Optional[AstPackage]  // SystemVerilog std package
    // @astgen ptr := m_stdPackageProcessp : Optional[AstClass]  // SystemVerilog std process class
    // @astgen ptr := m_dpiExportTriggerp : Optional[AstVarScope]  // DPI export trigger variable
    // @astgen ptr := m_delaySchedulerp : Optional[AstVar]  // Delay scheduler variable
    // @astgen ptr := m_nbaEventp : Optional[AstVarScope]  // NBA event variable
    // @astgen ptr := m_nbaEventTriggerp : Optional[AstVarScope]  // NBA event trigger
    // @astgen ptr := m_topScopep : Optional[AstTopScope]  // Singleton AstTopScope
    // @astgen ptr := m_stlFirstIterationp: Optional[AstVarScope]  // Settle first iteration flag
    VTimescale m_timeunit;  // Global time unit
    VTimescale m_timeprecision;  // Global time precision
    std::string m_resolved_ತುದಿ_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::ಗಣ<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstConst*, string> m_const_Orig_Param_ಹೆಸರುಗಳು;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::ENUM_ಕೊನೆ> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::ENUM_ಕೊನೆ> m_dumpTriggersFuncps{};

ಸಾರ್ವಜನಿಕ:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ ತಳ್ಳು_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.ಸೇರಿಸು(varp); }
    ಸ್ಥಿರಾಂಕ std::ಗಣ<AstVar*>& deferredParamVarps() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_deferredParamVarps; }
    ಶೂನ್ಯ clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    ಶೂನ್ಯ deleteContents();
    ಶೂನ್ಯ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ "$root"; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_Module* ತುದಿ_Modulep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {  // Top module in hierarchy
        ಹಿಂತಿರುಗಿ modulesp();  // First one in the list, for now
    }
    Ast_Type_ಕೋಷ್ಟಕ* typeTablep() { ಹಿಂತಿರುಗಿ m_typeTablep; }
    string astConstOrigParamName(ಸ್ಥಿರಾಂಕ AstConst* nodep) ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ astConstOrigParamName(ಸ್ಥಿರಾಂಕ AstConst* nodep, ಸ್ಥಿರಾಂಕ string& name);
    ಶೂನ್ಯ astConstOrigParamNameErase(ಸ್ಥಿರಾಂಕ AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* containingModule(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);
    // Forget remembered modules, as the tree has moved.
    ಶೂನ್ಯ clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dollarUnitPkgp; }
    ಶೂನ್ಯ dollarUnitPkgp(AstPackage* ಸ್ಥಿರಾಂಕ packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiExportTriggerp; }
    ಶೂನ್ಯ dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_delaySchedulerp; }
    ಶೂನ್ಯ delaySchedulerp(AstVar* ಸ್ಥಿರಾಂಕ varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nbaEventp; }
    ಶೂನ್ಯ nbaEventp(AstVarScope* ಸ್ಥಿರಾಂಕ varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nbaEventTriggerp; }
    ಶೂನ್ಯ nbaEventTriggerp(AstVarScope* ಸ್ಥಿರಾಂಕ varScopep) { m_nbaEventTriggerp = varScopep; }
    ಶೂನ್ಯ stdPackagep(AstPackage* ಸ್ಥಿರಾಂಕ packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stdPackagep; }
    ಶೂನ್ಯ stdPackageProcessp(AstClass* ಸ್ಥಿರಾಂಕ classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_ಸಾಲು*) ಸ್ಥಿರಾಂಕ;
    Ast_ತುದಿ_Scope* ತುದಿ_Scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತುದಿ_Scopep; }
    ಶೂನ್ಯ create_ತುದಿ_Scope(AstScope* scopep);
    VTimescale timeunit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಸ್ಥಿರಾಂಕ VTimescale& ಮೌಲ್ಯ) { m_timeunit = ಮೌಲ್ಯ; }
    VTimescale timeprecision() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timeprecision; }
    ಶೂನ್ಯ ಸಮಯ_Init() {
        m_timeunit = v3Global.opt.ಸಮಯ_Default_Unit();
        m_timeprecision = v3Global.opt.ಸಮಯ_Default_Prec();
    }
    ಶೂನ್ಯ timeprecision_ವಿಲೀನ(File_ಸಾಲು*, ಸ್ಥಿರಾಂಕ VTimescale& ಮೌಲ್ಯ);
    ಶೂನ್ಯ timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timescaleSpecified; }
    ಸ್ಥಿರಾಂಕ std::string& resolved_ತುದಿ_Module_Name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_resolved_ತುದಿ_Module_Name; }
    ಶೂನ್ಯ resolved_ತುದಿ_Module_Name(ಸ್ಥಿರಾಂಕ std::string& ಮೌಲ್ಯ) { m_resolved_ತುದಿ_Module_Name = ಮೌಲ್ಯ; }
    uint32_t nTraceCodes() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nTraceCodes; }
    ಶೂನ್ಯ nTraceCodes(uint32_t ಮೌಲ್ಯ) { m_nTraceCodes = ಮೌಲ್ಯ; }
    AstVarScope* stl_ಮೊದಲ_Iterationp();
    ಶೂನ್ಯ clear_Stl_ಮೊದಲ_Iterationp() { m_stl_ಮೊದಲ_Iterationp = nullptr; }
    ಸ್ಥಿರಾಂಕ std::string trace_Lib_ತುದಿ_Name() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ std::string& name = resolved_ತುದಿ_Module_Name();
        ಹಿಂತಿರುಗಿ prettyName(name.ಖಾಲಿಗಳು() ? v3Global.rootp()->ತುದಿ_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    ಶೂನ್ಯ ಕೂಡಿಸು_Eval_Stats(ಸ್ಥಿರಾಂಕ std::string& phase);
};
class AstPackageExport final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

ಸಾರ್ವಜನಿಕ:
    AstPackageExport(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& pkgName, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    string pkgName() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_pkgName; }
    string prettyPkgNameQ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_packagep; }
    ಶೂನ್ಯ packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class Ast_Package_Export_ನಕ್ಷತ್ರ_ನಕ್ಷತ್ರ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A package export *::* declaration
ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress noExplicitConstructor
    Ast_Package_Export_ನಕ್ಷತ್ರ_ನಕ್ಷತ್ರ(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Package_Export_ನಕ್ಷತ್ರ_ನಕ್ಷತ್ರ(fl) {}
    ASTGEN_MEMBERS_Ast_Package_Export_ನಕ್ಷತ್ರ_ನಕ್ಷತ್ರ;
};
class AstPackageImport final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

ಸಾರ್ವಜನಿಕ:
    AstPackageImport(File_ಸಾಲು* fl, AstPackage* packagep, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& pkgName, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    string pkgName() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_pkgName; }
    string prettyPkgNameQ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_packagep; }
    ಶೂನ್ಯ packagep(AstPackage* nodep) { m_packagep = nodep; }

ಖಾಸಗಿ:
    ಶೂನ್ಯ pkgNameFrom();
};
class AstPin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    ಪೂರ್ಣಾಂಕ m_pin_ಅಂಕೆ;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_param_ಮಾರ್ಗ;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
ಸಾರ್ವಜನಿಕ:
    AstPin(File_ಸಾಲು* fl, ಪೂರ್ಣಾಂಕ pin_ಅಂಕೆ, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_ಅಂಕೆ{pin_ಅಂಕೆ}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    ಶೂನ್ಯ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Pin name, ""=go by number
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string prettyOperatorName() ಸ್ಥಿರಾಂಕ override;
    bool dot_ನಕ್ಷತ್ರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ name() == ".*"; }  // Fake name for .* connections until linked
    ಪೂರ್ಣಾಂಕ pin_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pin_ಅಂಕೆ; }
    AstVar* modVarp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modVarp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modPTypep; }
    ಶೂನ್ಯ modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_param; }
    ಶೂನ್ಯ param(bool flag) { m_param = flag; }
    ಸ್ಥಿರಾಂಕ string& param_ಮಾರ್ಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_param_ಮಾರ್ಗ; }
    ಶೂನ್ಯ param_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& ಮಾರ್ಗ) { m_param_ಮಾರ್ಗ = ಮಾರ್ಗ; }
    bool svDotName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_svDotName; }
    ಶೂನ್ಯ svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_svImplicit; }
    ಶೂನ್ಯ svImplicit(bool flag) { m_svImplicit = flag; }
};
class Ast_ಪೋರ್ಟ್ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_pin_ಅಂಕೆ;  // Pin number
    ಸ್ಥಿರಾಂಕ string m_name;  // Name of pin
ಸಾರ್ವಜನಿಕ:
    Ast_ಪೋರ್ಟ್(File_ಸಾಲು* fl, ಪೂರ್ಣಾಂಕ pinnum, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_ಪೋರ್ಟ್(fl)
        , m_pin_ಅಂಕೆ{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ಪೋರ್ಟ್;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Port name
    ಪೂರ್ಣಾಂಕ pin_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pin_ಅಂಕೆ; }  // * = Pin number, for order based instantiation
};
class AstPragma final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    ಸ್ಥಿರಾಂಕ VPragmaType m_pragType;  // Type of pragma
    ಸ್ಥಿರಾಂಕ VTimescale m_timescale;  // For TIMEUNIT_SET
ಸಾರ್ವಜನಿಕ:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_ಸಾಲು* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_ಗಣ, fl, "Should use other constructor");
    }
    AstPragma(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_ಗಣ}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    VPragmaType pragType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timescale; }
};
class AstPropSpec final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

ಸಾರ್ವಜನಿಕ:
    AstPropSpec(File_ಸಾಲು* fl, Ast_Sen_ಅಂಶ* sensesp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* disablep, Ast_ಗಂಟು* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_propStrength; }
};
class AstPull final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := lhsp : AstNodeExpr

    ಸ್ಥಿರಾಂಕ bool m_direction;

ಸಾರ್ವಜನಿಕ:
    AstPull(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_direction; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class AstScope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A particular usage of a cell
    // Parents: MODULE
    // Children: NODEBLOCK
    // @astgen op1 := varsp : List[AstVarScope]
    // @astgen op2 := blocksp : List[AstNode] // Logic blocks/AstActive/AstCFunc/AstScopeName
    // @astgen op3 := inlinesp : List[AstCellInlineScope] // VPI records of flattened scopes
    //
    // Below scope and cell are nullptr if top scope
    // @astgen ptr := m_aboveScopep : Optional[AstScope]  // Scope above this one in the hierarchy
    // @astgen ptr := m_aboveCellp : Optional[AstCell]  // Cell above this in the hierarchy
    // @astgen ptr := m_modp : AstNodeModule  // Module scope corresponds to

    // An AstScope->name() is special: . indicates an uninlined scope, __DOT__ an inlined scope
    string m_name;  // Name
ಸಾರ್ವಜನಿಕ:
    AstScope(File_ಸಾಲು* fl, Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!m_modp);
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Scope name
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string pretty_Name_ಸಂದೇಶ() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    string nameDotless() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_Module* modp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    Ast_ಸಕ್ರಿಯ* comboActivep(bool create);
    //
    AstScope* aboveScopep() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_aboveScopep; }
    ಶೂನ್ಯ aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_aboveCellp; }
    ಶೂನ್ಯ aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_ತುದಿ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ aboveScopep() == nullptr; }  // At top of hierarchy
};
class Ast_Sen_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_ಅಂಚು_Type;  // Edge type
ಸಾರ್ವಜನಿಕ:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    Ast_Sen_ಅಂಶ(File_ಸಾಲು* fl, VEdgeType ಅಂಚು_Type, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* senp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr)
        : ASTGEN_SUPER_Sen_ಅಂಶ(fl)
        , m_ಅಂಚು_Type{ಅಂಚು_Type} {
        this->sensp(senp);
        this->condp(condp);
    }
    Ast_Sen_ಅಂಶ(File_ಸಾಲು* fl, Combo)
        : ASTGEN_SUPER_Sen_ಅಂಶ(fl)
        , m_ಅಂಚು_Type{VEdgeType::ET_COMBO} {}
    Ast_Sen_ಅಂಶ(File_ಸಾಲು* fl, Static)
        : ASTGEN_SUPER_Sen_ಅಂಶ(fl)
        , m_ಅಂಚು_Type{VEdgeType::ET_STATIC} {}
    Ast_Sen_ಅಂಶ(File_ಸಾಲು* fl, Initial)
        : ASTGEN_SUPER_Sen_ಅಂಶ(fl)
        , m_ಅಂಚು_Type{VEdgeType::ET_INITIAL} {}
    Ast_Sen_ಅಂಶ(File_ಸಾಲು* fl, InitialNBA)
        : ASTGEN_SUPER_Sen_ಅಂಶ(fl)
        , m_ಅಂಚು_Type{VEdgeType::ET_INITIAL_NBA} {}
    Ast_Sen_ಅಂಶ(File_ಸಾಲು* fl, Final)
        : ASTGEN_SUPER_Sen_ಅಂಶ(fl)
        , m_ಅಂಚು_Type{VEdgeType::ET_FINAL} {}
    Ast_Sen_ಅಂಶ(File_ಸಾಲು* fl, Never)
        : ASTGEN_SUPER_Sen_ಅಂಶ(fl)
        , m_ಅಂಚು_Type{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_Ast_Sen_ಅಂಶ;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಅಂಚು_Type() == VN_DBG_AS(samep, Sen_ಅಂಶ)->ಅಂಚು_Type();
    }
    VEdgeType ಅಂಚು_Type() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಚು_Type; }
    ಶೂನ್ಯ ಅಂಚು_Type(VEdgeType type) {
        m_ಅಂಚು_Type = type;
        edit_ಎಣಿಕೆ_Inc();
    }
    Ast_ಗಂಟು_Var_Ref* varrefp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VN_CAST(sensp(), ಗಂಟು_Var_Ref); }
    //
    bool isClocked() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಚು_Type().clockedStmt(); }
    bool is_Combo_Or_ನಕ್ಷತ್ರ() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ ಅಂಚು_Type() == VEdgeType::ET_COMBO || ಅಂಚು_Type() == VEdgeType::ET_COMBO_ನಕ್ಷತ್ರ;
    }
    bool is_Combo_ನಕ್ಷತ್ರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಚು_Type() == VEdgeType::ET_COMBO_ನಕ್ಷತ್ರ; }
    bool isHybrid() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಚು_Type() == VEdgeType::ET_HYBRID; }
    bool isStatic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಚು_Type() == VEdgeType::ET_STATIC; }
    bool isInitial() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಚು_Type() == VEdgeType::ET_INITIAL; }
    bool isFinal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಚು_Type() == VEdgeType::ET_FINAL; }
    bool isNever() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಅಂಚು_Type() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_ವೃಕ್ಷ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
ಸಾರ್ವಜನಿಕ:
    Ast_Sen_ವೃಕ್ಷ(File_ಸಾಲು* fl, Ast_Sen_ಅಂಶ* sensesp)
        : ASTGEN_SUPER_Sen_ವೃಕ್ಷ(fl) {
        ಕೂಡಿಸು_Sensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_ವೃಕ್ಷ;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    // METHODS
    bool hasClocked() ಸ್ಥಿರಾಂಕ;  // Includes a clocked statement
    bool has_ಅಂಚು() ಸ್ಥಿರಾಂಕ;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ಸ್ಥಿರಾಂಕ;  // Includes a STATIC SenItem
    bool hasInitial() ಸ್ಥಿರಾಂಕ;  // Includes a INITIAL SenItem
    bool hasFinal() ಸ್ಥಿರಾಂಕ;  // Includes a FINAL SenItem
    bool hasCombo() ಸ್ಥಿರಾಂಕ;  // Includes a COMBO SenItem
    bool hasHybrid() ಸ್ಥಿರಾಂಕ;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

ಸಾರ್ವಜನಿಕ:
    AstStrengthSpec(File_ಸಾಲು* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { ಹಿಂತಿರುಗಿ m_s0; }
    VStrength strength1() { ಹಿಂತಿರುಗಿ m_s1; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class AstSystemCSection final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ಸ್ಥಿರಾಂಕ VSystemCSectionType m_sectionType;  // The section type
    std::string m_ಪಠ್ಯ;  // The text content

ಸಾರ್ವಜನಿಕ:
    AstSystemCSection(File_ಸಾಲು* fl, VSystemCSectionType sectionType, ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_ಪಠ್ಯ{ಪಠ್ಯ} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sectionType; }
    ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }
    ಶೂನ್ಯ ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ std::string& ಮೌಲ್ಯ) { m_ಪಠ್ಯ = ಮೌಲ್ಯ; }
    ಶೂನ್ಯ dump(std::ostream&) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream&) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool is_ಶುದ್ಧ() override { ಹಿಂತಿರುಗಿ false; }
    bool isOutputter() override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಪಠ್ಯ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ಸ್ಥಿರಾಂಕ std::string m_ಪಠ್ಯ;  // The text to emit
ಸಾರ್ವಜನಿಕ:
    Ast_ಪಠ್ಯ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ)
        : ASTGEN_SUPER_ಪಠ್ಯ(fl)
        , m_ಪಠ್ಯ{ಪಠ್ಯ} {}
    ASTGEN_MEMBERS_Ast_ಪಠ್ಯ;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಪಠ್ಯ() == VN_DBG_AS(samep, ಪಠ್ಯ_2)->ಪಠ್ಯ();
    }
    ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }
};
class Ast_ಪಠ್ಯ_ಖಂಡ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ಸ್ಥಿರಾಂಕ std::string m_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ಸ್ಥಿರಾಂಕ std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ಸ್ಥಿರಾಂಕ std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಪಠ್ಯ_ಖಂಡ(File_ಸಾಲು* fl,  //
                          ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ = "",  //
                          ಸ್ಥಿರಾಂಕ std::string& separator = "",  //
                          ಸ್ಥಿರಾಂಕ std::string& suffix = "")
        : ASTGEN_SUPER_ಪಠ್ಯ_ಖಂಡ(fl)
        , m_ಪೂರ್ವಪ್ರತ್ಯಯ{ಪೂರ್ವಪ್ರತ್ಯಯ}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_Ast_ಪಠ್ಯ_ಖಂಡ;
    ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪೂರ್ವಪ್ರತ್ಯಯ; }
    ಸ್ಥಿರಾಂಕ std::string& separator() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_separator; }
    ಸ್ಥಿರಾಂಕ std::string& suffix() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_suffix; }
    // Add some text, or a node to this block
    ಶೂನ್ಯ ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) { ಕೂಡಿಸು_Nodesp(new Ast_ಪಠ್ಯ{fileline(), ಪಠ್ಯ}); }
    ಶೂನ್ಯ ಕೂಡಿಸು(Ast_ಗಂಟು* nodep) { ಕೂಡಿಸು_Nodesp(nodep); }
};
class Ast_ತುದಿ_Scope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    ಮಿತ್ರ class AstNetlist;  // Only the AstNetlist can create one
    Ast_ತುದಿ_Scope(File_ಸಾಲು* fl, AstScope* ascopep)
        : ASTGEN_SUPER_ತುದಿ_Scope(fl) {
        this->scopep(ascopep);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ತುದಿ_Scope;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* create_ತಾತ್ಕಾಲಿಕ(ಸ್ಥಿರಾಂಕ string& name, ಚಿಹ್ನೆರಹಿತ ಅಗಲ);
    AstVarScope* create_ತಾತ್ಕಾಲಿಕ(ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_DType* dtypep);
};
class Ast_Type_ಕೋಷ್ಟಕ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Container for hash of standard data types
    // @astgen op1 := typesp : List[AstNodeDType]
    //
    // @astgen ptr := m_constraintRefp : Optional[AstConstraintRefDType]
    // @astgen ptr := m_emptyQueuep : Optional[AstEmptyQueueDType]
    // @astgen ptr := m_queueIndexp : Optional[AstQueueDType]
    // @astgen ptr := m_streamp : Optional[AstStreamDType]
    // @astgen ptr := m_voidp : Optional[AstVoidDType]
    AstBasicDType* m_basicps[VBasicDTypeKwd::_ENUM_MAX]{};
    //
    using DetailedMap = std::map<VBasic_Type_ಕೀಲಿ, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

ಸಾರ್ವಜನಿಕ:
    explicit Ast_Type_ಕೋಷ್ಟಕ(File_ಸಾಲು* fl);
    ASTGEN_MEMBERS_Ast_Type_ಕೋಷ್ಟಕ;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* ಕಂಡುಹಿಡಿ_Basic_DType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd);
    AstBasicDType* ಕಂಡುಹಿಡಿ_Logic_ಬಿಟ್_DType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min,
                                     VSigning numeric);
    AstBasicDType* ಕಂಡುಹಿಡಿ_Logic_ಬಿಟ್_DType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, ಸ್ಥಿರಾಂಕ VNumRange& range,
                                     ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric);
    AstBasicDType* ಕಂಡುಹಿಡಿ_Create_Same_DType(AstBasicDType& ಗಂಟು);
    AstBasicDType* ಕಂಡುಹಿಡಿ_ಸೇರಿಸು_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* ಕಂಡುಹಿಡಿ_Constraint_Ref_DType(File_ಸಾಲು* fl);
    Ast_ಖಾಲಿಗಳು_ಸರತಿ_DType* ಕಂಡುಹಿಡಿ_ಖಾಲಿಗಳು_ಸರತಿ_DType(File_ಸಾಲು* fl);
    Ast_ಸರತಿ_DType* ಕಂಡುಹಿಡಿ_ಸರತಿ_ಸೂಚ್ಯಂಕ_DType(File_ಸಾಲು* fl);
    Ast_ಹರಿವು_DType* ಕಂಡುಹಿಡಿ_ಹರಿವು_DType(File_ಸಾಲು* fl);
    AstVoidDType* ಕಂಡುಹಿಡಿ_Void_DType(File_ಸಾಲು* fl);
    ಶೂನ್ಯ clearCache();
    ಶೂನ್ಯ repairCache();
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
};
class AstTypedef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_ಟ್ಯಾಗ್;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_ಅಂಕೆ;  // Declaration token number
    bool m_attr_ಬಹಿರಂಗ = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

ಸಾರ್ವಜನಿಕ:
    AstTypedef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* attrsp, VFlag_ಮಗು_DType,
               Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_ಅಂಕೆ{fl->token_ಅಂಕೆ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        ಮಗು_DTypep(dtp);  // Only for parser
        ಕೂಡಿಸು_Attrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_ಅಂಕೆ{fl->token_ಅಂಕೆ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ dtypep() ? dtypep() : ಮಗು_DTypep();
    }
    // METHODS
    uint32_t decl_Token_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_decl_Token_ಅಂಕೆ; }
    ಶೂನ್ಯ decl_Token_ಅಂಕೆ_ಗಣ_Min(uint32_t token_ಅಂಕೆ) override {
        m_decl_Token_ಅಂಕೆ = std::min(m_decl_Token_ಅಂಕೆ, token_ಅಂಕೆ);
    }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& flag) override { m_name = flag; }
    bool attr_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attr_ಬಹಿರಂಗ; }
    ಶೂನ್ಯ attr_ಬಹಿರಂಗ(bool flag) { m_attr_ಬಹಿರಂಗ = flag; }
    bool isHideLocal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isHideLocal; }
    ಶೂನ್ಯ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isHideProtected; }
    ಶೂನ್ಯ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isUnderClass; }
    ಶೂನ್ಯ isUnderClass(bool flag) { m_isUnderClass = flag; }
    ಶೂನ್ಯ ಟ್ಯಾಗ್(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) override { m_ಟ್ಯಾಗ್ = ಪಠ್ಯ; }
    string ಟ್ಯಾಗ್() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ಟ್ಯಾಗ್; }
};
class AstTypedefFwd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ಸ್ಥಿರಾಂಕ VFwdType m_fwdType;  // Forward type for lint check

ಸಾರ್ವಜನಿಕ:
    AstTypedefFwd(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    VFwdType fwdType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fwdType; }
};
class Ast_Udp_ಕೋಷ್ಟಕ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := linesp : List[AstUdpTableLine]
ಸಾರ್ವಜನಿಕ:
    Ast_Udp_ಕೋಷ್ಟಕ(File_ಸಾಲು* fl, Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು* linesp)
        : ASTGEN_SUPER_Udp_ಕೋಷ್ಟಕ(fl) {
        this->ಕೂಡಿಸು_Linesp(linesp);
        ಆದರೆ (!v3Global.has_ಕೋಷ್ಟಕ()) v3Global.ಗಣ_Has_ಕೋಷ್ಟಕ();
    }
    ASTGEN_MEMBERS_Ast_Udp_ಕೋಷ್ಟಕ;
};
class Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ bool m_udpIsCombo;  // Combinational or sequential UDP

ಸಾರ್ವಜನಿಕ:
    class UdpCombo {};
    Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು(UdpCombo, File_ಸಾಲು* fl, Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val* iFieldsp,
                    Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_ಕೋಷ್ಟಕ_ಸಾಲು(fl)
        , m_udpIsCombo{true} {
        ಕೂಡಿಸು_IFieldsp(iFieldsp);
        ಕೂಡಿಸು_OFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು(UdpSequential, File_ಸಾಲು* fl, Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val* iFieldsp,
                    Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val* oFieldsp1, Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_ಕೋಷ್ಟಕ_ಸಾಲು(fl)
        , m_udpIsCombo{false} {
        ಕೂಡಿಸು_IFieldsp(iFieldsp);
        ಕೂಡಿಸು_OFieldsp(oFieldsp1);
        ಕೂಡಿಸು_OFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ udpIsCombo() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_udpIsCombo; }
};
class Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    string m_ಪಠ್ಯ;  // Value character

ಸಾರ್ವಜನಿಕ:
    Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ)
        : ASTGEN_SUPER_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val(fl)
        , m_ಪಠ್ಯ{ಪಠ್ಯ} {}
    ASTGEN_MEMBERS_Ast_Udp_ಕೋಷ್ಟಕ_ಸಾಲು_Val;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }
    ಶೂನ್ಯ name(std::string ಸ್ಥಿರಾಂಕ& ಪಠ್ಯ) override VL_MT_STABLE { m_ಪಠ್ಯ = ಪಠ್ಯ; }
    string ಪಠ್ಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }
};
class AstVar final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A variable (in/out/wire/reg/param) inside a module
    //
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := delayp : Optional[AstDelay] // Net delay
    // Initial value that never changes (static const), or constructor argument for
    // MTASKSTATE variables
    // @astgen op3 := valuep : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //     Value is a DType for type parameter defaults
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse
    // @astgen ptr := m_sensIfacep : Optional[AstIface]  // Interface type to which reads from this
    //                                                      var are sensitive

    string m_name;  // Name of variable
    string m_origName;  // Original name before dot addition
    string m_ಟ್ಯಾಗ್;  // Holds the string of the verilator tag -- used in JSON output.
    VVarType m_varType;  // Type of variable
    VDirection m_direction;  // Direction input/output etc
    VDirection m_declDirection;  // Declared direction input/output etc
    VLifetime m_lifetime;  // Lifetime
    VRandAttr m_rand;  // Randomizability of this variable (rand, randc, etc)
    ಪೂರ್ಣಾಂಕ m_pin_ಅಂಕೆ = 0;  // For JSON, if non-zero the connection pin number
    ಪೂರ್ಣಾಂಕ m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
    bool m_libInoutEnable : 1;  // Port is the lowered library inout's enable
    bool m_ansi : 1;  // Params or pins declared in the module header, rather than the body
    bool m_declTyped : 1;  // Declared as type (for dedup check)
    bool m_tristate : 1;  // Inout or triwire or trireg
    bool m_primaryIO : 1;  // In/out to top level (or directly assigned from same)
    bool m_primaryClock : 1;  // In/out to top level and is, or combinationally drives a SenItem
    bool m_sc : 1;  // SystemC variable
    bool m_scSensitive : 1;  // SystemC sensitive() needed
    bool m_sig_ಬಹಿರಂಗ : 1;  // User C code accesses this signal or is top signal
    bool m_sig_Mod_ಬಹಿರಂಗ : 1;  // User C code accesses this signal and module
    bool m_sig_User_Rd_ಬಹಿರಂಗ : 1;  // User C code accesses this signal, read only
    bool m_sigUserRWPublic : 1;  // User C code accesses this signal, read-write
    bool m_ಬಳಸಿದ_Param : 1;  // Parameter is referenced (on link; later signals not setup)
    bool m_ಬಳಸಿದ_Loop_Idx : 1;  // Variable subject of for unrolling
    bool m_funcLocal : 1;  // Local variable for a function
    bool m_funcLocalSticky : 1;  // As m_funcLocal but remains set if var is moved to a static
    bool m_funcReturn : 1;  // Return variable for a function
    bool m_attrScBv : 1;  // User force bit vector attribute
    bool m_attrScBigUint : 1;  // User force sc_biguint attribute
    bool m_attrSFormat : 1;  // User sformat attribute
    bool m_attr_ವಿಭಜಿಸು_Var : 1;  // declared with split_var metacomment
    bool m_attrFsmState : 1;  // declared with fsm_state metacomment
    bool m_attrFsmArcInclCond : 1;  // declared with fsm_arc_include_cond metacomment
    VFsmExpandType::en m_attrFsmStateExpand : 2;  // FSM expansion type for FSM coverage
    bool m_const_Pool_ನಮೂದು : 1;  // Constant pool variable
    bool m_covergroupRefMember : 1;  // Persistent covergroup ref/const ref argument
    bool m_embeddedCovergroup : 1;  // Instance variable an embedded covergroup declares
    bool m_attrFileDescr : 1;  // File descriptor
    bool m_gotNansiType : 1;  // Linker saw Non-ANSI type declaration
    bool m_icoMaybeWritten : 1;  // Design might write this input signal - for ico change detect
    bool m_isClockingDriven : 1;  // Driven from inside by a clocking block output
    bool m_isConst : 1;  // Table contains constant data
    bool m_isContinuously : 1;  // Ever assigned continuously (for force/release)
    bool m_hasStrengthAssignment : 1;  // Is on LHS of assignment with strength specifier
    bool m_hasUserInit : 1;  // Has initial assignment by user at parse time
    bool m_isPulldown : 1;  // Tri0
    bool m_isPullup : 1;  // Tri1
    bool m_is_Iface_ಪೋಷಕ : 1;  // dtype is reference to interface present in this module
    bool m_is_Iface_Array_ವಿಭಜಿಸು : 1;  // Interface array that was split into element variables by
                                   // V3Param, until V3Width removes it
    bool m_is_ಆಂತರಿಕ : 1;  // Internal state, don't add to method pinter
    bool m_isIfaceParam : 1;  // Parameter belongs to an interface/modport
    bool m_isDpiOpenArray : 1;  // DPI import open array
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_noCReset : 1;  // Do not do automated CReset creation
    bool m_noReset : 1;  // Do not do automated reset/randomization
    bool m_noSubst : 1;  // Do not substitute out references
    bool m_sampled : 1;  // Sampled timing region
    bool m_substConstOnly : 1;  // Only substitute if constant
    bool m_overriddenParam : 1;  // Overridden parameter by #(...) or defparam
    bool m_untypedParam : 1;  // Parameter without a type or range, so of its value's type
    bool m_trace : 1;  // Trace this variable
    bool m_isLatched : 1;  // Not assigned in all control paths of combo always
    bool m_isForceable : 1;  // May be forced/released externally from user C code
    bool m_is_Forced_By_ಸಂಕೇತ : 1;  // May be forced/released from AstAssignForce/AstRelease
    bool m_isReadByDpi : 1;  // This variable can be read by a DPI Export
    bool m_isWrittenByDpi : 1;  // This variable can be written by a DPI Export
    bool m_isWrittenBySuspendable : 1;  // This variable can be written by a suspendable process
    bool m_ignorePostRead : 1;  // Ignore reads in 'Post' blocks during ordering
    bool m_ignorePostWrite : 1;  // Ignore writes in 'Post' blocks during ordering
    bool m_ignoreSchedWrite : 1;  // Ignore writes in scheduling (for special optimizations)
    bool m_dfgTriLowered : 1;  // Signal/temporary introduced by tristate lowering
    bool m_dfgAllowMultidriveTri : 1;  // Allow DFG MULTIDRIVEN warning for intentional tri nets
    bool m_globalConstrained : 1;  // Global constraint per IEEE 1800-2023 18.5.8
    bool m_isStdRandomizeArg : 1;  // Argument variable created for std::randomize (__Varg*)
    bool m_process_ಸರತಿ : 1;  // Process queue variable
    bool m_mtask_Cache_ಸಾಲು_Align : 1;  // Start MTask affinity group on a cache line
    ಶೂನ್ಯ init() {
        m_libInoutEnable = false;
        m_ansi = false;
        m_declTyped = false;
        m_tristate = false;
        m_primaryIO = false;
        m_primaryClock = false;
        m_sc = false;
        m_scSensitive = false;
        m_ಬಳಸಿದ_Param = false;
        m_ಬಳಸಿದ_Loop_Idx = false;
        m_sig_ಬಹಿರಂಗ = false;
        m_sig_Mod_ಬಹಿರಂಗ = false;
        m_sig_User_Rd_ಬಹಿರಂಗ = false;
        m_sigUserRWPublic = false;
        m_funcLocal = false;
        m_funcLocalSticky = false;
        m_funcReturn = false;
        m_attrScBv = false;
        m_attrScBigUint = false;
        m_attrSFormat = false;
        m_attr_ವಿಭಜಿಸು_Var = false;
        m_attrFsmState = false;
        m_attrFsmArcInclCond = false;
        m_attrFsmStateExpand = VFsmExpandType::DEFAULT;
        m_const_Pool_ನಮೂದು = false;
        m_covergroupRefMember = false;
        m_embeddedCovergroup = false;
        m_attrFileDescr = false;
        m_gotNansiType = false;
        m_icoMaybeWritten = false;
        m_isClockingDriven = false;
        m_isConst = false;
        m_isContinuously = false;
        m_hasStrengthAssignment = false;
        m_hasUserInit = false;
        m_isPulldown = false;
        m_isPullup = false;
        m_is_Iface_ಪೋಷಕ = false;
        m_is_Iface_Array_ವಿಭಜಿಸು = false;
        m_is_ಆಂತರಿಕ = false;
        m_isIfaceParam = false;
        m_isDpiOpenArray = false;
        m_isHideLocal = false;
        m_isHideProtected = false;
        m_noCReset = false;
        m_noReset = false;
        m_noSubst = false;
        m_sampled = false;
        m_substConstOnly = false;
        m_overriddenParam = false;
        m_untypedParam = false;
        m_trace = false;
        m_isLatched = false;
        m_isForceable = false;
        m_is_Forced_By_ಸಂಕೇತ = false;
        m_isReadByDpi = false;
        m_isWrittenByDpi = false;
        m_isWrittenBySuspendable = false;
        m_ignorePostRead = false;
        m_ignorePostWrite = false;
        m_ignoreSchedWrite = false;
        m_dfgTriLowered = false;
        m_dfgAllowMultidriveTri = false;
        m_globalConstrained = false;
        m_isStdRandomizeArg = false;
        m_process_ಸರತಿ = false;
        m_mtask_Cache_ಸಾಲು_Align = false;
    }

ಸಾರ್ವಜನಿಕ:
    AstVar(File_ಸಾಲು* fl, VVarType type, ಸ್ಥಿರಾಂಕ string& name, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ಮಗು_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_ಸಾಲು* fl, VVarType type, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_ಸಾಲು* fl, VVarType type, ಸ್ಥಿರಾಂಕ string& name, VFlagLogicPacked, ಪೂರ್ಣಾಂಕ wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_ಗಣ_Logic_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_ಸಾಲು* fl, VVarType type, ಸ್ಥಿರಾಂಕ string& name, VFlag_ಬಿಟ್_Packed, ಪೂರ್ಣಾಂಕ wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_ಗಣ_ಬಿಟ್_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_ಸಾಲು* fl, VVarType type, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ಆದರೆ (examplep->ಮಗು_DTypep()) ಮಗು_DTypep(examplep->ಮಗು_DTypep()->clone_ವೃಕ್ಷ(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಸಾಲೊಳಗೆ bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    string origName() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_origName; }  // * = Original name
    ಶೂನ್ಯ origName(ಸ್ಥಿರಾಂಕ string& name) { m_origName = name; }
    VVarType varType() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_varType; }  // * = Type of variable
    ಶೂನ್ಯ direction(ಸ್ಥಿರಾಂಕ VDirection& flag) {
        m_direction = flag;
        ಆದರೆ (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_direction; }
    bool isIO() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_direction != VDirection::NONE; }
    bool isVLIO() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = basicp();
        ಹಿಂತಿರುಗಿ isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    ಶೂನ್ಯ declDirection(ಸ್ಥಿರಾಂಕ VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_declDirection; }
    ಪೂರ್ಣಾಂಕ libInoutId() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libInoutId; }
    ಶೂನ್ಯ libInoutId(ಪೂರ್ಣಾಂಕ id) { m_libInoutId = id; }
    bool libInoutEnable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libInoutEnable; }
    ಶೂನ್ಯ libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    ಶೂನ್ಯ varType(VVarType type) { m_varType = type; }
    ಶೂನ್ಯ var_Type2_ಹೊರಹೋಗು() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    ಶೂನ್ಯ var_Type2_ಒಳ() {
        m_tristate = false;
        m_direction = VDirection::ಒಳಹರಿವು;
    }
    string scType() ಸ್ಥಿರಾಂಕ;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) ಸ್ಥಿರಾಂಕ;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) ಸ್ಥಿರಾಂಕ;
    string dpiTmpVarType(ಸ್ಥಿರಾಂಕ string& varName) ಸ್ಥಿರಾಂಕ;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) ಸ್ಥಿರಾಂಕ;
    string vlEnumType() ಸ್ಥಿರಾಂಕ;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) ಸ್ಥಿರಾಂಕ;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(ಸ್ಥಿರಾಂಕ string& propName) ಸ್ಥಿರಾಂಕ;  // Return VerilatorVarProps declaration
    ಶೂನ್ಯ combineType(VVarType type);
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* dtypeSkipRefp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ dtypep() ? dtypep() : ಮಗು_DTypep();
    }
    ಶೂನ್ಯ ansi(bool flag) { m_ansi = flag; }
    ಶೂನ್ಯ declTyped(bool flag) { m_declTyped = flag; }
    ಶೂನ್ಯ sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    ಶೂನ್ಯ attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    ಶೂನ್ಯ attrScBv(bool flag) { m_attrScBv = flag; }
    ಶೂನ್ಯ attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    ಶೂನ್ಯ attrSFormat(bool flag) { m_attrSFormat = flag; }
    ಶೂನ್ಯ attr_ವಿಭಜಿಸು_Var(bool flag) { m_attr_ವಿಭಜಿಸು_Var = flag; }
    ಶೂನ್ಯ attrFsmState(bool flag) { m_attrFsmState = flag; }
    ಶೂನ್ಯ attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    ಶೂನ್ಯ attrFsmStateExpand(ಸ್ಥಿರಾಂಕ VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool const_Pool_ನಮೂದು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_const_Pool_ನಮೂದು; }
    ಶೂನ್ಯ ಗಣ_Const_Pool_ನಮೂದು() { m_const_Pool_ನಮೂದು = true; }
    bool covergroupRefMember() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_covergroupRefMember; }
    ಶೂನ್ಯ covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_embeddedCovergroup; }
    ಶೂನ್ಯ embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    ಶೂನ್ಯ rand(ಸ್ಥಿರಾಂಕ VRandAttr flag) { m_rand = flag; }
    ಶೂನ್ಯ ಬಳಸಿದ_Param(bool flag) { m_ಬಳಸಿದ_Param = flag; }
    ಶೂನ್ಯ ಬಳಸಿದ_Loop_Idx(bool flag) { m_ಬಳಸಿದ_Loop_Idx = flag; }
    ಶೂನ್ಯ sig_ಬಹಿರಂಗ(bool flag) { m_sig_ಬಹಿರಂಗ = flag; }
    ಶೂನ್ಯ sig_Mod_ಬಹಿರಂಗ(bool flag) { m_sig_Mod_ಬಹಿರಂಗ = flag; }
    ಶೂನ್ಯ sig_User_Rd_ಬಹಿರಂಗ(bool flag) {
        m_sig_User_Rd_ಬಹಿರಂಗ = flag;
        ಆದರೆ (flag) sig_ಬಹಿರಂಗ(true);
    }
    ಶೂನ್ಯ sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        ಆದರೆ (flag) sig_User_Rd_ಬಹಿರಂಗ(true);
    }
    ಶೂನ್ಯ sc(bool flag) { m_sc = flag; }
    ಶೂನ್ಯ scSensitive(bool flag) { m_scSensitive = flag; }
    ಶೂನ್ಯ primaryIO(bool flag) { m_primaryIO = flag; }
    ಶೂನ್ಯ isConst(bool flag) { m_isConst = flag; }
    ಶೂನ್ಯ isContinuously(bool flag) { m_isContinuously = flag; }
    ಶೂನ್ಯ is_Iface_ಪೋಷಕ(bool flag) { m_is_Iface_ಪೋಷಕ = flag; }
    ಶೂನ್ಯ is_Iface_Array_ವಿಭಜಿಸು(bool flag) { m_is_Iface_Array_ವಿಭಜಿಸು = flag; }
    ಶೂನ್ಯ is_ಆಂತರಿಕ(bool flag) { m_is_ಆಂತರಿಕ = flag; }
    ಶೂನ್ಯ isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    ಶೂನ್ಯ funcLocal(bool flag) {
        m_funcLocal = flag;
        ಆದರೆ (flag) m_funcLocalSticky = true;
    }
    ಶೂನ್ಯ funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_gotNansiType; }
    ಶೂನ್ಯ gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasStrengthAssignment; }
    ಶೂನ್ಯ hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasUserInit; }
    ಶೂನ್ಯ hasUserInit(bool flag) { m_hasUserInit = flag; }
    ಶೂನ್ಯ icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_icoMaybeWritten; }
    ಶೂನ್ಯ isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isClockingDriven; }
    bool isDpiOpenArray() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_isDpiOpenArray; }
    ಶೂನ್ಯ isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isHideLocal; }
    ಶೂನ್ಯ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isHideProtected; }
    ಶೂನ್ಯ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noCReset; }
    ಶೂನ್ಯ noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noReset; }
    ಶೂನ್ಯ noReset(bool flag) { m_noReset = flag; }
    bool noSubst() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_noSubst; }
    ಶೂನ್ಯ noSubst(bool flag) { m_noSubst = flag; }
    bool process_ಸರತಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_process_ಸರತಿ; }
    ಶೂನ್ಯ process_ಸರತಿ(bool flag) { m_process_ಸರತಿ = flag; }
    bool sampled() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sampled; }
    ಶೂನ್ಯ sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_substConstOnly; }
    ಶೂನ್ಯ substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_overriddenParam; }
    ಶೂನ್ಯ overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_untypedParam; }
    ಶೂನ್ಯ untypedParam(bool flag) { m_untypedParam = flag; }
    ಶೂನ್ಯ trace(bool flag) { m_trace = flag; }
    ಶೂನ್ಯ isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isForceable; }
    ಶೂನ್ಯ ಗಣ_Forceable() { m_isForceable = true; }
    ಶೂನ್ಯ ಗಣ_Forced_By_ಸಂಕೇತ() { m_is_Forced_By_ಸಂಕೇತ = true; }
    bool isForced() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isForceable || m_is_Forced_By_ಸಂಕೇತ; }
    bool isReadByDpi() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isReadByDpi; }
    ಶೂನ್ಯ ಗಣ_Read_By_Dpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isWrittenByDpi; }
    ಶೂನ್ಯ ಗಣ_Written_By_Dpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isWrittenBySuspendable; }
    ಶೂನ್ಯ ಗಣ_Written_By_Suspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ignorePostRead; }
    ಶೂನ್ಯ ಗಣ_Ignore_Post_Read() { m_ignorePostRead = true; }
    bool ignorePostWrite() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ignorePostWrite; }
    ಶೂನ್ಯ ಗಣ_Ignore_Post_Write() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ignoreSchedWrite; }
    ಶೂನ್ಯ ಗಣ_Ignore_Sched_Write() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dfgTriLowered; }
    ಶೂನ್ಯ ಗಣ_Dfg_Tri_Lowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dfgAllowMultidriveTri; }
    ಶೂನ್ಯ ಗಣ_Dfg_Allow_Multidrive_Tri() { m_dfgAllowMultidriveTri = true; }
    ಶೂನ್ಯ globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_globalConstrained; }
    bool isStdRandomizeArg() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isStdRandomizeArg; }
    ಶೂನ್ಯ ಗಣ_Std_Randomize_Arg() { m_isStdRandomizeArg = true; }
    // METHODS
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    ಶೂನ್ಯ ಟ್ಯಾಗ್(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) override { m_ಟ್ಯಾಗ್ = ಪಠ್ಯ; }
    string ಟ್ಯಾಗ್() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ಟ್ಯಾಗ್; }
    bool isAnsi() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ansi; }
    bool isContinuously() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isContinuously; }
    bool isDeclTyped() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_declTyped; }
    bool isInout() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_direction.isInout(); }
    bool isInoutOrRef() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_direction.isInoutOrRef(); }
    bool is_ಒಳಹರಿವು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_direction.is_ಒಳಹರಿವು(); }
    bool isNonOutput() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_direction.isNonOutput(); }
    bool isReadOnly() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_direction.isReadOnly(); }
    bool isConstRef() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_direction.isConstRef(); }
    bool isRef() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_direction.isRef(); }
    bool isWritable() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_direction.isWritable(); }
    bool isTristate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_tristate; }
    bool isPrimaryIO() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_primaryIO; }
    bool isPrimaryInish() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_primaryClock; }
    ಶೂನ್ಯ ಗಣ_Primary_Clock() { m_primaryClock = true; }
    bool isIfaceRef() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType() == VVarType::IFACEREF; }
    ಶೂನ್ಯ ಗಣ_Iface_Ref() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool is_Iface_ಪೋಷಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_Iface_ಪೋಷಕ; }
    bool is_Iface_Array_ವಿಭಜಿಸು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_Iface_Array_ವಿಭಜಿಸು; }
    bool is_ಆಂತರಿಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_ಆಂತರಿಕ; }
    bool isIfaceParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isIfaceParam; }
    bool isSignal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType().isSignal(); }
    bool isNet() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType().isNet(); }
    bool isWor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType().isWor(); }
    bool isWiredNet() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType().isWiredNet(); }
    bool is_ತಾತ್ಕಾಲಿಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType().is_ತಾತ್ಕಾಲಿಕ(); }
    bool isClassMember() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType() == VVarType::MEMBER; }
    bool isVirtIface() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            ಹಿಂತಿರುಗಿ dtp->isVirtual();
        }
        ಹಿಂತಿರುಗಿ false;
    }
    bool is_Statement_ತಾತ್ಕಾಲಿಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType() == VVarType::STMTTEMP; }
    bool isXTemp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType() == VVarType::XTEMP; }
    bool isParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType().isParam(); }
    bool isGParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType() == VVarType::GPARAM; }
    bool isGenVar() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ isIO() || isSignal() || isClassMember() || is_ತಾತ್ಕಾಲಿಕ() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool is_ಬಿಟ್_Logic() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = basicp();
        ಹಿಂತಿರುಗಿ bdtypep && bdtypep->is_ಬಿಟ್_Logic();
    }
    bool is_ಬಳಸಿದ_Param() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸಿದ_Param; }
    bool is_ಬಳಸಿದ_Loop_Idx() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸಿದ_Loop_Idx; }
    bool mtask_Cache_ಸಾಲು_Align() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_mtask_Cache_ಸಾಲು_Align; }
    ಶೂನ್ಯ mtask_Cache_ಸಾಲು_Align(bool flag) { m_mtask_Cache_ಸಾಲು_Align = flag; }
    bool isSc() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_sc; }
    bool isScQuad() ಸ್ಥಿರಾಂಕ;
    bool isScBv() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    bool isScUint() ಸ್ಥಿರಾಂಕ;
    bool isScUintBool() ಸ್ಥಿರಾಂಕ;
    bool isScBigUint() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    bool isScSensitive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scSensitive; }
    bool is_Sig_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ;
    bool is_Sig_Mod_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sig_Mod_ಬಹಿರಂಗ && !isIfaceRef(); }
    bool is_Sig_User_Rd_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sig_User_Rd_ಬಹಿರಂಗ && !isIfaceRef(); }
    bool isSigUserRWPublic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_trace; }
    bool isRand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rand.isRand(); }
    bool isRandC() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rand.isRandC(); }
    bool isConst() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_isConst; }
    bool isLatched() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isLatched; }
    bool isFuncLocal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_funcLocal; }
    bool isFuncLocalSticky() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_funcLocalSticky; }
    bool isFuncReturn() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_funcReturn; }
    bool isPullup() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isPullup; }
    bool isPulldown() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isPulldown; }
    bool attrScBv() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attrScBv; }
    bool attrScBigUint() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attrScBigUint; }
    bool attrFileDescr() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attrFileDescr; }
    bool attrSFormat() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attrSFormat; }
    bool attr_ವಿಭಜಿಸು_Var() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attr_ವಿಭಜಿಸು_Var; }
    bool attrFsmState() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attrFsmState; }
    bool attrFsmArcInclCond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_attrFsmStateExpand; }
    AstIface* sensIfacep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sensIfacep; }
    VRandAttr rand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rand; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ lifetime(ಸ್ಥಿರಾಂಕ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lifetime; }
    ಶೂನ್ಯ pin_ಅಂಕೆ(ಪೂರ್ಣಾಂಕ id) { m_pin_ಅಂಕೆ = id; }
    ಪೂರ್ಣಾಂಕ pin_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pin_ಅಂಕೆ; }
    ಶೂನ್ಯ propagateAttrFrom(ಸ್ಥಿರಾಂಕ AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        ಆದರೆ (fromp->attrFileDescr()) attrFileDescr(true);
        ಆದರೆ (fromp->isContinuously()) isContinuously(true);
    }
    ಶೂನ್ಯ propagate_ವಿಭಜಿಸು_Attr_From(ಸ್ಥಿರಾಂಕ AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attr_ವಿಭಜಿಸು_Var(fromp->attr_ವಿಭಜಿಸು_Var());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cannot_ವಿಭಜಿಸು_ಬಗೆ_Reason() ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ propagateWrapAttrFrom(ಸ್ಥಿರಾಂಕ AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    ಶೂನ್ಯ combineType(ಸ್ಥಿರಾಂಕ AstVar* otherp);
    bool needsCReset() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ !is_Iface_ಪೋಷಕ() && !isIfaceRef() && !noReset() && !isParam() && !is_Statement_ತಾತ್ಕಾಲಿಕ()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    ಸ್ಥಿರ AstVar* scVarRecurse(Ast_ಗಂಟು* nodep);
};
class AstVarScope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A particular scoped usage of a variable
    // That is, as a module is used under multiple cells, we get a different
    // varscope for each var in the module
    // Parents: MODULE
    // Children: none
    //
    // @astgen ptr := m_scopep : Optional[AstScope]  // Scope variable is underneath
    // @astgen ptr := m_varp : Optional[AstVar]  // [AfterLink] Pointer to variable itself
    bool m_trace : 1;  // Tracing is turned on for this scope
    bool m_optimizeLifePost : 1;  // One half of an NBA pair using ShadowVariable scheme. Optimize.
    bool m_fork_By_ಮೌಲ್ಯ : 1;  // Set before a fork, whose processes get it by value
ಸಾರ್ವಜನಿಕ:
    AstVarScope(File_ಸಾಲು* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_ಮೌಲ್ಯ = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    ಶೂನ್ಯ cloneRelink() override {
        ಆದರೆ (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ scopep()->name() + "->" + varp()->name(); }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    AstVar* varp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_varp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_scopep; }  // Pointer to scope it's under
    ಶೂನ್ಯ scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_trace; }
    ಶೂನ್ಯ trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_optimizeLifePost; }
    ಶೂನ್ಯ optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fork_By_ಮೌಲ್ಯ; }
    ಶೂನ್ಯ fork_By_ಮೌಲ್ಯ(bool flag) { m_fork_By_ಮೌಲ್ಯ = flag; }
};

// === AstNodeConsItem ===
class Ast_Cons_Array_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_ಅಂಶ {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ಸ್ಥಿರಾಂಕ uint64_t m_ಸೂಚ್ಯಂಕ;  // Index of the element, with the array's lo() at 0
ಸಾರ್ವಜನಿಕ:
    Ast_Cons_Array_ಅಂಶ(File_ಸಾಲು* fl, uint64_t ಸೂಚ್ಯಂಕ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Cons_Array_ಅಂಶ(fl, valuep)
        , m_ಸೂಚ್ಯಂಕ{ಸೂಚ್ಯಂಕ} {}
    ASTGEN_MEMBERS_Ast_Cons_Array_ಅಂಶ;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), Cons_Array_ಅಂಶ));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_ಸೂಚ್ಯಂಕ == VN_DBG_AS(samep, Cons_Array_ಅಂಶ)->m_ಸೂಚ್ಯಂಕ;
    }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }  // By AstConsArray's map
    uint64_t ಸೂಚ್ಯಂಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸೂಚ್ಯಂಕ; }
};
class Ast_Cons_Assoc_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_ಅಂಶ {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
ಸಾರ್ವಜನಿಕ:
    Ast_Cons_Assoc_ಅಂಶ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* keyp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Cons_Assoc_ಅಂಶ(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Assoc_ಅಂಶ;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), Cons_Assoc_ಅಂಶ));
        ಹಿಂತಿರುಗಿ nullptr;
    }
};
class Ast_Cons_Dyn_Array_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_ಅಂಶ {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
ಸಾರ್ವಜನಿಕ:
    Ast_Cons_Dyn_Array_ಅಂಶ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_ಅಂಶ(fl, valuep) {}
    Ast_Cons_Dyn_Array_ಅಂಶ(File_ಸಾಲು* fl, AstConst* keyp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_ಅಂಶ(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Dyn_Array_ಅಂಶ;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), Cons_Dyn_Array_ಅಂಶ));
        ಹಿಂತಿರುಗಿ nullptr;
    }
};
class Ast_Cons_ಸರತಿ_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_ಅಂಶ {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
ಸಾರ್ವಜನಿಕ:
    Ast_Cons_ಸರತಿ_ಅಂಶ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Cons_ಸರತಿ_ಅಂಶ(fl, valuep) {}
    ASTGEN_MEMBERS_Ast_Cons_ಸರತಿ_ಅಂಶ;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), Cons_ಸರತಿ) && !VN_IS(backp(), Cons_ಸರತಿ_ಅಂಶ));
        ಹಿಂತಿರುಗಿ nullptr;
    }
};
class Ast_Cons_Struct_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_ಅಂಶ {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
ಸಾರ್ವಜನಿಕ:
    Ast_Cons_Struct_ಅಂಶ(File_ಸಾಲು* fl, AstMemberDType* memberp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Cons_Struct_ಅಂಶ(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_Ast_Cons_Struct_ಅಂಶ;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), Cons_Struct_ಅಂಶ));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    AstMemberDType* memberp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_memberp; }
};
class Ast_Cons_Wildcard_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_ಅಂಶ {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
ಸಾರ್ವಜನಿಕ:
    Ast_Cons_Wildcard_ಅಂಶ(File_ಸಾಲು* fl, AstConst* keyp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Cons_Wildcard_ಅಂಶ(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Wildcard_ಅಂಶ;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), Cons_Wildcard_ಅಂಶ));
        ಹಿಂತಿರುಗಿ nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsm_ಗಮ್ಯ;  // FSM destination state label
    string m_fsm_ಟ್ಯಾಗ್;  // FSM arc kind tag (e.g. reset, reset_include, default)
    ಪೂರ್ಣಾಂಕ m_ಸ್ಥಾನಾಂತರ;  // Offset column numbers to uniq-ify IFs
ಸಾರ್ವಜನಿಕ:
    AstCoverOtherDecl(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪುಟ, ಸ್ಥಿರಾಂಕ string& comment,
                      ಸ್ಥಿರಾಂಕ string& linescov, ಪೂರ್ಣಾಂಕ ಸ್ಥಾನಾಂತರ, ಸ್ಥಿರಾಂಕ string& fsmVar = "",
                      ಸ್ಥಿರಾಂಕ string& fsmFrom = "", ಸ್ಥಿರಾಂಕ string& fsm_ಗಮ್ಯ = "",
                      ಸ್ಥಿರಾಂಕ string& fsm_ಟ್ಯಾಗ್ = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, ಪುಟ, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsm_ಗಮ್ಯ{fsm_ಗಮ್ಯ}
        , m_fsm_ಟ್ಯಾಗ್{fsm_ಟ್ಯಾಗ್}
        , m_ಸ್ಥಾನಾಂತರ{ಸ್ಥಾನಾಂತರ} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ ಸ್ಥಾನಾಂತರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸ್ಥಾನಾಂತರ; }
    ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    ಸ್ಥಿರಾಂಕ string& linescov() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_linescov; }
    ಸ್ಥಿರಾಂಕ string& fsmVar() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fsmVar; }
    ಸ್ಥಿರಾಂಕ string& fsmFrom() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fsmFrom; }
    ಸ್ಥಿರಾಂಕ string& fsm_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fsm_ಗಮ್ಯ; }
    ಸ್ಥಿರಾಂಕ string& fsm_ಟ್ಯಾಗ್() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fsm_ಟ್ಯಾಗ್; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstCoverOtherDecl* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, CoverOtherDecl);
        ಹಿಂತಿರುಗಿ Super::same_ಗಂಟು(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ಸ್ಥಿರಾಂಕ VNumRange m_range;  // Packed array range covering each toggle bit
ಸಾರ್ವಜನಿಕ:
    AstCoverToggleDecl(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಪುಟ, ಸ್ಥಿರಾಂಕ string& comment,
                       ಸ್ಥಿರಾಂಕ VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, ಪುಟ, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        ಹಿಂತಿರುಗಿ 2 * m_range.elements();
    }
    ಸ್ಥಿರಾಂಕ VNumRange& range() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_range; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstCoverToggleDecl* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, CoverToggleDecl);
        ಹಿಂತಿರುಗಿ Super::same_ಗಂಟು(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // A function inside a module
ಸಾರ್ವಜನಿಕ:
    AstFunc(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* stmtp, Ast_ಗಂಟು* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_FTask* cloneType(ಸ್ಥಿರಾಂಕ string& name) override {
        ಹಿಂತಿರುಗಿ new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "function"; }
};
class AstLet final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstLet(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    Ast_ಗಂಟು_FTask* cloneType(ಸ್ಥಿರಾಂಕ string& name) override { ಹಿಂತಿರುಗಿ new AstLet{fileline(), name}; }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "let"; }
};
class AstProperty final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // A property inside a module
ಸಾರ್ವಜನಿಕ:
    AstProperty(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_FTask* cloneType(ಸ್ಥಿರಾಂಕ string& name) override {
        ಹಿಂತಿರುಗಿ new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "property"; }
};
class AstSequence final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
ಸಾರ್ವಜನಿಕ:
    AstSequence(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_FTask* cloneType(ಸ್ಥಿರಾಂಕ string& name) override {
        ಹಿಂತಿರುಗಿ new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "sequence"; }
    bool isReferenced() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_referenced; }
    ಶೂನ್ಯ isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // A task inside a module
ಸಾರ್ವಜನಿಕ:
    AstTask(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_ಗಂಟು_FTask* cloneType(ಸ್ಥಿರಾಂಕ string& name) override {
        ಹಿಂತಿರುಗಿ new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "task"; }
};

// === AstNodeFile ===
class AstCFile final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_ಮೂಲ : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
ಸಾರ್ವಜನಿಕ:
    AstCFile(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_ಮೂಲ{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    uint64_t complexityScore() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_complexityScore; }
    ಶೂನ್ಯ complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_slow; }
    ಶೂನ್ಯ slow(bool flag) { m_slow = flag; }
    bool ಮೂಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಮೂಲ; }
    ಶೂನ್ಯ ಮೂಲ(bool flag) { m_ಮೂಲ = flag; }
    bool support() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_support; }
    ಶೂನ್ಯ support(bool flag) VL_MT_ಸುರಕ್ಷಿತ { m_support = flag; }
};
class AstVFile final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_File {
    // Verilog output file
    // Parents:  NETLIST
ಸಾರ್ವಜನಿಕ:
    AstVFile(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
};
class AstCoverCross final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Func_Cov_ಅಂಶ {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
ಸಾರ್ವಜನಿಕ:
    AstCoverCross(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstCoverpointRef* itemsp,
                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        ಕೂಡಿಸು_Itemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class AstCoverpoint final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Func_Cov_ಅಂಶ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
ಸಾರ್ವಜನಿಕ:
    AstCoverpoint(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp = nullptr, Ast_ಗಂಟು* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        ಕೂಡಿಸು_Binsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};

// === AstNodeGen ===
class Ast_Gen_ಖಂಡ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ಸ್ಥಿರಾಂಕ bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ಸ್ಥಿರಾಂಕ bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

ಸಾರ್ವಜನಿಕ:
    Ast_Gen_ಖಂಡ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* itemsp, bool implied)
        : ASTGEN_SUPER_Gen_ಖಂಡ(fl)
        , m_name{name}
        , m_unnamed{name.ಖಾಲಿಗಳು()}
        , m_implied{implied} {
        this->ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_ಖಂಡ;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ std::string& name) override { m_name = name; }
    bool unnamed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unnamed; }
    bool implied() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_implied; }
    AstDefaultDisable* defaultDisablep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_defaultDisablep; }
    ಶೂನ್ಯ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(ಸ್ಥಿರಾಂಕ string& outer) ಸ್ಥಿರಾಂಕ;
};
class AstGenCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
ಸಾರ್ವಜನಿಕ:
    AstGenCase(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Gen_Case_ಅಂಶ* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstGenFor(File_ಸಾಲು* fl, Ast_ಗಂಟು* initsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* incsp, Ast_ಗಂಟು* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->ಕೂಡಿಸು_Initsp(initsp);
        this->condp(condp);
        this->ಕೂಡಿಸು_Incsp(incsp);
        this->ಕೂಡಿಸು_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstGenIf(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp, Ast_ಗಂಟು* thensp, Ast_ಗಂಟು* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->ಕೂಡಿಸು_Thensp(thensp);
        this->ಕೂಡಿಸು_Elsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtype_Name_ತುಂಬಿದ;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_ಅಂಕೆ;  // Declaration token number
    VBaseOverride m_ತಳ_Override;  // BaseOverride (inital/final/extends)
    bool m_hasRandVarsUpdate = false;  // Has updateRandVars method,
                                       // which updates pointers to rand variables in clone()
    bool m_covergroup = false;  // Is covergroup (TODO perhaps make a new Ast node type for CG?)
    bool m_extended = false;  // Is extension or extended by other classes
    bool m_interfaceClass = false;  // Interface class
    bool m_needRNG = false;  // Need RNG, uses srandom/randomize
    bool m_use_Virtual_ಬಹಿರಂಗ = false;  // Subclasses need virtual public as uses interface class
    bool m_virtual = false;  // Virtual class
    bool m_printedFrom = false;  // This class is printed from i.e. is used as format arg.
    // Mutable, as only a recursion guard of the const dtypeName(), so also not dumped
    mutable bool m_dtypeNameBusy = false;  // In dtypeName(), which a parameter may lead back to
    // Covergroup options (when m_covergroup is true)
    ಪೂರ್ಣಾಂಕ m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool ತುಂಬಿದ_2, ಸ್ಥಿರಾಂಕ string& scope) ಸ್ಥಿರಾಂಕ;
    string dtypeNameScope() ಸ್ಥಿರಾಂಕ;  // Prefix of dtypeName(true) for the scope declaring the class

ಸಾರ್ವಜನಿಕ:
    AstClass(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_ಅಂಕೆ{fl->token_ಅಂಕೆ()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ isCovergroup() ? "covergroup" : "class"; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool timescaleMatters() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    AstClassPackage* classOrPackagep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_covergroupEnclosingClassp;
    }
    ಶೂನ್ಯ covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_ಗಂಟು* membersp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ stmtsp(); }
    ಶೂನ್ಯ ಕೂಡಿಸು_Membersp(Ast_ಗಂಟು* nodep) { ಕೂಡಿಸು_Stmtsp(nodep); }
    bool hasRandVarsUpdate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasRandVarsUpdate; }
    ಶೂನ್ಯ hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_covergroup; }
    ಶೂನ್ಯ isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_extended; }
    ಶೂನ್ಯ isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_interfaceClass; }
    ಶೂನ್ಯ isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_virtual; }
    ಶೂನ್ಯ isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needRNG; }
    ಶೂನ್ಯ needRNG(bool flag) { m_needRNG = flag; }
    bool use_Virtual_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_use_Virtual_ಬಹಿರಂಗ; }
    ಶೂನ್ಯ use_Virtual_ಬಹಿರಂಗ(bool flag) { m_use_Virtual_ಬಹಿರಂಗ = flag; }
    ಶೂನ್ಯ markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtype_Name_ಒಳ(ಸ್ಥಿರಾಂಕ string& scope) ಸ್ಥಿರಾಂಕ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ಶೂನ್ಯ dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_dtype_Name_ತುಂಬಿದ.ಖಾಲಿಗಳು(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_ಸಂದೇಶ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    ಸ್ಥಿರ string embeddedCovergroupTypeName(ಸ್ಥಿರಾಂಕ string& name) { ಹಿಂತಿರುಗಿ "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ಸ್ಥಿರಾಂಕ;
    // Covergroup options accessors
    ಪೂರ್ಣಾಂಕ cgAutoBinMax() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cgAutoBinMax; }
    ಶೂನ್ಯ cgAutoBinMax(ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) { m_cgAutoBinMax = ಮೌಲ್ಯ; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    ಸ್ಥಿರ bool isClassExtendedFrom(ಸ್ಥಿರಾಂಕ AstClass* refClassp, ಸ್ಥಿರಾಂಕ AstClass* ತಳ_Classp);
    uint32_t decl_Token_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_decl_Token_ಅಂಕೆ; }
    ಶೂನ್ಯ decl_Token_ಅಂಕೆ_ಗಣ_Min(uint32_t token_ಅಂಕೆ) override {
        m_decl_Token_ಅಂಕೆ = std::min(m_decl_Token_ಅಂಕೆ, token_ಅಂಕೆ);
    }
    ಶೂನ್ಯ ತಳ_Override(ಸ್ಥಿರಾಂಕ VBaseOverride& flag) { m_ತಳ_Override = flag; }
    VBaseOverride ತಳ_Override() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಳ_Override; }
    // Return the lowest class extended from, or this class
    AstClass* ತಳ_Most_Classp();
    ಸ್ಥಿರ bool is_Cacheable_ಮಗು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    ಮಾದರಿ <typename T_Callable>
    ಶೂನ್ಯ foreachMember(ಸ್ಥಿರಾಂಕ T_Callable& f) {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_ಗಂಟು*>::ಮೌಲ್ಯ
                && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassExtends* ಸ್ಥಿರಾಂಕ cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (Ast_ಗಂಟು::ಖಾಸಗಿ_Type_Test<T_ಗಂಟು>(stmtp)) f(this, static_cast<T_ಗಂಟು*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    ಮಾದರಿ <typename T_Callable>
    bool existsMember(ಸ್ಥಿರಾಂಕ T_Callable& p) ಸ್ಥಿರಾಂಕ {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ T_ಗಂಟು*>::ಮೌಲ್ಯ
                && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassExtends* ಸ್ಥಿರಾಂಕ cextendsp = this->extendsp()) {
            ಆದರೆ (cextendsp->classp()->existsMember(p)) ಹಿಂತಿರುಗಿ true;
        }
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (Ast_ಗಂಟು::ಖಾಸಗಿ_Type_Test<T_ಗಂಟು>(stmtp)) {
                ಆದರೆ (p(this, static_cast<T_ಗಂಟು*>(stmtp))) ಹಿಂತಿರುಗಿ true;
            }
        }
        ಹಿಂತಿರುಗಿ false;
    }
};
class AstClassPackage final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
ಸಾರ್ವಜನಿಕ:
    AstClassPackage(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "classpackage"; }
    bool timescaleMatters() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    AstClass* classp() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_classp; }
    ಶೂನ್ಯ classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // An interface declaration
    string m_dtype_Name_ತುಂಬಿದ;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
ಸಾರ್ವಜನಿಕ:
    AstIface(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
    bool hasVirtualRef() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasVirtualRef; }
    ಶೂನ್ಯ ಗಣ_Has_Virtual_Ref() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ಶೂನ್ಯ dtypeNameFreeze();
};
class AstModule final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // A module declaration
    ಸ್ಥಿರಾಂಕ bool m_isChecker;  // Module represents a checker
    ಸ್ಥಿರಾಂಕ bool m_is_ಕಾರ್ಯಕ್ರಮ;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

ಸಾರ್ವಜನಿಕ:
    class Checker {};  // for constructor type-overload selection
    class ಕಾರ್ಯಕ್ರಮ {};  // for constructor type-overload selection
    AstModule(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_ಕಾರ್ಯಕ್ರಮ{false} {}
    AstModule(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_is_ಕಾರ್ಯಕ್ರಮ{false} {}
    AstModule(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname, ಕಾರ್ಯಕ್ರಮ)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_ಕಾರ್ಯಕ್ರಮ{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ m_isChecker ? "checker" : m_is_ಕಾರ್ಯಕ್ರಮ ? "program" : "module";
    }
    bool timescaleMatters() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isChecker() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isChecker; }
    bool is_ಕಾರ್ಯಕ್ರಮ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_ಕಾರ್ಯಕ್ರಮ; }
    bool hasGenericIface() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasGenericIface; }
    ಶೂನ್ಯ hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};
class Ast_Not_ಸಿಕ್ಕಿತು_Module final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // A missing module declaration
ಸಾರ್ವಜನಿಕ:
    Ast_Not_ಸಿಕ್ಕಿತು_Module(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname)
        : ASTGEN_SUPER_Not_ಸಿಕ್ಕಿತು_Module(fl, name, libname) {}
    ASTGEN_MEMBERS_Ast_Not_ಸಿಕ್ಕಿತು_Module;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "/*not-found-*/ module"; }
    bool timescaleMatters() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstPackage final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // A package declaration
ಸಾರ್ವಜನಿಕ:
    AstPackage(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "package"; }
    bool timescaleMatters() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // A primitive declaration
ಸಾರ್ವಜನಿಕ:
    AstPrimitive(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "primitive"; }
    bool timescaleMatters() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};

// === AstNodeProcedure ===
class AstAlways final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ಸ್ಥಿರಾಂಕ VAlwaysKwd m_keyword;

ಸಾರ್ವಜನಿಕ:
    AstAlways(File_ಸಾಲು* fl, VAlwaysKwd keyword, Ast_Sen_ವೃಕ್ಷ* sentreep, Ast_ಗಂಟು* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit ಸಾಲೊಳಗೆ AstAlways(Ast_ನಿಯೋಜಿಸು_W* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    VAlwaysKwd keyword() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_keyword; }
};
class AstAlwaysObserved final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

ಸಾರ್ವಜನಿಕ:
    AstAlwaysObserved(File_ಸಾಲು* fl, Ast_Sen_ವೃಕ್ಷ* sentreep, Ast_ಗಂಟು* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

ಸಾರ್ವಜನಿಕ:
    explicit AstAlwaysPost(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // Like always but Postponed scheduling region

ಸಾರ್ವಜನಿಕ:
    AstAlwaysPostponed(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

ಸಾರ್ವಜನಿಕ:
    explicit AstAlwaysPre(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

ಸಾರ್ವಜನಿಕ:
    AstAlwaysReactive(File_ಸಾಲು* fl, Ast_Sen_ವೃಕ್ಷ* sentreep, Ast_ಗಂಟು* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
ಸಾರ್ವಜನಿಕ:
    AstFinal(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
ಸಾರ್ವಜನಿಕ:
    AstInitial(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
ಸಾರ್ವಜನಿಕ:
    AstInitialAutomatic(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
ಸಾರ್ವಜನಿಕ:
    AstInitialStatic(File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
ಸಾರ್ವಜನಿಕ:
    AstBracketRange(File_ಸಾಲು* fl, Ast_ಗಂಟು* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    ವಾಸ್ತವಿಕ string emitC() { V3ERROR_NA_RETURN(""); }
    ವಾಸ್ತವಿಕ string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ false; }
};
class AstRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ಸ್ಥಿರಾಂಕ bool m_fromBracket = false;  // From C-style '[x]' declaration
ಸಾರ್ವಜನಿಕ:
    AstRange(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* leftp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    ಸಾಲೊಳಗೆ AstRange(File_ಸಾಲು* fl, ಪೂರ್ಣಾಂಕ ಎಡ_2, ಪೂರ್ಣಾಂಕ ಬಲ_2);
    ಸಾಲೊಳಗೆ AstRange(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ ಎಡ_Const() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ ಬಲ_Const() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಪೂರ್ಣಾಂಕ hiConst() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ l = ಎಡ_Const();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ r = ಬಲ_Const();
        ಹಿಂತಿರುಗಿ l > r ? l : r;
    }
    ಪೂರ್ಣಾಂಕ loConst() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ l = ಎಡ_Const();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ r = ಬಲ_Const();
        ಹಿಂತಿರುಗಿ l > r ? r : l;
    }
    ಪೂರ್ಣಾಂಕ elementsConst() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ hiConst() - loConst() + 1; }
    bool ascending() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಎಡ_Const() < ಬಲ_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_ಎಡ_ಗಮ್ಯ_ಬಲ() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ VNumRange{ಎಡ_Const(), ಬಲ_Const()}.seq_ಎಡ_ಗಮ್ಯ_ಬಲ();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_ಬಲ_ಗಮ್ಯ_ಎಡ() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ VNumRange{ಎಡ_Const(), ಬಲ_Const()}.seq_ಬಲ_ಗಮ್ಯ_ಎಡ();
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ವಾಸ್ತವಿಕ string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstRange* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, Range);
        ಹಿಂತಿರುಗಿ fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fromBracket; }
};
class AstUnsizedRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Range {
    // Unsized range specification, for open arrays
ಸಾರ್ವಜನಿಕ:
    explicit AstUnsizedRange(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    ವಾಸ್ತವಿಕ string emitC() { V3ERROR_NA_RETURN(""); }
    ವಾಸ್ತವಿಕ string emitVerilog() { ಹಿಂತಿರುಗಿ "[]"; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstWildcardRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Range {
    // Wildcard range specification, for wildcard index type associative arrays
ಸಾರ್ವಜನಿಕ:
    explicit AstWildcardRange(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    ವಾಸ್ತವಿಕ string emitC() { V3ERROR_NA_RETURN(""); }
    ವಾಸ್ತವಿಕ string emitVerilog() { ಹಿಂತಿರುಗಿ "[*]"; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* /*samep*/) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
