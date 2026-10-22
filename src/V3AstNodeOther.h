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

#ifndef VERILATOR_V3ASTNODES_H_
#define VERILATOR_V3ASTNODES_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

#ಅನ್ತರ್ಭಾವ <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_ಗಂಟು_Cons_Item VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    friend class Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ;  // For m_knownConstantTree
    mutable bool m_known_Constant_ವೃಕ್ಷ = false;  // Known to be a tree of constants

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Cons_Item(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : Ast_ಗಂಟು{t, fl} {
        this->valuep(valuep);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Cons_Item;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class Ast_ಗಂಟು_Cover_Decl VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    ಪೂರ್ಣಾಙ್ಕ m_bin_ಅಂಕೆ = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    ಪೂರ್ಣಾಙ್ಕ m_local_Bin_ಅಂಕೆ = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Cover_Decl(VNType t, File_ಸಾಲು* fl, ಧ್ರುವ string& page, ಧ್ರುವ string& comment)
        : Ast_ಗಂಟು(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Cover_Decl;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        ಯದಿ (m_dataDeclp
            && (m_dataDeclp == this || m_dataDeclp->m_dataDeclp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1 + 2 * INSTR_ಎಣಿಕೆ_LD; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಪೂರ್ಣಾಙ್ಕ bin_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bin_ಅಂಕೆ; }
    ಶೂನ್ಯ bin_ಅಂಕೆ(ಪೂರ್ಣಾಙ್ಕ flag) { m_bin_ಅಂಕೆ = flag; }
    ಪೂರ್ಣಾಙ್ಕ local_Bin_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_local_Bin_ಅಂಕೆ; }
    ಶೂನ್ಯ local_Bin_ಅಂಕೆ(ಪೂರ್ಣಾಙ್ಕ flag) { m_local_Bin_ಅಂಕೆ = flag; }
    bool perInstance() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_perInstance; }
    ಶೂನ್ಯ perInstance(bool flag) { m_perInstance = flag; }
    ಆಭಾಸ ಪೂರ್ಣಾಙ್ಕ size() ಧ್ರುವ = 0;
    ಧ್ರುವ string& comment() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_text; }  // text to insert in code
    ಧ್ರುವ string& page() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_page; }
    ಧ್ರುವ string& hier() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hier; }
    ಶೂನ್ಯ hier(ಧ್ರುವ string& flag) { m_hier = flag; }
    ಶೂನ್ಯ comment(ಧ್ರುವ string& flag) { m_text = flag; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ Ast_ಗಂಟು_Cover_Decl* ಧ್ರುವ asamep = VN_DBG_AS(samep, ಗಂಟು_Cover_Decl);
        ಪ್ರತ್ಯಾಗಮ (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    ಶೂನ್ಯ dataDeclp(Ast_ಗಂಟು_Cover_Decl* nodep) { m_dataDeclp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_ಗಂಟು_Cover_Decl* dataDeclNullp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dataDeclp; }
    Ast_ಗಂಟು_Cover_Decl* dataDeclThisp() { ಪ್ರತ್ಯಾಗಮ dataDeclNullp() ? dataDeclNullp() : this; }
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
    string m_ifacePortName;  // Interface port name for out-of-block definition (IEEE 25.8)
    uint64_t m_dpiOpenParent = 0;  // DPI import open array, if !=0, how many callees
    bool m_taskPublic : 1;  // Public task
    bool m_classMethod : 1;  // Class method
    bool m_didProto : 1;  // Did prototype processing
    bool m_prototype : 1;  // Just a prototype
    bool m_dpiExport : 1;  // DPI exported
    bool m_dpiImport : 1;  // DPI imported
    bool m_dpiContext : 1;  // DPI import context
    bool m_dpiOpenChild : 1;  // DPI import open array child wrapper
    bool m_dpiTask : 1;  // DPI import task (vs. void function)
    bool m_isConstructor : 1;  // Class constructor
    bool m_isExternProto : 1;  // Extern prototype
    bool m_isExternDef : 1;  // Extern definition
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_dpiPure : 1;  // DPI import pure (vs. virtual pure)
    bool m_keepAlive : 1;  // Disable dead function elimination
    bool m_needsSyms : 1;  // Requires vlSymsp argument
    bool m_pureVirtual : 1;  // Pure virtual
    bool m_recursive : 1;  // Recursive or part of recursion
    bool m_static : 1;  // Static method in class
    bool m_underGenerate : 1;  // Under generate (for warning)
    bool m_verilogFunction : 1;  // Declared by user as function (versus internal-made)
    bool m_verilogTask : 1;  // Declared by user as task (versus internal-made)
    bool m_virtual : 1;  // Virtual method in class
    bool m_needProcess : 1;  // Needs access to VlProcess of the caller
    bool m_isCovergroupSample : 1;  // Covergroup sample() method
    VBaseOverride m_baseOverride;  // BaseOverride (inital/final/extends)
    VLifetime m_lifetime;  // Default lifetime of local vars
    VIsCached m_purity;  // Pure state

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_FTask(VNType t, File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* stmtsp)
        : Ast_ಗಂಟು{t, fl}
        , m_name{name}
        , m_taskPublic{false}
        , m_classMethod{false}
        , m_didProto{false}
        , m_prototype{false}
        , m_dpiExport{false}
        , m_dpiImport{false}
        , m_dpiContext{false}
        , m_dpiOpenChild{false}
        , m_dpiTask{false}
        , m_isConstructor{false}
        , m_isExternProto{false}
        , m_isExternDef{false}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_dpiPure{false}
        , m_keepAlive{false}
        , m_needsSyms{true}
        , m_pureVirtual{false}
        , m_recursive{false}
        , m_static{false}
        , m_underGenerate{false}
        , m_verilogFunction{false}
        , m_verilogTask{false}
        , m_virtual{false}
        , m_needProcess{false}
        , m_isCovergroupSample{false} {
        addStmtsp(stmtsp);
        cname(name);  // Might be overridden by dpi import/export
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_FTask;
    ಆಭಾಸ Ast_ಗಂಟು_FTask* cloneType(ಧ್ರುವ string& name) = 0;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool isGateOptimizable() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string cname() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cname; }
    ಶೂನ್ಯ cname(ಧ್ರುವ string& cname) { m_cname = cname; }
    bool isFunction() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ fvarp() != nullptr; }
    // MORE ACCESSORS
    ಶೂನ್ಯ dpiOpenParentInc() { ++m_dpiOpenParent; }
    ಶೂನ್ಯ dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiOpenParent; }
    bool taskPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_taskPublic; }
    ಶೂನ್ಯ taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classMethod; }
    ಶೂನ್ಯ classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_didProto; }
    ಶೂನ್ಯ didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isExternProto; }
    ಶೂನ್ಯ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isExternDef; }
    ಶೂನ್ಯ isExternDef(bool flag) { m_isExternDef = flag; }
    ಧ್ರುವ string& ifacePortName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ifacePortName; }
    ಶೂನ್ಯ ifacePortName(ಧ್ರುವ string& name) { m_ifacePortName = name; }
    bool prototype() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_prototype; }
    ಶೂನ್ಯ prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiExport; }
    ಶೂನ್ಯ dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiImport; }
    ಶೂನ್ಯ dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiContext; }
    ಶೂನ್ಯ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiOpenChild; }
    ಶೂನ್ಯ dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiTask; }
    ಶೂನ್ಯ dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !m_dpiCDecl.empty(); }
    ಧ್ರುವ string& dpiCDecl() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiCDecl; }
    ಶೂನ್ಯ dpiCDecl(ಧ್ರುವ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isConstructor; }
    ಶೂನ್ಯ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isHideLocal; }
    ಶೂನ್ಯ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isHideProtected; }
    ಶೂನ್ಯ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiPure; }
    ಶೂನ್ಯ dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_keepAlive; }
    ಶೂನ್ಯ keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needsSyms; }
    ಶೂನ್ಯ needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pureVirtual; }
    ಶೂನ್ಯ pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_recursive; }
    ಶೂನ್ಯ recursive(bool flag) { m_recursive = flag; }
    bool isStatic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_static; }
    ಶೂನ್ಯ isStatic(bool flag) { m_static = flag; }
    bool underGenerate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_underGenerate; }
    ಶೂನ್ಯ underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_verilogFunction; }
    ಶೂನ್ಯ verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_verilogTask; }
    ಶೂನ್ಯ verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_virtual; }
    ಶೂನ್ಯ isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needProcess; }
    ಶೂನ್ಯ setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isCovergroupSample; }
    ಶೂನ್ಯ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    ಶೂನ್ಯ baseOverride(ಧ್ರುವ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_baseOverride; }
    ಶೂನ್ಯ lifetime(ಧ್ರುವ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lifetime; }
    bool isPure() override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಶೂನ್ಯ propagateAttrFrom(ಧ್ರುವ Ast_ಗಂಟು_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        classMethod(fromp->classMethod());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

ಗುಪ್ತ:
    bool getPurityRecurse() ಧ್ರುವ;
};
class Ast_ಗಂಟು_File VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_File(VNType t, File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : Ast_ಗಂಟು{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ಗಂಟು_File;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class Ast_ಗಂಟು_Func_Cov_Item VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Base class for functional coverage items (coverpoints, crosses)
ರಕ್ಷಿತ:
    string m_name;  // Item name

ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_Func_Cov_Item(VNType t, File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : Ast_ಗಂಟು{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Func_Cov_Item;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    bool maybePointedTo() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
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
    ಧ್ರುವ string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    ಪೂರ್ಣಾಙ್ಕ m_depth = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    ಪೂರ್ಣಾಙ್ಕ m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
    VLifetime m_lifetime;  // Lifetime
    VTimescale m_timeunit;  // Global time unit
    VOptionBool m_unconnectedDrive;  // State of `unconnected_drive
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

    bool m_modPublic : 1;  // Module has public references
    bool m_modTrace : 1;  // Tracing this module
    bool m_inLibrary : 1;  // From a library, no error if not used, never top level
    bool m_ctorVarReset : 1;  // Ctor needs to call ctor_var_reset
    bool m_dead : 1;  // LinkDot believes is dead; will remove in Dead visitors
    bool m_hasGParam : 1;  // Has global parameter (for link)
    bool m_hasParameterList : 1;  // Has #() for parameter declaration
    bool m_hierBlock : 1;  // Hierarchical Block marked by HIER_BLOCK pragma
    bool m_hierParams : 1;  // Block containing params for parameterized hier blocks
    bool m_recursive : 1;  // Recursive module
    bool m_recursiveClone : 1;  // If recursive, what module it clones, otherwise nullptr
    bool m_parameterizedTemplate : 1;  // True when at least one specialized clone exists;
                                       // set by V3Param::deepCloneModule. Suppresses
                                       // width/type errors on the unresolved template.
    bool m_verilatorLib : 1;  // Module is a stub for a Verilator produced --lib-create
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Module(VNType t, File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname)
        : Ast_ಗಂಟು{t, fl}
        , m_name{name}
        , m_origName{name}
        , m_libname{libname}
        , m_modPublic{false}
        , m_modTrace{false}
        , m_inLibrary{false}
        , m_ctorVarReset{false}
        , m_dead{false}
        , m_hasGParam{false}
        , m_hasParameterList{false}
        , m_hierBlock{false}
        , m_hierParams{false}
        , m_recursive{false}
        , m_recursiveClone{false}
        , m_parameterizedTemplate{false}
        , m_verilatorLib{false} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Module;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಆಭಾಸ bool timescaleMatters() ಧ್ರುವ = 0;
    ಅನ್ತಃಸ್ಥ bool isDollarUnit() ಧ್ರುವ;  // Is the $unit package
    ಅನ್ತಃಸ್ಥ bool isConstPool() ಧ್ರುವ;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(ಧ್ರುವ string& outer) ಧ್ರುವ;
    // ACCESSORS
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string origName() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_origName; }
    string someInstanceName() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_someInstanceName; }
    ಶೂನ್ಯ someInstanceName(ಧ್ರುವ string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_defaultDisablep; }
    ಶೂನ್ಯ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inLibrary; }
    ಶೂನ್ಯ inLibrary(bool flag) { m_inLibrary = flag; }
    ಶೂನ್ಯ depth(ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) { m_depth = ಮೌಲ್ಯ; }
    ಪೂರ್ಣಾಙ್ಕ depth() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_depth; }
    ಶೂನ್ಯ level(ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) { m_level = ಮೌಲ್ಯ; }
    ಪೂರ್ಣಾಙ್ಕ level() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_level; }
    string libname() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libname; }
    bool is_ತುದಿ() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ level() == 1; }
    bool modPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modPublic; }
    ಶೂನ್ಯ modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modTrace; }
    ಶೂನ್ಯ modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ctorVarReset; }
    ಶೂನ್ಯ ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dead; }
    ಶೂನ್ಯ dead(bool flag) { m_dead = flag; }
    bool hasGParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasGParam; }
    ಶೂನ್ಯ hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasParameterList; }
    ಶೂನ್ಯ hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hierBlock; }
    ಶೂನ್ಯ hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hierParams; }
    ಶೂನ್ಯ hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_recursive; }
    ಶೂನ್ಯ recursive(bool flag) { m_recursive = flag; }
    ಶೂನ್ಯ recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_recursiveClone; }
    bool parameterizedTemplate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_parameterizedTemplate; }
    ಶೂನ್ಯ parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    ಶೂನ್ಯ verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_verilatorLib; }
    VLifetime lifetime() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lifetime; }
    ಶೂನ್ಯ lifetime(ಧ್ರುವ VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unconnectedDrive; }
    ಶೂನ್ಯ unconnectedDrive(ಧ್ರುವ VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_ಗಂಟು_Procedure VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Procedure(VNType t, File_ಸಾಲು* fl, Ast_ಗಂಟು* stmtsp)
        : Ast_ಗಂಟು{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Procedure;
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isJustOneBodyStmt() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_suspendable; }
    ಶೂನ್ಯ setSuspendable() { m_suspendable = true; }
    bool needProcess() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needProcess; }
    ಶೂನ್ಯ setNeedProcess() { m_needProcess = true; }
};
class Ast_ಗಂಟು_Range VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A range, sized or unsized
ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Range(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Range;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

ಸಾರ್ವಜನಿಕ:
    AstActive(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_Sen_ವೃಕ್ಷ* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    // Statements are broken into pieces, as some must come before others.
    ಶೂನ್ಯ sentreep(Ast_Sen_ವೃಕ್ಷ* nodep) { m_sentreep = nodep; }
    Ast_Sen_ವೃಕ್ಷ* sentreep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sentreep; }
    // METHODS
    ಅನ್ತಃಸ್ಥ bool hasClocked() ಧ್ರುವ;
    ಅನ್ತಃಸ್ಥ bool hasCombo() ಧ್ರುವ;
    ಅನ್ತಃಸ್ಥ bool hasInitial() ಧ್ರುವ;
    ಅನ್ತಃಸ್ಥ bool hasStatic() ಧ್ರುವ;
};
class AstAlias final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
ಸಾರ್ವಜನಿಕ:
    AstAlias(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
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
    AstBind(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Bind Target name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
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
    bool m_declPrivate : 1;  // Declare it private
    bool m_keepIfEmpty : 1;  // Keep declaration and definition separate, even if empty
    bool m_slow : 1;  // Slow routine, called once or just at init time
    bool m_funcPublic : 1;  // From user public task/function
    bool m_isConstructor : 1;  // Is C class constructor
    bool m_isDestructor : 1;  // Is C class destructor
    bool m_isMethod : 1;  // Is inside a class definition
    bool m_isLoose : 1;  // Semantically this is a method, but is implemented as a function with
                         // an explicitly passed 'self' pointer as the first argument.  This can
                         // be slightly faster due to __restrict, and we do not declare in header
                         // so adding/removing loose functions doesn't recompile everything.
    bool m_isVirtual : 1;  // Virtual function
    bool m_entryPoint : 1;  // User may call into this top level function
    bool m_dpiPure : 1;  // Pure DPI function
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
    ಪೂರ್ಣಾಙ್ಕ m_cost;  // Function call cost
ಸಾರ್ವಜನಿಕ:
    AstCFunc(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstScope* scopep, ಧ್ರುವ string& rtnType = "")
        : ASTGEN_SUPER_CFunc(fl) {
        m_isConst = VBoolOrUnknown::BU_UNKNOWN;  // Unknown until analyzed
        m_scopep = scopep;
        m_name = name;
        m_rtnType = rtnType;
        m_isStatic = false;
        m_isTrace = false;
        m_dontCombine = false;
        m_declPrivate = false;
        m_keepIfEmpty = false;
        m_slow = false;
        m_funcPublic = false;
        m_isConstructor = false;
        m_isDestructor = false;
        m_isMethod = true;
        m_isLoose = false;
        m_isVirtual = false;
        m_needProcess = false;
        m_entryPoint = false;
        m_dpiPure = false;
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
        m_cost = v3Global.opt.instr_ಎಣಿಕೆ_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstCFunc* ಧ್ರುವ asamep = VN_DBG_AS(samep, CFunc);
        ಪ್ರತ್ಯಾಗಮ ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_cost; }
    VBoolOrUnknown isConst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isConst; }
    ಶೂನ್ಯ isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    ಶೂನ್ಯ isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isStatic; }
    ಶೂನ್ಯ isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_isTrace; }
    ಶೂನ್ಯ isTrace(bool flag) { m_isTrace = flag; }
    ಶೂನ್ಯ cname(ಧ್ರುವ string& name) { m_cname = name; }
    string cname() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cname; }
    AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopep; }
    ಶೂನ್ಯ scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ((m_rtnType == "") ? "void" : m_rtnType); }
    ಶೂನ್ಯ rtnType(ಧ್ರುವ string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dontCombine || isTrace() || entryPoint(); }
    ಶೂನ್ಯ dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_declPrivate; }
    ಶೂನ್ಯ declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_keepIfEmpty; }
    ಶೂನ್ಯ keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_slow; }
    ಶೂನ್ಯ slow(bool flag) { m_slow = flag; }
    bool funcPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcPublic; }
    ಶೂನ್ಯ funcPublic(bool flag) { m_funcPublic = flag; }
    ಶೂನ್ಯ argTypes(ಧ್ರುವ string& str) { m_argTypes = str; }
    string argTypes() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_argTypes; }
    ಶೂನ್ಯ ifdef(ಧ್ರುವ string& str) { m_ifdef = str; }
    string ifdef() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ifdef; }
    bool isConstructor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isConstructor; }
    ಶೂನ್ಯ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isDestructor; }
    ಶೂನ್ಯ isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isMethod; }
    ಶೂನ್ಯ isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isLoose; }
    ಶೂನ್ಯ isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ isMethod() && !isLoose(); }
    bool isVirtual() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isVirtual; }
    ಶೂನ್ಯ isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needProcess; }
    ಶೂನ್ಯ setNeedProcess() { m_needProcess = true; }
    bool entryPoint() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_entryPoint; }
    ಶೂನ್ಯ entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiPure; }
    ಶೂನ್ಯ dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiContext; }
    ಶೂನ್ಯ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_dpiExportDispatcher; }
    ಶೂನ್ಯ dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiExportImpl; }
    ಶೂನ್ಯ dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_dpiImportPrototype; }
    ಶೂನ್ಯ dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiImportWrapper; }
    ಶೂನ್ಯ dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !m_dpiCDecl.empty(); }
    ಧ್ರುವ string& dpiCDecl() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiCDecl; }
    ಶೂನ್ಯ dpiCDecl(ಧ್ರುವ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rtnType == "VlCoroutine"; }
    ಶೂನ್ಯ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_recursive; }
    ಶೂನ್ಯ unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_unlikely; }  // Note virtual override
    ಶೂನ್ಯ noLife(bool flag) { m_noLife = flag; }
    bool noLife() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noLife; }
    bool isCovergroupSample() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isCovergroupSample; }
    ಶೂನ್ಯ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_voidSelf; }
    ಶೂನ್ಯ voidSelf(bool flag) { m_voidSelf = flag; }
    ಶೂನ್ಯ cost(ಪೂರ್ಣಾಙ್ಕ cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ಧ್ರುವ string m_name;
    ಧ್ರುವ VUseType m_useType;  // What sort of use this is

ಸಾರ್ವಜನಿಕ:
    AstCUse(File_ಸಾಲು* fl, VUseType useType, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    VUseType useType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_useType; }
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
    ಪೂರ್ಣಾಙ್ಕ m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
ಸಾರ್ವಜನಿಕ:
    AstCell(File_ಸಾಲು* fl, File_ಸಾಲು* mfl, ಧ್ರುವ string& instName, ಧ್ರುವ string& modName,
            AstPin* pinsp, AstPin* paramsp, AstRange* rangep)
        : ASTGEN_SUPER_Cell(fl)
        , m_modNameFileline{mfl}
        , m_name{instName}
        , m_origName{instName}
        , m_modName{modName}
        , m_hasIfaceVar{false}
        , m_recursive{false}
        , m_trace{true} {
        addPinsp(pinsp);
        addParamsp(paramsp);
        addRangep(rangep);
    }
    ASTGEN_MEMBERS_AstCell;
    // No cloneRelink, we presume cloneee's want the same module linkages
    ಶೂನ್ಯ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    // ACCESSORS
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Cell name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string origName() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_origName; }  // * = Original name
    ಶೂನ್ಯ origName(ಧ್ರುವ string& name) { m_origName = name; }
    string modName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modName; }  // * = Instance name
    ಶೂನ್ಯ modName(ಧ್ರುವ string& name) { m_modName = name; }
    File_ಸಾಲು* modNameFileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modNameFileline; }
    Ast_ಗಂಟು_Module* modp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modp; }  // [AfterLink] = Pointer to module instantiated
    ಶೂನ್ಯ modp(Ast_ಗಂಟು_Module* nodep) { m_modp = nodep; }
    ಪೂರ್ಣಾಙ್ಕ arrayIdx() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_arrayIdx; }
    ಶೂನ್ಯ arrayIdx(ಪೂರ್ಣಾಙ್ಕ idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasIfaceVar; }
    ಶೂನ್ಯ hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    ಶೂನ್ಯ trace(bool flag) { m_trace = flag; }
    bool isTrace() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_trace; }
    ಶೂನ್ಯ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_recursive; }
};
class AstCellInline final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ಧ್ರುವ std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ಧ್ರುವ std::string m_origModName;  // __BEGIN__ or __FORK__
ಸಾರ್ವಜನಿಕ:
    AstCellInline(File_ಸಾಲು* fl, ಧ್ರುವ std::string& name, ಧ್ರುವ std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    // ACCESSORS
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    std::string origModName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_origModName; }
};
class AstCellInlineScope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ಧ್ರುವ std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
ಸಾರ್ವಜನಿಕ:
    AstCellInlineScope(File_ಸಾಲು* fl, ಧ್ರುವ std::string& name, ಧ್ರುವ std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    // ACCESSORS
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ std::string& name) override { m_name = name; }
    std::string origModName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_origModName; }
};
class AstCgOptionAssign final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ಧ್ರುವ VCoverOptionType m_optType;  // Option type
    ಧ್ರುವ string m_name;  // Original option name (for diagnostics on unknown options)
    ಧ್ರುವ bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstCgOptionAssign(File_ಸಾಲು* fl, bool typeOption, VCoverOptionType optType,
                      ಧ್ರುವ string& rawName, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    VCoverOptionType optType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_optType; }
    bool typeOption() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_typeOption; }
};
class AstClassExtends final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ಧ್ರುವ bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

