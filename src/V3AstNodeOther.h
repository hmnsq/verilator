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
#અન્તર્ભાવ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

#અન્તર્ભાવ <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_ગાંઠ_Cons_Item VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    friend class Ast_ગાંઠ_પદાવલિ;  // For m_knownConstantTree
    mutable bool m_known_Constant_વૃક્ષ = false;  // Known to be a tree of constants

સુરક્ષિત:
    Ast_ગાંઠ_Cons_Item(VNType t, File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* valuep)
        : Ast_ગાંઠ{t, fl} {
        this->valuep(valuep);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Cons_Item;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class Ast_ગાંઠ_Cover_Decl VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    પૂર્ણાંક m_bin_અંક = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    પૂર્ણાંક m_local_Bin_અંક = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
જાહેર:
    Ast_ગાંઠ_Cover_Decl(VNType t, File_લીટી* fl, અચલ string& page, અચલ string& comment)
        : Ast_ગાંઠ(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_ગાંઠ_Cover_Decl;
    અચલ અક્ષર* broken() અચલ override {
        જો (m_dataDeclp
            && (m_dataDeclp == this || m_dataDeclp->m_dataDeclp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        પાછા nullptr;
    }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 1 + 2 * INSTR_ગણતરી_LD; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    પૂર્ણાંક bin_અંક() અચલ { પાછા m_bin_અંક; }
    શૂન્ય bin_અંક(પૂર્ણાંક flag) { m_bin_અંક = flag; }
    પૂર્ણાંક local_Bin_અંક() અચલ { પાછા m_local_Bin_અંક; }
    શૂન્ય local_Bin_અંક(પૂર્ણાંક flag) { m_local_Bin_અંક = flag; }
    bool perInstance() અચલ { પાછા m_perInstance; }
    શૂન્ય perInstance(bool flag) { m_perInstance = flag; }
    આભાસી પૂર્ણાંક size() અચલ = 0;
    અચલ string& comment() અચલ { પાછા m_text; }  // text to insert in code
    અચલ string& page() અચલ { પાછા m_page; }
    અચલ string& hier() અચલ { પાછા m_hier; }
    શૂન્ય hier(અચલ string& flag) { m_hier = flag; }
    શૂન્ય comment(અચલ string& flag) { m_text = flag; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ Ast_ગાંઠ_Cover_Decl* અચલ asamep = VN_DBG_AS(samep, ગાંઠ_Cover_Decl);
        પાછા (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    શૂન્ય dataDeclp(Ast_ગાંઠ_Cover_Decl* nodep) { m_dataDeclp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_ગાંઠ_Cover_Decl* dataDeclNullp() અચલ { પાછા m_dataDeclp; }
    Ast_ગાંઠ_Cover_Decl* dataDeclThisp() { પાછા dataDeclNullp() ? dataDeclNullp() : this; }
};
class Ast_ગાંઠ_FTask VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
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

સુરક્ષિત:
    Ast_ગાંઠ_FTask(VNType t, File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* stmtsp)
        : Ast_ગાંઠ{t, fl}
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

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_FTask;
    આભાસી Ast_ગાંઠ_FTask* cloneType(અચલ string& name) = 0;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    bool isGateOptimizable() અચલ override {
        પાછા !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string cname() અચલ { પાછા m_cname; }
    શૂન્ય cname(અચલ string& cname) { m_cname = cname; }
    bool isFunction() અચલ { પાછા fvarp() != nullptr; }
    // MORE ACCESSORS
    શૂન્ય dpiOpenParentInc() { ++m_dpiOpenParent; }
    શૂન્ય dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() અચલ { પાછા m_dpiOpenParent; }
    bool taskPublic() અચલ { પાછા m_taskPublic; }
    શૂન્ય taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() અચલ { પાછા m_classMethod; }
    શૂન્ય classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() અચલ { પાછા m_didProto; }
    શૂન્ય didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() અચલ { પાછા m_isExternProto; }
    શૂન્ય isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() અચલ { પાછા m_isExternDef; }
    શૂન્ય isExternDef(bool flag) { m_isExternDef = flag; }
    અચલ string& ifacePortName() અચલ { પાછા m_ifacePortName; }
    શૂન્ય ifacePortName(અચલ string& name) { m_ifacePortName = name; }
    bool prototype() અચલ { પાછા m_prototype; }
    શૂન્ય prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() અચલ { પાછા m_dpiExport; }
    શૂન્ય dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() અચલ { પાછા m_dpiImport; }
    શૂન્ય dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() અચલ { પાછા m_dpiContext; }
    શૂન્ય dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() અચલ { પાછા m_dpiOpenChild; }
    શૂન્ય dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() અચલ { પાછા m_dpiTask; }
    શૂન્ય dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() અચલ { પાછા !m_dpiCDecl.empty(); }
    અચલ string& dpiCDecl() અચલ { પાછા m_dpiCDecl; }
    શૂન્ય dpiCDecl(અચલ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() અચલ { પાછા m_isConstructor; }
    શૂન્ય isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() અચલ { પાછા m_isHideLocal; }
    શૂન્ય isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() અચલ { પાછા m_isHideProtected; }
    શૂન્ય isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() અચલ { પાછા m_dpiPure; }
    શૂન્ય dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() અચલ { પાછા m_keepAlive; }
    શૂન્ય keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() અચલ { પાછા m_needsSyms; }
    શૂન્ય needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() અચલ { પાછા m_pureVirtual; }
    શૂન્ય pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() અચલ { પાછા m_recursive; }
    શૂન્ય recursive(bool flag) { m_recursive = flag; }
    bool isStatic() અચલ { પાછા m_static; }
    શૂન્ય isStatic(bool flag) { m_static = flag; }
    bool underGenerate() અચલ { પાછા m_underGenerate; }
    શૂન્ય underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() અચલ { પાછા m_verilogFunction; }
    શૂન્ય verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() અચલ { પાછા m_verilogTask; }
    શૂન્ય verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() અચલ { પાછા m_virtual; }
    શૂન્ય isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() અચલ { પાછા m_needProcess; }
    શૂન્ય setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() અચલ { પાછા m_isCovergroupSample; }
    શૂન્ય isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    શૂન્ય baseOverride(અચલ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() અચલ { પાછા m_baseOverride; }
    શૂન્ય lifetime(અચલ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() અચલ { પાછા m_lifetime; }
    bool isPure() override;
    અચલ અક્ષર* broken() અચલ override;
    શૂન્ય propagateAttrFrom(અચલ Ast_ગાંઠ_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        classMethod(fromp->classMethod());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

ખાનગી:
    bool getPurityRecurse() અચલ;
};
class Ast_ગાંઠ_File VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
જાહેર:
    Ast_ગાંઠ_File(VNType t, File_લીટી* fl, અચલ string& name)
        : Ast_ગાંઠ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ગાંઠ_File;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class Ast_ગાંઠ_Func_Cov_Item VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // Base class for functional coverage items (coverpoints, crosses)
સુરક્ષિત:
    string m_name;  // Item name

જાહેર:
    Ast_ગાંઠ_Func_Cov_Item(VNType t, File_લીટી* fl, અચલ string& name)
        : Ast_ગાંઠ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ગાંઠ_Func_Cov_Item;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    bool maybePointedTo() અચલ override { પાછા true; }
};
class Ast_ગાંઠ_Gen VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // Generate construct
જાહેર:
    Ast_ગાંઠ_Gen(VNType t, File_લીટી* fl)
        : Ast_ગાંઠ{t, fl} {}
    ASTGEN_MEMBERS_Ast_ગાંઠ_Gen;
};
class Ast_ગાંઠ_Module VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    અચલ string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    પૂર્ણાંક m_depth = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    પૂર્ણાંક m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
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
સુરક્ષિત:
    Ast_ગાંઠ_Module(VNType t, File_લીટી* fl, અચલ string& name, અચલ string& libname)
        : Ast_ગાંઠ{t, fl}
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

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Module;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    આભાસી bool timescaleMatters() અચલ = 0;
    અંતઃરેખ bool isDollarUnit() અચલ;  // Is the $unit package
    અંતઃરેખ bool isConstPool() અચલ;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(અચલ string& outer) અચલ;
    // ACCESSORS
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string origName() અચલ override { પાછા m_origName; }
    string someInstanceName() અચલ VL_MT_SAFE { પાછા m_someInstanceName; }
    શૂન્ય someInstanceName(અચલ string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() અચલ { પાછા m_defaultDisablep; }
    શૂન્ય defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() અચલ { પાછા m_inLibrary; }
    શૂન્ય inLibrary(bool flag) { m_inLibrary = flag; }
    શૂન્ય depth(પૂર્ણાંક કિંમત) { m_depth = કિંમત; }
    પૂર્ણાંક depth() અચલ VL_MT_SAFE { પાછા m_depth; }
    શૂન્ય level(પૂર્ણાંક કિંમત) { m_level = કિંમત; }
    પૂર્ણાંક level() અચલ VL_MT_SAFE { પાછા m_level; }
    string libname() અચલ { પાછા m_libname; }
    bool is_ટોચ() અચલ VL_MT_SAFE { પાછા level() == 1; }
    bool modPublic() અચલ { પાછા m_modPublic; }
    શૂન્ય modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() અચલ { પાછા m_modTrace; }
    શૂન્ય modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() અચલ { પાછા m_ctorVarReset; }
    શૂન્ય ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() અચલ { પાછા m_dead; }
    શૂન્ય dead(bool flag) { m_dead = flag; }
    bool hasGParam() અચલ { પાછા m_hasGParam; }
    શૂન્ય hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() અચલ { પાછા m_hasParameterList; }
    શૂન્ય hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() અચલ { પાછા m_hierBlock; }
    શૂન્ય hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() અચલ { પાછા m_hierParams; }
    શૂન્ય hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() અચલ { પાછા m_recursive; }
    શૂન્ય recursive(bool flag) { m_recursive = flag; }
    શૂન્ય recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() અચલ { પાછા m_recursiveClone; }
    bool parameterizedTemplate() અચલ { પાછા m_parameterizedTemplate; }
    શૂન્ય parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    શૂન્ય verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() અચલ { પાછા m_verilatorLib; }
    VLifetime lifetime() અચલ { પાછા m_lifetime; }
    શૂન્ય lifetime(અચલ VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    શૂન્ય timeunit(અચલ VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() અચલ { પાછા m_unconnectedDrive; }
    શૂન્ય unconnectedDrive(અચલ VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_ગાંઠ_Procedure VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
સુરક્ષિત:
    Ast_ગાંઠ_Procedure(VNType t, File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : Ast_ગાંઠ{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Procedure;
    // METHODS
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isJustOneBodyStmt() અચલ { પાછા stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() અચલ { પાછા m_suspendable; }
    શૂન્ય setSuspendable() { m_suspendable = true; }
    bool needProcess() અચલ { પાછા m_needProcess; }
    શૂન્ય setNeedProcess() { m_needProcess = true; }
};
class Ast_ગાંઠ_Range VL_NOT_FINAL : જાહેર Ast_ગાંઠ {
    // A range, sized or unsized
સુરક્ષિત:
    Ast_ગાંઠ_Range(VNType t, File_લીટી* fl)
        : Ast_ગાંઠ{t, fl} {}

જાહેર:
    ASTGEN_MEMBERS_Ast_ગાંઠ_Range;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : જાહેર Ast_ગાંઠ {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

જાહેર:
    AstActive(File_લીટી* fl, અચલ string& name, Ast_Sen_વૃક્ષ* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    // Statements are broken into pieces, as some must come before others.
    શૂન્ય sentreep(Ast_Sen_વૃક્ષ* nodep) { m_sentreep = nodep; }
    Ast_Sen_વૃક્ષ* sentreep() અચલ { પાછા m_sentreep; }
    // METHODS
    અંતઃરેખ bool hasClocked() અચલ;
    અંતઃરેખ bool hasCombo() અચલ;
    અંતઃરેખ bool hasInitial() અચલ;
    અંતઃરેખ bool hasStatic() અચલ;
};
class AstAlias final : જાહેર Ast_ગાંઠ {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
જાહેર:
    AstAlias(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : જાહેર Ast_ગાંઠ {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
જાહેર:
    AstAliasScope(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : જાહેર Ast_ગાંઠ {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
જાહેર:
    AstBind(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Bind Target name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
};
class AstCFunc final : જાહેર Ast_ગાંઠ {
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
    પૂર્ણાંક m_cost;  // Function call cost
જાહેર:
    AstCFunc(File_લીટી* fl, અચલ string& name, AstScope* scopep, અચલ string& rtnType = "")
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
        m_cost = v3Global.opt.instr_ગણતરી_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstCFunc* અચલ asamep = VN_DBG_AS(samep, CFunc);
        પાછા ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા m_cost; }
    VBoolOrUnknown isConst() અચલ { પાછા m_isConst; }
    શૂન્ય isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    શૂન્ય isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() અચલ { પાછા m_isStatic; }
    શૂન્ય isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() અચલ VL_MT_SAFE { પાછા m_isTrace; }
    શૂન્ય isTrace(bool flag) { m_isTrace = flag; }
    શૂન્ય cname(અચલ string& name) { m_cname = name; }
    string cname() અચલ { પાછા m_cname; }
    AstScope* scopep() અચલ { પાછા m_scopep; }
    શૂન્ય scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() અચલ { પાછા ((m_rtnType == "") ? "void" : m_rtnType); }
    શૂન્ય rtnType(અચલ string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() અચલ { પાછા m_dontCombine || isTrace() || entryPoint(); }
    શૂન્ય dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() અચલ { પાછા m_declPrivate; }
    શૂન્ય declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() અચલ VL_MT_SAFE { પાછા m_keepIfEmpty; }
    શૂન્ય keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() અચલ VL_MT_SAFE { પાછા m_slow; }
    શૂન્ય slow(bool flag) { m_slow = flag; }
    bool funcPublic() અચલ { પાછા m_funcPublic; }
    શૂન્ય funcPublic(bool flag) { m_funcPublic = flag; }
    શૂન્ય argTypes(અચલ string& str) { m_argTypes = str; }
    string argTypes() અચલ { પાછા m_argTypes; }
    શૂન્ય ifdef(અચલ string& str) { m_ifdef = str; }
    string ifdef() અચલ { પાછા m_ifdef; }
    bool isConstructor() અચલ { પાછા m_isConstructor; }
    શૂન્ય isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() અચલ { પાછા m_isDestructor; }
    શૂન્ય isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() અચલ { પાછા m_isMethod; }
    શૂન્ય isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() અચલ { પાછા m_isLoose; }
    શૂન્ય isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() અચલ { પાછા isMethod() && !isLoose(); }
    bool isVirtual() અચલ { પાછા m_isVirtual; }
    શૂન્ય isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() અચલ { પાછા m_needProcess; }
    શૂન્ય setNeedProcess() { m_needProcess = true; }
    bool entryPoint() અચલ { પાછા m_entryPoint; }
    શૂન્ય entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() અચલ { પાછા m_dpiPure; }
    શૂન્ય dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() અચલ { પાછા m_dpiContext; }
    શૂન્ય dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() અચલ VL_MT_SAFE { પાછા m_dpiExportDispatcher; }
    શૂન્ય dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() અચલ { પાછા m_dpiExportImpl; }
    શૂન્ય dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() અચલ VL_MT_SAFE { પાછા m_dpiImportPrototype; }
    શૂન્ય dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() અચલ { પાછા m_dpiImportWrapper; }
    શૂન્ય dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() અચલ { પાછા !m_dpiCDecl.empty(); }
    અચલ string& dpiCDecl() અચલ { પાછા m_dpiCDecl; }
    શૂન્ય dpiCDecl(અચલ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() અચલ { પાછા m_rtnType == "VlCoroutine"; }
    શૂન્ય recursive(bool flag) { m_recursive = flag; }
    bool recursive() અચલ { પાછા m_recursive; }
    શૂન્ય unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() અચલ override { પાછા m_unlikely; }  // Note virtual override
    શૂન્ય noLife(bool flag) { m_noLife = flag; }
    bool noLife() અચલ { પાછા m_noLife; }
    bool isCovergroupSample() અચલ { પાછા m_isCovergroupSample; }
    શૂન્ય isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() અચલ { પાછા m_voidSelf; }
    શૂન્ય voidSelf(bool flag) { m_voidSelf = flag; }
    શૂન્ય cost(પૂર્ણાંક cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() અચલ {
        પાછા !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : જાહેર Ast_ગાંઠ {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    અચલ string m_name;
    અચલ VUseType m_useType;  // What sort of use this is

જાહેર:
    AstCUse(File_લીટી* fl, VUseType useType, અચલ string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    VUseType useType() અચલ { પાછા m_useType; }
};
class AstCell final : જાહેર Ast_ગાંઠ {
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
    File_લીટી* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    પૂર્ણાંક m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
જાહેર:
    AstCell(File_લીટી* fl, File_લીટી* mfl, અચલ string& instName, અચલ string& modName,
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
    શૂન્ય cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    // ACCESSORS
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Cell name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string origName() અચલ override { પાછા m_origName; }  // * = Original name
    શૂન્ય origName(અચલ string& name) { m_origName = name; }
    string modName() અચલ { પાછા m_modName; }  // * = Instance name
    શૂન્ય modName(અચલ string& name) { m_modName = name; }
    File_લીટી* modNameFileline() અચલ { પાછા m_modNameFileline; }
    Ast_ગાંઠ_Module* modp() અચલ { પાછા m_modp; }  // [AfterLink] = Pointer to module instantiated
    શૂન્ય modp(Ast_ગાંઠ_Module* nodep) { m_modp = nodep; }
    પૂર્ણાંક arrayIdx() અચલ { પાછા m_arrayIdx; }
    શૂન્ય arrayIdx(પૂર્ણાંક idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() અચલ { પાછા m_hasIfaceVar; }
    શૂન્ય hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    શૂન્ય trace(bool flag) { m_trace = flag; }
    bool isTrace() અચલ { પાછા m_trace; }
    શૂન્ય recursive(bool flag) { m_recursive = flag; }
    bool recursive() અચલ { પાછા m_recursive; }
};
class AstCellInline final : જાહેર Ast_ગાંઠ {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    અચલ std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    અચલ std::string m_origModName;  // __BEGIN__ or __FORK__
જાહેર:
    AstCellInline(File_લીટી* fl, અચલ std::string& name, અચલ std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    // ACCESSORS
    std::string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    std::string origModName() અચલ { પાછા m_origModName; }
};
class AstCellInlineScope final : જાહેર Ast_ગાંઠ {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    અચલ std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
જાહેર:
    AstCellInlineScope(File_લીટી* fl, અચલ std::string& name, અચલ std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    // ACCESSORS
    std::string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ std::string& name) override { m_name = name; }
    std::string origModName() અચલ { પાછા m_origModName; }
};
class AstCgOptionAssign final : જાહેર Ast_ગાંઠ {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    અચલ VCoverOptionType m_optType;  // Option type
    અચલ string m_name;  // Original option name (for diagnostics on unknown options)
    અચલ bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
જાહેર:
    AstCgOptionAssign(File_લીટી* fl, bool typeOption, VCoverOptionType optType,
                      અચલ string& rawName, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    VCoverOptionType optType() અચલ { પાછા m_optType; }
    bool typeOption() અચલ { પાછા m_typeOption; }
};
class AstClassExtends final : જાહેર Ast_ગાંઠ {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    અચલ bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

જાહેર:
    AstClassExtends(File_લીટી* fl, Ast_ગાંઠ* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    string verilogKwd() અચલ override { પાછા isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() અચલ;
    AstClass* classp() અચલ;  // Like above, but throws error if nulll
    bool isImplements() અચલ { પાછા m_isImplements; }
    શૂન્ય parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() અચલ { પાછા m_parameterized; }
};
class AstClocking final : જાહેર Ast_ગાંઠ {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    અચલ bool m_isGlobal;  // True if global clocking

જાહેર:
    AstClocking(File_લીટી* fl, અચલ std::string& name, Ast_Sen_વૃક્ષ* sentreep, Ast_ગાંઠ* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    std::string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    bool isDefault() અચલ { પાછા m_isDefault; }
    bool isGlobal() અચલ { પાછા m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    શૂન્ય makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : જાહેર Ast_ગાંઠ {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

જાહેર:
    AstClockingItem(File_લીટી* fl, VDirection direction, Ast_ગાંઠ_પદાવલિ* skewp, Ast_ગાંઠ* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        જો (AstAssign* અચલ clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } નહીંતર {
            exprp(VN_AS(clockingDeclp, ગાંઠ_પદાવલિ));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    VDirection direction() અચલ { પાછા m_direction; }
    AstClockingItem* outputp() અચલ { પાછા m_outputp; }
    શૂન્ય outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
};
class AstConfig final : જાહેર Ast_ગાંઠ {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

જાહેર:
    AstConfig(File_લીટી* fl, અચલ std::string& libname, અચલ std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    std::string name() અચલ override VL_MT_STABLE { પાછા m_libname + "." + m_configname; }
    std::string libname() અચલ VL_MT_STABLE { પાછા m_libname; }
    std::string configname() અચલ VL_MT_STABLE { પાછા m_configname; }
};
class AstConfigCell final : જાહેર Ast_ગાંઠ {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

જાહેર:
    AstConfigCell(File_લીટી* fl, અચલ std::string& libname, અચલ std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() અચલ override VL_MT_STABLE { પાછા m_libname + "." + m_cellname; }
    std::string libname() અચલ VL_MT_STABLE { પાછા m_libname; }
    std::string cellname() અચલ VL_MT_STABLE { પાછા m_cellname; }
};
class AstConfigRule final : જાહેર Ast_ગાંઠ {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    અચલ bool m_isCell;  // Declared as "cell" versus "instance"

જાહેર:
    AstConfigRule(File_લીટી* fl, Ast_ગાંઠ* cellp, Ast_ગાંઠ* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() અચલ VL_MT_STABLE { પાછા m_isCell; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstConfigUse final : જાહેર Ast_ગાંઠ {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    અચલ bool m_isConfig;  // ":config"; Config, not module/primitive name

જાહેર:
    AstConfigUse(File_લીટી* fl, અચલ std::string& libname, અચલ std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() અચલ override VL_MT_STABLE { પાછા m_libname + "." + m_cellname; }
    std::string libname() અચલ VL_MT_STABLE { પાછા m_libname; }
    std::string cellname() અચલ VL_MT_STABLE { પાછા m_cellname; }
    bool isConfig() અચલ VL_MT_STABLE { પાછા m_isConfig; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstConstraint final : જાહેર Ast_ગાંઠ {
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
જાહેર:
    AstConstraint(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Scope name
    શૂન્ય name(અચલ string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    શૂન્ય baseOverride(અચલ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() અચલ { પાછા m_baseOverride; }
    bool isExternDef() અચલ { પાછા m_isExternDef; }
    શૂન્ય isExternDef(bool flag) { m_isExternDef = flag; }
    શૂન્ય isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() અચલ { પાછા m_isExternExplicit; }
    શૂન્ય isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() અચલ { પાછા m_isExternProto; }
    શૂન્ય isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() અચલ { પાછા m_isKwdPure; }
    શૂન્ય isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() અચલ { પાછા m_isStatic; }
};
class AstCoverBin final : જાહેર Ast_ગાંઠ {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    અચલ string m_name;  // Base name of the bin
    અચલ VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

જાહેર:
    AstCoverBin(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_ગાંઠ_પદાવલિ* iffp = nullptr)
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
    AstCoverBin(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ_પદાવલિ* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_લીટી* fl, અચલ string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_લીટી* fl, અચલ string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    VCoverBinsType binsType() અચલ { પાછા m_binsType; }
    bool isWildcard() અચલ { પાછા m_isWildcard; }
    bool isArray() અચલ { પાછા m_isArray; }
    શૂન્ય isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : જાહેર Ast_ગાંઠ {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    અચલ bool m_isNegated;  // Complement the selection within the cross product

જાહેર:
    AstCoverBinsof(File_લીટી* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_ગાંઠ* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    bool isNegated() અચલ { પાછા m_isNegated; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {  // LCOV_EXCL_START
        અચલ AstCoverBinsof* અચલ asamep = VN_DBG_AS(samep, CoverBinsof);
        પાછા m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : જાહેર Ast_ગાંઠ {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    અચલ string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    અચલ VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

જાહેર:
    AstCoverCrossBin(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* selectp, Ast_ગાંઠ_પદાવલિ* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    string verilogKwd() અચલ override;
    VCoverBinsType binsType() અચલ { પાછા m_binsType; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {  // LCOV_EXCL_START
        અચલ AstCoverCrossBin* અચલ asamep = VN_DBG_AS(samep, CoverCrossBin);
        પાછા m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : જાહેર Ast_ગાંઠ {
    // The complete bin-tuple space of the enclosing cross
    અચલ string m_name;  // Referenced cross name

જાહેર:
    AstCoverCrossRef(File_લીટી* fl, અચલ string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {  // LCOV_EXCL_START
        પાછા m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : જાહેર Ast_ગાંઠ {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    અચલ bool m_isOr;  // Union (||), rather than intersection (&&)

જાહેર:
    AstCoverCrossSelect(File_લીટી* fl, Ast_ગાંઠ* lhsp, Ast_ગાંઠ* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool isOr() અચલ { પાછા m_isOr; }
    string verilogKwd() અચલ override { પાછા isOr() ? "||" : "&&"; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {  // LCOV_EXCL_START
        પાછા m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : જાહેર Ast_ગાંઠ {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
જાહેર:
    AstCoverCrossWith(File_લીટી* fl, Ast_ગાંઠ* selectp, Ast_ગાંઠ_પદાવલિ* filterp, Ast_ગાંઠ_પદાવલિ* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : જાહેર Ast_ગાંઠ {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    અચલ VCoverOptionType m_optType;  // Option being assigned
    અચલ bool m_typeOption;  // type_option (vs option)

જાહેર:
    AstCoverOption(File_લીટી* fl, bool typeOption, VCoverOptionType optType, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    VCoverOptionType optType() અચલ { પાછા m_optType; }
    bool typeOption() અચલ { પાછા m_typeOption; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {  // LCOV_EXCL_START
        અચલ AstCoverOption* અચલ asamep = VN_DBG_AS(samep, CoverOption);
        પાછા m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : જાહેર Ast_ગાંઠ {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
જાહેર:
    AstCoverToggle(File_લીટી* fl, AstCoverInc* incp, Ast_ગાંઠ_પદાવલિ* origp, Ast_ગાંઠ_પદાવલિ* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા 3 + INSTR_ગણતરી_BRANCH + INSTR_ગણતરી_LD; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા true; }
    bool isOutputter() override {
        પાછા false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : જાહેર Ast_ગાંઠ {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    અચલ VTransRepType m_repType;

જાહેર:
    AstCoverTransItem(File_લીટી* fl, Ast_ગાંઠ* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstCoverTransSet final : જાહેર Ast_ગાંઠ {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
જાહેર:
    AstCoverTransSet(File_લીટી* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstCoverWith final : જાહેર Ast_ગાંઠ {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
જાહેર:
    AstCoverWith(File_લીટી* fl, Ast_ગાંઠ* subp, Ast_ગાંઠ_પદાવલિ* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        addSubp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : જાહેર Ast_ગાંઠ {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

જાહેર:
    AstCovergroup(File_લીટી* fl, અચલ string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_ગાંઠ* membersp, Ast_Sen_વૃક્ષ* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    bool maybePointedTo() અચલ override { પાછા true; }
};
class AstCoverpointRef final : જાહેર Ast_ગાંઠ {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    અચલ string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

જાહેર:
    AstCoverpointRef(File_લીટી* fl, અચલ string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
};
class AstDefParam final : જાહેર Ast_ગાંઠ {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
જાહેર:
    AstDefParam(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* pathp, અચલ string& name, Ast_ગાંઠ_પદાવલિ* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા true; }
};
class AstDefaultClocking final : જાહેર Ast_ગાંઠ {
    std::string m_name;  // Clocking block name

જાહેર:
    AstDefaultClocking(File_લીટી* fl, અચલ std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() અચલ override VL_MT_STABLE { પાછા m_name; }
};
class AstDefaultDisable final : જાહેર Ast_ગાંઠ {
    // @astgen op1 := condp : AstNodeExpr

જાહેર:
    AstDefaultDisable(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : જાહેર Ast_ગાંઠ {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
જાહેર:
    AstDpiExport(File_લીટી* fl, અચલ string& vname, અચલ string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string cname() અચલ { પાછા m_cname; }
    શૂન્ય cname(અચલ string& cname) { m_cname = cname; }
};
class AstElabDisplay final : જાહેર Ast_ગાંઠ {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

જાહેર:
    અંતઃરેખ AstElabDisplay(File_લીટી* fl, VDisplayType dispType, Ast_ગાંઠ_પદાવલિ* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!fmtp());
        પાછા nullptr;
    }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string verilogKwd() અચલ override { પાછા "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() અચલ override { પાછા false; }
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { પાછા true; }  // SPECIAL: $display makes output
    bool isUnlikely() અચલ override { પાછા true; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    પૂર્ણાંક instr_ગણતરી() અચલ override { પાછા INSTR_ગણતરી_PLI; }
    VDisplayType displayType() અચલ { પાછા m_displayType; }
    શૂન્ય displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : જાહેર Ast_ગાંઠ {
    // Represents something missing, e.g. a missing argument in FOREACH
જાહેર:
    explicit AstEmpty(File_લીટી* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstGenCaseItem final : જાહેર Ast_ગાંઠ {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
જાહેર:
    AstGenCaseItem(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condsp, Ast_ગાંઠ* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() અચલ { પાછા condsp() == nullptr; }
};
class AstImplicit final : જાહેર Ast_ગાંઠ {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
જાહેર:
    AstImplicit(File_લીટી* fl, Ast_ગાંઠ* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : જાહેર Ast_ગાંઠ {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
જાહેર:
    AstIntfRef(File_લીટી* fl, અચલ string& name, અચલ string& parentName, અચલ string& baseName,
               અચલ string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    string parentName() અચલ { પાછા m_parentName; }
    string baseName() અચલ { પાછા m_baseName; }
    string modportName() અચલ { પાછા m_modportName; }
};
class AstLibrary final : જાહેર Ast_ગાંઠ {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

જાહેર:
    AstLibrary(File_લીટી* fl, અચલ std::string& name, Ast_ગાંઠ_પદાવલિ* filesp, Ast_ગાંઠ_પદાવલિ* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() અચલ override VL_MT_STABLE { પાછા m_name; }
};
class AstModport final : જાહેર Ast_ગાંઠ {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
જાહેર:
    AstModport(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() અચલ override { પાછા "modport"; }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : જાહેર Ast_ગાંઠ {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
જાહેર:
    AstModportClockingRef(File_લીટી* fl, અચલ string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    શૂન્ય dump(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    AstClocking* clockingp() અચલ VL_MT_STABLE {
        પાછા m_clockingp;
    }  // [After Link] Pointer to clocking block
    શૂન્ય clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : જાહેર Ast_ગાંઠ {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
જાહેર:
    AstModportFTaskRef(File_લીટી* fl, અચલ string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    bool isImport() અચલ { પાછા !m_export; }
    bool isExport() અચલ { પાછા m_export; }
    Ast_ગાંઠ_FTask* ftaskp() અચલ { પાછા m_ftaskp; }  // [After Link] Pointer to variable
    શૂન્ય ftaskp(Ast_ગાંઠ_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : જાહેર Ast_ગાંઠ {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    અચલ VDirection m_direction;  // Direction of the variable (in/out)
જાહેર:
    AstModportVarRef(File_લીટી* fl, અચલ string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ_પદાવલિ* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    VDirection direction() અચલ { પાછા m_direction; }
    AstVar* varp() અચલ VL_MT_STABLE { પાછા m_varp; }  // [After Link] Pointer to variable
    શૂન્ય varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : જાહેર Ast_ગાંઠ {
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
    std::string m_resolved_ટોચ_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::set<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<અચલ AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<અચલ Ast_ગાંઠ*, અચલ Ast_ગાંઠ_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

જાહેર:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    અચલ અક્ષર* broken() અચલ override;
    શૂન્ય ધકેલો_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.ઉમેરો(varp); }
    અચલ std::set<AstVar*>& deferredParamVarps() અચલ { પાછા m_deferredParamVarps; }
    શૂન્ય clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    શૂન્ય deleteContents();
    શૂન્ય cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() અચલ override VL_MT_STABLE { પાછા "$root"; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    Ast_ગાંઠ_Module* ટોચ_Modulep() અચલ VL_MT_STABLE {  // Top module in hierarchy
        પાછા modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { પાછા m_typeTablep; }
    string astConstOrigParamName(અચલ AstConst* nodep) અચલ;
    શૂન્ય astConstOrigParamName(અચલ AstConst* nodep, અચલ string& name);
    શૂન્ય astConstOrigParamNameErase(અચલ AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    અચલ Ast_ગાંઠ_Module* containingModule(અચલ Ast_ગાંઠ* nodep);
    // Forget remembered modules, as the tree has moved.
    શૂન્ય clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() અચલ { પાછા m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() અચલ { પાછા m_dollarUnitPkgp; }
    શૂન્ય dollarUnitPkgp(AstPackage* અચલ packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() અચલ { પાછા m_dpiExportTriggerp; }
    શૂન્ય dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() અચલ { પાછા m_delaySchedulerp; }
    શૂન્ય delaySchedulerp(AstVar* અચલ varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() અચલ { પાછા m_nbaEventp; }
    શૂન્ય nbaEventp(AstVarScope* અચલ varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() અચલ { પાછા m_nbaEventTriggerp; }
    શૂન્ય nbaEventTriggerp(AstVarScope* અચલ varScopep) { m_nbaEventTriggerp = varScopep; }
    શૂન્ય stdPackagep(AstPackage* અચલ packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() અચલ { પાછા m_stdPackagep; }
    શૂન્ય stdPackageProcessp(AstClass* અચલ classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() અચલ { પાછા m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_લીટી*) અચલ;
    Ast_ટોચ_Scope* ટોચ_Scopep() અચલ { પાછા m_ટોચ_Scopep; }
    શૂન્ય create_ટોચ_Scope(AstScope* scopep);
    VTimescale timeunit() અચલ { પાછા m_timeunit; }
    શૂન્ય timeunit(અચલ VTimescale& કિંમત) { m_timeunit = કિંમત; }
    VTimescale timeprecision() અચલ { પાછા m_timeprecision; }
    શૂન્ય timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    શૂન્ય timeprecisionMerge(File_લીટી*, અચલ VTimescale& કિંમત);
    શૂન્ય timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() અચલ { પાછા m_timescaleSpecified; }
    અચલ std::string& resolved_ટોચ_Module_Name() અચલ { પાછા m_resolved_ટોચ_Module_Name; }
    શૂન્ય resolved_ટોચ_Module_Name(અચલ std::string& કિંમત) { m_resolved_ટોચ_Module_Name = કિંમત; }
    uint32_t nTraceCodes() અચલ { પાછા m_nTraceCodes; }
    શૂન્ય nTraceCodes(uint32_t કિંમત) { m_nTraceCodes = કિંમત; }
    AstVarScope* stlFirstIterationp();
    શૂન્ય clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    અચલ std::string trace_Lib_ટોચ_Name() અચલ {
        અચલ std::string& name = resolved_ટોચ_Module_Name();
        પાછા prettyName(name.empty() ? v3Global.rootp()->ટોચ_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    શૂન્ય addEvalStats(અચલ std::string& phase);
};
class AstPackageExport final : જાહેર Ast_ગાંઠ {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

જાહેર:
    AstPackageExport(File_લીટી* fl, અચલ string& pkgName, અચલ string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    string pkgName() અચલ VL_MT_STABLE { પાછા m_pkgName; }
    string prettyPkgNameQ() અચલ { પાછા "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() અચલ { પાછા m_packagep; }
    શૂન્ય packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : જાહેર Ast_ગાંઠ {
    // A package export *::* declaration
જાહેર:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_લીટી* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : જાહેર Ast_ગાંઠ {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

જાહેર:
    AstPackageImport(File_લીટી* fl, AstPackage* packagep, અચલ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_લીટી* fl, અચલ string& pkgName, અચલ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    string pkgName() અચલ VL_MT_STABLE { પાછા m_pkgName; }
    string prettyPkgNameQ() અચલ { પાછા "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() અચલ { પાછા m_packagep; }
    શૂન્ય packagep(AstPackage* nodep) { m_packagep = nodep; }

ખાનગી:
    શૂન્ય pkgNameFrom();
};
class AstPin final : જાહેર Ast_ગાંઠ {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    પૂર્ણાંક m_pin_અંક;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
જાહેર:
    AstPin(File_લીટી* fl, પૂર્ણાંક pin_અંક, અચલ string& name, Ast_ગાંઠ* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_અંક{pin_અંક}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    શૂન્ય cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Pin name, ""=go by number
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string prettyOperatorName() અચલ override;
    bool dotStar() અચલ { પાછા name() == ".*"; }  // Fake name for .* connections until linked
    પૂર્ણાંક pin_અંક() અચલ { પાછા m_pin_અંક; }
    AstVar* modVarp() અચલ { પાછા m_modVarp; }  // [After Link] Pointer to variable
    શૂન્ય modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() અચલ { પાછા m_modPTypep; }
    શૂન્ય modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() અચલ { પાછા m_param; }
    શૂન્ય param(bool flag) { m_param = flag; }
    અચલ string& paramPath() અચલ { પાછા m_paramPath; }
    શૂન્ય paramPath(અચલ string& path) { m_paramPath = path; }
    bool svDotName() અચલ { પાછા m_svDotName; }
    શૂન્ય svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() અચલ { પાછા m_svImplicit; }
    શૂન્ય svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : જાહેર Ast_ગાંઠ {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    અચલ પૂર્ણાંક m_pin_અંક;  // Pin number
    અચલ string m_name;  // Name of pin
જાહેર:
    AstPort(File_લીટી* fl, પૂર્ણાંક pinnum, અચલ string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_અંક{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Port name
    પૂર્ણાંક pin_અંક() અચલ { પાછા m_pin_અંક; }  // * = Pin number, for order based instantiation
};
class AstPragma final : જાહેર Ast_ગાંઠ {
    અચલ VPragmaType m_pragType;  // Type of pragma
    અચલ VTimescale m_timescale;  // For TIMEUNIT_SET
જાહેર:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_લીટી* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_લીટી* fl, અચલ VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    VPragmaType pragType() અચલ { પાછા m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() અચલ override { પાછા false; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() અચલ { પાછા m_timescale; }
};
class AstPropSpec final : જાહેર Ast_ગાંઠ {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

જાહેર:
    AstPropSpec(File_લીટી* fl, AstSenItem* sensesp, Ast_ગાંઠ_પદાવલિ* disablep, Ast_ગાંઠ* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool hasDType() અચલ override VL_MT_SAFE {
        પાછા true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() અચલ { પાછા m_propStrength; }
};
class AstPull final : જાહેર Ast_ગાંઠ {
    // @astgen op1 := lhsp : AstNodeExpr

    અચલ bool m_direction;

જાહેર:
    AstPull(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() અચલ { પાછા m_direction; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstScope final : જાહેર Ast_ગાંઠ {
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
જાહેર:
    AstScope(File_લીટી* fl, Ast_ગાંઠ_Module* modp, અચલ string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!m_modp);
        પાછા nullptr;
    }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Scope name
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    string pretty_Name_સંદેશ() અચલ override;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override;
    string nameDotless() અચલ;
    Ast_ગાંઠ_Module* modp() અચલ { પાછા m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() અચલ VL_MT_SAFE { પાછા m_aboveScopep; }
    શૂન્ય aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() અચલ { પાછા m_aboveCellp; }
    શૂન્ય aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_ટોચ() અચલ VL_MT_SAFE { પાછા aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : જાહેર Ast_ગાંઠ {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_edgeType;  // Edge type
જાહેર:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    AstSenItem(File_લીટી* fl, VEdgeType edgeType, Ast_ગાંઠ_પદાવલિ* senp, Ast_ગાંઠ_પદાવલિ* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_લીટી* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_લીટી* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_લીટી* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_લીટી* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_લીટી* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_લીટી* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() અચલ { પાછા m_edgeType; }
    શૂન્ય edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_ગણતરી_Inc();
    }
    Ast_ગાંઠ_Var_Ref* varrefp() અચલ { પાછા VN_CAST(sensp(), ગાંઠ_Var_Ref); }
    //
    bool isClocked() અચલ { પાછા edgeType().clockedStmt(); }
    bool isComboOrStar() અચલ {
        પાછા edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() અચલ { પાછા edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() અચલ { પાછા edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() અચલ { પાછા edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() અચલ { પાછા edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() અચલ { પાછા edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() અચલ { પાછા edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_વૃક્ષ final : જાહેર Ast_ગાંઠ {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
જાહેર:
    Ast_Sen_વૃક્ષ(File_લીટી* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_વૃક્ષ(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_વૃક્ષ;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    // METHODS
    bool hasClocked() અચલ;  // Includes a clocked statement
    bool hasEdge() અચલ;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() અચલ;  // Includes a STATIC SenItem
    bool hasInitial() અચલ;  // Includes a INITIAL SenItem
    bool hasFinal() અચલ;  // Includes a FINAL SenItem
    bool hasCombo() અચલ;  // Includes a COMBO SenItem
    bool hasHybrid() અચલ;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : જાહેર Ast_ગાંઠ {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

જાહેર:
    AstStrengthSpec(File_લીટી* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { પાછા m_s0; }
    VStrength strength1() { પાછા m_s1; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstSystemCSection final : જાહેર Ast_ગાંઠ {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    અચલ VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

જાહેર:
    AstSystemCSection(File_લીટી* fl, VSystemCSectionType sectionType, અચલ std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() અચલ { પાછા m_sectionType; }
    અચલ std::string& text() અચલ { પાછા m_text; }
    શૂન્ય text(અચલ std::string& કિંમત) { m_text = કિંમત; }
    શૂન્ય dump(std::ostream&) અચલ override;
    શૂન્ય dumpJson(std::ostream&) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ*) અચલ override { પાછા false; }
    bool isPure() override { પાછા false; }
    bool isOutputter() override { પાછા true; }
};
class AstText final : જાહેર Ast_ગાંઠ {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    અચલ std::string m_text;  // The text to emit
જાહેર:
    AstText(File_લીટી* fl, અચલ std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા text() == VN_DBG_AS(samep, Text)->text();
    }
    અચલ std::string& text() અચલ VL_MT_SAFE { પાછા m_text; }
};
class AstTextBlock final : જાહેર Ast_ગાંઠ {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    અચલ std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    અચલ std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    અચલ std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
જાહેર:
    explicit AstTextBlock(File_લીટી* fl,  //
                          અચલ std::string& prefix = "",  //
                          અચલ std::string& separator = "",  //
                          અચલ std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    અચલ std::string& prefix() અચલ { પાછા m_prefix; }
    અચલ std::string& separator() અચલ { પાછા m_separator; }
    અચલ std::string& suffix() અચલ { પાછા m_suffix; }
    // Add some text, or a node to this block
    શૂન્ય add(અચલ string& text) { addNodesp(new AstText{fileline(), text}); }
    શૂન્ય add(Ast_ગાંઠ* nodep) { addNodesp(nodep); }
};
class Ast_ટોચ_Scope final : જાહેર Ast_ગાંઠ {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    friend class AstNetlist;  // Only the AstNetlist can create one
    Ast_ટોચ_Scope(File_લીટી* fl, AstScope* ascopep)
        : ASTGEN_SUPER_ટોચ_Scope(fl) {
        this->scopep(ascopep);
    }

જાહેર:
    ASTGEN_MEMBERS_Ast_ટોચ_Scope;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* createTemp(અચલ string& name, unsigned width);
    AstVarScope* createTemp(અચલ string& name, Ast_ગાંઠ_DType* dtypep);
};
class AstTypeTable final : જાહેર Ast_ગાંઠ {
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
    using DetailedMap = std::map<VBasic_Type_ચાવી, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

જાહેર:
    explicit AstTypeTable(File_લીટી* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    શૂન્ય cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_લીટી* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_લીટી* fl, VBasicDTypeKwd kwd, પૂર્ણાંક width, પૂર્ણાંક widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_લીટી* fl, VBasicDTypeKwd kwd, અચલ VNumRange& range,
                                     પૂર્ણાંક widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& ગાંઠ);
    AstBasicDType* find_ઉમેરો_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_લીટી* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_લીટી* fl);
    AstQueueDType* findQueueIndexDType(File_લીટી* fl);
    AstStreamDType* findStreamDType(File_લીટી* fl);
    AstVoidDType* findVoidDType(File_લીટી* fl);
    શૂન્ય clearCache();
    શૂન્ય repairCache();
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
};
class AstTypedef final : જાહેર Ast_ગાંઠ {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_અંક;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

જાહેર:
    AstTypedef(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* attrsp, VFlagChildDType,
               Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_અંક{fl->token_અંક()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_અંક{fl->token_અંક()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() અચલ;
    Ast_ગાંઠ_DType* getChildDTypep() અચલ override { પાછા childDTypep(); }
    આભાસી Ast_ગાંઠ_DType* subDTypep() અચલ VL_MT_STABLE {
        પાછા dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_અંક() અચલ override { પાછા m_decl_Token_અંક; }
    શૂન્ય decl_Token_અંક_Set_Min(uint32_t token_અંક) override {
        m_decl_Token_અંક = std::min(m_decl_Token_અંક, token_અંક);
    }
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    શૂન્ય name(અચલ string& flag) override { m_name = flag; }
    bool attrPublic() અચલ { પાછા m_attrPublic; }
    શૂન્ય attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() અચલ { પાછા m_isHideLocal; }
    શૂન્ય isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() અચલ { પાછા m_isHideProtected; }
    શૂન્ય isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() અચલ { પાછા m_isUnderClass; }
    શૂન્ય isUnderClass(bool flag) { m_isUnderClass = flag; }
    શૂન્ય tag(અચલ string& text) override { m_tag = text; }
    string tag() અચલ override { પાછા m_tag; }
};
class AstTypedefFwd final : જાહેર Ast_ગાંઠ {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    અચલ VFwdType m_fwdType;  // Forward type for lint check

જાહેર:
    AstTypedefFwd(File_લીટી* fl, અચલ string& name, અચલ VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    VFwdType fwdType() અચલ { પાછા m_fwdType; }
};
class AstUdpTable final : જાહેર Ast_ગાંઠ {
    // @astgen op1 := linesp : List[AstUdpTableLine]
જાહેર:
    AstUdpTable(File_લીટી* fl, Ast_Udp_Table_લીટી* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        જો (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_લીટી final : જાહેર Ast_ગાંઠ {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
ખાનગી:
    અચલ bool m_udpIsCombo;  // Combinational or sequential UDP

જાહેર:
    class UdpCombo {};
    Ast_Udp_Table_લીટી(UdpCombo, File_લીટી* fl, Ast_Udp_Table_લીટી_Val* iFieldsp,
                    Ast_Udp_Table_લીટી_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_લીટી(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_લીટી(UdpSequential, File_લીટી* fl, Ast_Udp_Table_લીટી_Val* iFieldsp,
                    Ast_Udp_Table_લીટી_Val* oFieldsp1, Ast_Udp_Table_લીટી_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_લીટી(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_લીટી;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક udpIsCombo() અચલ { પાછા m_udpIsCombo; }
};
class Ast_Udp_Table_લીટી_Val final : જાહેર Ast_ગાંઠ {
    string m_text;  // Value character

જાહેર:
    Ast_Udp_Table_લીટી_Val(File_લીટી* fl, અચલ string& text)
        : ASTGEN_SUPER_Udp_Table_લીટી_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_લીટી_Val;
    string name() અચલ override VL_MT_STABLE { પાછા m_text; }
    શૂન્ય name(std::string અચલ& text) override VL_MT_STABLE { m_text = text; }
    string text() અચલ VL_MT_SAFE { પાછા m_text; }
};
class AstVar final : જાહેર Ast_ગાંઠ {
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
    પૂર્ણાંક m_pin_અંક = 0;  // For JSON, if non-zero the connection pin number
    પૂર્ણાંક m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
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
    bool m_mtask_Cache_લીટી_Align : 1;  // Start MTask affinity group on a cache line
    શૂન્ય init() {
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
        m_mtask_Cache_લીટી_Align = false;
    }

જાહેર:
    AstVar(File_લીટી* fl, VVarType type, અચલ string& name, VFlagChildDType, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_લીટી* fl, VVarType type, અચલ string& name, Ast_ગાંઠ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_લીટી* fl, VVarType type, અચલ string& name, VFlagLogicPacked, પૂર્ણાંક wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_લીટી* fl, VVarType type, અચલ string& name, VFlagBitPacked, પૂર્ણાંક wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_લીટી* fl, VVarType type, અચલ string& name, અચલ AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        જો (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_વૃક્ષ(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    અંતઃરેખ bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override;
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }  // * = Var name
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    string origName() અચલ override { પાછા m_origName; }  // * = Original name
    શૂન્ય origName(અચલ string& name) { m_origName = name; }
    VVarType varType() અચલ VL_MT_SAFE { પાછા m_varType; }  // * = Type of variable
    શૂન્ય direction(અચલ VDirection& flag) {
        m_direction = flag;
        જો (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() અચલ VL_MT_SAFE { પાછા m_direction; }
    bool isIO() અચલ VL_MT_SAFE { પાછા m_direction != VDirection::NONE; }
    bool isVLIO() અચલ {
        અચલ AstBasicDType* અચલ bdtypep = basicp();
        પાછા isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    શૂન્ય declDirection(અચલ VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() અચલ { પાછા m_declDirection; }
    પૂર્ણાંક libInoutId() અચલ { પાછા m_libInoutId; }
    શૂન્ય libInoutId(પૂર્ણાંક id) { m_libInoutId = id; }
    bool libInoutEnable() અચલ { પાછા m_libInoutEnable; }
    શૂન્ય libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    શૂન્ય varType(VVarType type) { m_varType = type; }
    શૂન્ય varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    શૂન્ય varType2In() {
        m_tristate = false;
        m_direction = VDirection::આગત;
    }
    string scType() અચલ;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) અચલ;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) અચલ;
    string dpiTmpVarType(અચલ string& varName) અચલ;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) અચલ;
    string vlEnumType() અચલ;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) અચલ;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(અચલ string& propName) અચલ;  // Return VerilatorVarProps declaration
    શૂન્ય combineType(VVarType type);
    Ast_ગાંઠ_DType* getChildDTypep() અચલ override { પાછા childDTypep(); }
    Ast_ગાંઠ_DType* dtypeSkipRefp() અચલ VL_MT_STABLE { પાછા subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() અચલ VL_MT_STABLE { પાછા subDTypep()->basicp(); }
    આભાસી Ast_ગાંઠ_DType* subDTypep() અચલ VL_MT_STABLE {
        પાછા dtypep() ? dtypep() : childDTypep();
    }
    શૂન્ય ansi(bool flag) { m_ansi = flag; }
    શૂન્ય declTyped(bool flag) { m_declTyped = flag; }
    શૂન્ય sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    શૂન્ય attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    શૂન્ય attrScBv(bool flag) { m_attrScBv = flag; }
    શૂન્ય attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    શૂન્ય attrSFormat(bool flag) { m_attrSFormat = flag; }
    શૂન્ય attrSplitVar(bool flag) { m_attrSplitVar = flag; }
    શૂન્ય attrFsmState(bool flag) { m_attrFsmState = flag; }
    શૂન્ય attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    શૂન્ય attrFsmStateExpand(અચલ VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() અચલ { પાછા m_constPoolEntry; }
    શૂન્ય setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() અચલ { પાછા m_covergroupRefMember; }
    શૂન્ય covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() અચલ { પાછા m_embeddedCovergroup; }
    શૂન્ય embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    શૂન્ય rand(અચલ VRandAttr flag) { m_rand = flag; }
    શૂન્ય usedParam(bool flag) { m_usedParam = flag; }
    શૂન્ય usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    શૂન્ય sigPublic(bool flag) { m_sigPublic = flag; }
    શૂન્ય sigModPublic(bool flag) { m_sigModPublic = flag; }
    શૂન્ય sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        જો (flag) sigPublic(true);
    }
    શૂન્ય sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        જો (flag) sigUserRdPublic(true);
    }
    શૂન્ય sc(bool flag) { m_sc = flag; }
    શૂન્ય scSensitive(bool flag) { m_scSensitive = flag; }
    શૂન્ય primaryIO(bool flag) { m_primaryIO = flag; }
    શૂન્ય isConst(bool flag) { m_isConst = flag; }
    શૂન્ય isContinuously(bool flag) { m_isContinuously = flag; }
    શૂન્ય isIfaceParent(bool flag) { m_isIfaceParent = flag; }
    શૂન્ય isIfaceArraySplit(bool flag) { m_isIfaceArraySplit = flag; }
    શૂન્ય isInternal(bool flag) { m_isInternal = flag; }
    શૂન્ય isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    શૂન્ય funcLocal(bool flag) {
        m_funcLocal = flag;
        જો (flag) m_funcLocalSticky = true;
    }
    શૂન્ય funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() અચલ { પાછા m_gotNansiType; }
    શૂન્ય gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() અચલ { પાછા m_hasStrengthAssignment; }
    શૂન્ય hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() અચલ { પાછા m_hasUserInit; }
    શૂન્ય hasUserInit(bool flag) { m_hasUserInit = flag; }
    શૂન્ય icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() અચલ { પાછા m_icoMaybeWritten; }
    શૂન્ય isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() અચલ { પાછા m_isClockingDriven; }
    bool isDpiOpenArray() અચલ VL_MT_SAFE { પાછા m_isDpiOpenArray; }
    શૂન્ય isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() અચલ { પાછા m_isHideLocal; }
    શૂન્ય isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() અચલ { પાછા m_isHideProtected; }
    શૂન્ય isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() અચલ { પાછા m_noCReset; }
    શૂન્ય noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() અચલ { પાછા m_noReset; }
    શૂન્ય noReset(bool flag) { m_noReset = flag; }
    bool noSubst() અચલ { પાછા m_noSubst; }
    શૂન્ય noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() અચલ { પાછા m_processQueue; }
    શૂન્ય processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() અચલ { પાછા m_sampled; }
    શૂન્ય sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() અચલ { પાછા m_substConstOnly; }
    શૂન્ય substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() અચલ { પાછા m_overriddenParam; }
    શૂન્ય overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() અચલ { પાછા m_untypedParam; }
    શૂન્ય untypedParam(bool flag) { m_untypedParam = flag; }
    શૂન્ય trace(bool flag) { m_trace = flag; }
    શૂન્ય isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() અચલ { પાછા m_isForceable; }
    શૂન્ય setForceable() { m_isForceable = true; }
    શૂન્ય setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() અચલ { પાછા m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() અચલ { પાછા m_isReadByDpi; }
    શૂન્ય setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() અચલ { પાછા m_isWrittenByDpi; }
    શૂન્ય setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() અચલ { પાછા m_isWrittenBySuspendable; }
    શૂન્ય setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() અચલ { પાછા m_ignorePostRead; }
    શૂન્ય setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() અચલ { પાછા m_ignorePostWrite; }
    શૂન્ય setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() અચલ { પાછા m_ignoreSchedWrite; }
    શૂન્ય setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() અચલ { પાછા m_dfgTriLowered; }
    શૂન્ય setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() અચલ { પાછા m_dfgAllowMultidriveTri; }
    શૂન્ય setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    શૂન્ય globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() અચલ { પાછા m_globalConstrained; }
    bool isStdRandomizeArg() અચલ { પાછા m_isStdRandomizeArg; }
    શૂન્ય setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    શૂન્ય name(અચલ string& name) override { m_name = name; }
    શૂન્ય tag(અચલ string& text) override { m_tag = text; }
    string tag() અચલ override { પાછા m_tag; }
    bool isAnsi() અચલ { પાછા m_ansi; }
    bool isContinuously() અચલ { પાછા m_isContinuously; }
    bool isDeclTyped() અચલ { પાછા m_declTyped; }
    bool isInout() અચલ { પાછા m_direction.isInout(); }
    bool isInoutOrRef() અચલ { પાછા m_direction.isInoutOrRef(); }
    bool is_આગત() અચલ { પાછા m_direction.is_આગત(); }
    bool isNonOutput() અચલ { પાછા m_direction.isNonOutput(); }
    bool isReadOnly() અચલ VL_MT_SAFE { પાછા m_direction.isReadOnly(); }
    bool isConstRef() અચલ VL_MT_SAFE { પાછા m_direction.isConstRef(); }
    bool isRef() અચલ VL_MT_SAFE { પાછા m_direction.isRef(); }
    bool isWritable() અચલ VL_MT_SAFE { પાછા m_direction.isWritable(); }
    bool isTristate() અચલ { પાછા m_tristate; }
    bool isPrimaryIO() અચલ VL_MT_SAFE { પાછા m_primaryIO; }
    bool isPrimaryInish() અચલ { પાછા isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() અચલ VL_MT_SAFE { પાછા m_primaryClock; }
    શૂન્ય setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() અચલ { પાછા varType() == VVarType::IFACEREF; }
    શૂન્ય setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() અચલ { પાછા m_isIfaceParent; }
    bool isIfaceArraySplit() અચલ { પાછા m_isIfaceArraySplit; }
    bool isInternal() અચલ { પાછા m_isInternal; }
    bool isIfaceParam() અચલ { પાછા m_isIfaceParam; }
    bool isSignal() અચલ { પાછા varType().isSignal(); }
    bool isNet() અચલ { પાછા varType().isNet(); }
    bool isWor() અચલ { પાછા varType().isWor(); }
    bool isWiredNet() અચલ { પાછા varType().isWiredNet(); }
    bool isTemp() અચલ { પાછા varType().isTemp(); }
    bool isClassMember() અચલ { પાછા varType() == VVarType::MEMBER; }
    bool isVirtIface() અચલ {
        જો (AstIfaceRefDType* અચલ dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            પાછા dtp->isVirtual();
        }
        પાછા false;
    }
    bool isStatementTemp() અચલ { પાછા varType() == VVarType::STMTTEMP; }
    bool isXTemp() અચલ { પાછા varType() == VVarType::XTEMP; }
    bool isParam() અચલ { પાછા varType().isParam(); }
    bool isGParam() અચલ { પાછા varType() == VVarType::GPARAM; }
    bool isGenVar() અચલ { પાછા varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() અચલ {
        પાછા isIO() || isSignal() || isClassMember() || isTemp() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() અચલ {
        પાછા !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() અચલ {
        અચલ AstBasicDType* અચલ bdtypep = basicp();
        પાછા bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() અચલ { પાછા m_usedParam; }
    bool isUsedLoopIdx() અચલ { પાછા m_usedLoopIdx; }
    bool mtask_Cache_લીટી_Align() અચલ { પાછા m_mtask_Cache_લીટી_Align; }
    શૂન્ય mtask_Cache_લીટી_Align(bool flag) { m_mtask_Cache_લીટી_Align = flag; }
    bool isSc() અચલ VL_MT_SAFE { પાછા m_sc; }
    bool isScQuad() અચલ;
    bool isScBv() અચલ VL_MT_STABLE;
    bool isScUint() અચલ;
    bool isScUintBool() અચલ;
    bool isScBigUint() અચલ VL_MT_STABLE;
    bool isScSensitive() અચલ { પાછા m_scSensitive; }
    bool isSigPublic() અચલ;
    bool isSigModPublic() અચલ { પાછા m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() અચલ { પાછા m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() અચલ { પાછા m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() અચલ { પાછા m_trace; }
    bool isRand() અચલ { પાછા m_rand.isRand(); }
    bool isRandC() અચલ { પાછા m_rand.isRandC(); }
    bool isConst() અચલ VL_MT_SAFE { પાછા m_isConst; }
    bool isLatched() અચલ { પાછા m_isLatched; }
    bool isFuncLocal() અચલ { પાછા m_funcLocal; }
    bool isFuncLocalSticky() અચલ { પાછા m_funcLocalSticky; }
    bool isFuncReturn() અચલ { પાછા m_funcReturn; }
    bool isPullup() અચલ { પાછા m_isPullup; }
    bool isPulldown() અચલ { પાછા m_isPulldown; }
    bool attrScBv() અચલ { પાછા m_attrScBv; }
    bool attrScBigUint() અચલ { પાછા m_attrScBigUint; }
    bool attrFileDescr() અચલ { પાછા m_attrFileDescr; }
    bool attrSFormat() અચલ { પાછા m_attrSFormat; }
    bool attrSplitVar() અચલ { પાછા m_attrSplitVar; }
    bool attrFsmState() અચલ { પાછા m_attrFsmState; }
    bool attrFsmArcInclCond() અચલ { પાછા m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() અચલ { પાછા m_attrFsmStateExpand; }
    AstIface* sensIfacep() અચલ { પાછા m_sensIfacep; }
    VRandAttr rand() અચલ { પાછા m_rand; }
    string verilogKwd() અચલ override;
    શૂન્ય lifetime(અચલ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() અચલ { પાછા m_lifetime; }
    શૂન્ય pin_અંક(પૂર્ણાંક id) { m_pin_અંક = id; }
    પૂર્ણાંક pin_અંક() અચલ { પાછા m_pin_અંક; }
    શૂન્ય propagateAttrFrom(અચલ AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        જો (fromp->attrFileDescr()) attrFileDescr(true);
        જો (fromp->isContinuously()) isContinuously(true);
    }
    શૂન્ય propagateSplitAttrFrom(અચલ AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    અચલ અક્ષર* cannotSplitKindReason() અચલ;
    શૂન્ય propagateWrapAttrFrom(અચલ AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    શૂન્ય combineType(અચલ AstVar* otherp);
    bool needsCReset() અચલ {
        પાછા !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !isStatementTemp()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    સ્થિર AstVar* scVarRecurse(Ast_ગાંઠ* nodep);
};
class AstVarScope final : જાહેર Ast_ગાંઠ {
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
    bool m_fork_By_કિંમત : 1;  // Set before a fork, whose processes get it by value
જાહેર:
    AstVarScope(File_લીટી* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_કિંમત = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    શૂન્ય cloneRelink() override {
        જો (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    string name() અચલ override VL_MT_STABLE { પાછા scopep()->name() + "->" + varp()->name(); }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override;
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    AstVar* varp() અચલ VL_MT_STABLE { પાછા m_varp; }  // [After Link] Pointer to variable
    શૂન્ય varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() અચલ VL_MT_STABLE { પાછા m_scopep; }  // Pointer to scope it's under
    શૂન્ય scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() અચલ { પાછા m_trace; }
    શૂન્ય trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() અચલ { પાછા m_optimizeLifePost; }
    શૂન્ય optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_કિંમત() અચલ { પાછા m_fork_By_કિંમત; }
    શૂન્ય fork_By_કિંમત(bool flag) { m_fork_By_કિંમત = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : જાહેર Ast_ગાંઠ_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    અચલ uint64_t m_index;  // Index of the element, with the array's lo() at 0
જાહેર:
    AstConsArrayItem(File_લીટી* fl, uint64_t index, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_index{index} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    અચલ અક્ષર* broken() અચલ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        પાછા nullptr;
    }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        પાછા m_index == VN_DBG_AS(samep, ConsArrayItem)->m_index;
    }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }  // By AstConsArray's map
    uint64_t index() અચલ { પાછા m_index; }
};
class AstConsAssocItem final : જાહેર Ast_ગાંઠ_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
જાહેર:
    AstConsAssocItem(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* keyp, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    અચલ અક્ષર* broken() અચલ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        પાછા nullptr;
    }
};
class AstConsDynArrayItem final : જાહેર Ast_ગાંઠ_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
જાહેર:
    AstConsDynArrayItem(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_લીટી* fl, AstConst* keyp, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    અચલ અક્ષર* broken() અચલ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        પાછા nullptr;
    }
};
class AstConsQueueItem final : જાહેર Ast_ગાંઠ_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
જાહેર:
    AstConsQueueItem(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    અચલ અક્ષર* broken() અચલ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        પાછા nullptr;
    }
};
class AstConsStructItem final : જાહેર Ast_ગાંઠ_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
જાહેર:
    AstConsStructItem(File_લીટી* fl, AstMemberDType* memberp, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    અચલ અક્ષર* broken() અચલ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        પાછા nullptr;
    }
    શૂન્ય dump(std::ostream& str) અચલ override;
    AstMemberDType* memberp() અચલ { પાછા m_memberp; }
};
class AstConsWildcardItem final : જાહેર Ast_ગાંઠ_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
જાહેર:
    AstConsWildcardItem(File_લીટી* fl, AstConst* keyp, Ast_ગાંઠ_પદાવલિ* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    અચલ અક્ષર* broken() અચલ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        પાછા nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : જાહેર Ast_ગાંઠ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    પૂર્ણાંક m_offset;  // Offset column numbers to uniq-ify IFs
જાહેર:
    AstCoverOtherDecl(File_લીટી* fl, અચલ string& page, અચલ string& comment,
                      અચલ string& linescov, પૂર્ણાંક offset, અચલ string& fsmVar = "",
                      અચલ string& fsmFrom = "", અચલ string& fsmTo = "",
                      અચલ string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક offset() અચલ { પાછા m_offset; }
    પૂર્ણાંક size() અચલ override { પાછા 1; }
    અચલ string& linescov() અચલ { પાછા m_linescov; }
    અચલ string& fsmVar() અચલ { પાછા m_fsmVar; }
    અચલ string& fsmFrom() અચલ { પાછા m_fsmFrom; }
    અચલ string& fsmTo() અચલ { પાછા m_fsmTo; }
    અચલ string& fsmTag() અચલ { પાછા m_fsmTag; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstCoverOtherDecl* અચલ asamep = VN_DBG_AS(samep, CoverOtherDecl);
        પાછા Super::same_ગાંઠ(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : જાહેર Ast_ગાંઠ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    અચલ VNumRange m_range;  // Packed array range covering each toggle bit
જાહેર:
    AstCoverToggleDecl(File_લીટી* fl, અચલ string& page, અચલ string& comment,
                       અચલ VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    પૂર્ણાંક size() અચલ override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        પાછા 2 * m_range.elements();
    }
    અચલ VNumRange& range() અચલ { પાછા m_range; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstCoverToggleDecl* અચલ asamep = VN_DBG_AS(samep, CoverToggleDecl);
        પાછા Super::same_ગાંઠ(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : જાહેર Ast_ગાંઠ_FTask {
    // A function inside a module
જાહેર:
    AstFunc(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* stmtp, Ast_ગાંઠ* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    Ast_ગાંઠ_FTask* cloneType(અચલ string& name) override {
        પાછા new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() અચલ override { પાછા "function"; }
};
class AstLet final : જાહેર Ast_ગાંઠ_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
જાહેર:
    AstLet(File_લીટી* fl, અચલ string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    અચલ અક્ષર* broken() અચલ override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_પદાવલિ));
        પાછા nullptr;
    }
    Ast_ગાંઠ_FTask* cloneType(અચલ string& name) override { પાછા new AstLet{fileline(), name}; }
    string verilogKwd() અચલ override { પાછા "let"; }
};
class AstProperty final : જાહેર Ast_ગાંઠ_FTask {
    // A property inside a module
જાહેર:
    AstProperty(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    Ast_ગાંઠ_FTask* cloneType(અચલ string& name) override {
        પાછા new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() અચલ override { પાછા "property"; }
};
class AstSequence final : જાહેર Ast_ગાંઠ_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
જાહેર:
    AstSequence(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool hasDType() અચલ override VL_MT_SAFE { પાછા true; }
    Ast_ગાંઠ_FTask* cloneType(અચલ string& name) override {
        પાછા new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() અચલ override { પાછા "sequence"; }
    bool isReferenced() અચલ { પાછા m_referenced; }
    શૂન્ય isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : જાહેર Ast_ગાંઠ_FTask {
    // A task inside a module
જાહેર:
    AstTask(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_ગાંઠ_FTask* cloneType(અચલ string& name) override {
        પાછા new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() અચલ override { પાછા "task"; }
};

// === AstNodeFile ===
class AstCFile final : જાહેર Ast_ગાંઠ_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
જાહેર:
    AstCFile(File_લીટી* fl, અચલ string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
    uint64_t complexityScore() અચલ { પાછા m_complexityScore; }
    શૂન્ય complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() અચલ { પાછા m_slow; }
    શૂન્ય slow(bool flag) { m_slow = flag; }
    bool source() અચલ { પાછા m_source; }
    શૂન્ય source(bool flag) { m_source = flag; }
    bool support() અચલ { પાછા m_support; }
    શૂન્ય support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : જાહેર Ast_ગાંઠ_File {
    // Verilog output file
    // Parents:  NETLIST
જાહેર:
    AstVFile(File_લીટી* fl, અચલ string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    શૂન્ય dump(std::ostream& str = std::cout) અચલ override;
    શૂન્ય dumpJson(std::ostream& str = std::cout) અચલ override;
};
class AstCoverCross final : જાહેર Ast_ગાંઠ_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
જાહેર:
    AstCoverCross(File_લીટી* fl, અચલ string& name, AstCoverpointRef* itemsp,
                  Ast_ગાંઠ_પદાવલિ* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstCoverpoint final : જાહેર Ast_ગાંઠ_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
જાહેર:
    AstCoverpoint(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ_પદાવલિ* exprp,
                  Ast_ગાંઠ_પદાવલિ* iffp = nullptr, Ast_ગાંઠ* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};

// === AstNodeGen ===
class AstGenBlock final : જાહેર Ast_ગાંઠ_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    અચલ bool m_unnamed;  // Originally unnamed (name change does not affect this)
    અચલ bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

જાહેર:
    AstGenBlock(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    std::string name() અચલ override VL_MT_STABLE { પાછા m_name; }
    શૂન્ય name(અચલ std::string& name) override { m_name = name; }
    bool unnamed() અચલ { પાછા m_unnamed; }
    bool implied() અચલ { પાછા m_implied; }
    AstDefaultDisable* defaultDisablep() અચલ { પાછા m_defaultDisablep; }
    શૂન્ય defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(અચલ string& outer) અચલ;
};
class AstGenCase final : જાહેર Ast_ગાંઠ_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
જાહેર:
    AstGenCase(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : જાહેર Ast_ગાંઠ_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
જાહેર:
    AstGenFor(File_લીટી* fl, Ast_ગાંઠ* initsp, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ* incsp, Ast_ગાંઠ* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : જાહેર Ast_ગાંઠ_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
જાહેર:
    AstGenIf(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* condp, Ast_ગાંઠ* thensp, Ast_ગાંઠ* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : જાહેર Ast_ગાંઠ_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_અંક;  // Declaration token number
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
    પૂર્ણાંક m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool full, અચલ string& scope) અચલ;
    string dtypeNameScope() અચલ;  // Prefix of dtypeName(true) for the scope declaring the class

જાહેર:
    AstClass(File_લીટી* fl, અચલ string& name, અચલ string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_અંક{fl->token_અંક()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() અચલ override { પાછા isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા true; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    bool timescaleMatters() અચલ override { પાછા false; }
    AstClassPackage* classOrPackagep() અચલ VL_MT_STABLE { પાછા m_classOrPackagep; }
    શૂન્ય classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() અચલ VL_MT_STABLE {
        પાછા m_covergroupEnclosingClassp;
    }
    શૂન્ય covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_ગાંઠ* membersp() અચલ VL_MT_STABLE { પાછા stmtsp(); }
    શૂન્ય addMembersp(Ast_ગાંઠ* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() અચલ { પાછા m_hasRandVarsUpdate; }
    શૂન્ય hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() અચલ { પાછા m_covergroup; }
    શૂન્ય isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() અચલ { પાછા m_extended; }
    શૂન્ય isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() અચલ { પાછા m_interfaceClass; }
    શૂન્ય isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() અચલ { પાછા m_virtual; }
    શૂન્ય isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() અચલ { પાછા m_needRNG; }
    શૂન્ય needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() અચલ { પાછા m_useVirtualPublic; }
    શૂન્ય useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    શૂન્ય markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() અચલ { પાછા m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) અચલ;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(અચલ string& scope) અચલ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    શૂન્ય dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() અચલ { પાછા !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_સંદેશ() અચલ override { પાછા hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    સ્થિર string embeddedCovergroupTypeName(અચલ string& name) { પાછા "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() અચલ;
    // Covergroup options accessors
    પૂર્ણાંક cgAutoBinMax() અચલ { પાછા m_cgAutoBinMax; }
    શૂન્ય cgAutoBinMax(પૂર્ણાંક કિંમત) { m_cgAutoBinMax = કિંમત; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    સ્થિર bool isClassExtendedFrom(અચલ AstClass* refClassp, અચલ AstClass* baseClassp);
    uint32_t decl_Token_અંક() અચલ override { પાછા m_decl_Token_અંક; }
    શૂન્ય decl_Token_અંક_Set_Min(uint32_t token_અંક) override {
        m_decl_Token_અંક = std::min(m_decl_Token_અંક, token_અંક);
    }
    શૂન્ય baseOverride(અચલ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() અચલ { પાછા m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    સ્થિર bool isCacheableChild(અચલ Ast_ગાંઠ* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    template <typename T_Callable>
    શૂન્ય foreachMember(અચલ T_Callable& f) {
        using T_ગાંઠ = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_ગાંઠ*>::કિંમત
                && std::is_base_of<Ast_ગાંઠ, T_ગાંઠ>::કિંમત,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        જો (અચલ AstClassExtends* અચલ cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        માટે (Ast_ગાંઠ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (Ast_ગાંઠ::privateTypeTest<T_ગાંઠ>(stmtp)) f(this, static_cast<T_ગાંઠ*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    template <typename T_Callable>
    bool existsMember(અચલ T_Callable& p) અચલ {
        using T_ગાંઠ = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, અચલ AstClass*, અચલ T_ગાંઠ*>::કિંમત
                && std::is_base_of<Ast_ગાંઠ, T_ગાંઠ>::કિંમત,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        જો (અચલ AstClassExtends* અચલ cextendsp = this->extendsp()) {
            જો (cextendsp->classp()->existsMember(p)) પાછા true;
        }
        માટે (Ast_ગાંઠ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (Ast_ગાંઠ::privateTypeTest<T_ગાંઠ>(stmtp)) {
                જો (p(this, static_cast<T_ગાંઠ*>(stmtp))) પાછા true;
            }
        }
        પાછા false;
    }
};
class AstClassPackage final : જાહેર Ast_ગાંઠ_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
જાહેર:
    AstClassPackage(File_લીટી* fl, અચલ string& name, અચલ string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() અચલ override { પાછા "classpackage"; }
    bool timescaleMatters() અચલ override { પાછા false; }
    AstClass* classp() અચલ VL_MT_SAFE { પાછા m_classp; }
    શૂન્ય classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : જાહેર Ast_ગાંઠ_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
જાહેર:
    AstIface(File_લીટી* fl, અચલ string& name, અચલ string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    string verilogKwd() અચલ override { પાછા "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() અચલ override { પાછા false; }
    bool hasVirtualRef() અચલ { પાછા m_hasVirtualRef; }
    શૂન્ય setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) અચલ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    શૂન્ય dtypeNameFreeze();
};
class AstModule final : જાહેર Ast_ગાંઠ_Module {
    // A module declaration
    અચલ bool m_isChecker;  // Module represents a checker
    અચલ bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

જાહેર:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_લીટી* fl, અચલ string& name, અચલ string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_લીટી* fl, અચલ string& name, અચલ string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_લીટી* fl, અચલ string& name, અચલ string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() અચલ override {
        પાછા m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() અચલ override { પાછા true; }
    bool isChecker() અચલ { પાછા m_isChecker; }
    bool isProgram() અચલ { પાછા m_isProgram; }
    bool hasGenericIface() અચલ { પાછા m_hasGenericIface; }
    શૂન્ય hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
};
class AstNotFoundModule final : જાહેર Ast_ગાંઠ_Module {
    // A missing module declaration
જાહેર:
    AstNotFoundModule(File_લીટી* fl, અચલ string& name, અચલ string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() અચલ override { પાછા "/*not-found-*/ module"; }
    bool timescaleMatters() અચલ override { પાછા false; }
};
class AstPackage final : જાહેર Ast_ગાંઠ_Module {
    // A package declaration
જાહેર:
    AstPackage(File_લીટી* fl, અચલ string& name, અચલ string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() અચલ override { પાછા "package"; }
    bool timescaleMatters() અચલ override { પાછા !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : જાહેર Ast_ગાંઠ_Module {
    // A primitive declaration
જાહેર:
    AstPrimitive(File_લીટી* fl, અચલ string& name, અચલ string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() અચલ override { પાછા "primitive"; }
    bool timescaleMatters() અચલ override { પાછા false; }
};

// === AstNodeProcedure ===
class AstAlways final : જાહેર Ast_ગાંઠ_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    અચલ VAlwaysKwd m_keyword;

જાહેર:
    AstAlways(File_લીટી* fl, VAlwaysKwd keyword, Ast_Sen_વૃક્ષ* sentreep, Ast_ગાંઠ* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit અંતઃરેખ AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    VAlwaysKwd keyword() અચલ { પાછા m_keyword; }
};
class AstAlwaysObserved final : જાહેર Ast_ગાંઠ_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

જાહેર:
    AstAlwaysObserved(File_લીટી* fl, Ast_Sen_વૃક્ષ* sentreep, Ast_ગાંઠ* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : જાહેર Ast_ગાંઠ_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

જાહેર:
    explicit AstAlwaysPost(File_લીટી* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : જાહેર Ast_ગાંઠ_Procedure {
    // Like always but Postponed scheduling region

જાહેર:
    AstAlwaysPostponed(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : જાહેર Ast_ગાંઠ_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

જાહેર:
    explicit AstAlwaysPre(File_લીટી* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : જાહેર Ast_ગાંઠ_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

જાહેર:
    AstAlwaysReactive(File_લીટી* fl, Ast_Sen_વૃક્ષ* sentreep, Ast_ગાંઠ* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : જાહેર Ast_ગાંઠ_Procedure {
જાહેર:
    AstFinal(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : જાહેર Ast_ગાંઠ_Procedure {
જાહેર:
    AstInitial(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : જાહેર Ast_ગાંઠ_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
જાહેર:
    AstInitialAutomatic(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : જાહેર Ast_ગાંઠ_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
જાહેર:
    AstInitialStatic(File_લીટી* fl, Ast_ગાંઠ* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : જાહેર Ast_ગાંઠ_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
જાહેર:
    AstBracketRange(File_લીટી* fl, Ast_ગાંઠ* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    આભાસી string emitC() { V3ERROR_NA_RETURN(""); }
    આભાસી string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() અચલ override VL_MT_SAFE { પાછા false; }
};
class AstRange final : જાહેર Ast_ગાંઠ_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    અચલ bool m_fromBracket = false;  // From C-style '[x]' declaration
જાહેર:
    AstRange(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* leftp, Ast_ગાંઠ_પદાવલિ* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    અંતઃરેખ AstRange(File_લીટી* fl, પૂર્ણાંક ડાબું, પૂર્ણાંક જમણું);
    અંતઃરેખ AstRange(File_લીટી* fl, અચલ VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    અંતઃરેખ પૂર્ણાંક ડાબું_Const() અચલ VL_MT_STABLE;
    અંતઃરેખ પૂર્ણાંક જમણું_Const() અચલ VL_MT_STABLE;
    પૂર્ણાંક hiConst() અચલ VL_MT_STABLE {
        અચલ પૂર્ણાંક l = ડાબું_Const();
        અચલ પૂર્ણાંક r = જમણું_Const();
        પાછા l > r ? l : r;
    }
    પૂર્ણાંક loConst() અચલ VL_MT_STABLE {
        અચલ પૂર્ણાંક l = ડાબું_Const();
        અચલ પૂર્ણાંક r = જમણું_Const();
        પાછા l > r ? r : l;
    }
    પૂર્ણાંક elementsConst() અચલ VL_MT_STABLE { પાછા hiConst() - loConst() + 1; }
    bool ascending() અચલ { પાછા ડાબું_Const() < જમણું_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_ડાબું_To_જમણું() અચલ {
        પાછા VNumRange{ડાબું_Const(), જમણું_Const()}.seq_ડાબું_To_જમણું();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_જમણું_To_ડાબું() અચલ {
        પાછા VNumRange{ડાબું_Const(), જમણું_Const()}.seq_જમણું_To_ડાબું();
    }
    શૂન્ય dump(std::ostream& str) અચલ override;
    શૂન્ય dumpJson(std::ostream& str) અચલ override;
    આભાસી string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* samep) અચલ override {
        અચલ AstRange* અચલ asamep = VN_DBG_AS(samep, Range);
        પાછા fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() અચલ { પાછા m_fromBracket; }
};
class AstUnsizedRange final : જાહેર Ast_ગાંઠ_Range {
    // Unsized range specification, for open arrays
જાહેર:
    explicit AstUnsizedRange(File_લીટી* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    આભાસી string emitC() { V3ERROR_NA_RETURN(""); }
    આભાસી string emitVerilog() { પાછા "[]"; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};
class AstWildcardRange final : જાહેર Ast_ગાંઠ_Range {
    // Wildcard range specification, for wildcard index type associative arrays
જાહેર:
    explicit AstWildcardRange(File_લીટી* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    આભાસી string emitC() { V3ERROR_NA_RETURN(""); }
    આભાસી string emitVerilog() { પાછા "[*]"; }
    bool same_ગાંઠ(અચલ Ast_ગાંઠ* /*samep*/) અચલ override { પાછા true; }
};

#endif  // Guard
