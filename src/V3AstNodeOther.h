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
#অন্তর্ভাব "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

#অন্তর্ভাব <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_গ্রন্থি_Cons_Item VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    friend class Ast_গ্রন্থি_রাশিমালা;  // For m_knownConstantTree
    mutable bool m_known_Constant_বৃক্ষ = false;  // Known to be a tree of constants

সুরক্ষিত:
    Ast_গ্রন্থি_Cons_Item(VNType t, File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* valuep)
        : Ast_গ্রন্থি{t, fl} {
        this->valuep(valuep);
    }

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Cons_Item;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class Ast_গ্রন্থি_Cover_Decl VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    পূর্ণসংখ্যা m_bin_অঙ্ক = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    পূর্ণসংখ্যা m_local_Bin_অঙ্ক = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
সর্বজনীন:
    Ast_গ্রন্থি_Cover_Decl(VNType t, File_সারি* fl, ধ্রুবক string& page, ধ্রুবক string& comment)
        : Ast_গ্রন্থি(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Cover_Decl;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        যদি (m_তথ্য_Declp
            && (m_তথ্য_Declp == this || m_তথ্য_Declp->m_তথ্য_Declp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        ফেরত nullptr;
    }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    পূর্ণসংখ্যা instr_গুনতি() ধ্রুবক override { ফেরত 1 + 2 * INSTR_গুনতি_LD; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    পূর্ণসংখ্যা bin_অঙ্ক() ধ্রুবক { ফেরত m_bin_অঙ্ক; }
    শূন্য bin_অঙ্ক(পূর্ণসংখ্যা flag) { m_bin_অঙ্ক = flag; }
    পূর্ণসংখ্যা local_Bin_অঙ্ক() ধ্রুবক { ফেরত m_local_Bin_অঙ্ক; }
    শূন্য local_Bin_অঙ্ক(পূর্ণসংখ্যা flag) { m_local_Bin_অঙ্ক = flag; }
    bool perInstance() ধ্রুবক { ফেরত m_perInstance; }
    শূন্য perInstance(bool flag) { m_perInstance = flag; }
    আভাসী পূর্ণসংখ্যা আকার_2() ধ্রুবক = 0;
    ধ্রুবক string& comment() ধ্রুবক { ফেরত m_text; }  // text to insert in code
    ধ্রুবক string& page() ধ্রুবক { ফেরত m_page; }
    ধ্রুবক string& hier() ধ্রুবক { ফেরত m_hier; }
    শূন্য hier(ধ্রুবক string& flag) { m_hier = flag; }
    শূন্য comment(ধ্রুবক string& flag) { m_text = flag; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক Ast_গ্রন্থি_Cover_Decl* ধ্রুবক asamep = VN_DBG_AS(samep, গ্রন্থি_Cover_Decl);
        ফেরত (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() ধ্রুবক override { ফেরত false; }
    শূন্য তথ্য_Declp(Ast_গ্রন্থি_Cover_Decl* nodep) { m_তথ্য_Declp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_গ্রন্থি_Cover_Decl* তথ্য_Decl_Nullp() ধ্রুবক { ফেরত m_তথ্য_Declp; }
    Ast_গ্রন্থি_Cover_Decl* তথ্য_Decl_Thisp() { ফেরত তথ্য_Decl_Nullp() ? তথ্য_Decl_Nullp() : this; }
};
class Ast_গ্রন্থি_FTask VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
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

সুরক্ষিত:
    Ast_গ্রন্থি_FTask(VNType t, File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* stmtsp)
        : Ast_গ্রন্থি{t, fl}
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

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_FTask;
    আভাসী Ast_গ্রন্থি_FTask* cloneType(ধ্রুবক string& name) = 0;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Var name
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool isGateOptimizable() ধ্রুবক override {
        ফেরত !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    string cname() ধ্রুবক { ফেরত m_cname; }
    শূন্য cname(ধ্রুবক string& cname) { m_cname = cname; }
    bool isFunction() ধ্রুবক { ফেরত fvarp() != nullptr; }
    // MORE ACCESSORS
    শূন্য dpiOpenParentInc() { ++m_dpiOpenParent; }
    শূন্য dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() ধ্রুবক { ফেরত m_dpiOpenParent; }
    bool taskPublic() ধ্রুবক { ফেরত m_taskPublic; }
    শূন্য taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() ধ্রুবক { ফেরত m_classMethod; }
    শূন্য classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() ধ্রুবক { ফেরত m_didProto; }
    শূন্য didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ধ্রুবক { ফেরত m_isExternProto; }
    শূন্য isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ধ্রুবক { ফেরত m_isExternDef; }
    শূন্য isExternDef(bool flag) { m_isExternDef = flag; }
    ধ্রুবক string& ifacePortName() ধ্রুবক { ফেরত m_ifacePortName; }
    শূন্য ifacePortName(ধ্রুবক string& name) { m_ifacePortName = name; }
    bool prototype() ধ্রুবক { ফেরত m_prototype; }
    শূন্য prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ধ্রুবক { ফেরত m_dpiExport; }
    শূন্য dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ধ্রুবক { ফেরত m_dpiImport; }
    শূন্য dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ধ্রুবক { ফেরত m_dpiContext; }
    শূন্য dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() ধ্রুবক { ফেরত m_dpiOpenChild; }
    শূন্য dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() ধ্রুবক { ফেরত m_dpiTask; }
    শূন্য dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ধ্রুবক { ফেরত !m_dpiCDecl.empty(); }
    ধ্রুবক string& dpiCDecl() ধ্রুবক { ফেরত m_dpiCDecl; }
    শূন্য dpiCDecl(ধ্রুবক string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() ধ্রুবক { ফেরত m_isConstructor; }
    শূন্য isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() ধ্রুবক { ফেরত m_isHideLocal; }
    শূন্য isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ধ্রুবক { ফেরত m_isHideProtected; }
    শূন্য isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() ধ্রুবক { ফেরত m_dpiPure; }
    শূন্য dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() ধ্রুবক { ফেরত m_keepAlive; }
    শূন্য keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() ধ্রুবক { ফেরত m_needsSyms; }
    শূন্য needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() ধ্রুবক { ফেরত m_pureVirtual; }
    শূন্য pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() ধ্রুবক { ফেরত m_recursive; }
    শূন্য recursive(bool flag) { m_recursive = flag; }
    bool isStatic() ধ্রুবক { ফেরত m_static; }
    শূন্য isStatic(bool flag) { m_static = flag; }
    bool underGenerate() ধ্রুবক { ফেরত m_underGenerate; }
    শূন্য underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() ধ্রুবক { ফেরত m_verilogFunction; }
    শূন্য verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() ধ্রুবক { ফেরত m_verilogTask; }
    শূন্য verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() ধ্রুবক { ফেরত m_virtual; }
    শূন্য isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() ধ্রুবক { ফেরত m_needProcess; }
    শূন্য setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() ধ্রুবক { ফেরত m_isCovergroupSample; }
    শূন্য isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    শূন্য baseOverride(ধ্রুবক VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ধ্রুবক { ফেরত m_baseOverride; }
    শূন্য lifetime(ধ্রুবক VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ধ্রুবক { ফেরত m_lifetime; }
    bool isPure() override;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override;
    শূন্য propagateAttrFrom(ধ্রুবক Ast_গ্রন্থি_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        classMethod(fromp->classMethod());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

ব্যক্তিগত:
    bool getPurityRecurse() ধ্রুবক;
};
class Ast_গ্রন্থি_File VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
সর্বজনীন:
    Ast_গ্রন্থি_File(VNType t, File_সারি* fl, ধ্রুবক string& name)
        : Ast_গ্রন্থি{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_গ্রন্থি_File;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* /*samep*/) ধ্রুবক override { ফেরত true; }
};
class Ast_গ্রন্থি_Func_Cov_Item VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // Base class for functional coverage items (coverpoints, crosses)
সুরক্ষিত:
    string m_name;  // Item name

সর্বজনীন:
    Ast_গ্রন্থি_Func_Cov_Item(VNType t, File_সারি* fl, ধ্রুবক string& name)
        : Ast_গ্রন্থি{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Func_Cov_Item;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    bool maybePointedTo() ধ্রুবক override { ফেরত true; }
};
class Ast_গ্রন্থি_Gen VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // Generate construct
সর্বজনীন:
    Ast_গ্রন্থি_Gen(VNType t, File_সারি* fl)
        : Ast_গ্রন্থি{t, fl} {}
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Gen;
};
class Ast_গ্রন্থি_Module VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    ধ্রুবক string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    পূর্ণসংখ্যা m_depth = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    পূর্ণসংখ্যা m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
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
সুরক্ষিত:
    Ast_গ্রন্থি_Module(VNType t, File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname)
        : Ast_গ্রন্থি{t, fl}
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

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Module;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    আভাসী bool timescaleMatters() ধ্রুবক = 0;
    অন্তঃরেখ bool isDollarUnit() ধ্রুবক;  // Is the $unit package
    অন্তঃরেখ bool isConstPool() ধ্রুবক;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(ধ্রুবক string& outer) ধ্রুবক;
    // ACCESSORS
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    string origName() ধ্রুবক override { ফেরত m_origName; }
    string someInstanceName() ধ্রুবক VL_MT_SAFE { ফেরত m_someInstanceName; }
    শূন্য someInstanceName(ধ্রুবক string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ধ্রুবক { ফেরত m_defaultDisablep; }
    শূন্য defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() ধ্রুবক { ফেরত m_inLibrary; }
    শূন্য inLibrary(bool flag) { m_inLibrary = flag; }
    শূন্য depth(পূর্ণসংখ্যা মান) { m_depth = মান; }
    পূর্ণসংখ্যা depth() ধ্রুবক VL_MT_SAFE { ফেরত m_depth; }
    শূন্য level(পূর্ণসংখ্যা মান) { m_level = মান; }
    পূর্ণসংখ্যা level() ধ্রুবক VL_MT_SAFE { ফেরত m_level; }
    string libname() ধ্রুবক { ফেরত m_libname; }
    bool is_শীর্ষ() ধ্রুবক VL_MT_SAFE { ফেরত level() == 1; }
    bool modPublic() ধ্রুবক { ফেরত m_modPublic; }
    শূন্য modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() ধ্রুবক { ফেরত m_modTrace; }
    শূন্য modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() ধ্রুবক { ফেরত m_ctorVarReset; }
    শূন্য ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() ধ্রুবক { ফেরত m_dead; }
    শূন্য dead(bool flag) { m_dead = flag; }
    bool hasGParam() ধ্রুবক { ফেরত m_hasGParam; }
    শূন্য hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() ধ্রুবক { ফেরত m_hasParameterList; }
    শূন্য hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() ধ্রুবক { ফেরত m_hierBlock; }
    শূন্য hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() ধ্রুবক { ফেরত m_hierParams; }
    শূন্য hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() ধ্রুবক { ফেরত m_recursive; }
    শূন্য recursive(bool flag) { m_recursive = flag; }
    শূন্য recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() ধ্রুবক { ফেরত m_recursiveClone; }
    bool parameterizedTemplate() ধ্রুবক { ফেরত m_parameterizedTemplate; }
    শূন্য parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    শূন্য verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() ধ্রুবক { ফেরত m_verilatorLib; }
    VLifetime lifetime() ধ্রুবক { ফেরত m_lifetime; }
    শূন্য lifetime(ধ্রুবক VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() ধ্রুবক { ফেরত m_timeunit; }
    শূন্য timeunit(ধ্রুবক VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() ধ্রুবক { ফেরত m_unconnectedDrive; }
    শূন্য unconnectedDrive(ধ্রুবক VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_গ্রন্থি_Procedure VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
সুরক্ষিত:
    Ast_গ্রন্থি_Procedure(VNType t, File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : Ast_গ্রন্থি{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Procedure;
    // METHODS
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool isJustOneBodyStmt() ধ্রুবক { ফেরত stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ধ্রুবক { ফেরত m_suspendable; }
    শূন্য setSuspendable() { m_suspendable = true; }
    bool needProcess() ধ্রুবক { ফেরত m_needProcess; }
    শূন্য setNeedProcess() { m_needProcess = true; }
};
class Ast_গ্রন্থি_Range VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // A range, sized or unsized
সুরক্ষিত:
    Ast_গ্রন্থি_Range(VNType t, File_সারি* fl)
        : Ast_গ্রন্থি{t, fl} {}

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Range;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : সর্বজনীন Ast_গ্রন্থি {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

সর্বজনীন:
    AstActive(File_সারি* fl, ধ্রুবক string& name, Ast_Sen_বৃক্ষ* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    // Statements are broken into pieces, as some must come before others.
    শূন্য sentreep(Ast_Sen_বৃক্ষ* nodep) { m_sentreep = nodep; }
    Ast_Sen_বৃক্ষ* sentreep() ধ্রুবক { ফেরত m_sentreep; }
    // METHODS
    অন্তঃরেখ bool hasClocked() ধ্রুবক;
    অন্তঃরেখ bool hasCombo() ধ্রুবক;
    অন্তঃরেখ bool hasInitial() ধ্রুবক;
    অন্তঃরেখ bool hasStatic() ধ্রুবক;
};
class AstAlias final : সর্বজনীন Ast_গ্রন্থি {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
সর্বজনীন:
    AstAlias(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : সর্বজনীন Ast_গ্রন্থি {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
সর্বজনীন:
    AstAliasScope(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : সর্বজনীন Ast_গ্রন্থি {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
সর্বজনীন:
    AstBind(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Bind Target name
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
};
class AstCFunc final : সর্বজনীন Ast_গ্রন্থি {
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
    পূর্ণসংখ্যা m_cost;  // Function call cost
সর্বজনীন:
    AstCFunc(File_সারি* fl, ধ্রুবক string& name, AstScope* scopep, ধ্রুবক string& rtnType = "")
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
        m_cost = v3Global.opt.instr_গুনতি_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstCFunc* ধ্রুবক asamep = VN_DBG_AS(samep, CFunc);
        ফেরত ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    পূর্ণসংখ্যা instr_গুনতি() ধ্রুবক override { ফেরত m_cost; }
    VBoolOrUnknown isConst() ধ্রুবক { ফেরত m_isConst; }
    শূন্য isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    শূন্য isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ধ্রুবক { ফেরত m_isStatic; }
    শূন্য isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ধ্রুবক VL_MT_SAFE { ফেরত m_isTrace; }
    শূন্য isTrace(bool flag) { m_isTrace = flag; }
    শূন্য cname(ধ্রুবক string& name) { m_cname = name; }
    string cname() ধ্রুবক { ফেরত m_cname; }
    AstScope* scopep() ধ্রুবক { ফেরত m_scopep; }
    শূন্য scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ধ্রুবক { ফেরত ((m_rtnType == "") ? "void" : m_rtnType); }
    শূন্য rtnType(ধ্রুবক string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ধ্রুবক { ফেরত m_dontCombine || isTrace() || entryPoint(); }
    শূন্য dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() ধ্রুবক { ফেরত m_declPrivate; }
    শূন্য declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() ধ্রুবক VL_MT_SAFE { ফেরত m_keepIfEmpty; }
    শূন্য keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() ধ্রুবক VL_MT_SAFE { ফেরত m_slow; }
    শূন্য slow(bool flag) { m_slow = flag; }
    bool funcPublic() ধ্রুবক { ফেরত m_funcPublic; }
    শূন্য funcPublic(bool flag) { m_funcPublic = flag; }
    শূন্য argTypes(ধ্রুবক string& str) { m_argTypes = str; }
    string argTypes() ধ্রুবক { ফেরত m_argTypes; }
    শূন্য ifdef(ধ্রুবক string& str) { m_ifdef = str; }
    string ifdef() ধ্রুবক { ফেরত m_ifdef; }
    bool isConstructor() ধ্রুবক { ফেরত m_isConstructor; }
    শূন্য isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ধ্রুবক { ফেরত m_isDestructor; }
    শূন্য isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() ধ্রুবক { ফেরত m_isMethod; }
    শূন্য isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() ধ্রুবক { ফেরত m_isLoose; }
    শূন্য isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() ধ্রুবক { ফেরত isMethod() && !isLoose(); }
    bool isVirtual() ধ্রুবক { ফেরত m_isVirtual; }
    শূন্য isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ধ্রুবক { ফেরত m_needProcess; }
    শূন্য setNeedProcess() { m_needProcess = true; }
    bool entryPoint() ধ্রুবক { ফেরত m_entryPoint; }
    শূন্য entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() ধ্রুবক { ফেরত m_dpiPure; }
    শূন্য dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() ধ্রুবক { ফেরত m_dpiContext; }
    শূন্য dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ধ্রুবক VL_MT_SAFE { ফেরত m_dpiExportDispatcher; }
    শূন্য dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ধ্রুবক { ফেরত m_dpiExportImpl; }
    শূন্য dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ধ্রুবক VL_MT_SAFE { ফেরত m_dpiImportPrototype; }
    শূন্য dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ধ্রুবক { ফেরত m_dpiImportWrapper; }
    শূন্য dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ধ্রুবক { ফেরত !m_dpiCDecl.empty(); }
    ধ্রুবক string& dpiCDecl() ধ্রুবক { ফেরত m_dpiCDecl; }
    শূন্য dpiCDecl(ধ্রুবক string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() ধ্রুবক { ফেরত m_rtnType == "VlCoroutine"; }
    শূন্য recursive(bool flag) { m_recursive = flag; }
    bool recursive() ধ্রুবক { ফেরত m_recursive; }
    শূন্য unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() ধ্রুবক override { ফেরত m_unlikely; }  // Note virtual override
    শূন্য noLife(bool flag) { m_noLife = flag; }
    bool noLife() ধ্রুবক { ফেরত m_noLife; }
    bool isCovergroupSample() ধ্রুবক { ফেরত m_isCovergroupSample; }
    শূন্য isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() ধ্রুবক { ফেরত m_voidSelf; }
    শূন্য voidSelf(bool flag) { m_voidSelf = flag; }
    শূন্য cost(পূর্ণসংখ্যা cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() ধ্রুবক {
        ফেরত !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : সর্বজনীন Ast_গ্রন্থি {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ধ্রুবক string m_name;
    ধ্রুবক VUseType m_useType;  // What sort of use this is

সর্বজনীন:
    AstCUse(File_সারি* fl, VUseType useType, ধ্রুবক string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    VUseType useType() ধ্রুবক { ফেরত m_useType; }
};
class AstCell final : সর্বজনীন Ast_গ্রন্থি {
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
    File_সারি* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    পূর্ণসংখ্যা m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
সর্বজনীন:
    AstCell(File_সারি* fl, File_সারি* mfl, ধ্রুবক string& instName, ধ্রুবক string& modName,
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
    শূন্য cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    // ACCESSORS
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Cell name
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    string origName() ধ্রুবক override { ফেরত m_origName; }  // * = Original name
    শূন্য origName(ধ্রুবক string& name) { m_origName = name; }
    string modName() ধ্রুবক { ফেরত m_modName; }  // * = Instance name
    শূন্য modName(ধ্রুবক string& name) { m_modName = name; }
    File_সারি* modNameFileline() ধ্রুবক { ফেরত m_modNameFileline; }
    Ast_গ্রন্থি_Module* modp() ধ্রুবক { ফেরত m_modp; }  // [AfterLink] = Pointer to module instantiated
    শূন্য modp(Ast_গ্রন্থি_Module* nodep) { m_modp = nodep; }
    পূর্ণসংখ্যা arrayIdx() ধ্রুবক { ফেরত m_arrayIdx; }
    শূন্য arrayIdx(পূর্ণসংখ্যা idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ধ্রুবক { ফেরত m_hasIfaceVar; }
    শূন্য hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    শূন্য trace(bool flag) { m_trace = flag; }
    bool isTrace() ধ্রুবক { ফেরত m_trace; }
    শূন্য recursive(bool flag) { m_recursive = flag; }
    bool recursive() ধ্রুবক { ফেরত m_recursive; }
};
class AstCellInline final : সর্বজনীন Ast_গ্রন্থি {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ধ্রুবক std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ধ্রুবক std::string m_origModName;  // __BEGIN__ or __FORK__
সর্বজনীন:
    AstCellInline(File_সারি* fl, ধ্রুবক std::string& name, ধ্রুবক std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    // ACCESSORS
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    std::string origModName() ধ্রুবক { ফেরত m_origModName; }
};
class AstCellInlineScope final : সর্বজনীন Ast_গ্রন্থি {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ধ্রুবক std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
সর্বজনীন:
    AstCellInlineScope(File_সারি* fl, ধ্রুবক std::string& name, ধ্রুবক std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    // ACCESSORS
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক std::string& name) override { m_name = name; }
    std::string origModName() ধ্রুবক { ফেরত m_origModName; }
};
class AstCgOptionAssign final : সর্বজনীন Ast_গ্রন্থি {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ধ্রুবক VCoverOptionType m_optType;  // Option type
    ধ্রুবক string m_name;  // Original option name (for diagnostics on unknown options)
    ধ্রুবক bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
সর্বজনীন:
    AstCgOptionAssign(File_সারি* fl, bool typeOption, VCoverOptionType optType,
                      ধ্রুবক string& rawName, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    VCoverOptionType optType() ধ্রুবক { ফেরত m_optType; }
    bool typeOption() ধ্রুবক { ফেরত m_typeOption; }
};
class AstClassExtends final : সর্বজনীন Ast_গ্রন্থি {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ধ্রুবক bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

সর্বজনীন:
    AstClassExtends(File_সারি* fl, Ast_গ্রন্থি* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    string verilogKwd() ধ্রুবক override { ফেরত isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ধ্রুবক;
    AstClass* classp() ধ্রুবক;  // Like above, but throws error if nulll
    bool isImplements() ধ্রুবক { ফেরত m_isImplements; }
    শূন্য parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ধ্রুবক { ফেরত m_parameterized; }
};
class AstClocking final : সর্বজনীন Ast_গ্রন্থি {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ধ্রুবক bool m_isGlobal;  // True if global clocking

সর্বজনীন:
    AstClocking(File_সারি* fl, ধ্রুবক std::string& name, Ast_Sen_বৃক্ষ* sentreep, Ast_গ্রন্থি* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool isDefault() ধ্রুবক { ফেরত m_isDefault; }
    bool isGlobal() ধ্রুবক { ফেরত m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    শূন্য makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : সর্বজনীন Ast_গ্রন্থি {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

সর্বজনীন:
    AstClockingItem(File_সারি* fl, VDirection direction, Ast_গ্রন্থি_রাশিমালা* skewp, Ast_গ্রন্থি* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        যদি (AstAssign* ধ্রুবক clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } অন্যথা {
            exprp(VN_AS(clockingDeclp, গ্রন্থি_রাশিমালা));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    VDirection direction() ধ্রুবক { ফেরত m_direction; }
    AstClockingItem* outputp() ধ্রুবক { ফেরত m_outputp; }
    শূন্য outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
};
class AstConfig final : সর্বজনীন Ast_গ্রন্থি {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

সর্বজনীন:
    AstConfig(File_সারি* fl, ধ্রুবক std::string& libname, ধ্রুবক std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_libname + "." + m_configname; }
    std::string libname() ধ্রুবক VL_MT_STABLE { ফেরত m_libname; }
    std::string configname() ধ্রুবক VL_MT_STABLE { ফেরত m_configname; }
};
class AstConfigCell final : সর্বজনীন Ast_গ্রন্থি {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

সর্বজনীন:
    AstConfigCell(File_সারি* fl, ধ্রুবক std::string& libname, ধ্রুবক std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_libname + "." + m_cellname; }
    std::string libname() ধ্রুবক VL_MT_STABLE { ফেরত m_libname; }
    std::string cellname() ধ্রুবক VL_MT_STABLE { ফেরত m_cellname; }
};
class AstConfigRule final : সর্বজনীন Ast_গ্রন্থি {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ধ্রুবক bool m_isCell;  // Declared as "cell" versus "instance"

সর্বজনীন:
    AstConfigRule(File_সারি* fl, Ast_গ্রন্থি* cellp, Ast_গ্রন্থি* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() ধ্রুবক VL_MT_STABLE { ফেরত m_isCell; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class AstConfigUse final : সর্বজনীন Ast_গ্রন্থি {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ধ্রুবক bool m_isConfig;  // ":config"; Config, not module/primitive name

সর্বজনীন:
    AstConfigUse(File_সারি* fl, ধ্রুবক std::string& libname, ধ্রুবক std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_libname + "." + m_cellname; }
    std::string libname() ধ্রুবক VL_MT_STABLE { ফেরত m_libname; }
    std::string cellname() ধ্রুবক VL_MT_STABLE { ফেরত m_cellname; }
    bool isConfig() ধ্রুবক VL_MT_STABLE { ফেরত m_isConfig; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class AstConstraint final : সর্বজনীন Ast_গ্রন্থি {
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
সর্বজনীন:
    AstConstraint(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Scope name
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ধ্রুবক override { ফেরত false; }
    bool isPredictOptimizable() ধ্রুবক override { ফেরত false; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* /*samep*/) ধ্রুবক override { ফেরত true; }
    শূন্য baseOverride(ধ্রুবক VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ধ্রুবক { ফেরত m_baseOverride; }
    bool isExternDef() ধ্রুবক { ফেরত m_isExternDef; }
    শূন্য isExternDef(bool flag) { m_isExternDef = flag; }
    শূন্য isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() ধ্রুবক { ফেরত m_isExternExplicit; }
    শূন্য isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() ধ্রুবক { ফেরত m_isExternProto; }
    শূন্য isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() ধ্রুবক { ফেরত m_isKwdPure; }
    শূন্য isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() ধ্রুবক { ফেরত m_isStatic; }
};
class AstCoverBin final : সর্বজনীন Ast_গ্রন্থি {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    ধ্রুবক string m_name;  // Base name of the bin
    ধ্রুবক VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

সর্বজনীন:
    AstCoverBin(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_গ্রন্থি_রাশিমালা* iffp = nullptr)
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
    AstCoverBin(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি_রাশিমালা* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_সারি* fl, ধ্রুবক string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_সারি* fl, ধ্রুবক string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    VCoverBinsType binsType() ধ্রুবক { ফেরত m_binsType; }
    bool isWildcard() ধ্রুবক { ফেরত m_isWildcard; }
    bool isArray() ধ্রুবক { ফেরত m_isArray; }
    শূন্য isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : সর্বজনীন Ast_গ্রন্থি {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ধ্রুবক bool m_isNegated;  // Complement the selection within the cross product

সর্বজনীন:
    AstCoverBinsof(File_সারি* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_গ্রন্থি* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    bool isNegated() ধ্রুবক { ফেরত m_isNegated; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {  // LCOV_EXCL_START
        ধ্রুবক AstCoverBinsof* ধ্রুবক asamep = VN_DBG_AS(samep, CoverBinsof);
        ফেরত m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : সর্বজনীন Ast_গ্রন্থি {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ধ্রুবক string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ধ্রুবক VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

সর্বজনীন:
    AstCoverCrossBin(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* selectp, Ast_গ্রন্থি_রাশিমালা* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    string verilogKwd() ধ্রুবক override;
    VCoverBinsType binsType() ধ্রুবক { ফেরত m_binsType; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {  // LCOV_EXCL_START
        ধ্রুবক AstCoverCrossBin* ধ্রুবক asamep = VN_DBG_AS(samep, CoverCrossBin);
        ফেরত m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : সর্বজনীন Ast_গ্রন্থি {
    // The complete bin-tuple space of the enclosing cross
    ধ্রুবক string m_name;  // Referenced cross name

সর্বজনীন:
    AstCoverCrossRef(File_সারি* fl, ধ্রুবক string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {  // LCOV_EXCL_START
        ফেরত m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : সর্বজনীন Ast_গ্রন্থি {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ধ্রুবক bool m_isOr;  // Union (||), rather than intersection (&&)

সর্বজনীন:
    AstCoverCrossSelect(File_সারি* fl, Ast_গ্রন্থি* lhsp, Ast_গ্রন্থি* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool isOr() ধ্রুবক { ফেরত m_isOr; }
    string verilogKwd() ধ্রুবক override { ফেরত isOr() ? "||" : "&&"; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {  // LCOV_EXCL_START
        ফেরত m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : সর্বজনীন Ast_গ্রন্থি {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
সর্বজনীন:
    AstCoverCrossWith(File_সারি* fl, Ast_গ্রন্থি* selectp, Ast_গ্রন্থি_রাশিমালা* filterp, Ast_গ্রন্থি_রাশিমালা* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : সর্বজনীন Ast_গ্রন্থি {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    ধ্রুবক VCoverOptionType m_optType;  // Option being assigned
    ধ্রুবক bool m_typeOption;  // type_option (vs option)

সর্বজনীন:
    AstCoverOption(File_সারি* fl, bool typeOption, VCoverOptionType optType, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    VCoverOptionType optType() ধ্রুবক { ফেরত m_optType; }
    bool typeOption() ধ্রুবক { ফেরত m_typeOption; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {  // LCOV_EXCL_START
        ধ্রুবক AstCoverOption* ধ্রুবক asamep = VN_DBG_AS(samep, CoverOption);
        ফেরত m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : সর্বজনীন Ast_গ্রন্থি {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
সর্বজনীন:
    AstCoverToggle(File_সারি* fl, AstCoverInc* incp, Ast_গ্রন্থি_রাশিমালা* origp, Ast_গ্রন্থি_রাশিমালা* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    পূর্ণসংখ্যা instr_গুনতি() ধ্রুবক override { ফেরত 3 + INSTR_গুনতি_BRANCH + INSTR_গুনতি_LD; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* /*samep*/) ধ্রুবক override { ফেরত true; }
    bool isGateOptimizable() ধ্রুবক override { ফেরত false; }
    bool isPredictOptimizable() ধ্রুবক override { ফেরত true; }
    bool isOutputter() override {
        ফেরত false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : সর্বজনীন Ast_গ্রন্থি {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ধ্রুবক VTransRepType m_repType;

সর্বজনীন:
    AstCoverTransItem(File_সারি* fl, Ast_গ্রন্থি* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class AstCoverTransSet final : সর্বজনীন Ast_গ্রন্থি {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
সর্বজনীন:
    AstCoverTransSet(File_সারি* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class AstCoverWith final : সর্বজনীন Ast_গ্রন্থি {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
সর্বজনীন:
    AstCoverWith(File_সারি* fl, Ast_গ্রন্থি* subp, Ast_গ্রন্থি_রাশিমালা* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        addSubp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : সর্বজনীন Ast_গ্রন্থি {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

সর্বজনীন:
    AstCovergroup(File_সারি* fl, ধ্রুবক string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_গ্রন্থি* membersp, Ast_Sen_বৃক্ষ* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    bool maybePointedTo() ধ্রুবক override { ফেরত true; }
};
class AstCoverpointRef final : সর্বজনীন Ast_গ্রন্থি {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ধ্রুবক string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

সর্বজনীন:
    AstCoverpointRef(File_সারি* fl, ধ্রুবক string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
};
class AstDefParam final : সর্বজনীন Ast_গ্রন্থি {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
সর্বজনীন:
    AstDefParam(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* pathp, ধ্রুবক string& name, Ast_গ্রন্থি_রাশিমালা* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি*) ধ্রুবক override { ফেরত true; }
};
class AstDefaultClocking final : সর্বজনীন Ast_গ্রন্থি {
    std::string m_name;  // Clocking block name

সর্বজনীন:
    AstDefaultClocking(File_সারি* fl, ধ্রুবক std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
};
class AstDefaultDisable final : সর্বজনীন Ast_গ্রন্থি {
    // @astgen op1 := condp : AstNodeExpr

সর্বজনীন:
    AstDefaultDisable(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : সর্বজনীন Ast_গ্রন্থি {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
সর্বজনীন:
    AstDpiExport(File_সারি* fl, ধ্রুবক string& vname, ধ্রুবক string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    string cname() ধ্রুবক { ফেরত m_cname; }
    শূন্য cname(ধ্রুবক string& cname) { m_cname = cname; }
};
class AstElabDisplay final : সর্বজনীন Ast_গ্রন্থি {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

সর্বজনীন:
    অন্তঃরেখ AstElabDisplay(File_সারি* fl, VDisplayType dispType, Ast_গ্রন্থি_রাশিমালা* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!fmtp());
        ফেরত nullptr;
    }
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    string verilogKwd() ধ্রুবক override { ফেরত "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() ধ্রুবক override { ফেরত false; }
    bool isPredictOptimizable() ধ্রুবক override { ফেরত false; }
    bool isPure() override { ফেরত false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ফেরত true; }  // SPECIAL: $display makes output
    bool isUnlikely() ধ্রুবক override { ফেরত true; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ফেরত displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    পূর্ণসংখ্যা instr_গুনতি() ধ্রুবক override { ফেরত INSTR_গুনতি_PLI; }
    VDisplayType displayType() ধ্রুবক { ফেরত m_displayType; }
    শূন্য displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : সর্বজনীন Ast_গ্রন্থি {
    // Represents something missing, e.g. a missing argument in FOREACH
সর্বজনীন:
    explicit AstEmpty(File_সারি* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* /*samep*/) ধ্রুবক override { ফেরত true; }
};
class AstGenCaseItem final : সর্বজনীন Ast_গ্রন্থি {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
সর্বজনীন:
    AstGenCaseItem(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* condsp, Ast_গ্রন্থি* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() ধ্রুবক { ফেরত condsp() == nullptr; }
};
class AstImplicit final : সর্বজনীন Ast_গ্রন্থি {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
সর্বজনীন:
    AstImplicit(File_সারি* fl, Ast_গ্রন্থি* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : সর্বজনীন Ast_গ্রন্থি {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
সর্বজনীন:
    AstIntfRef(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& parentName, ধ্রুবক string& baseName,
               ধ্রুবক string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    string parentName() ধ্রুবক { ফেরত m_parentName; }
    string baseName() ধ্রুবক { ফেরত m_baseName; }
    string modportName() ধ্রুবক { ফেরত m_modportName; }
};
class AstLibrary final : সর্বজনীন Ast_গ্রন্থি {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

সর্বজনীন:
    AstLibrary(File_সারি* fl, ধ্রুবক std::string& name, Ast_গ্রন্থি_রাশিমালা* filesp, Ast_গ্রন্থি_রাশিমালা* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
};
class AstModport final : সর্বজনীন Ast_গ্রন্থি {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
সর্বজনীন:
    AstModport(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() ধ্রুবক override { ফেরত "modport"; }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : সর্বজনীন Ast_গ্রন্থি {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
সর্বজনীন:
    AstModportClockingRef(File_সারি* fl, ধ্রুবক string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    AstClocking* clockingp() ধ্রুবক VL_MT_STABLE {
        ফেরত m_clockingp;
    }  // [After Link] Pointer to clocking block
    শূন্য clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : সর্বজনীন Ast_গ্রন্থি {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
সর্বজনীন:
    AstModportFTaskRef(File_সারি* fl, ধ্রুবক string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool isImport() ধ্রুবক { ফেরত !m_export; }
    bool isExport() ধ্রুবক { ফেরত m_export; }
    Ast_গ্রন্থি_FTask* ftaskp() ধ্রুবক { ফেরত m_ftaskp; }  // [After Link] Pointer to variable
    শূন্য ftaskp(Ast_গ্রন্থি_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : সর্বজনীন Ast_গ্রন্থি {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    ধ্রুবক VDirection m_direction;  // Direction of the variable (in/out)
সর্বজনীন:
    AstModportVarRef(File_সারি* fl, ধ্রুবক string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি_রাশিমালা* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    VDirection direction() ধ্রুবক { ফেরত m_direction; }
    AstVar* varp() ধ্রুবক VL_MT_STABLE { ফেরত m_varp; }  // [After Link] Pointer to variable
    শূন্য varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : সর্বজনীন Ast_গ্রন্থি {
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
    std::string m_resolved_শীর্ষ_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::set<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ধ্রুবক AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি*, ধ্রুবক Ast_গ্রন্থি_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

সর্বজনীন:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override;
    শূন্য ঠেলো_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.ঢোকাও(varp); }
    ধ্রুবক std::set<AstVar*>& deferredParamVarps() ধ্রুবক { ফেরত m_deferredParamVarps; }
    শূন্য clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    শূন্য deleteContents();
    শূন্য cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত "$root"; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    Ast_গ্রন্থি_Module* শীর্ষ_Modulep() ধ্রুবক VL_MT_STABLE {  // Top module in hierarchy
        ফেরত modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { ফেরত m_typeTablep; }
    string astConstOrigParamName(ধ্রুবক AstConst* nodep) ধ্রুবক;
    শূন্য astConstOrigParamName(ধ্রুবক AstConst* nodep, ধ্রুবক string& name);
    শূন্য astConstOrigParamNameErase(ধ্রুবক AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ধ্রুবক Ast_গ্রন্থি_Module* containingModule(ধ্রুবক Ast_গ্রন্থি* nodep);
    // Forget remembered modules, as the tree has moved.
    শূন্য clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() ধ্রুবক { ফেরত m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() ধ্রুবক { ফেরত m_dollarUnitPkgp; }
    শূন্য dollarUnitPkgp(AstPackage* ধ্রুবক packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() ধ্রুবক { ফেরত m_dpiExportTriggerp; }
    শূন্য dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() ধ্রুবক { ফেরত m_delaySchedulerp; }
    শূন্য delaySchedulerp(AstVar* ধ্রুবক varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() ধ্রুবক { ফেরত m_nbaEventp; }
    শূন্য nbaEventp(AstVarScope* ধ্রুবক varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() ধ্রুবক { ফেরত m_nbaEventTriggerp; }
    শূন্য nbaEventTriggerp(AstVarScope* ধ্রুবক varScopep) { m_nbaEventTriggerp = varScopep; }
    শূন্য stdPackagep(AstPackage* ধ্রুবক packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() ধ্রুবক { ফেরত m_stdPackagep; }
    শূন্য stdPackageProcessp(AstClass* ধ্রুবক classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() ধ্রুবক { ফেরত m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_সারি*) ধ্রুবক;
    Ast_শীর্ষ_Scope* শীর্ষ_Scopep() ধ্রুবক { ফেরত m_শীর্ষ_Scopep; }
    শূন্য create_শীর্ষ_Scope(AstScope* scopep);
    VTimescale timeunit() ধ্রুবক { ফেরত m_timeunit; }
    শূন্য timeunit(ধ্রুবক VTimescale& মান) { m_timeunit = মান; }
    VTimescale timeprecision() ধ্রুবক { ফেরত m_timeprecision; }
    শূন্য timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    শূন্য timeprecisionMerge(File_সারি*, ধ্রুবক VTimescale& মান);
    শূন্য timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ধ্রুবক { ফেরত m_timescaleSpecified; }
    ধ্রুবক std::string& resolved_শীর্ষ_Module_Name() ধ্রুবক { ফেরত m_resolved_শীর্ষ_Module_Name; }
    শূন্য resolved_শীর্ষ_Module_Name(ধ্রুবক std::string& মান) { m_resolved_শীর্ষ_Module_Name = মান; }
    uint32_t nTraceCodes() ধ্রুবক { ফেরত m_nTraceCodes; }
    শূন্য nTraceCodes(uint32_t মান) { m_nTraceCodes = মান; }
    AstVarScope* stlFirstIterationp();
    শূন্য clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    ধ্রুবক std::string trace_Lib_শীর্ষ_Name() ধ্রুবক {
        ধ্রুবক std::string& name = resolved_শীর্ষ_Module_Name();
        ফেরত prettyName(name.empty() ? v3Global.rootp()->শীর্ষ_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    শূন্য addEvalStats(ধ্রুবক std::string& phase);
};
class AstPackageExport final : সর্বজনীন Ast_গ্রন্থি {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

সর্বজনীন:
    AstPackageExport(File_সারি* fl, ধ্রুবক string& pkgName, ধ্রুবক string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    string pkgName() ধ্রুবক VL_MT_STABLE { ফেরত m_pkgName; }
    string prettyPkgNameQ() ধ্রুবক { ফেরত "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ধ্রুবক { ফেরত m_packagep; }
    শূন্য packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : সর্বজনীন Ast_গ্রন্থি {
    // A package export *::* declaration
সর্বজনীন:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_সারি* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : সর্বজনীন Ast_গ্রন্থি {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

সর্বজনীন:
    AstPackageImport(File_সারি* fl, AstPackage* packagep, ধ্রুবক string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_সারি* fl, ধ্রুবক string& pkgName, ধ্রুবক string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    string pkgName() ধ্রুবক VL_MT_STABLE { ফেরত m_pkgName; }
    string prettyPkgNameQ() ধ্রুবক { ফেরত "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ধ্রুবক { ফেরত m_packagep; }
    শূন্য packagep(AstPackage* nodep) { m_packagep = nodep; }

ব্যক্তিগত:
    শূন্য pkgNameFrom();
};
class AstPin final : সর্বজনীন Ast_গ্রন্থি {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    পূর্ণসংখ্যা m_pin_অঙ্ক;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
সর্বজনীন:
    AstPin(File_সারি* fl, পূর্ণসংখ্যা pin_অঙ্ক, ধ্রুবক string& name, Ast_গ্রন্থি* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_অঙ্ক{pin_অঙ্ক}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    শূন্য cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Pin name, ""=go by number
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    string prettyOperatorName() ধ্রুবক override;
    bool dotStar() ধ্রুবক { ফেরত name() == ".*"; }  // Fake name for .* connections until linked
    পূর্ণসংখ্যা pin_অঙ্ক() ধ্রুবক { ফেরত m_pin_অঙ্ক; }
    AstVar* modVarp() ধ্রুবক { ফেরত m_modVarp; }  // [After Link] Pointer to variable
    শূন্য modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ধ্রুবক { ফেরত m_modPTypep; }
    শূন্য modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ধ্রুবক { ফেরত m_param; }
    শূন্য param(bool flag) { m_param = flag; }
    ধ্রুবক string& paramPath() ধ্রুবক { ফেরত m_paramPath; }
    শূন্য paramPath(ধ্রুবক string& path) { m_paramPath = path; }
    bool svDotName() ধ্রুবক { ফেরত m_svDotName; }
    শূন্য svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ধ্রুবক { ফেরত m_svImplicit; }
    শূন্য svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : সর্বজনীন Ast_গ্রন্থি {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ধ্রুবক পূর্ণসংখ্যা m_pin_অঙ্ক;  // Pin number
    ধ্রুবক string m_name;  // Name of pin
সর্বজনীন:
    AstPort(File_সারি* fl, পূর্ণসংখ্যা pinnum, ধ্রুবক string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_অঙ্ক{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Port name
    পূর্ণসংখ্যা pin_অঙ্ক() ধ্রুবক { ফেরত m_pin_অঙ্ক; }  // * = Pin number, for order based instantiation
};
class AstPragma final : সর্বজনীন Ast_গ্রন্থি {
    ধ্রুবক VPragmaType m_pragType;  // Type of pragma
    ধ্রুবক VTimescale m_timescale;  // For TIMEUNIT_SET
সর্বজনীন:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_সারি* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_সারি* fl, ধ্রুবক VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    VPragmaType pragType() ধ্রুবক { ফেরত m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ধ্রুবক override { ফেরত false; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ফেরত pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ধ্রুবক { ফেরত m_timescale; }
};
class AstPropSpec final : সর্বজনীন Ast_গ্রন্থি {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

সর্বজনীন:
    AstPropSpec(File_সারি* fl, AstSenItem* sensesp, Ast_গ্রন্থি_রাশিমালা* disablep, Ast_গ্রন্থি* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool hasDType() ধ্রুবক override VL_MT_SAFE {
        ফেরত true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ধ্রুবক { ফেরত m_propStrength; }
};
class AstPull final : সর্বজনীন Ast_গ্রন্থি {
    // @astgen op1 := lhsp : AstNodeExpr

    ধ্রুবক bool m_direction;

সর্বজনীন:
    AstPull(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ফেরত direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ধ্রুবক { ফেরত m_direction; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class AstScope final : সর্বজনীন Ast_গ্রন্থি {
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
সর্বজনীন:
    AstScope(File_সারি* fl, Ast_গ্রন্থি_Module* modp, ধ্রুবক string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!m_modp);
        ফেরত nullptr;
    }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Scope name
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    string pretty_Name_বার্তা() ধ্রুবক override;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override;
    string nameDotless() ধ্রুবক;
    Ast_গ্রন্থি_Module* modp() ধ্রুবক { ফেরত m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() ধ্রুবক VL_MT_SAFE { ফেরত m_aboveScopep; }
    শূন্য aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ধ্রুবক { ফেরত m_aboveCellp; }
    শূন্য aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_শীর্ষ() ধ্রুবক VL_MT_SAFE { ফেরত aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : সর্বজনীন Ast_গ্রন্থি {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_edgeType;  // Edge type
সর্বজনীন:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    AstSenItem(File_সারি* fl, VEdgeType edgeType, Ast_গ্রন্থি_রাশিমালা* senp, Ast_গ্রন্থি_রাশিমালা* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_সারি* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_সারি* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_সারি* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_সারি* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_সারি* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_সারি* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ফেরত edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() ধ্রুবক { ফেরত m_edgeType; }
    শূন্য edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_গুনতি_Inc();
    }
    Ast_গ্রন্থি_Var_Ref* varrefp() ধ্রুবক { ফেরত VN_CAST(sensp(), গ্রন্থি_Var_Ref); }
    //
    bool isClocked() ধ্রুবক { ফেরত edgeType().clockedStmt(); }
    bool isComboOrStar() ধ্রুবক {
        ফেরত edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() ধ্রুবক { ফেরত edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() ধ্রুবক { ফেরত edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() ধ্রুবক { ফেরত edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() ধ্রুবক { ফেরত edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() ধ্রুবক { ফেরত edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() ধ্রুবক { ফেরত edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_বৃক্ষ final : সর্বজনীন Ast_গ্রন্থি {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
সর্বজনীন:
    Ast_Sen_বৃক্ষ(File_সারি* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_বৃক্ষ(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_বৃক্ষ;
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    // METHODS
    bool hasClocked() ধ্রুবক;  // Includes a clocked statement
    bool hasEdge() ধ্রুবক;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ধ্রুবক;  // Includes a STATIC SenItem
    bool hasInitial() ধ্রুবক;  // Includes a INITIAL SenItem
    bool hasFinal() ধ্রুবক;  // Includes a FINAL SenItem
    bool hasCombo() ধ্রুবক;  // Includes a COMBO SenItem
    bool hasHybrid() ধ্রুবক;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : সর্বজনীন Ast_গ্রন্থি {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

সর্বজনীন:
    AstStrengthSpec(File_সারি* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { ফেরত m_s0; }
    VStrength strength1() { ফেরত m_s1; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class AstSystemCSection final : সর্বজনীন Ast_গ্রন্থি {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ধ্রুবক VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

সর্বজনীন:
    AstSystemCSection(File_সারি* fl, VSystemCSectionType sectionType, ধ্রুবক std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ধ্রুবক { ফেরত m_sectionType; }
    ধ্রুবক std::string& text() ধ্রুবক { ফেরত m_text; }
    শূন্য text(ধ্রুবক std::string& মান) { m_text = মান; }
    শূন্য dump(std::ostream&) ধ্রুবক override;
    শূন্য dumpJson(std::ostream&) ধ্রুবক override;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি*) ধ্রুবক override { ফেরত false; }
    bool isPure() override { ফেরত false; }
    bool isOutputter() override { ফেরত true; }
};
class AstText final : সর্বজনীন Ast_গ্রন্থি {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ধ্রুবক std::string m_text;  // The text to emit
সর্বজনীন:
    AstText(File_সারি* fl, ধ্রুবক std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ফেরত text() == VN_DBG_AS(samep, Text)->text();
    }
    ধ্রুবক std::string& text() ধ্রুবক VL_MT_SAFE { ফেরত m_text; }
};
class AstTextBlock final : সর্বজনীন Ast_গ্রন্থি {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ধ্রুবক std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ধ্রুবক std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ধ্রুবক std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
সর্বজনীন:
    explicit AstTextBlock(File_সারি* fl,  //
                          ধ্রুবক std::string& prefix = "",  //
                          ধ্রুবক std::string& separator = "",  //
                          ধ্রুবক std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    ধ্রুবক std::string& prefix() ধ্রুবক { ফেরত m_prefix; }
    ধ্রুবক std::string& separator() ধ্রুবক { ফেরত m_separator; }
    ধ্রুবক std::string& suffix() ধ্রুবক { ফেরত m_suffix; }
    // Add some text, or a node to this block
    শূন্য add(ধ্রুবক string& text) { addNodesp(new AstText{fileline(), text}); }
    শূন্য add(Ast_গ্রন্থি* nodep) { addNodesp(nodep); }
};
class Ast_শীর্ষ_Scope final : সর্বজনীন Ast_গ্রন্থি {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    friend class AstNetlist;  // Only the AstNetlist can create one
    Ast_শীর্ষ_Scope(File_সারি* fl, AstScope* ascopep)
        : ASTGEN_SUPER_শীর্ষ_Scope(fl) {
        this->scopep(ascopep);
    }

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_শীর্ষ_Scope;
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* create_অস্থায়ী(ধ্রুবক string& name, unsigned width);
    AstVarScope* create_অস্থায়ী(ধ্রুবক string& name, Ast_গ্রন্থি_DType* dtypep);
};
class AstTypeTable final : সর্বজনীন Ast_গ্রন্থি {
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
    using DetailedMap = std::map<VBasic_Type_চাবি, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

সর্বজনীন:
    explicit AstTypeTable(File_সারি* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    শূন্য cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_সারি* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_সারি* fl, VBasicDTypeKwd kwd, পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_সারি* fl, VBasicDTypeKwd kwd, ধ্রুবক VNumRange& range,
                                     পূর্ণসংখ্যা widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& গ্রন্থি);
    AstBasicDType* find_ঢোকাও_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_সারি* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_সারি* fl);
    AstQueueDType* find_Queue_সূচক_DType(File_সারি* fl);
    AstStreamDType* findStreamDType(File_সারি* fl);
    AstVoidDType* findVoidDType(File_সারি* fl);
    শূন্য clearCache();
    শূন্য repairCache();
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
};
class AstTypedef final : সর্বজনীন Ast_গ্রন্থি {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_অঙ্ক;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

সর্বজনীন:
    AstTypedef(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* attrsp, VFlagChildDType,
               Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_অঙ্ক{fl->token_অঙ্ক()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_অঙ্ক{fl->token_অঙ্ক()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() ধ্রুবক;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    আভাসী Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক VL_MT_STABLE {
        ফেরত dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_অঙ্ক() ধ্রুবক override { ফেরত m_decl_Token_অঙ্ক; }
    শূন্য decl_Token_অঙ্ক_Set_Min(uint32_t token_অঙ্ক) override {
        m_decl_Token_অঙ্ক = std::min(m_decl_Token_অঙ্ক, token_অঙ্ক);
    }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    শূন্য name(ধ্রুবক string& flag) override { m_name = flag; }
    bool attrPublic() ধ্রুবক { ফেরত m_attrPublic; }
    শূন্য attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() ধ্রুবক { ফেরত m_isHideLocal; }
    শূন্য isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ধ্রুবক { ফেরত m_isHideProtected; }
    শূন্য isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ধ্রুবক { ফেরত m_isUnderClass; }
    শূন্য isUnderClass(bool flag) { m_isUnderClass = flag; }
    শূন্য tag(ধ্রুবক string& text) override { m_tag = text; }
    string tag() ধ্রুবক override { ফেরত m_tag; }
};
class AstTypedefFwd final : সর্বজনীন Ast_গ্রন্থি {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ধ্রুবক VFwdType m_fwdType;  // Forward type for lint check

সর্বজনীন:
    AstTypedefFwd(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    VFwdType fwdType() ধ্রুবক { ফেরত m_fwdType; }
};
class AstUdpTable final : সর্বজনীন Ast_গ্রন্থি {
    // @astgen op1 := linesp : List[AstUdpTableLine]
সর্বজনীন:
    AstUdpTable(File_সারি* fl, Ast_Udp_Table_সারি* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        যদি (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_সারি final : সর্বজনীন Ast_গ্রন্থি {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
ব্যক্তিগত:
    ধ্রুবক bool m_udpIsCombo;  // Combinational or sequential UDP

সর্বজনীন:
    class UdpCombo {};
    Ast_Udp_Table_সারি(UdpCombo, File_সারি* fl, Ast_Udp_Table_সারি_Val* iFieldsp,
                    Ast_Udp_Table_সারি_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_সারি(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_সারি(UdpSequential, File_সারি* fl, Ast_Udp_Table_সারি_Val* iFieldsp,
                    Ast_Udp_Table_সারি_Val* oFieldsp1, Ast_Udp_Table_সারি_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_সারি(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_সারি;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    পূর্ণসংখ্যা udpIsCombo() ধ্রুবক { ফেরত m_udpIsCombo; }
};
class Ast_Udp_Table_সারি_Val final : সর্বজনীন Ast_গ্রন্থি {
    string m_text;  // Value character

সর্বজনীন:
    Ast_Udp_Table_সারি_Val(File_সারি* fl, ধ্রুবক string& text)
        : ASTGEN_SUPER_Udp_Table_সারি_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_সারি_Val;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_text; }
    শূন্য name(std::string ধ্রুবক& text) override VL_MT_STABLE { m_text = text; }
    string text() ধ্রুবক VL_MT_SAFE { ফেরত m_text; }
};
class AstVar final : সর্বজনীন Ast_গ্রন্থি {
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
    পূর্ণসংখ্যা m_pin_অঙ্ক = 0;  // For JSON, if non-zero the connection pin number
    পূর্ণসংখ্যা m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
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
    bool m_mtask_Cache_সারি_Align : 1;  // Start MTask affinity group on a cache line
    শূন্য init() {
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
        m_mtask_Cache_সারি_Align = false;
    }

সর্বজনীন:
    AstVar(File_সারি* fl, VVarType type, ধ্রুবক string& name, VFlagChildDType, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_সারি* fl, VVarType type, ধ্রুবক string& name, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_সারি* fl, VVarType type, ধ্রুবক string& name, VFlagLogicPacked, পূর্ণসংখ্যা wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_সারি* fl, VVarType type, ধ্রুবক string& name, VFlagBitPacked, পূর্ণসংখ্যা wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_সারি* fl, VVarType type, ধ্রুবক string& name, ধ্রুবক AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        যদি (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_বৃক্ষ(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    অন্তঃরেখ bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Var name
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    string origName() ধ্রুবক override { ফেরত m_origName; }  // * = Original name
    শূন্য origName(ধ্রুবক string& name) { m_origName = name; }
    VVarType varType() ধ্রুবক VL_MT_SAFE { ফেরত m_varType; }  // * = Type of variable
    শূন্য direction(ধ্রুবক VDirection& flag) {
        m_direction = flag;
        যদি (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ধ্রুবক VL_MT_SAFE { ফেরত m_direction; }
    bool isIO() ধ্রুবক VL_MT_SAFE { ফেরত m_direction != VDirection::NONE; }
    bool isVLIO() ধ্রুবক {
        ধ্রুবক AstBasicDType* ধ্রুবক bdtypep = basicp();
        ফেরত isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    শূন্য declDirection(ধ্রুবক VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() ধ্রুবক { ফেরত m_declDirection; }
    পূর্ণসংখ্যা libInoutId() ধ্রুবক { ফেরত m_libInoutId; }
    শূন্য libInoutId(পূর্ণসংখ্যা id) { m_libInoutId = id; }
    bool libInoutEnable() ধ্রুবক { ফেরত m_libInoutEnable; }
    শূন্য libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    শূন্য varType(VVarType type) { m_varType = type; }
    শূন্য varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    শূন্য varType2In() {
        m_tristate = false;
        m_direction = VDirection::প্রবেশ;
    }
    string scType() ধ্রুবক;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) ধ্রুবক;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) ধ্রুবক;
    string dpiTmpVarType(ধ্রুবক string& varName) ধ্রুবক;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) ধ্রুবক;
    string vlEnumType() ধ্রুবক;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) ধ্রুবক;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(ধ্রুবক string& propName) ধ্রুবক;  // Return VerilatorVarProps declaration
    শূন্য combineType(VVarType type);
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* dtypeSkipRefp() ধ্রুবক VL_MT_STABLE { ফেরত subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ধ্রুবক VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    আভাসী Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক VL_MT_STABLE {
        ফেরত dtypep() ? dtypep() : childDTypep();
    }
    শূন্য ansi(bool flag) { m_ansi = flag; }
    শূন্য declTyped(bool flag) { m_declTyped = flag; }
    শূন্য sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    শূন্য attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    শূন্য attrScBv(bool flag) { m_attrScBv = flag; }
    শূন্য attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    শূন্য attrSFormat(bool flag) { m_attrSFormat = flag; }
    শূন্য attrSplitVar(bool flag) { m_attrSplitVar = flag; }
    শূন্য attrFsmState(bool flag) { m_attrFsmState = flag; }
    শূন্য attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    শূন্য attrFsmStateExpand(ধ্রুবক VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() ধ্রুবক { ফেরত m_constPoolEntry; }
    শূন্য setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() ধ্রুবক { ফেরত m_covergroupRefMember; }
    শূন্য covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ধ্রুবক { ফেরত m_embeddedCovergroup; }
    শূন্য embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    শূন্য rand(ধ্রুবক VRandAttr flag) { m_rand = flag; }
    শূন্য usedParam(bool flag) { m_usedParam = flag; }
    শূন্য usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    শূন্য sigPublic(bool flag) { m_sigPublic = flag; }
    শূন্য sigModPublic(bool flag) { m_sigModPublic = flag; }
    শূন্য sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        যদি (flag) sigPublic(true);
    }
    শূন্য sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        যদি (flag) sigUserRdPublic(true);
    }
    শূন্য sc(bool flag) { m_sc = flag; }
    শূন্য scSensitive(bool flag) { m_scSensitive = flag; }
    শূন্য primaryIO(bool flag) { m_primaryIO = flag; }
    শূন্য isConst(bool flag) { m_isConst = flag; }
    শূন্য isContinuously(bool flag) { m_isContinuously = flag; }
    শূন্য isIfaceParent(bool flag) { m_isIfaceParent = flag; }
    শূন্য isIfaceArraySplit(bool flag) { m_isIfaceArraySplit = flag; }
    শূন্য isInternal(bool flag) { m_isInternal = flag; }
    শূন্য isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    শূন্য funcLocal(bool flag) {
        m_funcLocal = flag;
        যদি (flag) m_funcLocalSticky = true;
    }
    শূন্য funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() ধ্রুবক { ফেরত m_gotNansiType; }
    শূন্য gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() ধ্রুবক { ফেরত m_hasStrengthAssignment; }
    শূন্য hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() ধ্রুবক { ফেরত m_hasUserInit; }
    শূন্য hasUserInit(bool flag) { m_hasUserInit = flag; }
    শূন্য icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() ধ্রুবক { ফেরত m_icoMaybeWritten; }
    শূন্য isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() ধ্রুবক { ফেরত m_isClockingDriven; }
    bool isDpiOpenArray() ধ্রুবক VL_MT_SAFE { ফেরত m_isDpiOpenArray; }
    শূন্য isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() ধ্রুবক { ফেরত m_isHideLocal; }
    শূন্য isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ধ্রুবক { ফেরত m_isHideProtected; }
    শূন্য isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() ধ্রুবক { ফেরত m_noCReset; }
    শূন্য noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() ধ্রুবক { ফেরত m_noReset; }
    শূন্য noReset(bool flag) { m_noReset = flag; }
    bool noSubst() ধ্রুবক { ফেরত m_noSubst; }
    শূন্য noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() ধ্রুবক { ফেরত m_processQueue; }
    শূন্য processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() ধ্রুবক { ফেরত m_sampled; }
    শূন্য sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() ধ্রুবক { ফেরত m_substConstOnly; }
    শূন্য substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() ধ্রুবক { ফেরত m_overriddenParam; }
    শূন্য overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() ধ্রুবক { ফেরত m_untypedParam; }
    শূন্য untypedParam(bool flag) { m_untypedParam = flag; }
    শূন্য trace(bool flag) { m_trace = flag; }
    শূন্য isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() ধ্রুবক { ফেরত m_isForceable; }
    শূন্য setForceable() { m_isForceable = true; }
    শূন্য setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() ধ্রুবক { ফেরত m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() ধ্রুবক { ফেরত m_isReadByDpi; }
    শূন্য setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ধ্রুবক { ফেরত m_isWrittenByDpi; }
    শূন্য setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ধ্রুবক { ফেরত m_isWrittenBySuspendable; }
    শূন্য setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ধ্রুবক { ফেরত m_ignorePostRead; }
    শূন্য setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() ধ্রুবক { ফেরত m_ignorePostWrite; }
    শূন্য setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ধ্রুবক { ফেরত m_ignoreSchedWrite; }
    শূন্য setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ধ্রুবক { ফেরত m_dfgTriLowered; }
    শূন্য setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ধ্রুবক { ফেরত m_dfgAllowMultidriveTri; }
    শূন্য setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    শূন্য globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ধ্রুবক { ফেরত m_globalConstrained; }
    bool isStdRandomizeArg() ধ্রুবক { ফেরত m_isStdRandomizeArg; }
    শূন্য setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    শূন্য tag(ধ্রুবক string& text) override { m_tag = text; }
    string tag() ধ্রুবক override { ফেরত m_tag; }
    bool isAnsi() ধ্রুবক { ফেরত m_ansi; }
    bool isContinuously() ধ্রুবক { ফেরত m_isContinuously; }
    bool isDeclTyped() ধ্রুবক { ফেরত m_declTyped; }
    bool isInout() ধ্রুবক { ফেরত m_direction.isInout(); }
    bool isInoutOrRef() ধ্রুবক { ফেরত m_direction.isInoutOrRef(); }
    bool is_প্রবেশ() ধ্রুবক { ফেরত m_direction.is_প্রবেশ(); }
    bool isNonOutput() ধ্রুবক { ফেরত m_direction.isNonOutput(); }
    bool isReadOnly() ধ্রুবক VL_MT_SAFE { ফেরত m_direction.isReadOnly(); }
    bool isConstRef() ধ্রুবক VL_MT_SAFE { ফেরত m_direction.isConstRef(); }
    bool isRef() ধ্রুবক VL_MT_SAFE { ফেরত m_direction.isRef(); }
    bool isWritable() ধ্রুবক VL_MT_SAFE { ফেরত m_direction.isWritable(); }
    bool isTristate() ধ্রুবক { ফেরত m_tristate; }
    bool isPrimaryIO() ধ্রুবক VL_MT_SAFE { ফেরত m_primaryIO; }
    bool isPrimaryInish() ধ্রুবক { ফেরত isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ধ্রুবক VL_MT_SAFE { ফেরত m_primaryClock; }
    শূন্য setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() ধ্রুবক { ফেরত varType() == VVarType::IFACEREF; }
    শূন্য setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() ধ্রুবক { ফেরত m_isIfaceParent; }
    bool isIfaceArraySplit() ধ্রুবক { ফেরত m_isIfaceArraySplit; }
    bool isInternal() ধ্রুবক { ফেরত m_isInternal; }
    bool isIfaceParam() ধ্রুবক { ফেরত m_isIfaceParam; }
    bool isSignal() ধ্রুবক { ফেরত varType().isSignal(); }
    bool isNet() ধ্রুবক { ফেরত varType().isNet(); }
    bool isWor() ধ্রুবক { ফেরত varType().isWor(); }
    bool isWiredNet() ধ্রুবক { ফেরত varType().isWiredNet(); }
    bool is_অস্থায়ী() ধ্রুবক { ফেরত varType().is_অস্থায়ী(); }
    bool isClassMember() ধ্রুবক { ফেরত varType() == VVarType::MEMBER; }
    bool isVirtIface() ধ্রুবক {
        যদি (AstIfaceRefDType* ধ্রুবক dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            ফেরত dtp->isVirtual();
        }
        ফেরত false;
    }
    bool is_Statement_অস্থায়ী() ধ্রুবক { ফেরত varType() == VVarType::STMTTEMP; }
    bool isXTemp() ধ্রুবক { ফেরত varType() == VVarType::XTEMP; }
    bool isParam() ধ্রুবক { ফেরত varType().isParam(); }
    bool isGParam() ধ্রুবক { ফেরত varType() == VVarType::GPARAM; }
    bool isGenVar() ধ্রুবক { ফেরত varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ধ্রুবক {
        ফেরত isIO() || isSignal() || isClassMember() || is_অস্থায়ী() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ধ্রুবক {
        ফেরত !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() ধ্রুবক {
        ধ্রুবক AstBasicDType* ধ্রুবক bdtypep = basicp();
        ফেরত bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() ধ্রুবক { ফেরত m_usedParam; }
    bool isUsedLoopIdx() ধ্রুবক { ফেরত m_usedLoopIdx; }
    bool mtask_Cache_সারি_Align() ধ্রুবক { ফেরত m_mtask_Cache_সারি_Align; }
    শূন্য mtask_Cache_সারি_Align(bool flag) { m_mtask_Cache_সারি_Align = flag; }
    bool isSc() ধ্রুবক VL_MT_SAFE { ফেরত m_sc; }
    bool isScQuad() ধ্রুবক;
    bool isScBv() ধ্রুবক VL_MT_STABLE;
    bool isScUint() ধ্রুবক;
    bool isScUintBool() ধ্রুবক;
    bool isScBigUint() ধ্রুবক VL_MT_STABLE;
    bool isScSensitive() ধ্রুবক { ফেরত m_scSensitive; }
    bool isSigPublic() ধ্রুবক;
    bool isSigModPublic() ধ্রুবক { ফেরত m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() ধ্রুবক { ফেরত m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() ধ্রুবক { ফেরত m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ধ্রুবক { ফেরত m_trace; }
    bool isRand() ধ্রুবক { ফেরত m_rand.isRand(); }
    bool isRandC() ধ্রুবক { ফেরত m_rand.isRandC(); }
    bool isConst() ধ্রুবক VL_MT_SAFE { ফেরত m_isConst; }
    bool isLatched() ধ্রুবক { ফেরত m_isLatched; }
    bool isFuncLocal() ধ্রুবক { ফেরত m_funcLocal; }
    bool isFuncLocalSticky() ধ্রুবক { ফেরত m_funcLocalSticky; }
    bool isFuncReturn() ধ্রুবক { ফেরত m_funcReturn; }
    bool isPullup() ধ্রুবক { ফেরত m_isPullup; }
    bool isPulldown() ধ্রুবক { ফেরত m_isPulldown; }
    bool attrScBv() ধ্রুবক { ফেরত m_attrScBv; }
    bool attrScBigUint() ধ্রুবক { ফেরত m_attrScBigUint; }
    bool attrFileDescr() ধ্রুবক { ফেরত m_attrFileDescr; }
    bool attrSFormat() ধ্রুবক { ফেরত m_attrSFormat; }
    bool attrSplitVar() ধ্রুবক { ফেরত m_attrSplitVar; }
    bool attrFsmState() ধ্রুবক { ফেরত m_attrFsmState; }
    bool attrFsmArcInclCond() ধ্রুবক { ফেরত m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ধ্রুবক { ফেরত m_attrFsmStateExpand; }
    AstIface* sensIfacep() ধ্রুবক { ফেরত m_sensIfacep; }
    VRandAttr rand() ধ্রুবক { ফেরত m_rand; }
    string verilogKwd() ধ্রুবক override;
    শূন্য lifetime(ধ্রুবক VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ধ্রুবক { ফেরত m_lifetime; }
    শূন্য pin_অঙ্ক(পূর্ণসংখ্যা id) { m_pin_অঙ্ক = id; }
    পূর্ণসংখ্যা pin_অঙ্ক() ধ্রুবক { ফেরত m_pin_অঙ্ক; }
    শূন্য propagateAttrFrom(ধ্রুবক AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        যদি (fromp->attrFileDescr()) attrFileDescr(true);
        যদি (fromp->isContinuously()) isContinuously(true);
    }
    শূন্য propagateSplitAttrFrom(ধ্রুবক AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ধ্রুবক অক্ষর* cannotSplitKindReason() ধ্রুবক;
    শূন্য propagateWrapAttrFrom(ধ্রুবক AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    শূন্য combineType(ধ্রুবক AstVar* otherp);
    bool needsCReset() ধ্রুবক {
        ফেরত !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !is_Statement_অস্থায়ী()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    স্থির AstVar* scVarRecurse(Ast_গ্রন্থি* nodep);
};
class AstVarScope final : সর্বজনীন Ast_গ্রন্থি {
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
    bool m_fork_By_মান : 1;  // Set before a fork, whose processes get it by value
সর্বজনীন:
    AstVarScope(File_সারি* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_মান = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    শূন্য cloneRelink() override {
        যদি (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত scopep()->name() + "->" + varp()->name(); }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    AstVar* varp() ধ্রুবক VL_MT_STABLE { ফেরত m_varp; }  // [After Link] Pointer to variable
    শূন্য varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ধ্রুবক VL_MT_STABLE { ফেরত m_scopep; }  // Pointer to scope it's under
    শূন্য scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ধ্রুবক { ফেরত m_trace; }
    শূন্য trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ধ্রুবক { ফেরত m_optimizeLifePost; }
    শূন্য optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_মান() ধ্রুবক { ফেরত m_fork_By_মান; }
    শূন্য fork_By_মান(bool flag) { m_fork_By_মান = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : সর্বজনীন Ast_গ্রন্থি_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ধ্রুবক uint64_t m_সূচক;  // Index of the element, with the array's lo() at 0
সর্বজনীন:
    AstConsArrayItem(File_সারি* fl, uint64_t সূচক, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_সূচক{সূচক} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        ফেরত nullptr;
    }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ফেরত m_সূচক == VN_DBG_AS(samep, ConsArrayItem)->m_সূচক;
    }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }  // By AstConsArray's map
    uint64_t সূচক() ধ্রুবক { ফেরত m_সূচক; }
};
class AstConsAssocItem final : সর্বজনীন Ast_গ্রন্থি_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
সর্বজনীন:
    AstConsAssocItem(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* keyp, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        ফেরত nullptr;
    }
};
class AstConsDynArrayItem final : সর্বজনীন Ast_গ্রন্থি_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
সর্বজনীন:
    AstConsDynArrayItem(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_সারি* fl, AstConst* keyp, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        ফেরত nullptr;
    }
};
class AstConsQueueItem final : সর্বজনীন Ast_গ্রন্থি_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
সর্বজনীন:
    AstConsQueueItem(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        ফেরত nullptr;
    }
};
class AstConsStructItem final : সর্বজনীন Ast_গ্রন্থি_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
সর্বজনীন:
    AstConsStructItem(File_সারি* fl, AstMemberDType* memberp, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        ফেরত nullptr;
    }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    AstMemberDType* memberp() ধ্রুবক { ফেরত m_memberp; }
};
class AstConsWildcardItem final : সর্বজনীন Ast_গ্রন্থি_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
সর্বজনীন:
    AstConsWildcardItem(File_সারি* fl, AstConst* keyp, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        ফেরত nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : সর্বজনীন Ast_গ্রন্থি_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    পূর্ণসংখ্যা m_offset;  // Offset column numbers to uniq-ify IFs
সর্বজনীন:
    AstCoverOtherDecl(File_সারি* fl, ধ্রুবক string& page, ধ্রুবক string& comment,
                      ধ্রুবক string& linescov, পূর্ণসংখ্যা offset, ধ্রুবক string& fsmVar = "",
                      ধ্রুবক string& fsmFrom = "", ধ্রুবক string& fsmTo = "",
                      ধ্রুবক string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    পূর্ণসংখ্যা offset() ধ্রুবক { ফেরত m_offset; }
    পূর্ণসংখ্যা আকার_2() ধ্রুবক override { ফেরত 1; }
    ধ্রুবক string& linescov() ধ্রুবক { ফেরত m_linescov; }
    ধ্রুবক string& fsmVar() ধ্রুবক { ফেরত m_fsmVar; }
    ধ্রুবক string& fsmFrom() ধ্রুবক { ফেরত m_fsmFrom; }
    ধ্রুবক string& fsmTo() ধ্রুবক { ফেরত m_fsmTo; }
    ধ্রুবক string& fsmTag() ধ্রুবক { ফেরত m_fsmTag; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstCoverOtherDecl* ধ্রুবক asamep = VN_DBG_AS(samep, CoverOtherDecl);
        ফেরত Super::same_গ্রন্থি(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : সর্বজনীন Ast_গ্রন্থি_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ধ্রুবক VNumRange m_range;  // Packed array range covering each toggle bit
সর্বজনীন:
    AstCoverToggleDecl(File_সারি* fl, ধ্রুবক string& page, ধ্রুবক string& comment,
                       ধ্রুবক VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    পূর্ণসংখ্যা আকার_2() ধ্রুবক override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        ফেরত 2 * m_range.elements();
    }
    ধ্রুবক VNumRange& range() ধ্রুবক { ফেরত m_range; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstCoverToggleDecl* ধ্রুবক asamep = VN_DBG_AS(samep, CoverToggleDecl);
        ফেরত Super::same_গ্রন্থি(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : সর্বজনীন Ast_গ্রন্থি_FTask {
    // A function inside a module
সর্বজনীন:
    AstFunc(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* stmtp, Ast_গ্রন্থি* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    Ast_গ্রন্থি_FTask* cloneType(ধ্রুবক string& name) override {
        ফেরত new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ধ্রুবক override { ফেরত "function"; }
};
class AstLet final : সর্বজনীন Ast_গ্রন্থি_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
সর্বজনীন:
    AstLet(File_সারি* fl, ধ্রুবক string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_রাশিমালা));
        ফেরত nullptr;
    }
    Ast_গ্রন্থি_FTask* cloneType(ধ্রুবক string& name) override { ফেরত new AstLet{fileline(), name}; }
    string verilogKwd() ধ্রুবক override { ফেরত "let"; }
};
class AstProperty final : সর্বজনীন Ast_গ্রন্থি_FTask {
    // A property inside a module
সর্বজনীন:
    AstProperty(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    Ast_গ্রন্থি_FTask* cloneType(ধ্রুবক string& name) override {
        ফেরত new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ধ্রুবক override { ফেরত "property"; }
};
class AstSequence final : সর্বজনীন Ast_গ্রন্থি_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
সর্বজনীন:
    AstSequence(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    Ast_গ্রন্থি_FTask* cloneType(ধ্রুবক string& name) override {
        ফেরত new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ধ্রুবক override { ফেরত "sequence"; }
    bool isReferenced() ধ্রুবক { ফেরত m_referenced; }
    শূন্য isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : সর্বজনীন Ast_গ্রন্থি_FTask {
    // A task inside a module
সর্বজনীন:
    AstTask(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_গ্রন্থি_FTask* cloneType(ধ্রুবক string& name) override {
        ফেরত new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ধ্রুবক override { ফেরত "task"; }
};

// === AstNodeFile ===
class AstCFile final : সর্বজনীন Ast_গ্রন্থি_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
সর্বজনীন:
    AstCFile(File_সারি* fl, ধ্রুবক string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    uint64_t complexityScore() ধ্রুবক { ফেরত m_complexityScore; }
    শূন্য complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ধ্রুবক { ফেরত m_slow; }
    শূন্য slow(bool flag) { m_slow = flag; }
    bool source() ধ্রুবক { ফেরত m_source; }
    শূন্য source(bool flag) { m_source = flag; }
    bool support() ধ্রুবক { ফেরত m_support; }
    শূন্য support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : সর্বজনীন Ast_গ্রন্থি_File {
    // Verilog output file
    // Parents:  NETLIST
সর্বজনীন:
    AstVFile(File_সারি* fl, ধ্রুবক string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
};
class AstCoverCross final : সর্বজনীন Ast_গ্রন্থি_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
সর্বজনীন:
    AstCoverCross(File_সারি* fl, ধ্রুবক string& name, AstCoverpointRef* itemsp,
                  Ast_গ্রন্থি_রাশিমালা* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class AstCoverpoint final : সর্বজনীন Ast_গ্রন্থি_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
সর্বজনীন:
    AstCoverpoint(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি_রাশিমালা* exprp,
                  Ast_গ্রন্থি_রাশিমালা* iffp = nullptr, Ast_গ্রন্থি* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};

// === AstNodeGen ===
class AstGenBlock final : সর্বজনীন Ast_গ্রন্থি_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ধ্রুবক bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ধ্রুবক bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

সর্বজনীন:
    AstGenBlock(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক std::string& name) override { m_name = name; }
    bool unnamed() ধ্রুবক { ফেরত m_unnamed; }
    bool implied() ধ্রুবক { ফেরত m_implied; }
    AstDefaultDisable* defaultDisablep() ধ্রুবক { ফেরত m_defaultDisablep; }
    শূন্য defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(ধ্রুবক string& outer) ধ্রুবক;
};
class AstGenCase final : সর্বজনীন Ast_গ্রন্থি_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
সর্বজনীন:
    AstGenCase(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : সর্বজনীন Ast_গ্রন্থি_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
সর্বজনীন:
    AstGenFor(File_সারি* fl, Ast_গ্রন্থি* initsp, Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি* incsp, Ast_গ্রন্থি* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : সর্বজনীন Ast_গ্রন্থি_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
সর্বজনীন:
    AstGenIf(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* condp, Ast_গ্রন্থি* thensp, Ast_গ্রন্থি* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : সর্বজনীন Ast_গ্রন্থি_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_অঙ্ক;  // Declaration token number
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
    পূর্ণসংখ্যা m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool full, ধ্রুবক string& scope) ধ্রুবক;
    string dtypeNameScope() ধ্রুবক;  // Prefix of dtypeName(true) for the scope declaring the class

সর্বজনীন:
    AstClass(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_অঙ্ক{fl->token_অঙ্ক()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ধ্রুবক override { ফেরত isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    bool timescaleMatters() ধ্রুবক override { ফেরত false; }
    AstClassPackage* classOrPackagep() ধ্রুবক VL_MT_STABLE { ফেরত m_classOrPackagep; }
    শূন্য classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ধ্রুবক VL_MT_STABLE {
        ফেরত m_covergroupEnclosingClassp;
    }
    শূন্য covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_গ্রন্থি* membersp() ধ্রুবক VL_MT_STABLE { ফেরত stmtsp(); }
    শূন্য addMembersp(Ast_গ্রন্থি* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() ধ্রুবক { ফেরত m_hasRandVarsUpdate; }
    শূন্য hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() ধ্রুবক { ফেরত m_covergroup; }
    শূন্য isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() ধ্রুবক { ফেরত m_extended; }
    শূন্য isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() ধ্রুবক { ফেরত m_interfaceClass; }
    শূন্য isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() ধ্রুবক { ফেরত m_virtual; }
    শূন্য isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() ধ্রুবক { ফেরত m_needRNG; }
    শূন্য needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() ধ্রুবক { ফেরত m_useVirtualPublic; }
    শূন্য useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    শূন্য markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ধ্রুবক { ফেরত m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) ধ্রুবক;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(ধ্রুবক string& scope) ধ্রুবক;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    শূন্য dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ধ্রুবক { ফেরত !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_বার্তা() ধ্রুবক override { ফেরত hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    স্থির string embeddedCovergroupTypeName(ধ্রুবক string& name) { ফেরত "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ধ্রুবক;
    // Covergroup options accessors
    পূর্ণসংখ্যা cgAutoBinMax() ধ্রুবক { ফেরত m_cgAutoBinMax; }
    শূন্য cgAutoBinMax(পূর্ণসংখ্যা মান) { m_cgAutoBinMax = মান; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    স্থির bool isClassExtendedFrom(ধ্রুবক AstClass* refClassp, ধ্রুবক AstClass* baseClassp);
    uint32_t decl_Token_অঙ্ক() ধ্রুবক override { ফেরত m_decl_Token_অঙ্ক; }
    শূন্য decl_Token_অঙ্ক_Set_Min(uint32_t token_অঙ্ক) override {
        m_decl_Token_অঙ্ক = std::min(m_decl_Token_অঙ্ক, token_অঙ্ক);
    }
    শূন্য baseOverride(ধ্রুবক VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ধ্রুবক { ফেরত m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    স্থির bool isCacheableChild(ধ্রুবক Ast_গ্রন্থি* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    template <typename T_Callable>
    শূন্য foreachMember(ধ্রুবক T_Callable& f) {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_গ্রন্থি*>::মান
                && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        যদি (ধ্রুবক AstClassExtends* ধ্রুবক cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        জন্য (Ast_গ্রন্থি* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (Ast_গ্রন্থি::privateTypeTest<T_গ্রন্থি>(stmtp)) f(this, static_cast<T_গ্রন্থি*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    template <typename T_Callable>
    bool existsMember(ধ্রুবক T_Callable& p) ধ্রুবক {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ধ্রুবক AstClass*, ধ্রুবক T_গ্রন্থি*>::মান
                && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        যদি (ধ্রুবক AstClassExtends* ধ্রুবক cextendsp = this->extendsp()) {
            যদি (cextendsp->classp()->existsMember(p)) ফেরত true;
        }
        জন্য (Ast_গ্রন্থি* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (Ast_গ্রন্থি::privateTypeTest<T_গ্রন্থি>(stmtp)) {
                যদি (p(this, static_cast<T_গ্রন্থি*>(stmtp))) ফেরত true;
            }
        }
        ফেরত false;
    }
};
class AstClassPackage final : সর্বজনীন Ast_গ্রন্থি_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
সর্বজনীন:
    AstClassPackage(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ধ্রুবক override { ফেরত "classpackage"; }
    bool timescaleMatters() ধ্রুবক override { ফেরত false; }
    AstClass* classp() ধ্রুবক VL_MT_SAFE { ফেরত m_classp; }
    শূন্য classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : সর্বজনীন Ast_গ্রন্থি_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
সর্বজনীন:
    AstIface(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string verilogKwd() ধ্রুবক override { ফেরত "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ধ্রুবক override { ফেরত false; }
    bool hasVirtualRef() ধ্রুবক { ফেরত m_hasVirtualRef; }
    শূন্য setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) ধ্রুবক;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    শূন্য dtypeNameFreeze();
};
class AstModule final : সর্বজনীন Ast_গ্রন্থি_Module {
    // A module declaration
    ধ্রুবক bool m_isChecker;  // Module represents a checker
    ধ্রুবক bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

সর্বজনীন:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ধ্রুবক override {
        ফেরত m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() ধ্রুবক override { ফেরত true; }
    bool isChecker() ধ্রুবক { ফেরত m_isChecker; }
    bool isProgram() ধ্রুবক { ফেরত m_isProgram; }
    bool hasGenericIface() ধ্রুবক { ফেরত m_hasGenericIface; }
    শূন্য hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};
class AstNotFoundModule final : সর্বজনীন Ast_গ্রন্থি_Module {
    // A missing module declaration
সর্বজনীন:
    AstNotFoundModule(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() ধ্রুবক override { ফেরত "/*not-found-*/ module"; }
    bool timescaleMatters() ধ্রুবক override { ফেরত false; }
};
class AstPackage final : সর্বজনীন Ast_গ্রন্থি_Module {
    // A package declaration
সর্বজনীন:
    AstPackage(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ধ্রুবক override { ফেরত "package"; }
    bool timescaleMatters() ধ্রুবক override { ফেরত !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : সর্বজনীন Ast_গ্রন্থি_Module {
    // A primitive declaration
সর্বজনীন:
    AstPrimitive(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ধ্রুবক override { ফেরত "primitive"; }
    bool timescaleMatters() ধ্রুবক override { ফেরত false; }
};

// === AstNodeProcedure ===
class AstAlways final : সর্বজনীন Ast_গ্রন্থি_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ধ্রুবক VAlwaysKwd m_keyword;

সর্বজনীন:
    AstAlways(File_সারি* fl, VAlwaysKwd keyword, Ast_Sen_বৃক্ষ* sentreep, Ast_গ্রন্থি* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit অন্তঃরেখ AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    VAlwaysKwd keyword() ধ্রুবক { ফেরত m_keyword; }
};
class AstAlwaysObserved final : সর্বজনীন Ast_গ্রন্থি_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

সর্বজনীন:
    AstAlwaysObserved(File_সারি* fl, Ast_Sen_বৃক্ষ* sentreep, Ast_গ্রন্থি* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : সর্বজনীন Ast_গ্রন্থি_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

সর্বজনীন:
    explicit AstAlwaysPost(File_সারি* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : সর্বজনীন Ast_গ্রন্থি_Procedure {
    // Like always but Postponed scheduling region

সর্বজনীন:
    AstAlwaysPostponed(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : সর্বজনীন Ast_গ্রন্থি_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

সর্বজনীন:
    explicit AstAlwaysPre(File_সারি* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : সর্বজনীন Ast_গ্রন্থি_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

সর্বজনীন:
    AstAlwaysReactive(File_সারি* fl, Ast_Sen_বৃক্ষ* sentreep, Ast_গ্রন্থি* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : সর্বজনীন Ast_গ্রন্থি_Procedure {
সর্বজনীন:
    AstFinal(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : সর্বজনীন Ast_গ্রন্থি_Procedure {
সর্বজনীন:
    AstInitial(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : সর্বজনীন Ast_গ্রন্থি_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
সর্বজনীন:
    AstInitialAutomatic(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : সর্বজনীন Ast_গ্রন্থি_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
সর্বজনীন:
    AstInitialStatic(File_সারি* fl, Ast_গ্রন্থি* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : সর্বজনীন Ast_গ্রন্থি_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
সর্বজনীন:
    AstBracketRange(File_সারি* fl, Ast_গ্রন্থি* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    আভাসী string emitC() { V3ERROR_NA_RETURN(""); }
    আভাসী string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* /*samep*/) ধ্রুবক override { ফেরত true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত false; }
};
class AstRange final : সর্বজনীন Ast_গ্রন্থি_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ধ্রুবক bool m_fromBracket = false;  // From C-style '[x]' declaration
সর্বজনীন:
    AstRange(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* leftp, Ast_গ্রন্থি_রাশিমালা* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    অন্তঃরেখ AstRange(File_সারি* fl, পূর্ণসংখ্যা বাম, পূর্ণসংখ্যা ডান);
    অন্তঃরেখ AstRange(File_সারি* fl, ধ্রুবক VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    অন্তঃরেখ পূর্ণসংখ্যা বাম_Const() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ পূর্ণসংখ্যা ডান_Const() ধ্রুবক VL_MT_STABLE;
    পূর্ণসংখ্যা hiConst() ধ্রুবক VL_MT_STABLE {
        ধ্রুবক পূর্ণসংখ্যা l = বাম_Const();
        ধ্রুবক পূর্ণসংখ্যা r = ডান_Const();
        ফেরত l > r ? l : r;
    }
    পূর্ণসংখ্যা loConst() ধ্রুবক VL_MT_STABLE {
        ধ্রুবক পূর্ণসংখ্যা l = বাম_Const();
        ধ্রুবক পূর্ণসংখ্যা r = ডান_Const();
        ফেরত l > r ? r : l;
    }
    পূর্ণসংখ্যা elementsConst() ধ্রুবক VL_MT_STABLE { ফেরত hiConst() - loConst() + 1; }
    bool ascending() ধ্রুবক { ফেরত বাম_Const() < ডান_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_বাম_To_ডান() ধ্রুবক {
        ফেরত VNumRange{বাম_Const(), ডান_Const()}.seq_বাম_To_ডান();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_ডান_To_বাম() ধ্রুবক {
        ফেরত VNumRange{বাম_Const(), ডান_Const()}.seq_ডান_To_বাম();
    }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    আভাসী string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstRange* ধ্রুবক asamep = VN_DBG_AS(samep, Range);
        ফেরত fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ধ্রুবক { ফেরত m_fromBracket; }
};
class AstUnsizedRange final : সর্বজনীন Ast_গ্রন্থি_Range {
    // Unsized range specification, for open arrays
সর্বজনীন:
    explicit AstUnsizedRange(File_সারি* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    আভাসী string emitC() { V3ERROR_NA_RETURN(""); }
    আভাসী string emitVerilog() { ফেরত "[]"; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* /*samep*/) ধ্রুবক override { ফেরত true; }
};
class AstWildcardRange final : সর্বজনীন Ast_গ্রন্থি_Range {
    // Wildcard range specification, for wildcard index type associative arrays
সর্বজনীন:
    explicit AstWildcardRange(File_সারি* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    আভাসী string emitC() { V3ERROR_NA_RETURN(""); }
    আভাসী string emitVerilog() { ফেরত "[*]"; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* /*samep*/) ধ্রুবক override { ফেরত true; }
};

#endif  // Guard