ಸಾರ್ವಜನಿಕ:
    AstClassExtends(File_ಸಾಲು* fl, Ast_ಗಂಟು* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ಧ್ರುವ;
    AstClass* classp() ಧ್ರುವ;  // Like above, but throws error if nulll
    bool isImplements() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isImplements; }
    ಶೂನ್ಯ parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_parameterized; }
};
class AstClocking final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ಧ್ರುವ bool m_isGlobal;  // True if global clocking

ಸಾರ್ವಜನಿಕ:
    AstClocking(File_ಸಾಲು* fl, ಧ್ರುವ std::string& name, Ast_Sen_ವೃಕ್ಷ* sentreep, Ast_ಗಂಟು* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool isDefault() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isDefault; }
    bool isGlobal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    ಶೂನ್ಯ makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

ಸಾರ್ವಜನಿಕ:
    AstClockingItem(File_ಸಾಲು* fl, VDirection direction, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* skewp, Ast_ಗಂಟು* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        ಯದಿ (AstAssign* ಧ್ರುವ clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } ಅನ್ಯಥಾ {
            exprp(VN_AS(clockingDeclp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    VDirection direction() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_direction; }
    AstClockingItem* outputp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outputp; }
    ಶೂನ್ಯ outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstConfig final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

ಸಾರ್ವಜನಿಕ:
    AstConfig(File_ಸಾಲು* fl, ಧ್ರುವ std::string& libname, ಧ್ರುವ std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_libname + "." + m_configname; }
    std::string libname() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_libname; }
    std::string configname() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_configname; }
};
class AstConfigCell final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

