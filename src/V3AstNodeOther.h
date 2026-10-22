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
#അന്തര്ഭാവ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

#അന്തര്ഭാവ <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_കണ്ണി_Cons_Item VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    friend class Ast_കണ്ണി_വ്യഞ്ജകം;  // For m_knownConstantTree
    mutable bool m_known_Constant_വൃക്ഷം = false;  // Known to be a tree of constants

സംരക്ഷിത:
    Ast_കണ്ണി_Cons_Item(VNType t, File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : Ast_കണ്ണി{t, fl} {
        this->valuep(valuep);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Cons_Item;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class Ast_കണ്ണി_Cover_Decl VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    പൂർണ്ണസംഖ്യ m_bin_അക്കം = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    പൂർണ്ണസംഖ്യ m_local_Bin_അക്കം = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
പൊതു:
    Ast_കണ്ണി_Cover_Decl(VNType t, File_വരി* fl, സ്ഥിരം string& page, സ്ഥിരം string& comment)
        : Ast_കണ്ണി(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_കണ്ണി_Cover_Decl;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        എങ്കിൽ (m_dataDeclp
            && (m_dataDeclp == this || m_dataDeclp->m_dataDeclp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        തിരികെ nullptr;
    }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 1 + 2 * INSTR_എണ്ണം_LD; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    പൂർണ്ണസംഖ്യ bin_അക്കം() സ്ഥിരം { തിരികെ m_bin_അക്കം; }
    ശൂന്യം bin_അക്കം(പൂർണ്ണസംഖ്യ flag) { m_bin_അക്കം = flag; }
    പൂർണ്ണസംഖ്യ local_Bin_അക്കം() സ്ഥിരം { തിരികെ m_local_Bin_അക്കം; }
    ശൂന്യം local_Bin_അക്കം(പൂർണ്ണസംഖ്യ flag) { m_local_Bin_അക്കം = flag; }
    bool perInstance() സ്ഥിരം { തിരികെ m_perInstance; }
    ശൂന്യം perInstance(bool flag) { m_perInstance = flag; }
    മായാ പൂർണ്ണസംഖ്യ size() സ്ഥിരം = 0;
    സ്ഥിരം string& comment() സ്ഥിരം { തിരികെ m_text; }  // text to insert in code
    സ്ഥിരം string& page() സ്ഥിരം { തിരികെ m_page; }
    സ്ഥിരം string& hier() സ്ഥിരം { തിരികെ m_hier; }
    ശൂന്യം hier(സ്ഥിരം string& flag) { m_hier = flag; }
    ശൂന്യം comment(സ്ഥിരം string& flag) { m_text = flag; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം Ast_കണ്ണി_Cover_Decl* സ്ഥിരം asamep = VN_DBG_AS(samep, കണ്ണി_Cover_Decl);
        തിരികെ (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    ശൂന്യം dataDeclp(Ast_കണ്ണി_Cover_Decl* nodep) { m_dataDeclp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_കണ്ണി_Cover_Decl* dataDeclNullp() സ്ഥിരം { തിരികെ m_dataDeclp; }
    Ast_കണ്ണി_Cover_Decl* dataDeclThisp() { തിരികെ dataDeclNullp() ? dataDeclNullp() : this; }
};
class Ast_കണ്ണി_FTask VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
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

സംരക്ഷിത:
    Ast_കണ്ണി_FTask(VNType t, File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* stmtsp)
        : Ast_കണ്ണി{t, fl}
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

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_FTask;
    മായാ Ast_കണ്ണി_FTask* cloneType(സ്ഥിരം string& name) = 0;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    bool isGateOptimizable() സ്ഥിരം override {
        തിരികെ !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string cname() സ്ഥിരം { തിരികെ m_cname; }
    ശൂന്യം cname(സ്ഥിരം string& cname) { m_cname = cname; }
    bool isFunction() സ്ഥിരം { തിരികെ fvarp() != nullptr; }
    // MORE ACCESSORS
    ശൂന്യം dpiOpenParentInc() { ++m_dpiOpenParent; }
    ശൂന്യം dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() സ്ഥിരം { തിരികെ m_dpiOpenParent; }
    bool taskPublic() സ്ഥിരം { തിരികെ m_taskPublic; }
    ശൂന്യം taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() സ്ഥിരം { തിരികെ m_classMethod; }
    ശൂന്യം classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() സ്ഥിരം { തിരികെ m_didProto; }
    ശൂന്യം didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() സ്ഥിരം { തിരികെ m_isExternProto; }
    ശൂന്യം isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() സ്ഥിരം { തിരികെ m_isExternDef; }
    ശൂന്യം isExternDef(bool flag) { m_isExternDef = flag; }
    സ്ഥിരം string& ifacePortName() സ്ഥിരം { തിരികെ m_ifacePortName; }
    ശൂന്യം ifacePortName(സ്ഥിരം string& name) { m_ifacePortName = name; }
    bool prototype() സ്ഥിരം { തിരികെ m_prototype; }
    ശൂന്യം prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() സ്ഥിരം { തിരികെ m_dpiExport; }
    ശൂന്യം dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() സ്ഥിരം { തിരികെ m_dpiImport; }
    ശൂന്യം dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() സ്ഥിരം { തിരികെ m_dpiContext; }
    ശൂന്യം dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() സ്ഥിരം { തിരികെ m_dpiOpenChild; }
    ശൂന്യം dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() സ്ഥിരം { തിരികെ m_dpiTask; }
    ശൂന്യം dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() സ്ഥിരം { തിരികെ !m_dpiCDecl.empty(); }
    സ്ഥിരം string& dpiCDecl() സ്ഥിരം { തിരികെ m_dpiCDecl; }
    ശൂന്യം dpiCDecl(സ്ഥിരം string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() സ്ഥിരം { തിരികെ m_isConstructor; }
    ശൂന്യം isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() സ്ഥിരം { തിരികെ m_isHideLocal; }
    ശൂന്യം isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() സ്ഥിരം { തിരികെ m_isHideProtected; }
    ശൂന്യം isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() സ്ഥിരം { തിരികെ m_dpiPure; }
    ശൂന്യം dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() സ്ഥിരം { തിരികെ m_keepAlive; }
    ശൂന്യം keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() സ്ഥിരം { തിരികെ m_needsSyms; }
    ശൂന്യം needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() സ്ഥിരം { തിരികെ m_pureVirtual; }
    ശൂന്യം pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() സ്ഥിരം { തിരികെ m_recursive; }
    ശൂന്യം recursive(bool flag) { m_recursive = flag; }
    bool isStatic() സ്ഥിരം { തിരികെ m_static; }
    ശൂന്യം isStatic(bool flag) { m_static = flag; }
    bool underGenerate() സ്ഥിരം { തിരികെ m_underGenerate; }
    ശൂന്യം underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() സ്ഥിരം { തിരികെ m_verilogFunction; }
    ശൂന്യം verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() സ്ഥിരം { തിരികെ m_verilogTask; }
    ശൂന്യം verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() സ്ഥിരം { തിരികെ m_virtual; }
    ശൂന്യം isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() സ്ഥിരം { തിരികെ m_needProcess; }
    ശൂന്യം setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() സ്ഥിരം { തിരികെ m_isCovergroupSample; }
    ശൂന്യം isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    ശൂന്യം baseOverride(സ്ഥിരം VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() സ്ഥിരം { തിരികെ m_baseOverride; }
    ശൂന്യം lifetime(സ്ഥിരം VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() സ്ഥിരം { തിരികെ m_lifetime; }
    bool isPure() override;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    ശൂന്യം propagateAttrFrom(സ്ഥിരം Ast_കണ്ണി_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        classMethod(fromp->classMethod());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

സ്വകാര്യ:
    bool getPurityRecurse() സ്ഥിരം;
};
class Ast_കണ്ണി_File VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
പൊതു:
    Ast_കണ്ണി_File(VNType t, File_വരി* fl, സ്ഥിരം string& name)
        : Ast_കണ്ണി{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_കണ്ണി_File;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class Ast_കണ്ണി_Func_Cov_Item VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // Base class for functional coverage items (coverpoints, crosses)
സംരക്ഷിത:
    string m_name;  // Item name

പൊതു:
    Ast_കണ്ണി_Func_Cov_Item(VNType t, File_വരി* fl, സ്ഥിരം string& name)
        : Ast_കണ്ണി{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_കണ്ണി_Func_Cov_Item;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    bool maybePointedTo() സ്ഥിരം override { തിരികെ true; }
};
class Ast_കണ്ണി_Gen VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // Generate construct
പൊതു:
    Ast_കണ്ണി_Gen(VNType t, File_വരി* fl)
        : Ast_കണ്ണി{t, fl} {}
    ASTGEN_MEMBERS_Ast_കണ്ണി_Gen;
};
class Ast_കണ്ണി_Module VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    സ്ഥിരം string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    പൂർണ്ണസംഖ്യ m_depth = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    പൂർണ്ണസംഖ്യ m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
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
സംരക്ഷിത:
    Ast_കണ്ണി_Module(VNType t, File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname)
        : Ast_കണ്ണി{t, fl}
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

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Module;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    മായാ bool timescaleMatters() സ്ഥിരം = 0;
    വരിയിൽ bool isDollarUnit() സ്ഥിരം;  // Is the $unit package
    വരിയിൽ bool isConstPool() സ്ഥിരം;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(സ്ഥിരം string& outer) സ്ഥിരം;
    // ACCESSORS
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string origName() സ്ഥിരം override { തിരികെ m_origName; }
    string someInstanceName() സ്ഥിരം VL_MT_SAFE { തിരികെ m_someInstanceName; }
    ശൂന്യം someInstanceName(സ്ഥിരം string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() സ്ഥിരം { തിരികെ m_defaultDisablep; }
    ശൂന്യം defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() സ്ഥിരം { തിരികെ m_inLibrary; }
    ശൂന്യം inLibrary(bool flag) { m_inLibrary = flag; }
    ശൂന്യം depth(പൂർണ്ണസംഖ്യ മൂല്യം) { m_depth = മൂല്യം; }
    പൂർണ്ണസംഖ്യ depth() സ്ഥിരം VL_MT_SAFE { തിരികെ m_depth; }
    ശൂന്യം level(പൂർണ്ണസംഖ്യ മൂല്യം) { m_level = മൂല്യം; }
    പൂർണ്ണസംഖ്യ level() സ്ഥിരം VL_MT_SAFE { തിരികെ m_level; }
    string libname() സ്ഥിരം { തിരികെ m_libname; }
    bool is_മുകൾ() സ്ഥിരം VL_MT_SAFE { തിരികെ level() == 1; }
    bool modPublic() സ്ഥിരം { തിരികെ m_modPublic; }
    ശൂന്യം modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() സ്ഥിരം { തിരികെ m_modTrace; }
    ശൂന്യം modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() സ്ഥിരം { തിരികെ m_ctorVarReset; }
    ശൂന്യം ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() സ്ഥിരം { തിരികെ m_dead; }
    ശൂന്യം dead(bool flag) { m_dead = flag; }
    bool hasGParam() സ്ഥിരം { തിരികെ m_hasGParam; }
    ശൂന്യം hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() സ്ഥിരം { തിരികെ m_hasParameterList; }
    ശൂന്യം hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() സ്ഥിരം { തിരികെ m_hierBlock; }
    ശൂന്യം hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() സ്ഥിരം { തിരികെ m_hierParams; }
    ശൂന്യം hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() സ്ഥിരം { തിരികെ m_recursive; }
    ശൂന്യം recursive(bool flag) { m_recursive = flag; }
    ശൂന്യം recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() സ്ഥിരം { തിരികെ m_recursiveClone; }
    bool parameterizedTemplate() സ്ഥിരം { തിരികെ m_parameterizedTemplate; }
    ശൂന്യം parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    ശൂന്യം verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() സ്ഥിരം { തിരികെ m_verilatorLib; }
    VLifetime lifetime() സ്ഥിരം { തിരികെ m_lifetime; }
    ശൂന്യം lifetime(സ്ഥിരം VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() സ്ഥിരം { തിരികെ m_unconnectedDrive; }
    ശൂന്യം unconnectedDrive(സ്ഥിരം VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_കണ്ണി_Procedure VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
സംരക്ഷിത:
    Ast_കണ്ണി_Procedure(VNType t, File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : Ast_കണ്ണി{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Procedure;
    // METHODS
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isJustOneBodyStmt() സ്ഥിരം { തിരികെ stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() സ്ഥിരം { തിരികെ m_suspendable; }
    ശൂന്യം setSuspendable() { m_suspendable = true; }
    bool needProcess() സ്ഥിരം { തിരികെ m_needProcess; }
    ശൂന്യം setNeedProcess() { m_needProcess = true; }
};
class Ast_കണ്ണി_Range VL_NOT_FINAL : പൊതു Ast_കണ്ണി {
    // A range, sized or unsized
സംരക്ഷിത:
    Ast_കണ്ണി_Range(VNType t, File_വരി* fl)
        : Ast_കണ്ണി{t, fl} {}

പൊതു:
    ASTGEN_MEMBERS_Ast_കണ്ണി_Range;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : പൊതു Ast_കണ്ണി {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

പൊതു:
    AstActive(File_വരി* fl, സ്ഥിരം string& name, Ast_Sen_വൃക്ഷം* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    // Statements are broken into pieces, as some must come before others.
    ശൂന്യം sentreep(Ast_Sen_വൃക്ഷം* nodep) { m_sentreep = nodep; }
    Ast_Sen_വൃക്ഷം* sentreep() സ്ഥിരം { തിരികെ m_sentreep; }
    // METHODS
    വരിയിൽ bool hasClocked() സ്ഥിരം;
    വരിയിൽ bool hasCombo() സ്ഥിരം;
    വരിയിൽ bool hasInitial() സ്ഥിരം;
    വരിയിൽ bool hasStatic() സ്ഥിരം;
};
class AstAlias final : പൊതു Ast_കണ്ണി {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
പൊതു:
    AstAlias(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : പൊതു Ast_കണ്ണി {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
പൊതു:
    AstAliasScope(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : പൊതു Ast_കണ്ണി {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
പൊതു:
    AstBind(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Bind Target name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
};
class AstCFunc final : പൊതു Ast_കണ്ണി {
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
    പൂർണ്ണസംഖ്യ m_cost;  // Function call cost
പൊതു:
    AstCFunc(File_വരി* fl, സ്ഥിരം string& name, AstScope* scopep, സ്ഥിരം string& rtnType = "")
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
        m_cost = v3Global.opt.instr_എണ്ണം_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstCFunc* സ്ഥിരം asamep = VN_DBG_AS(samep, CFunc);
        തിരികെ ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ m_cost; }
    VBoolOrUnknown isConst() സ്ഥിരം { തിരികെ m_isConst; }
    ശൂന്യം isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    ശൂന്യം isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() സ്ഥിരം { തിരികെ m_isStatic; }
    ശൂന്യം isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() സ്ഥിരം VL_MT_SAFE { തിരികെ m_isTrace; }
    ശൂന്യം isTrace(bool flag) { m_isTrace = flag; }
    ശൂന്യം cname(സ്ഥിരം string& name) { m_cname = name; }
    string cname() സ്ഥിരം { തിരികെ m_cname; }
    AstScope* scopep() സ്ഥിരം { തിരികെ m_scopep; }
    ശൂന്യം scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() സ്ഥിരം { തിരികെ ((m_rtnType == "") ? "void" : m_rtnType); }
    ശൂന്യം rtnType(സ്ഥിരം string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() സ്ഥിരം { തിരികെ m_dontCombine || isTrace() || entryPoint(); }
    ശൂന്യം dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() സ്ഥിരം { തിരികെ m_declPrivate; }
    ശൂന്യം declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() സ്ഥിരം VL_MT_SAFE { തിരികെ m_keepIfEmpty; }
    ശൂന്യം keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() സ്ഥിരം VL_MT_SAFE { തിരികെ m_slow; }
    ശൂന്യം slow(bool flag) { m_slow = flag; }
    bool funcPublic() സ്ഥിരം { തിരികെ m_funcPublic; }
    ശൂന്യം funcPublic(bool flag) { m_funcPublic = flag; }
    ശൂന്യം argTypes(സ്ഥിരം string& str) { m_argTypes = str; }
    string argTypes() സ്ഥിരം { തിരികെ m_argTypes; }
    ശൂന്യം ifdef(സ്ഥിരം string& str) { m_ifdef = str; }
    string ifdef() സ്ഥിരം { തിരികെ m_ifdef; }
    bool isConstructor() സ്ഥിരം { തിരികെ m_isConstructor; }
    ശൂന്യം isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() സ്ഥിരം { തിരികെ m_isDestructor; }
    ശൂന്യം isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() സ്ഥിരം { തിരികെ m_isMethod; }
    ശൂന്യം isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() സ്ഥിരം { തിരികെ m_isLoose; }
    ശൂന്യം isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() സ്ഥിരം { തിരികെ isMethod() && !isLoose(); }
    bool isVirtual() സ്ഥിരം { തിരികെ m_isVirtual; }
    ശൂന്യം isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() സ്ഥിരം { തിരികെ m_needProcess; }
    ശൂന്യം setNeedProcess() { m_needProcess = true; }
    bool entryPoint() സ്ഥിരം { തിരികെ m_entryPoint; }
    ശൂന്യം entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() സ്ഥിരം { തിരികെ m_dpiPure; }
    ശൂന്യം dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() സ്ഥിരം { തിരികെ m_dpiContext; }
    ശൂന്യം dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() സ്ഥിരം VL_MT_SAFE { തിരികെ m_dpiExportDispatcher; }
    ശൂന്യം dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() സ്ഥിരം { തിരികെ m_dpiExportImpl; }
    ശൂന്യം dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() സ്ഥിരം VL_MT_SAFE { തിരികെ m_dpiImportPrototype; }
    ശൂന്യം dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() സ്ഥിരം { തിരികെ m_dpiImportWrapper; }
    ശൂന്യം dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() സ്ഥിരം { തിരികെ !m_dpiCDecl.empty(); }
    സ്ഥിരം string& dpiCDecl() സ്ഥിരം { തിരികെ m_dpiCDecl; }
    ശൂന്യം dpiCDecl(സ്ഥിരം string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() സ്ഥിരം { തിരികെ m_rtnType == "VlCoroutine"; }
    ശൂന്യം recursive(bool flag) { m_recursive = flag; }
    bool recursive() സ്ഥിരം { തിരികെ m_recursive; }
    ശൂന്യം unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() സ്ഥിരം override { തിരികെ m_unlikely; }  // Note virtual override
    ശൂന്യം noLife(bool flag) { m_noLife = flag; }
    bool noLife() സ്ഥിരം { തിരികെ m_noLife; }
    bool isCovergroupSample() സ്ഥിരം { തിരികെ m_isCovergroupSample; }
    ശൂന്യം isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() സ്ഥിരം { തിരികെ m_voidSelf; }
    ശൂന്യം voidSelf(bool flag) { m_voidSelf = flag; }
    ശൂന്യം cost(പൂർണ്ണസംഖ്യ cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() സ്ഥിരം {
        തിരികെ !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : പൊതു Ast_കണ്ണി {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    സ്ഥിരം string m_name;
    സ്ഥിരം VUseType m_useType;  // What sort of use this is

പൊതു:
    AstCUse(File_വരി* fl, VUseType useType, സ്ഥിരം string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    VUseType useType() സ്ഥിരം { തിരികെ m_useType; }
};
class AstCell final : പൊതു Ast_കണ്ണി {
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
    File_വരി* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    പൂർണ്ണസംഖ്യ m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
പൊതു:
    AstCell(File_വരി* fl, File_വരി* mfl, സ്ഥിരം string& instName, സ്ഥിരം string& modName,
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
    ശൂന്യം cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    // ACCESSORS
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Cell name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string origName() സ്ഥിരം override { തിരികെ m_origName; }  // * = Original name
    ശൂന്യം origName(സ്ഥിരം string& name) { m_origName = name; }
    string modName() സ്ഥിരം { തിരികെ m_modName; }  // * = Instance name
    ശൂന്യം modName(സ്ഥിരം string& name) { m_modName = name; }
    File_വരി* modNameFileline() സ്ഥിരം { തിരികെ m_modNameFileline; }
    Ast_കണ്ണി_Module* modp() സ്ഥിരം { തിരികെ m_modp; }  // [AfterLink] = Pointer to module instantiated
    ശൂന്യം modp(Ast_കണ്ണി_Module* nodep) { m_modp = nodep; }
    പൂർണ്ണസംഖ്യ arrayIdx() സ്ഥിരം { തിരികെ m_arrayIdx; }
    ശൂന്യം arrayIdx(പൂർണ്ണസംഖ്യ idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() സ്ഥിരം { തിരികെ m_hasIfaceVar; }
    ശൂന്യം hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    ശൂന്യം trace(bool flag) { m_trace = flag; }
    bool isTrace() സ്ഥിരം { തിരികെ m_trace; }
    ശൂന്യം recursive(bool flag) { m_recursive = flag; }
    bool recursive() സ്ഥിരം { തിരികെ m_recursive; }
};
class AstCellInline final : പൊതു Ast_കണ്ണി {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    സ്ഥിരം std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    സ്ഥിരം std::string m_origModName;  // __BEGIN__ or __FORK__
പൊതു:
    AstCellInline(File_വരി* fl, സ്ഥിരം std::string& name, സ്ഥിരം std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    // ACCESSORS
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    std::string origModName() സ്ഥിരം { തിരികെ m_origModName; }
};
class AstCellInlineScope final : പൊതു Ast_കണ്ണി {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    സ്ഥിരം std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
പൊതു:
    AstCellInlineScope(File_വരി* fl, സ്ഥിരം std::string& name, സ്ഥിരം std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    // ACCESSORS
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം std::string& name) override { m_name = name; }
    std::string origModName() സ്ഥിരം { തിരികെ m_origModName; }
};
class AstCgOptionAssign final : പൊതു Ast_കണ്ണി {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    സ്ഥിരം VCoverOptionType m_optType;  // Option type
    സ്ഥിരം string m_name;  // Original option name (for diagnostics on unknown options)
    സ്ഥിരം bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
പൊതു:
    AstCgOptionAssign(File_വരി* fl, bool typeOption, VCoverOptionType optType,
                      സ്ഥിരം string& rawName, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    VCoverOptionType optType() സ്ഥിരം { തിരികെ m_optType; }
    bool typeOption() സ്ഥിരം { തിരികെ m_typeOption; }
};
class AstClassExtends final : പൊതു Ast_കണ്ണി {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    സ്ഥിരം bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

പൊതു:
    AstClassExtends(File_വരി* fl, Ast_കണ്ണി* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    string verilogKwd() സ്ഥിരം override { തിരികെ isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() സ്ഥിരം;
    AstClass* classp() സ്ഥിരം;  // Like above, but throws error if nulll
    bool isImplements() സ്ഥിരം { തിരികെ m_isImplements; }
    ശൂന്യം parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() സ്ഥിരം { തിരികെ m_parameterized; }
};
class AstClocking final : പൊതു Ast_കണ്ണി {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    സ്ഥിരം bool m_isGlobal;  // True if global clocking

പൊതു:
    AstClocking(File_വരി* fl, സ്ഥിരം std::string& name, Ast_Sen_വൃക്ഷം* sentreep, Ast_കണ്ണി* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    bool isDefault() സ്ഥിരം { തിരികെ m_isDefault; }
    bool isGlobal() സ്ഥിരം { തിരികെ m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    ശൂന്യം makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : പൊതു Ast_കണ്ണി {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

പൊതു:
    AstClockingItem(File_വരി* fl, VDirection direction, Ast_കണ്ണി_വ്യഞ്ജകം* skewp, Ast_കണ്ണി* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        എങ്കിൽ (AstAssign* സ്ഥിരം clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } അല്ലെങ്കിൽ {
            exprp(VN_AS(clockingDeclp, കണ്ണി_വ്യഞ്ജകം));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    VDirection direction() സ്ഥിരം { തിരികെ m_direction; }
    AstClockingItem* outputp() സ്ഥിരം { തിരികെ m_outputp; }
    ശൂന്യം outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
};
class AstConfig final : പൊതു Ast_കണ്ണി {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

പൊതു:
    AstConfig(File_വരി* fl, സ്ഥിരം std::string& libname, സ്ഥിരം std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_libname + "." + m_configname; }
    std::string libname() സ്ഥിരം VL_MT_STABLE { തിരികെ m_libname; }
    std::string configname() സ്ഥിരം VL_MT_STABLE { തിരികെ m_configname; }
};
class AstConfigCell final : പൊതു Ast_കണ്ണി {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

പൊതു:
    AstConfigCell(File_വരി* fl, സ്ഥിരം std::string& libname, സ്ഥിരം std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_libname + "." + m_cellname; }
    std::string libname() സ്ഥിരം VL_MT_STABLE { തിരികെ m_libname; }
    std::string cellname() സ്ഥിരം VL_MT_STABLE { തിരികെ m_cellname; }
};
class AstConfigRule final : പൊതു Ast_കണ്ണി {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    സ്ഥിരം bool m_isCell;  // Declared as "cell" versus "instance"

പൊതു:
    AstConfigRule(File_വരി* fl, Ast_കണ്ണി* cellp, Ast_കണ്ണി* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() സ്ഥിരം VL_MT_STABLE { തിരികെ m_isCell; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstConfigUse final : പൊതു Ast_കണ്ണി {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    സ്ഥിരം bool m_isConfig;  // ":config"; Config, not module/primitive name

പൊതു:
    AstConfigUse(File_വരി* fl, സ്ഥിരം std::string& libname, സ്ഥിരം std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_libname + "." + m_cellname; }
    std::string libname() സ്ഥിരം VL_MT_STABLE { തിരികെ m_libname; }
    std::string cellname() സ്ഥിരം VL_MT_STABLE { തിരികെ m_cellname; }
    bool isConfig() സ്ഥിരം VL_MT_STABLE { തിരികെ m_isConfig; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstConstraint final : പൊതു Ast_കണ്ണി {
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
പൊതു:
    AstConstraint(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Scope name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    ശൂന്യം baseOverride(സ്ഥിരം VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() സ്ഥിരം { തിരികെ m_baseOverride; }
    bool isExternDef() സ്ഥിരം { തിരികെ m_isExternDef; }
    ശൂന്യം isExternDef(bool flag) { m_isExternDef = flag; }
    ശൂന്യം isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() സ്ഥിരം { തിരികെ m_isExternExplicit; }
    ശൂന്യം isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() സ്ഥിരം { തിരികെ m_isExternProto; }
    ശൂന്യം isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() സ്ഥിരം { തിരികെ m_isKwdPure; }
    ശൂന്യം isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() സ്ഥിരം { തിരികെ m_isStatic; }
};
class AstCoverBin final : പൊതു Ast_കണ്ണി {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    സ്ഥിരം string m_name;  // Base name of the bin
    സ്ഥിരം VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

പൊതു:
    AstCoverBin(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_കണ്ണി_വ്യഞ്ജകം* iffp = nullptr)
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
    AstCoverBin(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി_വ്യഞ്ജകം* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_വരി* fl, സ്ഥിരം string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_വരി* fl, സ്ഥിരം string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    VCoverBinsType binsType() സ്ഥിരം { തിരികെ m_binsType; }
    bool isWildcard() സ്ഥിരം { തിരികെ m_isWildcard; }
    bool isArray() സ്ഥിരം { തിരികെ m_isArray; }
    ശൂന്യം isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : പൊതു Ast_കണ്ണി {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    സ്ഥിരം bool m_isNegated;  // Complement the selection within the cross product

പൊതു:
    AstCoverBinsof(File_വരി* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_കണ്ണി* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    bool isNegated() സ്ഥിരം { തിരികെ m_isNegated; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {  // LCOV_EXCL_START
        സ്ഥിരം AstCoverBinsof* സ്ഥിരം asamep = VN_DBG_AS(samep, CoverBinsof);
        തിരികെ m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : പൊതു Ast_കണ്ണി {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    സ്ഥിരം string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    സ്ഥിരം VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

പൊതു:
    AstCoverCrossBin(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* selectp, Ast_കണ്ണി_വ്യഞ്ജകം* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    string verilogKwd() സ്ഥിരം override;
    VCoverBinsType binsType() സ്ഥിരം { തിരികെ m_binsType; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {  // LCOV_EXCL_START
        സ്ഥിരം AstCoverCrossBin* സ്ഥിരം asamep = VN_DBG_AS(samep, CoverCrossBin);
        തിരികെ m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : പൊതു Ast_കണ്ണി {
    // The complete bin-tuple space of the enclosing cross
    സ്ഥിരം string m_name;  // Referenced cross name

പൊതു:
    AstCoverCrossRef(File_വരി* fl, സ്ഥിരം string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {  // LCOV_EXCL_START
        തിരികെ m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : പൊതു Ast_കണ്ണി {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    സ്ഥിരം bool m_isOr;  // Union (||), rather than intersection (&&)

പൊതു:
    AstCoverCrossSelect(File_വരി* fl, Ast_കണ്ണി* lhsp, Ast_കണ്ണി* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool isOr() സ്ഥിരം { തിരികെ m_isOr; }
    string verilogKwd() സ്ഥിരം override { തിരികെ isOr() ? "||" : "&&"; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {  // LCOV_EXCL_START
        തിരികെ m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : പൊതു Ast_കണ്ണി {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
പൊതു:
    AstCoverCrossWith(File_വരി* fl, Ast_കണ്ണി* selectp, Ast_കണ്ണി_വ്യഞ്ജകം* filterp, Ast_കണ്ണി_വ്യഞ്ജകം* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : പൊതു Ast_കണ്ണി {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    സ്ഥിരം VCoverOptionType m_optType;  // Option being assigned
    സ്ഥിരം bool m_typeOption;  // type_option (vs option)

പൊതു:
    AstCoverOption(File_വരി* fl, bool typeOption, VCoverOptionType optType, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    VCoverOptionType optType() സ്ഥിരം { തിരികെ m_optType; }
    bool typeOption() സ്ഥിരം { തിരികെ m_typeOption; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {  // LCOV_EXCL_START
        സ്ഥിരം AstCoverOption* സ്ഥിരം asamep = VN_DBG_AS(samep, CoverOption);
        തിരികെ m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : പൊതു Ast_കണ്ണി {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
പൊതു:
    AstCoverToggle(File_വരി* fl, AstCoverInc* incp, Ast_കണ്ണി_വ്യഞ്ജകം* origp, Ast_കണ്ണി_വ്യഞ്ജകം* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ 3 + INSTR_എണ്ണം_BRANCH + INSTR_എണ്ണം_LD; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ true; }
    bool isOutputter() override {
        തിരികെ false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : പൊതു Ast_കണ്ണി {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    സ്ഥിരം VTransRepType m_repType;

പൊതു:
    AstCoverTransItem(File_വരി* fl, Ast_കണ്ണി* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstCoverTransSet final : പൊതു Ast_കണ്ണി {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
പൊതു:
    AstCoverTransSet(File_വരി* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstCoverWith final : പൊതു Ast_കണ്ണി {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
പൊതു:
    AstCoverWith(File_വരി* fl, Ast_കണ്ണി* subp, Ast_കണ്ണി_വ്യഞ്ജകം* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        addSubp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : പൊതു Ast_കണ്ണി {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

പൊതു:
    AstCovergroup(File_വരി* fl, സ്ഥിരം string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_കണ്ണി* membersp, Ast_Sen_വൃക്ഷം* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    bool maybePointedTo() സ്ഥിരം override { തിരികെ true; }
};
class AstCoverpointRef final : പൊതു Ast_കണ്ണി {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    സ്ഥിരം string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

പൊതു:
    AstCoverpointRef(File_വരി* fl, സ്ഥിരം string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
};
class AstDefParam final : പൊതു Ast_കണ്ണി {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
പൊതു:
    AstDefParam(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* pathp, സ്ഥിരം string& name, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ true; }
};
class AstDefaultClocking final : പൊതു Ast_കണ്ണി {
    std::string m_name;  // Clocking block name

പൊതു:
    AstDefaultClocking(File_വരി* fl, സ്ഥിരം std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
};
class AstDefaultDisable final : പൊതു Ast_കണ്ണി {
    // @astgen op1 := condp : AstNodeExpr

പൊതു:
    AstDefaultDisable(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : പൊതു Ast_കണ്ണി {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
പൊതു:
    AstDpiExport(File_വരി* fl, സ്ഥിരം string& vname, സ്ഥിരം string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string cname() സ്ഥിരം { തിരികെ m_cname; }
    ശൂന്യം cname(സ്ഥിരം string& cname) { m_cname = cname; }
};
class AstElabDisplay final : പൊതു Ast_കണ്ണി {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

പൊതു:
    വരിയിൽ AstElabDisplay(File_വരി* fl, VDisplayType dispType, Ast_കണ്ണി_വ്യഞ്ജകം* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!fmtp());
        തിരികെ nullptr;
    }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string verilogKwd() സ്ഥിരം override { തിരികെ "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { തിരികെ true; }  // SPECIAL: $display makes output
    bool isUnlikely() സ്ഥിരം override { തിരികെ true; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം override { തിരികെ INSTR_എണ്ണം_PLI; }
    VDisplayType displayType() സ്ഥിരം { തിരികെ m_displayType; }
    ശൂന്യം displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : പൊതു Ast_കണ്ണി {
    // Represents something missing, e.g. a missing argument in FOREACH
പൊതു:
    explicit AstEmpty(File_വരി* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstGenCaseItem final : പൊതു Ast_കണ്ണി {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
പൊതു:
    AstGenCaseItem(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condsp, Ast_കണ്ണി* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() സ്ഥിരം { തിരികെ condsp() == nullptr; }
};
class AstImplicit final : പൊതു Ast_കണ്ണി {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
പൊതു:
    AstImplicit(File_വരി* fl, Ast_കണ്ണി* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : പൊതു Ast_കണ്ണി {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
പൊതു:
    AstIntfRef(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& parentName, സ്ഥിരം string& baseName,
               സ്ഥിരം string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    string parentName() സ്ഥിരം { തിരികെ m_parentName; }
    string baseName() സ്ഥിരം { തിരികെ m_baseName; }
    string modportName() സ്ഥിരം { തിരികെ m_modportName; }
};
class AstLibrary final : പൊതു Ast_കണ്ണി {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

പൊതു:
    AstLibrary(File_വരി* fl, സ്ഥിരം std::string& name, Ast_കണ്ണി_വ്യഞ്ജകം* filesp, Ast_കണ്ണി_വ്യഞ്ജകം* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
};
class AstModport final : പൊതു Ast_കണ്ണി {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
പൊതു:
    AstModport(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "modport"; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : പൊതു Ast_കണ്ണി {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
പൊതു:
    AstModportClockingRef(File_വരി* fl, സ്ഥിരം string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    AstClocking* clockingp() സ്ഥിരം VL_MT_STABLE {
        തിരികെ m_clockingp;
    }  // [After Link] Pointer to clocking block
    ശൂന്യം clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : പൊതു Ast_കണ്ണി {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
പൊതു:
    AstModportFTaskRef(File_വരി* fl, സ്ഥിരം string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    bool isImport() സ്ഥിരം { തിരികെ !m_export; }
    bool isExport() സ്ഥിരം { തിരികെ m_export; }
    Ast_കണ്ണി_FTask* ftaskp() സ്ഥിരം { തിരികെ m_ftaskp; }  // [After Link] Pointer to variable
    ശൂന്യം ftaskp(Ast_കണ്ണി_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : പൊതു Ast_കണ്ണി {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    സ്ഥിരം VDirection m_direction;  // Direction of the variable (in/out)
പൊതു:
    AstModportVarRef(File_വരി* fl, സ്ഥിരം string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    VDirection direction() സ്ഥിരം { തിരികെ m_direction; }
    AstVar* varp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_varp; }  // [After Link] Pointer to variable
    ശൂന്യം varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : പൊതു Ast_കണ്ണി {
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
    std::string m_resolved_മുകൾ_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::set<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<സ്ഥിരം AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി*, സ്ഥിരം Ast_കണ്ണി_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

പൊതു:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override;
    ശൂന്യം തള്ളുക_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.തിരുകുക(varp); }
    സ്ഥിരം std::set<AstVar*>& deferredParamVarps() സ്ഥിരം { തിരികെ m_deferredParamVarps; }
    ശൂന്യം clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    ശൂന്യം deleteContents();
    ശൂന്യം cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ "$root"; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    Ast_കണ്ണി_Module* മുകൾ_Modulep() സ്ഥിരം VL_MT_STABLE {  // Top module in hierarchy
        തിരികെ modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { തിരികെ m_typeTablep; }
    string astConstOrigParamName(സ്ഥിരം AstConst* nodep) സ്ഥിരം;
    ശൂന്യം astConstOrigParamName(സ്ഥിരം AstConst* nodep, സ്ഥിരം string& name);
    ശൂന്യം astConstOrigParamNameErase(സ്ഥിരം AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    സ്ഥിരം Ast_കണ്ണി_Module* containingModule(സ്ഥിരം Ast_കണ്ണി* nodep);
    // Forget remembered modules, as the tree has moved.
    ശൂന്യം clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() സ്ഥിരം { തിരികെ m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() സ്ഥിരം { തിരികെ m_dollarUnitPkgp; }
    ശൂന്യം dollarUnitPkgp(AstPackage* സ്ഥിരം packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() സ്ഥിരം { തിരികെ m_dpiExportTriggerp; }
    ശൂന്യം dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() സ്ഥിരം { തിരികെ m_delaySchedulerp; }
    ശൂന്യം delaySchedulerp(AstVar* സ്ഥിരം varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() സ്ഥിരം { തിരികെ m_nbaEventp; }
    ശൂന്യം nbaEventp(AstVarScope* സ്ഥിരം varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() സ്ഥിരം { തിരികെ m_nbaEventTriggerp; }
    ശൂന്യം nbaEventTriggerp(AstVarScope* സ്ഥിരം varScopep) { m_nbaEventTriggerp = varScopep; }
    ശൂന്യം stdPackagep(AstPackage* സ്ഥിരം packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() സ്ഥിരം { തിരികെ m_stdPackagep; }
    ശൂന്യം stdPackageProcessp(AstClass* സ്ഥിരം classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() സ്ഥിരം { തിരികെ m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_വരി*) സ്ഥിരം;
    Ast_മുകൾ_Scope* മുകൾ_Scopep() സ്ഥിരം { തിരികെ m_മുകൾ_Scopep; }
    ശൂന്യം create_മുകൾ_Scope(AstScope* scopep);
    VTimescale timeunit() സ്ഥിരം { തിരികെ m_timeunit; }
    ശൂന്യം timeunit(സ്ഥിരം VTimescale& മൂല്യം) { m_timeunit = മൂല്യം; }
    VTimescale timeprecision() സ്ഥിരം { തിരികെ m_timeprecision; }
    ശൂന്യം timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    ശൂന്യം timeprecisionMerge(File_വരി*, സ്ഥിരം VTimescale& മൂല്യം);
    ശൂന്യം timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() സ്ഥിരം { തിരികെ m_timescaleSpecified; }
    സ്ഥിരം std::string& resolved_മുകൾ_Module_Name() സ്ഥിരം { തിരികെ m_resolved_മുകൾ_Module_Name; }
    ശൂന്യം resolved_മുകൾ_Module_Name(സ്ഥിരം std::string& മൂല്യം) { m_resolved_മുകൾ_Module_Name = മൂല്യം; }
    uint32_t nTraceCodes() സ്ഥിരം { തിരികെ m_nTraceCodes; }
    ശൂന്യം nTraceCodes(uint32_t മൂല്യം) { m_nTraceCodes = മൂല്യം; }
    AstVarScope* stlFirstIterationp();
    ശൂന്യം clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    സ്ഥിരം std::string trace_Lib_മുകൾ_Name() സ്ഥിരം {
        സ്ഥിരം std::string& name = resolved_മുകൾ_Module_Name();
        തിരികെ prettyName(name.empty() ? v3Global.rootp()->മുകൾ_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    ശൂന്യം addEvalStats(സ്ഥിരം std::string& phase);
};
class AstPackageExport final : പൊതു Ast_കണ്ണി {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

പൊതു:
    AstPackageExport(File_വരി* fl, സ്ഥിരം string& pkgName, സ്ഥിരം string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    string pkgName() സ്ഥിരം VL_MT_STABLE { തിരികെ m_pkgName; }
    string prettyPkgNameQ() സ്ഥിരം { തിരികെ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() സ്ഥിരം { തിരികെ m_packagep; }
    ശൂന്യം packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : പൊതു Ast_കണ്ണി {
    // A package export *::* declaration
പൊതു:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_വരി* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : പൊതു Ast_കണ്ണി {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

പൊതു:
    AstPackageImport(File_വരി* fl, AstPackage* packagep, സ്ഥിരം string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_വരി* fl, സ്ഥിരം string& pkgName, സ്ഥിരം string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    string pkgName() സ്ഥിരം VL_MT_STABLE { തിരികെ m_pkgName; }
    string prettyPkgNameQ() സ്ഥിരം { തിരികെ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() സ്ഥിരം { തിരികെ m_packagep; }
    ശൂന്യം packagep(AstPackage* nodep) { m_packagep = nodep; }

സ്വകാര്യ:
    ശൂന്യം pkgNameFrom();
};
class AstPin final : പൊതു Ast_കണ്ണി {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    പൂർണ്ണസംഖ്യ m_pin_അക്കം;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
പൊതു:
    AstPin(File_വരി* fl, പൂർണ്ണസംഖ്യ pin_അക്കം, സ്ഥിരം string& name, Ast_കണ്ണി* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_അക്കം{pin_അക്കം}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    ശൂന്യം cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Pin name, ""=go by number
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string prettyOperatorName() സ്ഥിരം override;
    bool dotStar() സ്ഥിരം { തിരികെ name() == ".*"; }  // Fake name for .* connections until linked
    പൂർണ്ണസംഖ്യ pin_അക്കം() സ്ഥിരം { തിരികെ m_pin_അക്കം; }
    AstVar* modVarp() സ്ഥിരം { തിരികെ m_modVarp; }  // [After Link] Pointer to variable
    ശൂന്യം modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() സ്ഥിരം { തിരികെ m_modPTypep; }
    ശൂന്യം modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() സ്ഥിരം { തിരികെ m_param; }
    ശൂന്യം param(bool flag) { m_param = flag; }
    സ്ഥിരം string& paramPath() സ്ഥിരം { തിരികെ m_paramPath; }
    ശൂന്യം paramPath(സ്ഥിരം string& path) { m_paramPath = path; }
    bool svDotName() സ്ഥിരം { തിരികെ m_svDotName; }
    ശൂന്യം svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() സ്ഥിരം { തിരികെ m_svImplicit; }
    ശൂന്യം svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : പൊതു Ast_കണ്ണി {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_pin_അക്കം;  // Pin number
    സ്ഥിരം string m_name;  // Name of pin
പൊതു:
    AstPort(File_വരി* fl, പൂർണ്ണസംഖ്യ pinnum, സ്ഥിരം string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_അക്കം{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Port name
    പൂർണ്ണസംഖ്യ pin_അക്കം() സ്ഥിരം { തിരികെ m_pin_അക്കം; }  // * = Pin number, for order based instantiation
};
class AstPragma final : പൊതു Ast_കണ്ണി {
    സ്ഥിരം VPragmaType m_pragType;  // Type of pragma
    സ്ഥിരം VTimescale m_timescale;  // For TIMEUNIT_SET
പൊതു:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_വരി* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_വരി* fl, സ്ഥിരം VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    VPragmaType pragType() സ്ഥിരം { തിരികെ m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() സ്ഥിരം override { തിരികെ false; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() സ്ഥിരം { തിരികെ m_timescale; }
};
class AstPropSpec final : പൊതു Ast_കണ്ണി {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

പൊതു:
    AstPropSpec(File_വരി* fl, AstSenItem* sensesp, Ast_കണ്ണി_വ്യഞ്ജകം* disablep, Ast_കണ്ണി* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool hasDType() സ്ഥിരം override VL_MT_SAFE {
        തിരികെ true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() സ്ഥിരം { തിരികെ m_propStrength; }
};
class AstPull final : പൊതു Ast_കണ്ണി {
    // @astgen op1 := lhsp : AstNodeExpr

    സ്ഥിരം bool m_direction;

പൊതു:
    AstPull(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() സ്ഥിരം { തിരികെ m_direction; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstScope final : പൊതു Ast_കണ്ണി {
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
പൊതു:
    AstScope(File_വരി* fl, Ast_കണ്ണി_Module* modp, സ്ഥിരം string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!m_modp);
        തിരികെ nullptr;
    }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Scope name
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    string pretty_Name_സന്ദേശം() സ്ഥിരം override;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override;
    string nameDotless() സ്ഥിരം;
    Ast_കണ്ണി_Module* modp() സ്ഥിരം { തിരികെ m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() സ്ഥിരം VL_MT_SAFE { തിരികെ m_aboveScopep; }
    ശൂന്യം aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() സ്ഥിരം { തിരികെ m_aboveCellp; }
    ശൂന്യം aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_മുകൾ() സ്ഥിരം VL_MT_SAFE { തിരികെ aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : പൊതു Ast_കണ്ണി {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_edgeType;  // Edge type
പൊതു:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    AstSenItem(File_വരി* fl, VEdgeType edgeType, Ast_കണ്ണി_വ്യഞ്ജകം* senp, Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_വരി* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_വരി* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_വരി* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_വരി* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_വരി* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_വരി* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() സ്ഥിരം { തിരികെ m_edgeType; }
    ശൂന്യം edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_എണ്ണം_Inc();
    }
    Ast_കണ്ണി_Var_Ref* varrefp() സ്ഥിരം { തിരികെ VN_CAST(sensp(), കണ്ണി_Var_Ref); }
    //
    bool isClocked() സ്ഥിരം { തിരികെ edgeType().clockedStmt(); }
    bool isComboOrStar() സ്ഥിരം {
        തിരികെ edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() സ്ഥിരം { തിരികെ edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() സ്ഥിരം { തിരികെ edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() സ്ഥിരം { തിരികെ edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() സ്ഥിരം { തിരികെ edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() സ്ഥിരം { തിരികെ edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() സ്ഥിരം { തിരികെ edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_വൃക്ഷം final : പൊതു Ast_കണ്ണി {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
പൊതു:
    Ast_Sen_വൃക്ഷം(File_വരി* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_വൃക്ഷം(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_വൃക്ഷം;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    // METHODS
    bool hasClocked() സ്ഥിരം;  // Includes a clocked statement
    bool hasEdge() സ്ഥിരം;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() സ്ഥിരം;  // Includes a STATIC SenItem
    bool hasInitial() സ്ഥിരം;  // Includes a INITIAL SenItem
    bool hasFinal() സ്ഥിരം;  // Includes a FINAL SenItem
    bool hasCombo() സ്ഥിരം;  // Includes a COMBO SenItem
    bool hasHybrid() സ്ഥിരം;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : പൊതു Ast_കണ്ണി {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

പൊതു:
    AstStrengthSpec(File_വരി* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { തിരികെ m_s0; }
    VStrength strength1() { തിരികെ m_s1; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstSystemCSection final : പൊതു Ast_കണ്ണി {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    സ്ഥിരം VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

പൊതു:
    AstSystemCSection(File_വരി* fl, VSystemCSectionType sectionType, സ്ഥിരം std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() സ്ഥിരം { തിരികെ m_sectionType; }
    സ്ഥിരം std::string& text() സ്ഥിരം { തിരികെ m_text; }
    ശൂന്യം text(സ്ഥിരം std::string& മൂല്യം) { m_text = മൂല്യം; }
    ശൂന്യം dump(std::ostream&) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream&) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം override { തിരികെ false; }
    bool isPure() override { തിരികെ false; }
    bool isOutputter() override { തിരികെ true; }
};
class AstText final : പൊതു Ast_കണ്ണി {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    സ്ഥിരം std::string m_text;  // The text to emit
പൊതു:
    AstText(File_വരി* fl, സ്ഥിരം std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ text() == VN_DBG_AS(samep, Text)->text();
    }
    സ്ഥിരം std::string& text() സ്ഥിരം VL_MT_SAFE { തിരികെ m_text; }
};
class AstTextBlock final : പൊതു Ast_കണ്ണി {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    സ്ഥിരം std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    സ്ഥിരം std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    സ്ഥിരം std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
പൊതു:
    explicit AstTextBlock(File_വരി* fl,  //
                          സ്ഥിരം std::string& prefix = "",  //
                          സ്ഥിരം std::string& separator = "",  //
                          സ്ഥിരം std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    സ്ഥിരം std::string& prefix() സ്ഥിരം { തിരികെ m_prefix; }
    സ്ഥിരം std::string& separator() സ്ഥിരം { തിരികെ m_separator; }
    സ്ഥിരം std::string& suffix() സ്ഥിരം { തിരികെ m_suffix; }
    // Add some text, or a node to this block
    ശൂന്യം add(സ്ഥിരം string& text) { addNodesp(new AstText{fileline(), text}); }
    ശൂന്യം add(Ast_കണ്ണി* nodep) { addNodesp(nodep); }
};
class Ast_മുകൾ_Scope final : പൊതു Ast_കണ്ണി {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    friend class AstNetlist;  // Only the AstNetlist can create one
    Ast_മുകൾ_Scope(File_വരി* fl, AstScope* ascopep)
        : ASTGEN_SUPER_മുകൾ_Scope(fl) {
        this->scopep(ascopep);
    }

പൊതു:
    ASTGEN_MEMBERS_Ast_മുകൾ_Scope;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* createTemp(സ്ഥിരം string& name, unsigned width);
    AstVarScope* createTemp(സ്ഥിരം string& name, Ast_കണ്ണി_DType* dtypep);
};
class AstTypeTable final : പൊതു Ast_കണ്ണി {
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
    using DetailedMap = std::map<VBasic_Type_താക്കോൽ, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

പൊതു:
    explicit AstTypeTable(File_വരി* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    ശൂന്യം cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_വരി* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_വരി* fl, VBasicDTypeKwd kwd, പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_വരി* fl, VBasicDTypeKwd kwd, സ്ഥിരം VNumRange& range,
                                     പൂർണ്ണസംഖ്യ widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& കണ്ണി);
    AstBasicDType* find_തിരുകുക_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_വരി* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_വരി* fl);
    AstQueueDType* findQueueIndexDType(File_വരി* fl);
    AstStreamDType* findStreamDType(File_വരി* fl);
    AstVoidDType* findVoidDType(File_വരി* fl);
    ശൂന്യം clearCache();
    ശൂന്യം repairCache();
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
};
class AstTypedef final : പൊതു Ast_കണ്ണി {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_അക്കം;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

പൊതു:
    AstTypedef(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* attrsp, VFlagChildDType,
               Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_അക്കം{fl->token_അക്കം()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_അക്കം{fl->token_അക്കം()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() സ്ഥിരം;
    Ast_കണ്ണി_DType* getChildDTypep() സ്ഥിരം override { തിരികെ childDTypep(); }
    മായാ Ast_കണ്ണി_DType* subDTypep() സ്ഥിരം VL_MT_STABLE {
        തിരികെ dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_അക്കം() സ്ഥിരം override { തിരികെ m_decl_Token_അക്കം; }
    ശൂന്യം decl_Token_അക്കം_Set_Min(uint32_t token_അക്കം) override {
        m_decl_Token_അക്കം = std::min(m_decl_Token_അക്കം, token_അക്കം);
    }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    ശൂന്യം name(സ്ഥിരം string& flag) override { m_name = flag; }
    bool attrPublic() സ്ഥിരം { തിരികെ m_attrPublic; }
    ശൂന്യം attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() സ്ഥിരം { തിരികെ m_isHideLocal; }
    ശൂന്യം isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() സ്ഥിരം { തിരികെ m_isHideProtected; }
    ശൂന്യം isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() സ്ഥിരം { തിരികെ m_isUnderClass; }
    ശൂന്യം isUnderClass(bool flag) { m_isUnderClass = flag; }
    ശൂന്യം tag(സ്ഥിരം string& text) override { m_tag = text; }
    string tag() സ്ഥിരം override { തിരികെ m_tag; }
};
class AstTypedefFwd final : പൊതു Ast_കണ്ണി {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    സ്ഥിരം VFwdType m_fwdType;  // Forward type for lint check

പൊതു:
    AstTypedefFwd(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    VFwdType fwdType() സ്ഥിരം { തിരികെ m_fwdType; }
};
class AstUdpTable final : പൊതു Ast_കണ്ണി {
    // @astgen op1 := linesp : List[AstUdpTableLine]
പൊതു:
    AstUdpTable(File_വരി* fl, Ast_Udp_Table_വരി* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        എങ്കിൽ (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_വരി final : പൊതു Ast_കണ്ണി {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
സ്വകാര്യ:
    സ്ഥിരം bool m_udpIsCombo;  // Combinational or sequential UDP

പൊതു:
    class UdpCombo {};
    Ast_Udp_Table_വരി(UdpCombo, File_വരി* fl, Ast_Udp_Table_വരി_Val* iFieldsp,
                    Ast_Udp_Table_വരി_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_വരി(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_വരി(UdpSequential, File_വരി* fl, Ast_Udp_Table_വരി_Val* iFieldsp,
                    Ast_Udp_Table_വരി_Val* oFieldsp1, Ast_Udp_Table_വരി_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_വരി(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_വരി;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ udpIsCombo() സ്ഥിരം { തിരികെ m_udpIsCombo; }
};
class Ast_Udp_Table_വരി_Val final : പൊതു Ast_കണ്ണി {
    string m_text;  // Value character

പൊതു:
    Ast_Udp_Table_വരി_Val(File_വരി* fl, സ്ഥിരം string& text)
        : ASTGEN_SUPER_Udp_Table_വരി_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_വരി_Val;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_text; }
    ശൂന്യം name(std::string സ്ഥിരം& text) override VL_MT_STABLE { m_text = text; }
    string text() സ്ഥിരം VL_MT_SAFE { തിരികെ m_text; }
};
class AstVar final : പൊതു Ast_കണ്ണി {
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
    പൂർണ്ണസംഖ്യ m_pin_അക്കം = 0;  // For JSON, if non-zero the connection pin number
    പൂർണ്ണസംഖ്യ m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
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
    bool m_mtask_Cache_വരി_Align : 1;  // Start MTask affinity group on a cache line
    ശൂന്യം init() {
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
        m_mtask_Cache_വരി_Align = false;
    }

പൊതു:
    AstVar(File_വരി* fl, VVarType type, സ്ഥിരം string& name, VFlagChildDType, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_വരി* fl, VVarType type, സ്ഥിരം string& name, Ast_കണ്ണി_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_വരി* fl, VVarType type, സ്ഥിരം string& name, VFlagLogicPacked, പൂർണ്ണസംഖ്യ wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_വരി* fl, VVarType type, സ്ഥിരം string& name, VFlagBitPacked, പൂർണ്ണസംഖ്യ wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_വരി* fl, VVarType type, സ്ഥിരം string& name, സ്ഥിരം AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        എങ്കിൽ (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_വൃക്ഷം(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    വരിയിൽ bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override;
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }  // * = Var name
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    string origName() സ്ഥിരം override { തിരികെ m_origName; }  // * = Original name
    ശൂന്യം origName(സ്ഥിരം string& name) { m_origName = name; }
    VVarType varType() സ്ഥിരം VL_MT_SAFE { തിരികെ m_varType; }  // * = Type of variable
    ശൂന്യം direction(സ്ഥിരം VDirection& flag) {
        m_direction = flag;
        എങ്കിൽ (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() സ്ഥിരം VL_MT_SAFE { തിരികെ m_direction; }
    bool isIO() സ്ഥിരം VL_MT_SAFE { തിരികെ m_direction != VDirection::NONE; }
    bool isVLIO() സ്ഥിരം {
        സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep = basicp();
        തിരികെ isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    ശൂന്യം declDirection(സ്ഥിരം VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() സ്ഥിരം { തിരികെ m_declDirection; }
    പൂർണ്ണസംഖ്യ libInoutId() സ്ഥിരം { തിരികെ m_libInoutId; }
    ശൂന്യം libInoutId(പൂർണ്ണസംഖ്യ id) { m_libInoutId = id; }
    bool libInoutEnable() സ്ഥിരം { തിരികെ m_libInoutEnable; }
    ശൂന്യം libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    ശൂന്യം varType(VVarType type) { m_varType = type; }
    ശൂന്യം varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    ശൂന്യം varType2In() {
        m_tristate = false;
        m_direction = VDirection::നിവേശം;
    }
    string scType() സ്ഥിരം;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) സ്ഥിരം;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) സ്ഥിരം;
    string dpiTmpVarType(സ്ഥിരം string& varName) സ്ഥിരം;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) സ്ഥിരം;
    string vlEnumType() സ്ഥിരം;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) സ്ഥിരം;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(സ്ഥിരം string& propName) സ്ഥിരം;  // Return VerilatorVarProps declaration
    ശൂന്യം combineType(VVarType type);
    Ast_കണ്ണി_DType* getChildDTypep() സ്ഥിരം override { തിരികെ childDTypep(); }
    Ast_കണ്ണി_DType* dtypeSkipRefp() സ്ഥിരം VL_MT_STABLE { തിരികെ subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() സ്ഥിരം VL_MT_STABLE { തിരികെ subDTypep()->basicp(); }
    മായാ Ast_കണ്ണി_DType* subDTypep() സ്ഥിരം VL_MT_STABLE {
        തിരികെ dtypep() ? dtypep() : childDTypep();
    }
    ശൂന്യം ansi(bool flag) { m_ansi = flag; }
    ശൂന്യം declTyped(bool flag) { m_declTyped = flag; }
    ശൂന്യം sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    ശൂന്യം attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    ശൂന്യം attrScBv(bool flag) { m_attrScBv = flag; }
    ശൂന്യം attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    ശൂന്യം attrSFormat(bool flag) { m_attrSFormat = flag; }
    ശൂന്യം attrSplitVar(bool flag) { m_attrSplitVar = flag; }
    ശൂന്യം attrFsmState(bool flag) { m_attrFsmState = flag; }
    ശൂന്യം attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    ശൂന്യം attrFsmStateExpand(സ്ഥിരം VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() സ്ഥിരം { തിരികെ m_constPoolEntry; }
    ശൂന്യം setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() സ്ഥിരം { തിരികെ m_covergroupRefMember; }
    ശൂന്യം covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() സ്ഥിരം { തിരികെ m_embeddedCovergroup; }
    ശൂന്യം embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    ശൂന്യം rand(സ്ഥിരം VRandAttr flag) { m_rand = flag; }
    ശൂന്യം usedParam(bool flag) { m_usedParam = flag; }
    ശൂന്യം usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    ശൂന്യം sigPublic(bool flag) { m_sigPublic = flag; }
    ശൂന്യം sigModPublic(bool flag) { m_sigModPublic = flag; }
    ശൂന്യം sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        എങ്കിൽ (flag) sigPublic(true);
    }
    ശൂന്യം sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        എങ്കിൽ (flag) sigUserRdPublic(true);
    }
    ശൂന്യം sc(bool flag) { m_sc = flag; }
    ശൂന്യം scSensitive(bool flag) { m_scSensitive = flag; }
    ശൂന്യം primaryIO(bool flag) { m_primaryIO = flag; }
    ശൂന്യം isConst(bool flag) { m_isConst = flag; }
    ശൂന്യം isContinuously(bool flag) { m_isContinuously = flag; }
    ശൂന്യം isIfaceParent(bool flag) { m_isIfaceParent = flag; }
    ശൂന്യം isIfaceArraySplit(bool flag) { m_isIfaceArraySplit = flag; }
    ശൂന്യം isInternal(bool flag) { m_isInternal = flag; }
    ശൂന്യം isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    ശൂന്യം funcLocal(bool flag) {
        m_funcLocal = flag;
        എങ്കിൽ (flag) m_funcLocalSticky = true;
    }
    ശൂന്യം funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() സ്ഥിരം { തിരികെ m_gotNansiType; }
    ശൂന്യം gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() സ്ഥിരം { തിരികെ m_hasStrengthAssignment; }
    ശൂന്യം hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() സ്ഥിരം { തിരികെ m_hasUserInit; }
    ശൂന്യം hasUserInit(bool flag) { m_hasUserInit = flag; }
    ശൂന്യം icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() സ്ഥിരം { തിരികെ m_icoMaybeWritten; }
    ശൂന്യം isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() സ്ഥിരം { തിരികെ m_isClockingDriven; }
    bool isDpiOpenArray() സ്ഥിരം VL_MT_SAFE { തിരികെ m_isDpiOpenArray; }
    ശൂന്യം isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() സ്ഥിരം { തിരികെ m_isHideLocal; }
    ശൂന്യം isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() സ്ഥിരം { തിരികെ m_isHideProtected; }
    ശൂന്യം isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() സ്ഥിരം { തിരികെ m_noCReset; }
    ശൂന്യം noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() സ്ഥിരം { തിരികെ m_noReset; }
    ശൂന്യം noReset(bool flag) { m_noReset = flag; }
    bool noSubst() സ്ഥിരം { തിരികെ m_noSubst; }
    ശൂന്യം noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() സ്ഥിരം { തിരികെ m_processQueue; }
    ശൂന്യം processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() സ്ഥിരം { തിരികെ m_sampled; }
    ശൂന്യം sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() സ്ഥിരം { തിരികെ m_substConstOnly; }
    ശൂന്യം substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() സ്ഥിരം { തിരികെ m_overriddenParam; }
    ശൂന്യം overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() സ്ഥിരം { തിരികെ m_untypedParam; }
    ശൂന്യം untypedParam(bool flag) { m_untypedParam = flag; }
    ശൂന്യം trace(bool flag) { m_trace = flag; }
    ശൂന്യം isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() സ്ഥിരം { തിരികെ m_isForceable; }
    ശൂന്യം setForceable() { m_isForceable = true; }
    ശൂന്യം setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() സ്ഥിരം { തിരികെ m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() സ്ഥിരം { തിരികെ m_isReadByDpi; }
    ശൂന്യം setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() സ്ഥിരം { തിരികെ m_isWrittenByDpi; }
    ശൂന്യം setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() സ്ഥിരം { തിരികെ m_isWrittenBySuspendable; }
    ശൂന്യം setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() സ്ഥിരം { തിരികെ m_ignorePostRead; }
    ശൂന്യം setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() സ്ഥിരം { തിരികെ m_ignorePostWrite; }
    ശൂന്യം setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() സ്ഥിരം { തിരികെ m_ignoreSchedWrite; }
    ശൂന്യം setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() സ്ഥിരം { തിരികെ m_dfgTriLowered; }
    ശൂന്യം setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() സ്ഥിരം { തിരികെ m_dfgAllowMultidriveTri; }
    ശൂന്യം setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    ശൂന്യം globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() സ്ഥിരം { തിരികെ m_globalConstrained; }
    bool isStdRandomizeArg() സ്ഥിരം { തിരികെ m_isStdRandomizeArg; }
    ശൂന്യം setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    ശൂന്യം name(സ്ഥിരം string& name) override { m_name = name; }
    ശൂന്യം tag(സ്ഥിരം string& text) override { m_tag = text; }
    string tag() സ്ഥിരം override { തിരികെ m_tag; }
    bool isAnsi() സ്ഥിരം { തിരികെ m_ansi; }
    bool isContinuously() സ്ഥിരം { തിരികെ m_isContinuously; }
    bool isDeclTyped() സ്ഥിരം { തിരികെ m_declTyped; }
    bool isInout() സ്ഥിരം { തിരികെ m_direction.isInout(); }
    bool isInoutOrRef() സ്ഥിരം { തിരികെ m_direction.isInoutOrRef(); }
    bool is_നിവേശം() സ്ഥിരം { തിരികെ m_direction.is_നിവേശം(); }
    bool isNonOutput() സ്ഥിരം { തിരികെ m_direction.isNonOutput(); }
    bool isReadOnly() സ്ഥിരം VL_MT_SAFE { തിരികെ m_direction.isReadOnly(); }
    bool isConstRef() സ്ഥിരം VL_MT_SAFE { തിരികെ m_direction.isConstRef(); }
    bool isRef() സ്ഥിരം VL_MT_SAFE { തിരികെ m_direction.isRef(); }
    bool isWritable() സ്ഥിരം VL_MT_SAFE { തിരികെ m_direction.isWritable(); }
    bool isTristate() സ്ഥിരം { തിരികെ m_tristate; }
    bool isPrimaryIO() സ്ഥിരം VL_MT_SAFE { തിരികെ m_primaryIO; }
    bool isPrimaryInish() സ്ഥിരം { തിരികെ isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() സ്ഥിരം VL_MT_SAFE { തിരികെ m_primaryClock; }
    ശൂന്യം setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() സ്ഥിരം { തിരികെ varType() == VVarType::IFACEREF; }
    ശൂന്യം setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() സ്ഥിരം { തിരികെ m_isIfaceParent; }
    bool isIfaceArraySplit() സ്ഥിരം { തിരികെ m_isIfaceArraySplit; }
    bool isInternal() സ്ഥിരം { തിരികെ m_isInternal; }
    bool isIfaceParam() സ്ഥിരം { തിരികെ m_isIfaceParam; }
    bool isSignal() സ്ഥിരം { തിരികെ varType().isSignal(); }
    bool isNet() സ്ഥിരം { തിരികെ varType().isNet(); }
    bool isWor() സ്ഥിരം { തിരികെ varType().isWor(); }
    bool isWiredNet() സ്ഥിരം { തിരികെ varType().isWiredNet(); }
    bool isTemp() സ്ഥിരം { തിരികെ varType().isTemp(); }
    bool isClassMember() സ്ഥിരം { തിരികെ varType() == VVarType::MEMBER; }
    bool isVirtIface() സ്ഥിരം {
        എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            തിരികെ dtp->isVirtual();
        }
        തിരികെ false;
    }
    bool isStatementTemp() സ്ഥിരം { തിരികെ varType() == VVarType::STMTTEMP; }
    bool isXTemp() സ്ഥിരം { തിരികെ varType() == VVarType::XTEMP; }
    bool isParam() സ്ഥിരം { തിരികെ varType().isParam(); }
    bool isGParam() സ്ഥിരം { തിരികെ varType() == VVarType::GPARAM; }
    bool isGenVar() സ്ഥിരം { തിരികെ varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() സ്ഥിരം {
        തിരികെ isIO() || isSignal() || isClassMember() || isTemp() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() സ്ഥിരം {
        തിരികെ !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() സ്ഥിരം {
        സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep = basicp();
        തിരികെ bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() സ്ഥിരം { തിരികെ m_usedParam; }
    bool isUsedLoopIdx() സ്ഥിരം { തിരികെ m_usedLoopIdx; }
    bool mtask_Cache_വരി_Align() സ്ഥിരം { തിരികെ m_mtask_Cache_വരി_Align; }
    ശൂന്യം mtask_Cache_വരി_Align(bool flag) { m_mtask_Cache_വരി_Align = flag; }
    bool isSc() സ്ഥിരം VL_MT_SAFE { തിരികെ m_sc; }
    bool isScQuad() സ്ഥിരം;
    bool isScBv() സ്ഥിരം VL_MT_STABLE;
    bool isScUint() സ്ഥിരം;
    bool isScUintBool() സ്ഥിരം;
    bool isScBigUint() സ്ഥിരം VL_MT_STABLE;
    bool isScSensitive() സ്ഥിരം { തിരികെ m_scSensitive; }
    bool isSigPublic() സ്ഥിരം;
    bool isSigModPublic() സ്ഥിരം { തിരികെ m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() സ്ഥിരം { തിരികെ m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() സ്ഥിരം { തിരികെ m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() സ്ഥിരം { തിരികെ m_trace; }
    bool isRand() സ്ഥിരം { തിരികെ m_rand.isRand(); }
    bool isRandC() സ്ഥിരം { തിരികെ m_rand.isRandC(); }
    bool isConst() സ്ഥിരം VL_MT_SAFE { തിരികെ m_isConst; }
    bool isLatched() സ്ഥിരം { തിരികെ m_isLatched; }
    bool isFuncLocal() സ്ഥിരം { തിരികെ m_funcLocal; }
    bool isFuncLocalSticky() സ്ഥിരം { തിരികെ m_funcLocalSticky; }
    bool isFuncReturn() സ്ഥിരം { തിരികെ m_funcReturn; }
    bool isPullup() സ്ഥിരം { തിരികെ m_isPullup; }
    bool isPulldown() സ്ഥിരം { തിരികെ m_isPulldown; }
    bool attrScBv() സ്ഥിരം { തിരികെ m_attrScBv; }
    bool attrScBigUint() സ്ഥിരം { തിരികെ m_attrScBigUint; }
    bool attrFileDescr() സ്ഥിരം { തിരികെ m_attrFileDescr; }
    bool attrSFormat() സ്ഥിരം { തിരികെ m_attrSFormat; }
    bool attrSplitVar() സ്ഥിരം { തിരികെ m_attrSplitVar; }
    bool attrFsmState() സ്ഥിരം { തിരികെ m_attrFsmState; }
    bool attrFsmArcInclCond() സ്ഥിരം { തിരികെ m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() സ്ഥിരം { തിരികെ m_attrFsmStateExpand; }
    AstIface* sensIfacep() സ്ഥിരം { തിരികെ m_sensIfacep; }
    VRandAttr rand() സ്ഥിരം { തിരികെ m_rand; }
    string verilogKwd() സ്ഥിരം override;
    ശൂന്യം lifetime(സ്ഥിരം VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() സ്ഥിരം { തിരികെ m_lifetime; }
    ശൂന്യം pin_അക്കം(പൂർണ്ണസംഖ്യ id) { m_pin_അക്കം = id; }
    പൂർണ്ണസംഖ്യ pin_അക്കം() സ്ഥിരം { തിരികെ m_pin_അക്കം; }
    ശൂന്യം propagateAttrFrom(സ്ഥിരം AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        എങ്കിൽ (fromp->attrFileDescr()) attrFileDescr(true);
        എങ്കിൽ (fromp->isContinuously()) isContinuously(true);
    }
    ശൂന്യം propagateSplitAttrFrom(സ്ഥിരം AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    സ്ഥിരം അക്ഷരം* cannotSplitKindReason() സ്ഥിരം;
    ശൂന്യം propagateWrapAttrFrom(സ്ഥിരം AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    ശൂന്യം combineType(സ്ഥിരം AstVar* otherp);
    bool needsCReset() സ്ഥിരം {
        തിരികെ !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !isStatementTemp()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    സ്ഥിതം AstVar* scVarRecurse(Ast_കണ്ണി* nodep);
};
class AstVarScope final : പൊതു Ast_കണ്ണി {
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
    bool m_fork_By_മൂല്യം : 1;  // Set before a fork, whose processes get it by value
പൊതു:
    AstVarScope(File_വരി* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_മൂല്യം = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    ശൂന്യം cloneRelink() override {
        എങ്കിൽ (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ scopep()->name() + "->" + varp()->name(); }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override;
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    AstVar* varp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_varp; }  // [After Link] Pointer to variable
    ശൂന്യം varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() സ്ഥിരം VL_MT_STABLE { തിരികെ m_scopep; }  // Pointer to scope it's under
    ശൂന്യം scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() സ്ഥിരം { തിരികെ m_trace; }
    ശൂന്യം trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() സ്ഥിരം { തിരികെ m_optimizeLifePost; }
    ശൂന്യം optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_മൂല്യം() സ്ഥിരം { തിരികെ m_fork_By_മൂല്യം; }
    ശൂന്യം fork_By_മൂല്യം(bool flag) { m_fork_By_മൂല്യം = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : പൊതു Ast_കണ്ണി_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    സ്ഥിരം uint64_t m_index;  // Index of the element, with the array's lo() at 0
പൊതു:
    AstConsArrayItem(File_വരി* fl, uint64_t index, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_index{index} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        തിരികെ nullptr;
    }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        തിരികെ m_index == VN_DBG_AS(samep, ConsArrayItem)->m_index;
    }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }  // By AstConsArray's map
    uint64_t index() സ്ഥിരം { തിരികെ m_index; }
};
class AstConsAssocItem final : പൊതു Ast_കണ്ണി_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
പൊതു:
    AstConsAssocItem(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* keyp, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        തിരികെ nullptr;
    }
};
class AstConsDynArrayItem final : പൊതു Ast_കണ്ണി_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
പൊതു:
    AstConsDynArrayItem(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_വരി* fl, AstConst* keyp, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        തിരികെ nullptr;
    }
};
class AstConsQueueItem final : പൊതു Ast_കണ്ണി_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
പൊതു:
    AstConsQueueItem(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        തിരികെ nullptr;
    }
};
class AstConsStructItem final : പൊതു Ast_കണ്ണി_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
പൊതു:
    AstConsStructItem(File_വരി* fl, AstMemberDType* memberp, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        തിരികെ nullptr;
    }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    AstMemberDType* memberp() സ്ഥിരം { തിരികെ m_memberp; }
};
class AstConsWildcardItem final : പൊതു Ast_കണ്ണി_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
പൊതു:
    AstConsWildcardItem(File_വരി* fl, AstConst* keyp, Ast_കണ്ണി_വ്യഞ്ജകം* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        തിരികെ nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : പൊതു Ast_കണ്ണി_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    പൂർണ്ണസംഖ്യ m_offset;  // Offset column numbers to uniq-ify IFs
പൊതു:
    AstCoverOtherDecl(File_വരി* fl, സ്ഥിരം string& page, സ്ഥിരം string& comment,
                      സ്ഥിരം string& linescov, പൂർണ്ണസംഖ്യ offset, സ്ഥിരം string& fsmVar = "",
                      സ്ഥിരം string& fsmFrom = "", സ്ഥിരം string& fsmTo = "",
                      സ്ഥിരം string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ offset() സ്ഥിരം { തിരികെ m_offset; }
    പൂർണ്ണസംഖ്യ size() സ്ഥിരം override { തിരികെ 1; }
    സ്ഥിരം string& linescov() സ്ഥിരം { തിരികെ m_linescov; }
    സ്ഥിരം string& fsmVar() സ്ഥിരം { തിരികെ m_fsmVar; }
    സ്ഥിരം string& fsmFrom() സ്ഥിരം { തിരികെ m_fsmFrom; }
    സ്ഥിരം string& fsmTo() സ്ഥിരം { തിരികെ m_fsmTo; }
    സ്ഥിരം string& fsmTag() സ്ഥിരം { തിരികെ m_fsmTag; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstCoverOtherDecl* സ്ഥിരം asamep = VN_DBG_AS(samep, CoverOtherDecl);
        തിരികെ Super::same_കണ്ണി(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : പൊതു Ast_കണ്ണി_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    സ്ഥിരം VNumRange m_range;  // Packed array range covering each toggle bit
പൊതു:
    AstCoverToggleDecl(File_വരി* fl, സ്ഥിരം string& page, സ്ഥിരം string& comment,
                       സ്ഥിരം VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    പൂർണ്ണസംഖ്യ size() സ്ഥിരം override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        തിരികെ 2 * m_range.elements();
    }
    സ്ഥിരം VNumRange& range() സ്ഥിരം { തിരികെ m_range; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstCoverToggleDecl* സ്ഥിരം asamep = VN_DBG_AS(samep, CoverToggleDecl);
        തിരികെ Super::same_കണ്ണി(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : പൊതു Ast_കണ്ണി_FTask {
    // A function inside a module
പൊതു:
    AstFunc(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* stmtp, Ast_കണ്ണി* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    Ast_കണ്ണി_FTask* cloneType(സ്ഥിരം string& name) override {
        തിരികെ new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "function"; }
};
class AstLet final : പൊതു Ast_കണ്ണി_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
പൊതു:
    AstLet(File_വരി* fl, സ്ഥിരം string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_വ്യഞ്ജകം));
        തിരികെ nullptr;
    }
    Ast_കണ്ണി_FTask* cloneType(സ്ഥിരം string& name) override { തിരികെ new AstLet{fileline(), name}; }
    string verilogKwd() സ്ഥിരം override { തിരികെ "let"; }
};
class AstProperty final : പൊതു Ast_കണ്ണി_FTask {
    // A property inside a module
പൊതു:
    AstProperty(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    Ast_കണ്ണി_FTask* cloneType(സ്ഥിരം string& name) override {
        തിരികെ new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "property"; }
};
class AstSequence final : പൊതു Ast_കണ്ണി_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
പൊതു:
    AstSequence(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool hasDType() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    Ast_കണ്ണി_FTask* cloneType(സ്ഥിരം string& name) override {
        തിരികെ new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "sequence"; }
    bool isReferenced() സ്ഥിരം { തിരികെ m_referenced; }
    ശൂന്യം isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : പൊതു Ast_കണ്ണി_FTask {
    // A task inside a module
പൊതു:
    AstTask(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_കണ്ണി_FTask* cloneType(സ്ഥിരം string& name) override {
        തിരികെ new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() സ്ഥിരം override { തിരികെ "task"; }
};

// === AstNodeFile ===
class AstCFile final : പൊതു Ast_കണ്ണി_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
പൊതു:
    AstCFile(File_വരി* fl, സ്ഥിരം string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
    uint64_t complexityScore() സ്ഥിരം { തിരികെ m_complexityScore; }
    ശൂന്യം complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() സ്ഥിരം { തിരികെ m_slow; }
    ശൂന്യം slow(bool flag) { m_slow = flag; }
    bool source() സ്ഥിരം { തിരികെ m_source; }
    ശൂന്യം source(bool flag) { m_source = flag; }
    bool support() സ്ഥിരം { തിരികെ m_support; }
    ശൂന്യം support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : പൊതു Ast_കണ്ണി_File {
    // Verilog output file
    // Parents:  NETLIST
പൊതു:
    AstVFile(File_വരി* fl, സ്ഥിരം string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str = std::cout) സ്ഥിരം override;
};
class AstCoverCross final : പൊതു Ast_കണ്ണി_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
പൊതു:
    AstCoverCross(File_വരി* fl, സ്ഥിരം string& name, AstCoverpointRef* itemsp,
                  Ast_കണ്ണി_വ്യഞ്ജകം* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstCoverpoint final : പൊതു Ast_കണ്ണി_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
പൊതു:
    AstCoverpoint(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി_വ്യഞ്ജകം* exprp,
                  Ast_കണ്ണി_വ്യഞ്ജകം* iffp = nullptr, Ast_കണ്ണി* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};

// === AstNodeGen ===
class AstGenBlock final : പൊതു Ast_കണ്ണി_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    സ്ഥിരം bool m_unnamed;  // Originally unnamed (name change does not affect this)
    സ്ഥിരം bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

പൊതു:
    AstGenBlock(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
    ശൂന്യം name(സ്ഥിരം std::string& name) override { m_name = name; }
    bool unnamed() സ്ഥിരം { തിരികെ m_unnamed; }
    bool implied() സ്ഥിരം { തിരികെ m_implied; }
    AstDefaultDisable* defaultDisablep() സ്ഥിരം { തിരികെ m_defaultDisablep; }
    ശൂന്യം defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(സ്ഥിരം string& outer) സ്ഥിരം;
};
class AstGenCase final : പൊതു Ast_കണ്ണി_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
പൊതു:
    AstGenCase(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : പൊതു Ast_കണ്ണി_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
പൊതു:
    AstGenFor(File_വരി* fl, Ast_കണ്ണി* initsp, Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി* incsp, Ast_കണ്ണി* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : പൊതു Ast_കണ്ണി_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
പൊതു:
    AstGenIf(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി* thensp, Ast_കണ്ണി* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : പൊതു Ast_കണ്ണി_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_അക്കം;  // Declaration token number
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
    പൂർണ്ണസംഖ്യ m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool full, സ്ഥിരം string& scope) സ്ഥിരം;
    string dtypeNameScope() സ്ഥിരം;  // Prefix of dtypeName(true) for the scope declaring the class

പൊതു:
    AstClass(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_അക്കം{fl->token_അക്കം()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() സ്ഥിരം override { തിരികെ isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ true; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    bool timescaleMatters() സ്ഥിരം override { തിരികെ false; }
    AstClassPackage* classOrPackagep() സ്ഥിരം VL_MT_STABLE { തിരികെ m_classOrPackagep; }
    ശൂന്യം classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() സ്ഥിരം VL_MT_STABLE {
        തിരികെ m_covergroupEnclosingClassp;
    }
    ശൂന്യം covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_കണ്ണി* membersp() സ്ഥിരം VL_MT_STABLE { തിരികെ stmtsp(); }
    ശൂന്യം addMembersp(Ast_കണ്ണി* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() സ്ഥിരം { തിരികെ m_hasRandVarsUpdate; }
    ശൂന്യം hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() സ്ഥിരം { തിരികെ m_covergroup; }
    ശൂന്യം isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() സ്ഥിരം { തിരികെ m_extended; }
    ശൂന്യം isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() സ്ഥിരം { തിരികെ m_interfaceClass; }
    ശൂന്യം isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() സ്ഥിരം { തിരികെ m_virtual; }
    ശൂന്യം isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() സ്ഥിരം { തിരികെ m_needRNG; }
    ശൂന്യം needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() സ്ഥിരം { തിരികെ m_useVirtualPublic; }
    ശൂന്യം useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    ശൂന്യം markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() സ്ഥിരം { തിരികെ m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) സ്ഥിരം;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(സ്ഥിരം string& scope) സ്ഥിരം;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ശൂന്യം dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() സ്ഥിരം { തിരികെ !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_സന്ദേശം() സ്ഥിരം override { തിരികെ hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    സ്ഥിതം string embeddedCovergroupTypeName(സ്ഥിരം string& name) { തിരികെ "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() സ്ഥിരം;
    // Covergroup options accessors
    പൂർണ്ണസംഖ്യ cgAutoBinMax() സ്ഥിരം { തിരികെ m_cgAutoBinMax; }
    ശൂന്യം cgAutoBinMax(പൂർണ്ണസംഖ്യ മൂല്യം) { m_cgAutoBinMax = മൂല്യം; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    സ്ഥിതം bool isClassExtendedFrom(സ്ഥിരം AstClass* refClassp, സ്ഥിരം AstClass* baseClassp);
    uint32_t decl_Token_അക്കം() സ്ഥിരം override { തിരികെ m_decl_Token_അക്കം; }
    ശൂന്യം decl_Token_അക്കം_Set_Min(uint32_t token_അക്കം) override {
        m_decl_Token_അക്കം = std::min(m_decl_Token_അക്കം, token_അക്കം);
    }
    ശൂന്യം baseOverride(സ്ഥിരം VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() സ്ഥിരം { തിരികെ m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    സ്ഥിതം bool isCacheableChild(സ്ഥിരം Ast_കണ്ണി* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    template <typename T_Callable>
    ശൂന്യം foreachMember(സ്ഥിരം T_Callable& f) {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_കണ്ണി*>::മൂല്യം
                && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        എങ്കിൽ (സ്ഥിരം AstClassExtends* സ്ഥിരം cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        വേണ്ടി (Ast_കണ്ണി* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (Ast_കണ്ണി::privateTypeTest<T_കണ്ണി>(stmtp)) f(this, static_cast<T_കണ്ണി*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    template <typename T_Callable>
    bool existsMember(സ്ഥിരം T_Callable& p) സ്ഥിരം {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, സ്ഥിരം AstClass*, സ്ഥിരം T_കണ്ണി*>::മൂല്യം
                && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        എങ്കിൽ (സ്ഥിരം AstClassExtends* സ്ഥിരം cextendsp = this->extendsp()) {
            എങ്കിൽ (cextendsp->classp()->existsMember(p)) തിരികെ true;
        }
        വേണ്ടി (Ast_കണ്ണി* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (Ast_കണ്ണി::privateTypeTest<T_കണ്ണി>(stmtp)) {
                എങ്കിൽ (p(this, static_cast<T_കണ്ണി*>(stmtp))) തിരികെ true;
            }
        }
        തിരികെ false;
    }
};
class AstClassPackage final : പൊതു Ast_കണ്ണി_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
പൊതു:
    AstClassPackage(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() സ്ഥിരം override { തിരികെ "classpackage"; }
    bool timescaleMatters() സ്ഥിരം override { തിരികെ false; }
    AstClass* classp() സ്ഥിരം VL_MT_SAFE { തിരികെ m_classp; }
    ശൂന്യം classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : പൊതു Ast_കണ്ണി_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
പൊതു:
    AstIface(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    string verilogKwd() സ്ഥിരം override { തിരികെ "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() സ്ഥിരം override { തിരികെ false; }
    bool hasVirtualRef() സ്ഥിരം { തിരികെ m_hasVirtualRef; }
    ശൂന്യം setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) സ്ഥിരം;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ശൂന്യം dtypeNameFreeze();
};
class AstModule final : പൊതു Ast_കണ്ണി_Module {
    // A module declaration
    സ്ഥിരം bool m_isChecker;  // Module represents a checker
    സ്ഥിരം bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

പൊതു:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() സ്ഥിരം override {
        തിരികെ m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() സ്ഥിരം override { തിരികെ true; }
    bool isChecker() സ്ഥിരം { തിരികെ m_isChecker; }
    bool isProgram() സ്ഥിരം { തിരികെ m_isProgram; }
    bool hasGenericIface() സ്ഥിരം { തിരികെ m_hasGenericIface; }
    ശൂന്യം hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
};
class AstNotFoundModule final : പൊതു Ast_കണ്ണി_Module {
    // A missing module declaration
പൊതു:
    AstNotFoundModule(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() സ്ഥിരം override { തിരികെ "/*not-found-*/ module"; }
    bool timescaleMatters() സ്ഥിരം override { തിരികെ false; }
};
class AstPackage final : പൊതു Ast_കണ്ണി_Module {
    // A package declaration
പൊതു:
    AstPackage(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() സ്ഥിരം override { തിരികെ "package"; }
    bool timescaleMatters() സ്ഥിരം override { തിരികെ !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : പൊതു Ast_കണ്ണി_Module {
    // A primitive declaration
പൊതു:
    AstPrimitive(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() സ്ഥിരം override { തിരികെ "primitive"; }
    bool timescaleMatters() സ്ഥിരം override { തിരികെ false; }
};

// === AstNodeProcedure ===
class AstAlways final : പൊതു Ast_കണ്ണി_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    സ്ഥിരം VAlwaysKwd m_keyword;

പൊതു:
    AstAlways(File_വരി* fl, VAlwaysKwd keyword, Ast_Sen_വൃക്ഷം* sentreep, Ast_കണ്ണി* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit വരിയിൽ AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    VAlwaysKwd keyword() സ്ഥിരം { തിരികെ m_keyword; }
};
class AstAlwaysObserved final : പൊതു Ast_കണ്ണി_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

പൊതു:
    AstAlwaysObserved(File_വരി* fl, Ast_Sen_വൃക്ഷം* sentreep, Ast_കണ്ണി* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : പൊതു Ast_കണ്ണി_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

പൊതു:
    explicit AstAlwaysPost(File_വരി* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : പൊതു Ast_കണ്ണി_Procedure {
    // Like always but Postponed scheduling region

പൊതു:
    AstAlwaysPostponed(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : പൊതു Ast_കണ്ണി_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

പൊതു:
    explicit AstAlwaysPre(File_വരി* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : പൊതു Ast_കണ്ണി_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

പൊതു:
    AstAlwaysReactive(File_വരി* fl, Ast_Sen_വൃക്ഷം* sentreep, Ast_കണ്ണി* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : പൊതു Ast_കണ്ണി_Procedure {
പൊതു:
    AstFinal(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : പൊതു Ast_കണ്ണി_Procedure {
പൊതു:
    AstInitial(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : പൊതു Ast_കണ്ണി_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
പൊതു:
    AstInitialAutomatic(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : പൊതു Ast_കണ്ണി_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
പൊതു:
    AstInitialStatic(File_വരി* fl, Ast_കണ്ണി* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : പൊതു Ast_കണ്ണി_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
പൊതു:
    AstBracketRange(File_വരി* fl, Ast_കണ്ണി* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    മായാ string emitC() { V3ERROR_NA_RETURN(""); }
    മായാ string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() സ്ഥിരം override VL_MT_SAFE { തിരികെ false; }
};
class AstRange final : പൊതു Ast_കണ്ണി_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    സ്ഥിരം bool m_fromBracket = false;  // From C-style '[x]' declaration
പൊതു:
    AstRange(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* leftp, Ast_കണ്ണി_വ്യഞ്ജകം* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    വരിയിൽ AstRange(File_വരി* fl, പൂർണ്ണസംഖ്യ ഇടത്, പൂർണ്ണസംഖ്യ വലത്);
    വരിയിൽ AstRange(File_വരി* fl, സ്ഥിരം VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    വരിയിൽ പൂർണ്ണസംഖ്യ ഇടത്_Const() സ്ഥിരം VL_MT_STABLE;
    വരിയിൽ പൂർണ്ണസംഖ്യ വലത്_Const() സ്ഥിരം VL_MT_STABLE;
    പൂർണ്ണസംഖ്യ hiConst() സ്ഥിരം VL_MT_STABLE {
        സ്ഥിരം പൂർണ്ണസംഖ്യ l = ഇടത്_Const();
        സ്ഥിരം പൂർണ്ണസംഖ്യ r = വലത്_Const();
        തിരികെ l > r ? l : r;
    }
    പൂർണ്ണസംഖ്യ loConst() സ്ഥിരം VL_MT_STABLE {
        സ്ഥിരം പൂർണ്ണസംഖ്യ l = ഇടത്_Const();
        സ്ഥിരം പൂർണ്ണസംഖ്യ r = വലത്_Const();
        തിരികെ l > r ? r : l;
    }
    പൂർണ്ണസംഖ്യ elementsConst() സ്ഥിരം VL_MT_STABLE { തിരികെ hiConst() - loConst() + 1; }
    bool ascending() സ്ഥിരം { തിരികെ ഇടത്_Const() < വലത്_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_ഇടത്_To_വലത്() സ്ഥിരം {
        തിരികെ VNumRange{ഇടത്_Const(), വലത്_Const()}.seq_ഇടത്_To_വലത്();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_വലത്_To_ഇടത്() സ്ഥിരം {
        തിരികെ VNumRange{ഇടത്_Const(), വലത്_Const()}.seq_വലത്_To_ഇടത്();
    }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം override;
    ശൂന്യം dumpJson(std::ostream& str) സ്ഥിരം override;
    മായാ string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം override {
        സ്ഥിരം AstRange* സ്ഥിരം asamep = VN_DBG_AS(samep, Range);
        തിരികെ fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() സ്ഥിരം { തിരികെ m_fromBracket; }
};
class AstUnsizedRange final : പൊതു Ast_കണ്ണി_Range {
    // Unsized range specification, for open arrays
പൊതു:
    explicit AstUnsizedRange(File_വരി* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    മായാ string emitC() { V3ERROR_NA_RETURN(""); }
    മായാ string emitVerilog() { തിരികെ "[]"; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};
class AstWildcardRange final : പൊതു Ast_കണ്ണി_Range {
    // Wildcard range specification, for wildcard index type associative arrays
പൊതു:
    explicit AstWildcardRange(File_വരി* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    മായാ string emitC() { V3ERROR_NA_RETURN(""); }
    മായാ string emitVerilog() { തിരികെ "[*]"; }
    bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി* /*samep*/) സ്ഥിരം override { തിരികെ true; }
};

#endif  // Guard