ಸಾರ್ವಜನಿಕ:
    AstConfigCell(File_ಸಾಲು* fl, ಧ್ರುವ std::string& libname, ಧ್ರುವ std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_libname + "." + m_cellname; }
    std::string libname() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_libname; }
    std::string cellname() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_cellname; }
};
class AstConfigRule final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ಧ್ರುವ bool m_isCell;  // Declared as "cell" versus "instance"

ಸಾರ್ವಜನಿಕ:
    AstConfigRule(File_ಸಾಲು* fl, Ast_ಗಂಟು* cellp, Ast_ಗಂಟು* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_isCell; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class AstConfigUse final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ಧ್ರುವ bool m_isConfig;  // ":config"; Config, not module/primitive name

ಸಾರ್ವಜನಿಕ:
    AstConfigUse(File_ಸಾಲು* fl, ಧ್ರುವ std::string& libname, ಧ್ರುವ std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_libname + "." + m_cellname; }
    std::string libname() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_libname; }
    std::string cellname() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_cellname; }
    bool isConfig() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_isConfig; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class AstConstraint final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Constraint
    // @astgen op1 := itemsp : List[AstNode]
    // @astgen op2 := classOrPackagep : Optional[AstNode]
    string m_name;  // Name of constraint
    VBaseOverride m_baseOverride;  // BaseOverride (inital/final/extends)
    bool m_isExternDef = false;  // Extern prototype definition
    bool m_isExternExplicit = false;  // Explicit prototype declaration (has extern)
    bool m_isExternProto = false;  // Prototype declaration (implicit or explicit)
    bool m_isKwdPure = false;  // Pure constraint
    bool m_isStatic = false;  // Static constraint
ಸಾರ್ವಜನಿಕ:
    AstConstraint(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Scope name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ baseOverride(ಧ್ರುವ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_baseOverride; }
    bool isExternDef() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isExternDef; }
    ಶೂನ್ಯ isExternDef(bool flag) { m_isExternDef = flag; }
    ಶೂನ್ಯ isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isExternExplicit; }
    ಶೂನ್ಯ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isExternProto; }
    ಶೂನ್ಯ isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isKwdPure; }
    ಶೂನ್ಯ isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isStatic; }
};
class AstCoverBin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    ಧ್ರುವ string m_name;  // Base name of the bin
    ಧ್ರುವ VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

ಸಾರ್ವಜನಿಕ:
    AstCoverBin(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp = nullptr)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{isIllegal ? VCoverBinsType::BINS_ILLEGAL
                               : (isIgnore ? VCoverBinsType::BINS_IGNORE
                                           : (isWildcard ? VCoverBinsType::BINS_WILDCARD
                                                         : VCoverBinsType::BINS_USER))}
        , m_isWildcard{isWildcard} {
        addRangesp(rangesp);
        this->iffp(iffp);
    }
    // Constructor for automatic bins
    AstCoverBin(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_ಸಾಲು* fl, ಧ್ರುವ string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    VCoverBinsType binsType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_binsType; }
    bool isWildcard() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isWildcard; }
    bool isArray() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isArray; }
    ಶೂನ್ಯ isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ಧ್ರುವ bool m_isNegated;  // Complement the selection within the cross product

ಸಾರ್ವಜನಿಕ:
    AstCoverBinsof(File_ಸಾಲು* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_ಗಂಟು* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    bool isNegated() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isNegated; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {  // LCOV_EXCL_START
        ಧ್ರುವ AstCoverBinsof* ಧ್ರುವ asamep = VN_DBG_AS(samep, CoverBinsof);
        ಪ್ರತ್ಯಾಗಮ m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ಧ್ರುವ string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ಧ್ರುವ VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

ಸಾರ್ವಜನಿಕ:
    AstCoverCrossBin(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* selectp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    string verilogKwd() ಧ್ರುವ override;
    VCoverBinsType binsType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_binsType; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {  // LCOV_EXCL_START
        ಧ್ರುವ AstCoverCrossBin* ಧ್ರುವ asamep = VN_DBG_AS(samep, CoverCrossBin);
        ಪ್ರತ್ಯಾಗಮ m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // The complete bin-tuple space of the enclosing cross
    ಧ್ರುವ string m_name;  // Referenced cross name

ಸಾರ್ವಜನಿಕ:
    AstCoverCrossRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {  // LCOV_EXCL_START
        ಪ್ರತ್ಯಾಗಮ m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ಧ್ರುವ bool m_isOr;  // Union (||), rather than intersection (&&)

ಸಾರ್ವಜನಿಕ:
    AstCoverCrossSelect(File_ಸಾಲು* fl, Ast_ಗಂಟು* lhsp, Ast_ಗಂಟು* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool isOr() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isOr; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ isOr() ? "||" : "&&"; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {  // LCOV_EXCL_START
        ಪ್ರತ್ಯಾಗಮ m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
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
    ಧ್ರುವ VCoverOptionType m_optType;  // Option being assigned
    ಧ್ರುವ bool m_typeOption;  // type_option (vs option)

ಸಾರ್ವಜನಿಕ:
    AstCoverOption(File_ಸಾಲು* fl, bool typeOption, VCoverOptionType optType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    VCoverOptionType optType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_optType; }
    bool typeOption() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_typeOption; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {  // LCOV_EXCL_START
        ಧ್ರುವ AstCoverOption* ಧ್ರುವ asamep = VN_DBG_AS(samep, CoverOption);
        ಪ್ರತ್ಯಾಗಮ m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
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
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 3 + INSTR_ಎಣಿಕೆ_BRANCH + INSTR_ಎಣಿಕೆ_LD; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isOutputter() override {
        ಪ್ರತ್ಯಾಗಮ false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ಧ್ರುವ VTransRepType m_repType;

ಸಾರ್ವಜನಿಕ:
    AstCoverTransItem(File_ಸಾಲು* fl, Ast_ಗಂಟು* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class AstCoverTransSet final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
ಸಾರ್ವಜನಿಕ:
    AstCoverTransSet(File_ಸಾಲು* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
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
        addSubp(subp);
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
    AstCovergroup(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_ಗಂಟು* membersp, Ast_Sen_ವೃಕ್ಷ* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    bool maybePointedTo() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCoverpointRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ಧ್ರುವ string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

ಸಾರ್ವಜನಿಕ:
    AstCoverpointRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
};
class AstDefParam final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
ಸಾರ್ವಜನಿಕ:
    AstDefParam(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pathp, ಧ್ರುವ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstDefaultClocking final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    std::string m_name;  // Clocking block name

ಸಾರ್ವಜನಿಕ:
    AstDefaultClocking(File_ಸಾಲು* fl, ಧ್ರುವ std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
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
    AstDpiExport(File_ಸಾಲು* fl, ಧ್ರುವ string& vname, ಧ್ರುವ string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string cname() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cname; }
    ಶೂನ್ಯ cname(ಧ್ರುವ string& cname) { m_cname = cname; }
};
class AstElabDisplay final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

ಸಾರ್ವಜನಿಕ:
    ಅನ್ತಃಸ್ಥ AstElabDisplay(File_ಸಾಲು* fl, VDisplayType dispType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!fmtp());
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ INSTR_ಎಣಿಕೆ_PLI; }
    VDisplayType displayType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_displayType; }
    ಶೂನ್ಯ displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents something missing, e.g. a missing argument in FOREACH
ಸಾರ್ವಜನಿಕ:
    explicit AstEmpty(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstGenCaseItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstGenCaseItem(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condsp, Ast_ಗಂಟು* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ condsp() == nullptr; }
};
class AstImplicit final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
ಸಾರ್ವಜನಿಕ:
    AstImplicit(File_ಸಾಲು* fl, Ast_ಗಂಟು* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
ಸಾರ್ವಜನಿಕ:
    AstIntfRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& parentName, ಧ್ರುವ string& baseName,
               ಧ್ರುವ string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    string parentName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_parentName; }
    string baseName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_baseName; }
    string modportName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modportName; }
};
class AstLibrary final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

ಸಾರ್ವಜನಿಕ:
    AstLibrary(File_ಸಾಲು* fl, ಧ್ರುವ std::string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filesp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
};
class AstModport final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
ಸಾರ್ವಜನಿಕ:
    AstModport(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "modport"; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
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
    AstModportClockingRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    AstClocking* clockingp() ಧ್ರುವ VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_clockingp;
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
    AstModportFTaskRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool isImport() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !m_export; }
    bool isExport() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_export; }
    Ast_ಗಂಟು_FTask* ftaskp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ftaskp; }  // [After Link] Pointer to variable
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
    ಧ್ರುವ VDirection m_direction;  // Direction of the variable (in/out)
ಸಾರ್ವಜನಿಕ:
    AstModportVarRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    VDirection direction() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_direction; }
    AstVar* varp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_varp; }  // [After Link] Pointer to variable
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
    std::set<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ಧ್ರುವ AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು*, ಧ್ರುವ Ast_ಗಂಟು_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

ಸಾರ್ವಜನಿಕ:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಶೂನ್ಯ ತಳ್ಳು_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.ಸೇರಿಸು(varp); }
    ಧ್ರುವ std::set<AstVar*>& deferredParamVarps() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_deferredParamVarps; }
    ಶೂನ್ಯ clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    ಶೂನ್ಯ deleteContents();
    ಶೂನ್ಯ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ "$root"; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    Ast_ಗಂಟು_Module* ತುದಿ_Modulep() ಧ್ರುವ VL_MT_STABLE {  // Top module in hierarchy
        ಪ್ರತ್ಯಾಗಮ modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { ಪ್ರತ್ಯಾಗಮ m_typeTablep; }
    string astConstOrigParamName(ಧ್ರುವ AstConst* nodep) ಧ್ರುವ;
    ಶೂನ್ಯ astConstOrigParamName(ಧ್ರುವ AstConst* nodep, ಧ್ರುವ string& name);
    ಶೂನ್ಯ astConstOrigParamNameErase(ಧ್ರುವ AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ಧ್ರುವ Ast_ಗಂಟು_Module* containingModule(ಧ್ರುವ Ast_ಗಂಟು* nodep);
    // Forget remembered modules, as the tree has moved.
    ಶೂನ್ಯ clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dollarUnitPkgp; }
    ಶೂನ್ಯ dollarUnitPkgp(AstPackage* ಧ್ರುವ packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiExportTriggerp; }
    ಶೂನ್ಯ dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_delaySchedulerp; }
    ಶೂನ್ಯ delaySchedulerp(AstVar* ಧ್ರುವ varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nbaEventp; }
    ಶೂನ್ಯ nbaEventp(AstVarScope* ಧ್ರುವ varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nbaEventTriggerp; }
    ಶೂನ್ಯ nbaEventTriggerp(AstVarScope* ಧ್ರುವ varScopep) { m_nbaEventTriggerp = varScopep; }
    ಶೂನ್ಯ stdPackagep(AstPackage* ಧ್ರುವ packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stdPackagep; }
    ಶೂನ್ಯ stdPackageProcessp(AstClass* ಧ್ರುವ classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_ಸಾಲು*) ಧ್ರುವ;
    Ast_ತುದಿ_Scope* ತುದಿ_Scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ತುದಿ_Scopep; }
    ಶೂನ್ಯ create_ತುದಿ_Scope(AstScope* scopep);
    VTimescale timeunit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeunit; }
    ಶೂನ್ಯ timeunit(ಧ್ರುವ VTimescale& ಮೌಲ್ಯ) { m_timeunit = ಮೌಲ್ಯ; }
    VTimescale timeprecision() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeprecision; }
    ಶೂನ್ಯ timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    ಶೂನ್ಯ timeprecisionMerge(File_ಸಾಲು*, ಧ್ರುವ VTimescale& ಮೌಲ್ಯ);
    ಶೂನ್ಯ timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timescaleSpecified; }
    ಧ್ರುವ std::string& resolved_ತುದಿ_Module_Name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_resolved_ತುದಿ_Module_Name; }
    ಶೂನ್ಯ resolved_ತುದಿ_Module_Name(ಧ್ರುವ std::string& ಮೌಲ್ಯ) { m_resolved_ತುದಿ_Module_Name = ಮೌಲ್ಯ; }
    uint32_t nTraceCodes() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nTraceCodes; }
    ಶೂನ್ಯ nTraceCodes(uint32_t ಮೌಲ್ಯ) { m_nTraceCodes = ಮೌಲ್ಯ; }
    AstVarScope* stlFirstIterationp();
    ಶೂನ್ಯ clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    ಧ್ರುವ std::string trace_Lib_ತುದಿ_Name() ಧ್ರುವ {
        ಧ್ರುವ std::string& name = resolved_ತುದಿ_Module_Name();
        ಪ್ರತ್ಯಾಗಮ prettyName(name.empty() ? v3Global.rootp()->ತುದಿ_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    ಶೂನ್ಯ addEvalStats(ಧ್ರುವ std::string& phase);
};
class AstPackageExport final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

ಸಾರ್ವಜನಿಕ:
    AstPackageExport(File_ಸಾಲು* fl, ಧ್ರುವ string& pkgName, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    string pkgName() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_pkgName; }
    string prettyPkgNameQ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_packagep; }
    ಶೂನ್ಯ packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A package export *::* declaration
ಸಾರ್ವಜನಿಕ:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

ಸಾರ್ವಜನಿಕ:
    AstPackageImport(File_ಸಾಲು* fl, AstPackage* packagep, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_ಸಾಲು* fl, ಧ್ರುವ string& pkgName, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    string pkgName() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_pkgName; }
    string prettyPkgNameQ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_packagep; }
    ಶೂನ್ಯ packagep(AstPackage* nodep) { m_packagep = nodep; }

ಗುಪ್ತ:
    ಶೂನ್ಯ pkgNameFrom();
};
class AstPin final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    ಪೂರ್ಣಾಙ್ಕ m_pin_ಅಂಕೆ;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
ಸಾರ್ವಜನಿಕ:
    AstPin(File_ಸಾಲು* fl, ಪೂರ್ಣಾಙ್ಕ pin_ಅಂಕೆ, ಧ್ರುವ string& name, Ast_ಗಂಟು* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_ಅಂಕೆ{pin_ಅಂಕೆ}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    ಶೂನ್ಯ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Pin name, ""=go by number
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string prettyOperatorName() ಧ್ರುವ override;
    bool dotStar() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ name() == ".*"; }  // Fake name for .* connections until linked
    ಪೂರ್ಣಾಙ್ಕ pin_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pin_ಅಂಕೆ; }
    AstVar* modVarp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modVarp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modPTypep; }
    ಶೂನ್ಯ modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_param; }
    ಶೂನ್ಯ param(bool flag) { m_param = flag; }
    ಧ್ರುವ string& paramPath() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_paramPath; }
    ಶೂನ್ಯ paramPath(ಧ್ರುವ string& path) { m_paramPath = path; }
    bool svDotName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_svDotName; }
    ಶೂನ್ಯ svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_svImplicit; }
    ಶೂನ್ಯ svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_pin_ಅಂಕೆ;  // Pin number
    ಧ್ರುವ string m_name;  // Name of pin
ಸಾರ್ವಜನಿಕ:
    AstPort(File_ಸಾಲು* fl, ಪೂರ್ಣಾಙ್ಕ pinnum, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_ಅಂಕೆ{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Port name
    ಪೂರ್ಣಾಙ್ಕ pin_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pin_ಅಂಕೆ; }  // * = Pin number, for order based instantiation
};
class AstPragma final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    ಧ್ರುವ VPragmaType m_pragType;  // Type of pragma
    ಧ್ರುವ VTimescale m_timescale;  // For TIMEUNIT_SET
ಸಾರ್ವಜನಿಕ:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_ಸಾಲು* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_ಸಾಲು* fl, ಧ್ರುವ VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    VPragmaType pragType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timescale; }
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
    AstPropSpec(File_ಸಾಲು* fl, AstSenItem* sensesp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* disablep, Ast_ಗಂಟು* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_propStrength; }
};
class AstPull final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := lhsp : AstNodeExpr

    ಧ್ರುವ bool m_direction;

ಸಾರ್ವಜನಿಕ:
    AstPull(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_direction; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
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
    AstScope(File_ಸಾಲು* fl, Ast_ಗಂಟು_Module* modp, ಧ್ರುವ string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!m_modp);
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Scope name
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string pretty_Name_ಸಂದೇಶ() ಧ್ರುವ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    string nameDotless() ಧ್ರುವ;
    Ast_ಗಂಟು_Module* modp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_aboveScopep; }
    ಶೂನ್ಯ aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_aboveCellp; }
    ಶೂನ್ಯ aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_ತುದಿ() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_edgeType;  // Edge type
ಸಾರ್ವಜನಿಕ:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    AstSenItem(File_ಸಾಲು* fl, VEdgeType edgeType, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* senp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_ಸಾಲು* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_ಸಾಲು* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_ಸಾಲು* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_ಸಾಲು* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_ಸಾಲು* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_ಸಾಲು* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_edgeType; }
    ಶೂನ್ಯ edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_ಎಣಿಕೆ_Inc();
    }
    Ast_ಗಂಟು_Var_Ref* varrefp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VN_CAST(sensp(), ಗಂಟು_Var_Ref); }
    //
    bool isClocked() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ edgeType().clockedStmt(); }
    bool isComboOrStar() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_ವೃಕ್ಷ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
ಸಾರ್ವಜನಿಕ:
    Ast_Sen_ವೃಕ್ಷ(File_ಸಾಲು* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_ವೃಕ್ಷ(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_ವೃಕ್ಷ;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    // METHODS
    bool hasClocked() ಧ್ರುವ;  // Includes a clocked statement
    bool hasEdge() ಧ್ರುವ;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ಧ್ರುವ;  // Includes a STATIC SenItem
    bool hasInitial() ಧ್ರುವ;  // Includes a INITIAL SenItem
    bool hasFinal() ಧ್ರುವ;  // Includes a FINAL SenItem
    bool hasCombo() ಧ್ರುವ;  // Includes a COMBO SenItem
    bool hasHybrid() ಧ್ರುವ;  // Includes a HYBRID SenItem
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
    VStrength strength0() { ಪ್ರತ್ಯಾಗಮ m_s0; }
    VStrength strength1() { ಪ್ರತ್ಯಾಗಮ m_s1; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class AstSystemCSection final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ಧ್ರುವ VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

ಸಾರ್ವಜನಿಕ:
    AstSystemCSection(File_ಸಾಲು* fl, VSystemCSectionType sectionType, ಧ್ರುವ std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sectionType; }
    ಧ್ರುವ std::string& text() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_text; }
    ಶೂನ್ಯ text(ಧ್ರುವ std::string& ಮೌಲ್ಯ) { m_text = ಮೌಲ್ಯ; }
    ಶೂನ್ಯ dump(std::ostream&) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream&) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು*) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isPure() override { ಪ್ರತ್ಯಾಗಮ false; }
    bool isOutputter() override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstText final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ಧ್ರುವ std::string m_text;  // The text to emit
ಸಾರ್ವಜನಿಕ:
    AstText(File_ಸಾಲು* fl, ಧ್ರುವ std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ text() == VN_DBG_AS(samep, Text)->text();
    }
    ಧ್ರುವ std::string& text() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_text; }
};
class AstTextBlock final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ಧ್ರುವ std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ಧ್ರುವ std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ಧ್ರುವ std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
ಸಾರ್ವಜನಿಕ:
    explicit AstTextBlock(File_ಸಾಲು* fl,  //
                          ಧ್ರುವ std::string& prefix = "",  //
                          ಧ್ರುವ std::string& separator = "",  //
                          ಧ್ರುವ std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    ಧ್ರುವ std::string& prefix() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_prefix; }
    ಧ್ರುವ std::string& separator() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_separator; }
    ಧ್ರುವ std::string& suffix() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_suffix; }
    // Add some text, or a node to this block
    ಶೂನ್ಯ add(ಧ್ರುವ string& text) { addNodesp(new AstText{fileline(), text}); }
    ಶೂನ್ಯ add(Ast_ಗಂಟು* nodep) { addNodesp(nodep); }
};
class Ast_ತುದಿ_Scope final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    friend class AstNetlist;  // Only the AstNetlist can create one
    Ast_ತುದಿ_Scope(File_ಸಾಲು* fl, AstScope* ascopep)
        : ASTGEN_SUPER_ತುದಿ_Scope(fl) {
        this->scopep(ascopep);
    }

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ತುದಿ_Scope;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* createTemp(ಧ್ರುವ string& name, unsigned width);
    AstVarScope* createTemp(ಧ್ರುವ string& name, Ast_ಗಂಟು_DType* dtypep);
};
class AstTypeTable final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
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
    explicit AstTypeTable(File_ಸಾಲು* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, ಧ್ರುವ VNumRange& range,
                                     ಪೂರ್ಣಾಙ್ಕ widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& ಗಂಟು);
    AstBasicDType* find_ಸೇರಿಸು_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_ಸಾಲು* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_ಸಾಲು* fl);
    AstQueueDType* findQueueIndexDType(File_ಸಾಲು* fl);
    AstStreamDType* findStreamDType(File_ಸಾಲು* fl);
    AstVoidDType* findVoidDType(File_ಸಾಲು* fl);
    ಶೂನ್ಯ clearCache();
    ಶೂನ್ಯ repairCache();
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
};
class AstTypedef final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_ಅಂಕೆ;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

ಸಾರ್ವಜನಿಕ:
    AstTypedef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* attrsp, VFlagChildDType,
               Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_ಅಂಕೆ{fl->token_ಅಂಕೆ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_ಅಂಕೆ{fl->token_ಅಂಕೆ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() ಧ್ರುವ;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    ಆಭಾಸ Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_ಅಂಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_decl_Token_ಅಂಕೆ; }
    ಶೂನ್ಯ decl_Token_ಅಂಕೆ_Set_Min(uint32_t token_ಅಂಕೆ) override {
        m_decl_Token_ಅಂಕೆ = std::min(m_decl_Token_ಅಂಕೆ, token_ಅಂಕೆ);
    }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& flag) override { m_name = flag; }
    bool attrPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrPublic; }
    ಶೂನ್ಯ attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isHideLocal; }
    ಶೂನ್ಯ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isHideProtected; }
    ಶೂನ್ಯ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isUnderClass; }
    ಶೂನ್ಯ isUnderClass(bool flag) { m_isUnderClass = flag; }
    ಶೂನ್ಯ tag(ಧ್ರುವ string& text) override { m_tag = text; }
    string tag() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_tag; }
};
class AstTypedefFwd final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ಧ್ರುವ VFwdType m_fwdType;  // Forward type for lint check

ಸಾರ್ವಜನಿಕ:
    AstTypedefFwd(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    VFwdType fwdType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fwdType; }
};
class AstUdpTable final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := linesp : List[AstUdpTableLine]
ಸಾರ್ವಜನಿಕ:
    AstUdpTable(File_ಸಾಲು* fl, Ast_Udp_Table_ಸಾಲು* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        ಯದಿ (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_ಸಾಲು final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
ಗುಪ್ತ:
    ಧ್ರುವ bool m_udpIsCombo;  // Combinational or sequential UDP

ಸಾರ್ವಜನಿಕ:
    class UdpCombo {};
    Ast_Udp_Table_ಸಾಲು(UdpCombo, File_ಸಾಲು* fl, Ast_Udp_Table_ಸಾಲು_Val* iFieldsp,
                    Ast_Udp_Table_ಸಾಲು_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_ಸಾಲು(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_ಸಾಲು(UdpSequential, File_ಸಾಲು* fl, Ast_Udp_Table_ಸಾಲು_Val* iFieldsp,
                    Ast_Udp_Table_ಸಾಲು_Val* oFieldsp1, Ast_Udp_Table_ಸಾಲು_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_ಸಾಲು(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_ಸಾಲು;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ udpIsCombo() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_udpIsCombo; }
};
class Ast_Udp_Table_ಸಾಲು_Val final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    string m_text;  // Value character

ಸಾರ್ವಜನಿಕ:
    Ast_Udp_Table_ಸಾಲು_Val(File_ಸಾಲು* fl, ಧ್ರುವ string& text)
        : ASTGEN_SUPER_Udp_Table_ಸಾಲು_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_ಸಾಲು_Val;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_text; }
    ಶೂನ್ಯ name(std::string ಧ್ರುವ& text) override VL_MT_STABLE { m_text = text; }
    string text() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_text; }
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
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    VVarType m_varType;  // Type of variable
    VDirection m_direction;  // Direction input/output etc
    VDirection m_declDirection;  // Declared direction input/output etc
    VLifetime m_lifetime;  // Lifetime
    VRandAttr m_rand;  // Randomizability of this variable (rand, randc, etc)
    ಪೂರ್ಣಾಙ್ಕ m_pin_ಅಂಕೆ = 0;  // For JSON, if non-zero the connection pin number
    ಪೂರ್ಣಾಙ್ಕ m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
    bool m_libInoutEnable : 1;  // Port is the lowered library inout's enable
    bool m_ansi : 1;  // Params or pins declared in the module header, rather than the body
    bool m_declTyped : 1;  // Declared as type (for dedup check)
    bool m_tristate : 1;  // Inout or triwire or trireg
    bool m_primaryIO : 1;  // In/out to top level (or directly assigned from same)
    bool m_primaryClock : 1;  // In/out to top level and is, or combinationally drives a SenItem
    bool m_sc : 1;  // SystemC variable
    bool m_scSensitive : 1;  // SystemC sensitive() needed
    bool m_sigPublic : 1;  // User C code accesses this signal or is top signal
    bool m_sigModPublic : 1;  // User C code accesses this signal and module
    bool m_sigUserRdPublic : 1;  // User C code accesses this signal, read only
    bool m_sigUserRWPublic : 1;  // User C code accesses this signal, read-write
    bool m_usedParam : 1;  // Parameter is referenced (on link; later signals not setup)
    bool m_usedLoopIdx : 1;  // Variable subject of for unrolling
    bool m_funcLocal : 1;  // Local variable for a function
    bool m_funcLocalSticky : 1;  // As m_funcLocal but remains set if var is moved to a static
    bool m_funcReturn : 1;  // Return variable for a function
    bool m_attrScBv : 1;  // User force bit vector attribute
    bool m_attrScBigUint : 1;  // User force sc_biguint attribute
    bool m_attrSFormat : 1;  // User sformat attribute
    bool m_attrSplitVar : 1;  // declared with split_var metacomment
    bool m_attrFsmState : 1;  // declared with fsm_state metacomment
    bool m_attrFsmArcInclCond : 1;  // declared with fsm_arc_include_cond metacomment
    VFsmExpandType::en m_attrFsmStateExpand : 2;  // FSM expansion type for FSM coverage
    bool m_constPoolEntry : 1;  // Constant pool variable
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
    bool m_isIfaceParent : 1;  // dtype is reference to interface present in this module
    bool m_isIfaceArraySplit : 1;  // Interface array that was split into element variables by
                                   // V3Param, until V3Width removes it
    bool m_isInternal : 1;  // Internal state, don't add to method pinter
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
    bool m_isForcedByCode : 1;  // May be forced/released from AstAssignForce/AstRelease
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
    bool m_processQueue : 1;  // Process queue variable
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
        m_usedParam = false;
        m_usedLoopIdx = false;
        m_sigPublic = false;
        m_sigModPublic = false;
        m_sigUserRdPublic = false;
        m_sigUserRWPublic = false;
        m_funcLocal = false;
        m_funcLocalSticky = false;
        m_funcReturn = false;
        m_attrScBv = false;
        m_attrScBigUint = false;
        m_attrSFormat = false;
        m_attrSplitVar = false;
        m_attrFsmState = false;
        m_attrFsmArcInclCond = false;
        m_attrFsmStateExpand = VFsmExpandType::DEFAULT;
        m_constPoolEntry = false;
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
        m_isIfaceParent = false;
        m_isIfaceArraySplit = false;
        m_isInternal = false;
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
        m_isForcedByCode = false;
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
        m_processQueue = false;
        m_mtask_Cache_ಸಾಲು_Align = false;
    }

ಸಾರ್ವಜನಿಕ:
    AstVar(File_ಸಾಲು* fl, VVarType type, ಧ್ರುವ string& name, VFlagChildDType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_ಸಾಲು* fl, VVarType type, ಧ್ರುವ string& name, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_ಸಾಲು* fl, VVarType type, ಧ್ರುವ string& name, VFlagLogicPacked, ಪೂರ್ಣಾಙ್ಕ wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_ಸಾಲು* fl, VVarType type, ಧ್ರುವ string& name, VFlagBitPacked, ಪೂರ್ಣಾಙ್ಕ wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_ಸಾಲು* fl, VVarType type, ಧ್ರುವ string& name, ಧ್ರುವ AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ಯದಿ (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_ವೃಕ್ಷ(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಅನ್ತಃಸ್ಥ bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    string origName() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_origName; }  // * = Original name
    ಶೂನ್ಯ origName(ಧ್ರುವ string& name) { m_origName = name; }
    VVarType varType() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_varType; }  // * = Type of variable
    ಶೂನ್ಯ direction(ಧ್ರುವ VDirection& flag) {
        m_direction = flag;
        ಯದಿ (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_direction; }
    bool isIO() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_direction != VDirection::NONE; }
    bool isVLIO() ಧ್ರುವ {
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ bdtypep = basicp();
        ಪ್ರತ್ಯಾಗಮ isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    ಶೂನ್ಯ declDirection(ಧ್ರುವ VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_declDirection; }
    ಪೂರ್ಣಾಙ್ಕ libInoutId() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libInoutId; }
    ಶೂನ್ಯ libInoutId(ಪೂರ್ಣಾಙ್ಕ id) { m_libInoutId = id; }
    bool libInoutEnable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libInoutEnable; }
    ಶೂನ್ಯ libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    ಶೂನ್ಯ varType(VVarType type) { m_varType = type; }
    ಶೂನ್ಯ varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    ಶೂನ್ಯ varType2In() {
        m_tristate = false;
        m_direction = VDirection::ಒಳಹರಿವು;
    }
    string scType() ಧ್ರುವ;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) ಧ್ರುವ;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) ಧ್ರುವ;
    string dpiTmpVarType(ಧ್ರುವ string& varName) ಧ್ರುವ;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) ಧ್ರುವ;
    string vlEnumType() ಧ್ರುವ;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) ಧ್ರುವ;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(ಧ್ರುವ string& propName) ಧ್ರುವ;  // Return VerilatorVarProps declaration
    ಶೂನ್ಯ combineType(VVarType type);
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* dtypeSkipRefp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    ಆಭಾಸ Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ dtypep() ? dtypep() : childDTypep();
    }
    ಶೂನ್ಯ ansi(bool flag) { m_ansi = flag; }
    ಶೂನ್ಯ declTyped(bool flag) { m_declTyped = flag; }
    ಶೂನ್ಯ sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    ಶೂನ್ಯ attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    ಶೂನ್ಯ attrScBv(bool flag) { m_attrScBv = flag; }
    ಶೂನ್ಯ attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    ಶೂನ್ಯ attrSFormat(bool flag) { m_attrSFormat = flag; }
    ಶೂನ್ಯ attrSplitVar(bool flag) { m_attrSplitVar = flag; }
    ಶೂನ್ಯ attrFsmState(bool flag) { m_attrFsmState = flag; }
    ಶೂನ್ಯ attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    ಶೂನ್ಯ attrFsmStateExpand(ಧ್ರುವ VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constPoolEntry; }
    ಶೂನ್ಯ setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_covergroupRefMember; }
    ಶೂನ್ಯ covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_embeddedCovergroup; }
    ಶೂನ್ಯ embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    ಶೂನ್ಯ rand(ಧ್ರುವ VRandAttr flag) { m_rand = flag; }
    ಶೂನ್ಯ usedParam(bool flag) { m_usedParam = flag; }
    ಶೂನ್ಯ usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    ಶೂನ್ಯ sigPublic(bool flag) { m_sigPublic = flag; }
    ಶೂನ್ಯ sigModPublic(bool flag) { m_sigModPublic = flag; }
    ಶೂನ್ಯ sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        ಯದಿ (flag) sigPublic(true);
    }
    ಶೂನ್ಯ sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        ಯದಿ (flag) sigUserRdPublic(true);
    }
    ಶೂನ್ಯ sc(bool flag) { m_sc = flag; }
    ಶೂನ್ಯ scSensitive(bool flag) { m_scSensitive = flag; }
    ಶೂನ್ಯ primaryIO(bool flag) { m_primaryIO = flag; }
    ಶೂನ್ಯ isConst(bool flag) { m_isConst = flag; }
    ಶೂನ್ಯ isContinuously(bool flag) { m_isContinuously = flag; }
    ಶೂನ್ಯ isIfaceParent(bool flag) { m_isIfaceParent = flag; }
    ಶೂನ್ಯ isIfaceArraySplit(bool flag) { m_isIfaceArraySplit = flag; }
    ಶೂನ್ಯ isInternal(bool flag) { m_isInternal = flag; }
    ಶೂನ್ಯ isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    ಶೂನ್ಯ funcLocal(bool flag) {
        m_funcLocal = flag;
        ಯದಿ (flag) m_funcLocalSticky = true;
    }
    ಶೂನ್ಯ funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_gotNansiType; }
    ಶೂನ್ಯ gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasStrengthAssignment; }
    ಶೂನ್ಯ hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasUserInit; }
    ಶೂನ್ಯ hasUserInit(bool flag) { m_hasUserInit = flag; }
    ಶೂನ್ಯ icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_icoMaybeWritten; }
    ಶೂನ್ಯ isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isClockingDriven; }
    bool isDpiOpenArray() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_isDpiOpenArray; }
    ಶೂನ್ಯ isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isHideLocal; }
    ಶೂನ್ಯ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isHideProtected; }
    ಶೂನ್ಯ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noCReset; }
    ಶೂನ್ಯ noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noReset; }
    ಶೂನ್ಯ noReset(bool flag) { m_noReset = flag; }
    bool noSubst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_noSubst; }
    ಶೂನ್ಯ noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_processQueue; }
    ಶೂನ್ಯ processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sampled; }
    ಶೂನ್ಯ sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_substConstOnly; }
    ಶೂನ್ಯ substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_overriddenParam; }
    ಶೂನ್ಯ overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_untypedParam; }
    ಶೂನ್ಯ untypedParam(bool flag) { m_untypedParam = flag; }
    ಶೂನ್ಯ trace(bool flag) { m_trace = flag; }
    ಶೂನ್ಯ isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isForceable; }
    ಶೂನ್ಯ setForceable() { m_isForceable = true; }
    ಶೂನ್ಯ setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isReadByDpi; }
    ಶೂನ್ಯ setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isWrittenByDpi; }
    ಶೂನ್ಯ setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isWrittenBySuspendable; }
    ಶೂನ್ಯ setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ignorePostRead; }
    ಶೂನ್ಯ setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ignorePostWrite; }
    ಶೂನ್ಯ setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ignoreSchedWrite; }
    ಶೂನ್ಯ setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dfgTriLowered; }
    ಶೂನ್ಯ setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dfgAllowMultidriveTri; }
    ಶೂನ್ಯ setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    ಶೂನ್ಯ globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_globalConstrained; }
    bool isStdRandomizeArg() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isStdRandomizeArg; }
    ಶೂನ್ಯ setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    ಶೂನ್ಯ tag(ಧ್ರುವ string& text) override { m_tag = text; }
    string tag() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_tag; }
    bool isAnsi() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ansi; }
    bool isContinuously() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isContinuously; }
    bool isDeclTyped() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_declTyped; }
    bool isInout() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_direction.isInout(); }
    bool isInoutOrRef() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_direction.isInoutOrRef(); }
    bool is_ಒಳಹರಿವು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_direction.is_ಒಳಹರಿವು(); }
    bool isNonOutput() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_direction.isNonOutput(); }
    bool isReadOnly() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_direction.isReadOnly(); }
    bool isConstRef() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_direction.isConstRef(); }
    bool isRef() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_direction.isRef(); }
    bool isWritable() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_direction.isWritable(); }
    bool isTristate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_tristate; }
    bool isPrimaryIO() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_primaryIO; }
    bool isPrimaryInish() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_primaryClock; }
    ಶೂನ್ಯ setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType() == VVarType::IFACEREF; }
    ಶೂನ್ಯ setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isIfaceParent; }
    bool isIfaceArraySplit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isIfaceArraySplit; }
    bool isInternal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isInternal; }
    bool isIfaceParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isIfaceParam; }
    bool isSignal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType().isSignal(); }
    bool isNet() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType().isNet(); }
    bool isWor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType().isWor(); }
    bool isWiredNet() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType().isWiredNet(); }
    bool isTemp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType().isTemp(); }
    bool isClassMember() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType() == VVarType::MEMBER; }
    bool isVirtIface() ಧ್ರುವ {
        ಯದಿ (AstIfaceRefDType* ಧ್ರುವ dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            ಪ್ರತ್ಯಾಗಮ dtp->isVirtual();
        }
        ಪ್ರತ್ಯಾಗಮ false;
    }
    bool isStatementTemp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType() == VVarType::STMTTEMP; }
    bool isXTemp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType() == VVarType::XTEMP; }
    bool isParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType().isParam(); }
    bool isGParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType() == VVarType::GPARAM; }
    bool isGenVar() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ isIO() || isSignal() || isClassMember() || isTemp() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() ಧ್ರುವ {
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ bdtypep = basicp();
        ಪ್ರತ್ಯಾಗಮ bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_usedParam; }
    bool isUsedLoopIdx() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_usedLoopIdx; }
    bool mtask_Cache_ಸಾಲು_Align() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_mtask_Cache_ಸಾಲು_Align; }
    ಶೂನ್ಯ mtask_Cache_ಸಾಲು_Align(bool flag) { m_mtask_Cache_ಸಾಲು_Align = flag; }
    bool isSc() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_sc; }
    bool isScQuad() ಧ್ರುವ;
    bool isScBv() ಧ್ರುವ VL_MT_STABLE;
    bool isScUint() ಧ್ರುವ;
    bool isScUintBool() ಧ್ರುವ;
    bool isScBigUint() ಧ್ರುವ VL_MT_STABLE;
    bool isScSensitive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scSensitive; }
    bool isSigPublic() ಧ್ರುವ;
    bool isSigModPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_trace; }
    bool isRand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rand.isRand(); }
    bool isRandC() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rand.isRandC(); }
    bool isConst() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_isConst; }
    bool isLatched() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isLatched; }
    bool isFuncLocal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcLocal; }
    bool isFuncLocalSticky() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcLocalSticky; }
    bool isFuncReturn() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcReturn; }
    bool isPullup() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isPullup; }
    bool isPulldown() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isPulldown; }
    bool attrScBv() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrScBv; }
    bool attrScBigUint() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrScBigUint; }
    bool attrFileDescr() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrFileDescr; }
    bool attrSFormat() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrSFormat; }
    bool attrSplitVar() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrSplitVar; }
    bool attrFsmState() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrFsmState; }
    bool attrFsmArcInclCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_attrFsmStateExpand; }
    AstIface* sensIfacep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_sensIfacep; }
    VRandAttr rand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rand; }
    string verilogKwd() ಧ್ರುವ override;
    ಶೂನ್ಯ lifetime(ಧ್ರುವ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lifetime; }
    ಶೂನ್ಯ pin_ಅಂಕೆ(ಪೂರ್ಣಾಙ್ಕ id) { m_pin_ಅಂಕೆ = id; }
    ಪೂರ್ಣಾಙ್ಕ pin_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pin_ಅಂಕೆ; }
    ಶೂನ್ಯ propagateAttrFrom(ಧ್ರುವ AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        ಯದಿ (fromp->attrFileDescr()) attrFileDescr(true);
        ಯದಿ (fromp->isContinuously()) isContinuously(true);
    }
    ಶೂನ್ಯ propagateSplitAttrFrom(ಧ್ರುವ AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ಧ್ರುವ ವರ್ಣ* cannotSplitKindReason() ಧ್ರುವ;
    ಶೂನ್ಯ propagateWrapAttrFrom(ಧ್ರುವ AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    ಶೂನ್ಯ combineType(ಧ್ರುವ AstVar* otherp);
    bool needsCReset() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !isStatementTemp()
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
        ಯದಿ (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ scopep()->name() + "->" + varp()->name(); }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    AstVar* varp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_varp; }  // [After Link] Pointer to variable
    ಶೂನ್ಯ varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_scopep; }  // Pointer to scope it's under
    ಶೂನ್ಯ scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_trace; }
    ಶೂನ್ಯ trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_optimizeLifePost; }
    ಶೂನ್ಯ optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_ಮೌಲ್ಯ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fork_By_ಮೌಲ್ಯ; }
    ಶೂನ್ಯ fork_By_ಮೌಲ್ಯ(bool flag) { m_fork_By_ಮೌಲ್ಯ = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ಧ್ರುವ uint64_t m_index;  // Index of the element, with the array's lo() at 0
ಸಾರ್ವಜನಿಕ:
    AstConsArrayItem(File_ಸಾಲು* fl, uint64_t index, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_index{index} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_index == VN_DBG_AS(samep, ConsArrayItem)->m_index;
    }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }  // By AstConsArray's map
    uint64_t index() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_index; }
};
class AstConsAssocItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
ಸಾರ್ವಜನಿಕ:
    AstConsAssocItem(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* keyp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};
class AstConsDynArrayItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
ಸಾರ್ವಜನಿಕ:
    AstConsDynArrayItem(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_ಸಾಲು* fl, AstConst* keyp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};
class AstConsQueueItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
ಸಾರ್ವಜನಿಕ:
    AstConsQueueItem(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};
class AstConsStructItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
ಸಾರ್ವಜನಿಕ:
    AstConsStructItem(File_ಸಾಲು* fl, AstMemberDType* memberp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    AstMemberDType* memberp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_memberp; }
};
class AstConsWildcardItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
ಸಾರ್ವಜನಿಕ:
    AstConsWildcardItem(File_ಸಾಲು* fl, AstConst* keyp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    ಪೂರ್ಣಾಙ್ಕ m_offset;  // Offset column numbers to uniq-ify IFs
ಸಾರ್ವಜನಿಕ:
    AstCoverOtherDecl(File_ಸಾಲು* fl, ಧ್ರುವ string& page, ಧ್ರುವ string& comment,
                      ಧ್ರುವ string& linescov, ಪೂರ್ಣಾಙ್ಕ offset, ಧ್ರುವ string& fsmVar = "",
                      ಧ್ರುವ string& fsmFrom = "", ಧ್ರುವ string& fsmTo = "",
                      ಧ್ರುವ string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ offset() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_offset; }
    ಪೂರ್ಣಾಙ್ಕ size() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    ಧ್ರುವ string& linescov() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_linescov; }
    ಧ್ರುವ string& fsmVar() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fsmVar; }
    ಧ್ರುವ string& fsmFrom() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fsmFrom; }
    ಧ್ರುವ string& fsmTo() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fsmTo; }
    ಧ್ರುವ string& fsmTag() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fsmTag; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstCoverOtherDecl* ಧ್ರುವ asamep = VN_DBG_AS(samep, CoverOtherDecl);
        ಪ್ರತ್ಯಾಗಮ Super::same_ಗಂಟು(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ಧ್ರುವ VNumRange m_range;  // Packed array range covering each toggle bit
ಸಾರ್ವಜನಿಕ:
    AstCoverToggleDecl(File_ಸಾಲು* fl, ಧ್ರುವ string& page, ಧ್ರುವ string& comment,
                       ಧ್ರುವ VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ size() ಧ್ರುವ override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        ಪ್ರತ್ಯಾಗಮ 2 * m_range.elements();
    }
    ಧ್ರುವ VNumRange& range() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_range; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstCoverToggleDecl* ಧ್ರುವ asamep = VN_DBG_AS(samep, CoverToggleDecl);
        ಪ್ರತ್ಯಾಗಮ Super::same_ಗಂಟು(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // A function inside a module
ಸಾರ್ವಜನಿಕ:
    AstFunc(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* stmtp, Ast_ಗಂಟು* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_FTask* cloneType(ಧ್ರುವ string& name) override {
        ಪ್ರತ್ಯಾಗಮ new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "function"; }
};
class AstLet final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
ಸಾರ್ವಜನಿಕ:
    AstLet(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    Ast_ಗಂಟು_FTask* cloneType(ಧ್ರುವ string& name) override { ಪ್ರತ್ಯಾಗಮ new AstLet{fileline(), name}; }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "let"; }
};
class AstProperty final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // A property inside a module
ಸಾರ್ವಜನಿಕ:
    AstProperty(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_FTask* cloneType(ಧ್ರುವ string& name) override {
        ಪ್ರತ್ಯಾಗಮ new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "property"; }
};
class AstSequence final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
ಸಾರ್ವಜನಿಕ:
    AstSequence(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_FTask* cloneType(ಧ್ರುವ string& name) override {
        ಪ್ರತ್ಯಾಗಮ new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "sequence"; }
    bool isReferenced() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_referenced; }
    ಶೂನ್ಯ isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_FTask {
    // A task inside a module
ಸಾರ್ವಜನಿಕ:
    AstTask(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_ಗಂಟು_FTask* cloneType(ಧ್ರುವ string& name) override {
        ಪ್ರತ್ಯಾಗಮ new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "task"; }
};

// === AstNodeFile ===
class AstCFile final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
ಸಾರ್ವಜನಿಕ:
    AstCFile(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    uint64_t complexityScore() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_complexityScore; }
    ಶೂನ್ಯ complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_slow; }
    ಶೂನ್ಯ slow(bool flag) { m_slow = flag; }
    bool source() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_source; }
    ಶೂನ್ಯ source(bool flag) { m_source = flag; }
    bool support() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_support; }
    ಶೂನ್ಯ support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_File {
    // Verilog output file
    // Parents:  NETLIST
ಸಾರ್ವಜನಿಕ:
    AstVFile(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
};
class AstCoverCross final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
ಸಾರ್ವಜನಿಕ:
    AstCoverCross(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstCoverpointRef* itemsp,
                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class AstCoverpoint final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
ಸಾರ್ವಜನಿಕ:
    AstCoverpoint(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp,
                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* iffp = nullptr, Ast_ಗಂಟು* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};

// === AstNodeGen ===
class AstGenBlock final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ಧ್ರುವ bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ಧ್ರುವ bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

ಸಾರ್ವಜನಿಕ:
    AstGenBlock(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ std::string& name) override { m_name = name; }
    bool unnamed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unnamed; }
    bool implied() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_implied; }
    AstDefaultDisable* defaultDisablep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_defaultDisablep; }
    ಶೂನ್ಯ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(ಧ್ರುವ string& outer) ಧ್ರುವ;
};
class AstGenCase final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
ಸಾರ್ವಜನಿಕ:
    AstGenCase(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
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
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
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
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_ಅಂಕೆ;  // Declaration token number
    VBaseOverride m_baseOverride;  // BaseOverride (inital/final/extends)
    bool m_hasRandVarsUpdate = false;  // Has updateRandVars method,
                                       // which updates pointers to rand variables in clone()
    bool m_covergroup = false;  // Is covergroup (TODO perhaps make a new Ast node type for CG?)
    bool m_extended = false;  // Is extension or extended by other classes
    bool m_interfaceClass = false;  // Interface class
    bool m_needRNG = false;  // Need RNG, uses srandom/randomize
    bool m_useVirtualPublic = false;  // Subclasses need virtual public as uses interface class
    bool m_virtual = false;  // Virtual class
    bool m_printedFrom = false;  // This class is printed from i.e. is used as format arg.
    // Mutable, as only a recursion guard of the const dtypeName(), so also not dumped
    mutable bool m_dtypeNameBusy = false;  // In dtypeName(), which a parameter may lead back to
    // Covergroup options (when m_covergroup is true)
    ಪೂರ್ಣಾಙ್ಕ m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool full, ಧ್ರುವ string& scope) ಧ್ರುವ;
    string dtypeNameScope() ಧ್ರುವ;  // Prefix of dtypeName(true) for the scope declaring the class

ಸಾರ್ವಜನಿಕ:
    AstClass(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_ಅಂಕೆ{fl->token_ಅಂಕೆ()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    bool timescaleMatters() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    AstClassPackage* classOrPackagep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ಧ್ರುವ VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_covergroupEnclosingClassp;
    }
    ಶೂನ್ಯ covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_ಗಂಟು* membersp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ stmtsp(); }
    ಶೂನ್ಯ addMembersp(Ast_ಗಂಟು* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasRandVarsUpdate; }
    ಶೂನ್ಯ hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_covergroup; }
    ಶೂನ್ಯ isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_extended; }
    ಶೂನ್ಯ isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_interfaceClass; }
    ಶೂನ್ಯ isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_virtual; }
    ಶೂನ್ಯ isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needRNG; }
    ಶೂನ್ಯ needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_useVirtualPublic; }
    ಶೂನ್ಯ useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    ಶೂನ್ಯ markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) ಧ್ರುವ;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(ಧ್ರುವ string& scope) ಧ್ರುವ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ಶೂನ್ಯ dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_ಸಂದೇಶ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    ಸ್ಥಿರ string embeddedCovergroupTypeName(ಧ್ರುವ string& name) { ಪ್ರತ್ಯಾಗಮ "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ಧ್ರುವ;
    // Covergroup options accessors
    ಪೂರ್ಣಾಙ್ಕ cgAutoBinMax() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cgAutoBinMax; }
    ಶೂನ್ಯ cgAutoBinMax(ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) { m_cgAutoBinMax = ಮೌಲ್ಯ; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    ಸ್ಥಿರ bool isClassExtendedFrom(ಧ್ರುವ AstClass* refClassp, ಧ್ರುವ AstClass* baseClassp);
    uint32_t decl_Token_ಅಂಕೆ() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_decl_Token_ಅಂಕೆ; }
    ಶೂನ್ಯ decl_Token_ಅಂಕೆ_Set_Min(uint32_t token_ಅಂಕೆ) override {
        m_decl_Token_ಅಂಕೆ = std::min(m_decl_Token_ಅಂಕೆ, token_ಅಂಕೆ);
    }
    ಶೂನ್ಯ baseOverride(ಧ್ರುವ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    ಸ್ಥಿರ bool isCacheableChild(ಧ್ರುವ Ast_ಗಂಟು* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    template <typename T_Callable>
    ಶೂನ್ಯ foreachMember(ಧ್ರುವ T_Callable& f) {
        using T_ಗಂಟು = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_ಗಂಟು*>::ಮೌಲ್ಯ
                && std::is_base_of<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        ಯದಿ (ಧ್ರುವ AstClassExtends* ಧ್ರುವ cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (Ast_ಗಂಟು::privateTypeTest<T_ಗಂಟು>(stmtp)) f(this, static_cast<T_ಗಂಟು*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    template <typename T_Callable>
    bool existsMember(ಧ್ರುವ T_Callable& p) ಧ್ರುವ {
        using T_ಗಂಟು = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ಧ್ರುವ AstClass*, ಧ್ರುವ T_ಗಂಟು*>::ಮೌಲ್ಯ
                && std::is_base_of<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        ಯದಿ (ಧ್ರುವ AstClassExtends* ಧ್ರುವ cextendsp = this->extendsp()) {
            ಯದಿ (cextendsp->classp()->existsMember(p)) ಪ್ರತ್ಯಾಗಮ true;
        }
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (Ast_ಗಂಟು::privateTypeTest<T_ಗಂಟು>(stmtp)) {
                ಯದಿ (p(this, static_cast<T_ಗಂಟು*>(stmtp))) ಪ್ರತ್ಯಾಗಮ true;
            }
        }
        ಪ್ರತ್ಯಾಗಮ false;
    }
};
class AstClassPackage final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
ಸಾರ್ವಜನಿಕ:
    AstClassPackage(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "classpackage"; }
    bool timescaleMatters() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    AstClass* classp() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_classp; }
    ಶೂನ್ಯ classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
ಸಾರ್ವಜನಿಕ:
    AstIface(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
    bool hasVirtualRef() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasVirtualRef; }
    ಶೂನ್ಯ setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) ಧ್ರುವ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ಶೂನ್ಯ dtypeNameFreeze();
};
class AstModule final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // A module declaration
    ಧ್ರುವ bool m_isChecker;  // Module represents a checker
    ಧ್ರುವ bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

ಸಾರ್ವಜನಿಕ:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isChecker() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isChecker; }
    bool isProgram() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isProgram; }
    bool hasGenericIface() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasGenericIface; }
    ಶೂನ್ಯ hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};
class AstNotFoundModule final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // A missing module declaration
ಸಾರ್ವಜನಿಕ:
    AstNotFoundModule(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "/*not-found-*/ module"; }
    bool timescaleMatters() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstPackage final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // A package declaration
ಸಾರ್ವಜನಿಕ:
    AstPackage(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "package"; }
    bool timescaleMatters() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Module {
    // A primitive declaration
ಸಾರ್ವಜನಿಕ:
    AstPrimitive(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "primitive"; }
    bool timescaleMatters() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};

// === AstNodeProcedure ===
class AstAlways final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ಧ್ರುವ VAlwaysKwd m_keyword;

ಸಾರ್ವಜನಿಕ:
    AstAlways(File_ಸಾಲು* fl, VAlwaysKwd keyword, Ast_Sen_ವೃಕ್ಷ* sentreep, Ast_ಗಂಟು* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit ಅನ್ತಃಸ್ಥ AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    VAlwaysKwd keyword() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_keyword; }
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
    ಆಭಾಸ string emitC() { V3ERROR_NA_RETURN(""); }
    ಆಭಾಸ string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ಧ್ರುವ bool m_fromBracket = false;  // From C-style '[x]' declaration
ಸಾರ್ವಜನಿಕ:
    AstRange(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* leftp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    ಅನ್ತಃಸ್ಥ AstRange(File_ಸಾಲು* fl, ಪೂರ್ಣಾಙ್ಕ ಎಡ, ಪೂರ್ಣಾಙ್ಕ ಬಲ);
    ಅನ್ತಃಸ್ಥ AstRange(File_ಸಾಲು* fl, ಧ್ರುವ VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ ಎಡ_Const() ಧ್ರುವ VL_MT_STABLE;
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ ಬಲ_Const() ಧ್ರುವ VL_MT_STABLE;
    ಪೂರ್ಣಾಙ್ಕ hiConst() ಧ್ರುವ VL_MT_STABLE {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ l = ಎಡ_Const();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ r = ಬಲ_Const();
        ಪ್ರತ್ಯಾಗಮ l > r ? l : r;
    }
    ಪೂರ್ಣಾಙ್ಕ loConst() ಧ್ರುವ VL_MT_STABLE {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ l = ಎಡ_Const();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ r = ಬಲ_Const();
        ಪ್ರತ್ಯಾಗಮ l > r ? r : l;
    }
    ಪೂರ್ಣಾಙ್ಕ elementsConst() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ hiConst() - loConst() + 1; }
    bool ascending() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಎಡ_Const() < ಬಲ_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_ಎಡ_To_ಬಲ() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ VNumRange{ಎಡ_Const(), ಬಲ_Const()}.seq_ಎಡ_To_ಬಲ();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_ಬಲ_To_ಎಡ() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ VNumRange{ಎಡ_Const(), ಬಲ_Const()}.seq_ಬಲ_To_ಎಡ();
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಆಭಾಸ string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstRange* ಧ್ರುವ asamep = VN_DBG_AS(samep, Range);
        ಪ್ರತ್ಯಾಗಮ fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fromBracket; }
};
class AstUnsizedRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Range {
    // Unsized range specification, for open arrays
ಸಾರ್ವಜನಿಕ:
    explicit AstUnsizedRange(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    ಆಭಾಸ string emitC() { V3ERROR_NA_RETURN(""); }
    ಆಭಾಸ string emitVerilog() { ಪ್ರತ್ಯಾಗಮ "[]"; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstWildcardRange final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Range {
    // Wildcard range specification, for wildcard index type associative arrays
ಸಾರ್ವಜನಿಕ:
    explicit AstWildcardRange(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    ಆಭಾಸ string emitC() { V3ERROR_NA_RETURN(""); }
    ಆಭಾಸ string emitVerilog() { ಪ್ರತ್ಯಾಗಮ "[*]"; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* /*samep*/) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};

#endif  // Guard
