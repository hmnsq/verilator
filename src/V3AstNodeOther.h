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
#அந்தர்பாவ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

#அந்தர்பாவ <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_முனை_Cons_Item VL_NOT_FINAL : பொது Ast_முனை {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    friend class Ast_முனை_கோவை;  // For m_knownConstantTree
    mutable bool m_known_Constant_மரம் = false;  // Known to be a tree of constants

பாதுகாக்கப்பட்ட:
    Ast_முனை_Cons_Item(VNType t, File_வரி* fl, Ast_முனை_கோவை* valuep)
        : Ast_முனை{t, fl} {
        this->valuep(valuep);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Cons_Item;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class Ast_முனை_Cover_Decl VL_NOT_FINAL : பொது Ast_முனை {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    முழு m_bin_இலக்கம் = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    முழு m_local_Bin_இலக்கம் = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
பொது:
    Ast_முனை_Cover_Decl(VNType t, File_வரி* fl, மாறிலி string& page, மாறிலி string& comment)
        : Ast_முனை(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_முனை_Cover_Decl;
    மாறிலி எழுத்து* broken() மாறிலி override {
        எனில் (m_dataDeclp
            && (m_dataDeclp == this || m_dataDeclp->m_dataDeclp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        திரும்பு nullptr;
    }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 1 + 2 * INSTR_மொத்தம்_LD; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    முழு bin_இலக்கம்() மாறிலி { திரும்பு m_bin_இலக்கம்; }
    வெறுமை bin_இலக்கம்(முழு flag) { m_bin_இலக்கம் = flag; }
    முழு local_Bin_இலக்கம்() மாறிலி { திரும்பு m_local_Bin_இலக்கம்; }
    வெறுமை local_Bin_இலக்கம்(முழு flag) { m_local_Bin_இலக்கம் = flag; }
    bool perInstance() மாறிலி { திரும்பு m_perInstance; }
    வெறுமை perInstance(bool flag) { m_perInstance = flag; }
    மெய்நிகர் முழு size() மாறிலி = 0;
    மாறிலி string& comment() மாறிலி { திரும்பு m_text; }  // text to insert in code
    மாறிலி string& page() மாறிலி { திரும்பு m_page; }
    மாறிலி string& hier() மாறிலி { திரும்பு m_hier; }
    வெறுமை hier(மாறிலி string& flag) { m_hier = flag; }
    வெறுமை comment(மாறிலி string& flag) { m_text = flag; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி Ast_முனை_Cover_Decl* மாறிலி asamep = VN_DBG_AS(samep, முனை_Cover_Decl);
        திரும்பு (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    வெறுமை dataDeclp(Ast_முனை_Cover_Decl* nodep) { m_dataDeclp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_முனை_Cover_Decl* dataDeclNullp() மாறிலி { திரும்பு m_dataDeclp; }
    Ast_முனை_Cover_Decl* dataDeclThisp() { திரும்பு dataDeclNullp() ? dataDeclNullp() : this; }
};
class Ast_முனை_FTask VL_NOT_FINAL : பொது Ast_முனை {
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

பாதுகாக்கப்பட்ட:
    Ast_முனை_FTask(VNType t, File_வரி* fl, மாறிலி string& name, Ast_முனை* stmtsp)
        : Ast_முனை{t, fl}
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

பொது:
    ASTGEN_MEMBERS_Ast_முனை_FTask;
    மெய்நிகர் Ast_முனை_FTask* cloneType(மாறிலி string& name) = 0;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool isGateOptimizable() மாறிலி override {
        திரும்பு !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string cname() மாறிலி { திரும்பு m_cname; }
    வெறுமை cname(மாறிலி string& cname) { m_cname = cname; }
    bool isFunction() மாறிலி { திரும்பு fvarp() != nullptr; }
    // MORE ACCESSORS
    வெறுமை dpiOpenParentInc() { ++m_dpiOpenParent; }
    வெறுமை dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() மாறிலி { திரும்பு m_dpiOpenParent; }
    bool taskPublic() மாறிலி { திரும்பு m_taskPublic; }
    வெறுமை taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() மாறிலி { திரும்பு m_classMethod; }
    வெறுமை classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() மாறிலி { திரும்பு m_didProto; }
    வெறுமை didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() மாறிலி { திரும்பு m_isExternProto; }
    வெறுமை isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() மாறிலி { திரும்பு m_isExternDef; }
    வெறுமை isExternDef(bool flag) { m_isExternDef = flag; }
    மாறிலி string& ifacePortName() மாறிலி { திரும்பு m_ifacePortName; }
    வெறுமை ifacePortName(மாறிலி string& name) { m_ifacePortName = name; }
    bool prototype() மாறிலி { திரும்பு m_prototype; }
    வெறுமை prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() மாறிலி { திரும்பு m_dpiExport; }
    வெறுமை dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() மாறிலி { திரும்பு m_dpiImport; }
    வெறுமை dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() மாறிலி { திரும்பு m_dpiContext; }
    வெறுமை dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() மாறிலி { திரும்பு m_dpiOpenChild; }
    வெறுமை dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() மாறிலி { திரும்பு m_dpiTask; }
    வெறுமை dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() மாறிலி { திரும்பு !m_dpiCDecl.empty(); }
    மாறிலி string& dpiCDecl() மாறிலி { திரும்பு m_dpiCDecl; }
    வெறுமை dpiCDecl(மாறிலி string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() மாறிலி { திரும்பு m_isConstructor; }
    வெறுமை isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() மாறிலி { திரும்பு m_isHideLocal; }
    வெறுமை isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() மாறிலி { திரும்பு m_isHideProtected; }
    வெறுமை isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() மாறிலி { திரும்பு m_dpiPure; }
    வெறுமை dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() மாறிலி { திரும்பு m_keepAlive; }
    வெறுமை keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() மாறிலி { திரும்பு m_needsSyms; }
    வெறுமை needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() மாறிலி { திரும்பு m_pureVirtual; }
    வெறுமை pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() மாறிலி { திரும்பு m_recursive; }
    வெறுமை recursive(bool flag) { m_recursive = flag; }
    bool isStatic() மாறிலி { திரும்பு m_static; }
    வெறுமை isStatic(bool flag) { m_static = flag; }
    bool underGenerate() மாறிலி { திரும்பு m_underGenerate; }
    வெறுமை underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() மாறிலி { திரும்பு m_verilogFunction; }
    வெறுமை verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() மாறிலி { திரும்பு m_verilogTask; }
    வெறுமை verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() மாறிலி { திரும்பு m_virtual; }
    வெறுமை isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() மாறிலி { திரும்பு m_needProcess; }
    வெறுமை setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() மாறிலி { திரும்பு m_isCovergroupSample; }
    வெறுமை isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    வெறுமை baseOverride(மாறிலி VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() மாறிலி { திரும்பு m_baseOverride; }
    வெறுமை lifetime(மாறிலி VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() மாறிலி { திரும்பு m_lifetime; }
    bool isPure() override;
    மாறிலி எழுத்து* broken() மாறிலி override;
    வெறுமை propagateAttrFrom(மாறிலி Ast_முனை_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        classMethod(fromp->classMethod());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

தனிப்பட்ட:
    bool getPurityRecurse() மாறிலி;
};
class Ast_முனை_File VL_NOT_FINAL : பொது Ast_முனை {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
பொது:
    Ast_முனை_File(VNType t, File_வரி* fl, மாறிலி string& name)
        : Ast_முனை{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_முனை_File;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class Ast_முனை_Func_Cov_Item VL_NOT_FINAL : பொது Ast_முனை {
    // Base class for functional coverage items (coverpoints, crosses)
பாதுகாக்கப்பட்ட:
    string m_name;  // Item name

பொது:
    Ast_முனை_Func_Cov_Item(VNType t, File_வரி* fl, மாறிலி string& name)
        : Ast_முனை{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_முனை_Func_Cov_Item;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    bool maybePointedTo() மாறிலி override { திரும்பு true; }
};
class Ast_முனை_Gen VL_NOT_FINAL : பொது Ast_முனை {
    // Generate construct
பொது:
    Ast_முனை_Gen(VNType t, File_வரி* fl)
        : Ast_முனை{t, fl} {}
    ASTGEN_MEMBERS_Ast_முனை_Gen;
};
class Ast_முனை_Module VL_NOT_FINAL : பொது Ast_முனை {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    மாறிலி string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    முழு m_depth = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    முழு m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
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
பாதுகாக்கப்பட்ட:
    Ast_முனை_Module(VNType t, File_வரி* fl, மாறிலி string& name, மாறிலி string& libname)
        : Ast_முனை{t, fl}
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

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Module;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    மெய்நிகர் bool timescaleMatters() மாறிலி = 0;
    உள்வரி bool isDollarUnit() மாறிலி;  // Is the $unit package
    உள்வரி bool isConstPool() மாறிலி;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(மாறிலி string& outer) மாறிலி;
    // ACCESSORS
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string origName() மாறிலி override { திரும்பு m_origName; }
    string someInstanceName() மாறிலி VL_MT_SAFE { திரும்பு m_someInstanceName; }
    வெறுமை someInstanceName(மாறிலி string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() மாறிலி { திரும்பு m_defaultDisablep; }
    வெறுமை defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() மாறிலி { திரும்பு m_inLibrary; }
    வெறுமை inLibrary(bool flag) { m_inLibrary = flag; }
    வெறுமை depth(முழு மதிப்பு) { m_depth = மதிப்பு; }
    முழு depth() மாறிலி VL_MT_SAFE { திரும்பு m_depth; }
    வெறுமை level(முழு மதிப்பு) { m_level = மதிப்பு; }
    முழு level() மாறிலி VL_MT_SAFE { திரும்பு m_level; }
    string libname() மாறிலி { திரும்பு m_libname; }
    bool is_உச்சி() மாறிலி VL_MT_SAFE { திரும்பு level() == 1; }
    bool modPublic() மாறிலி { திரும்பு m_modPublic; }
    வெறுமை modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() மாறிலி { திரும்பு m_modTrace; }
    வெறுமை modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() மாறிலி { திரும்பு m_ctorVarReset; }
    வெறுமை ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() மாறிலி { திரும்பு m_dead; }
    வெறுமை dead(bool flag) { m_dead = flag; }
    bool hasGParam() மாறிலி { திரும்பு m_hasGParam; }
    வெறுமை hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() மாறிலி { திரும்பு m_hasParameterList; }
    வெறுமை hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() மாறிலி { திரும்பு m_hierBlock; }
    வெறுமை hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() மாறிலி { திரும்பு m_hierParams; }
    வெறுமை hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() மாறிலி { திரும்பு m_recursive; }
    வெறுமை recursive(bool flag) { m_recursive = flag; }
    வெறுமை recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() மாறிலி { திரும்பு m_recursiveClone; }
    bool parameterizedTemplate() மாறிலி { திரும்பு m_parameterizedTemplate; }
    வெறுமை parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    வெறுமை verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() மாறிலி { திரும்பு m_verilatorLib; }
    VLifetime lifetime() மாறிலி { திரும்பு m_lifetime; }
    வெறுமை lifetime(மாறிலி VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    வெறுமை timeunit(மாறிலி VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() மாறிலி { திரும்பு m_unconnectedDrive; }
    வெறுமை unconnectedDrive(மாறிலி VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_முனை_Procedure VL_NOT_FINAL : பொது Ast_முனை {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
பாதுகாக்கப்பட்ட:
    Ast_முனை_Procedure(VNType t, File_வரி* fl, Ast_முனை* stmtsp)
        : Ast_முனை{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Procedure;
    // METHODS
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isJustOneBodyStmt() மாறிலி { திரும்பு stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() மாறிலி { திரும்பு m_suspendable; }
    வெறுமை setSuspendable() { m_suspendable = true; }
    bool needProcess() மாறிலி { திரும்பு m_needProcess; }
    வெறுமை setNeedProcess() { m_needProcess = true; }
};
class Ast_முனை_Range VL_NOT_FINAL : பொது Ast_முனை {
    // A range, sized or unsized
பாதுகாக்கப்பட்ட:
    Ast_முனை_Range(VNType t, File_வரி* fl)
        : Ast_முனை{t, fl} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Range;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : பொது Ast_முனை {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

பொது:
    AstActive(File_வரி* fl, மாறிலி string& name, Ast_Sen_மரம்* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    // Statements are broken into pieces, as some must come before others.
    வெறுமை sentreep(Ast_Sen_மரம்* nodep) { m_sentreep = nodep; }
    Ast_Sen_மரம்* sentreep() மாறிலி { திரும்பு m_sentreep; }
    // METHODS
    உள்வரி bool hasClocked() மாறிலி;
    உள்வரி bool hasCombo() மாறிலி;
    உள்வரி bool hasInitial() மாறிலி;
    உள்வரி bool hasStatic() மாறிலி;
};
class AstAlias final : பொது Ast_முனை {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
பொது:
    AstAlias(File_வரி* fl, Ast_முனை_கோவை* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : பொது Ast_முனை {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
பொது:
    AstAliasScope(File_வரி* fl, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : பொது Ast_முனை {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
பொது:
    AstBind(File_வரி* fl, மாறிலி string& name, Ast_முனை* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Bind Target name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
};
class AstCFunc final : பொது Ast_முனை {
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
    முழு m_cost;  // Function call cost
பொது:
    AstCFunc(File_வரி* fl, மாறிலி string& name, AstScope* scopep, மாறிலி string& rtnType = "")
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
        m_cost = v3Global.opt.instr_மொத்தம்_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstCFunc* மாறிலி asamep = VN_DBG_AS(samep, CFunc);
        திரும்பு ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு m_cost; }
    VBoolOrUnknown isConst() மாறிலி { திரும்பு m_isConst; }
    வெறுமை isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    வெறுமை isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() மாறிலி { திரும்பு m_isStatic; }
    வெறுமை isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() மாறிலி VL_MT_SAFE { திரும்பு m_isTrace; }
    வெறுமை isTrace(bool flag) { m_isTrace = flag; }
    வெறுமை cname(மாறிலி string& name) { m_cname = name; }
    string cname() மாறிலி { திரும்பு m_cname; }
    AstScope* scopep() மாறிலி { திரும்பு m_scopep; }
    வெறுமை scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() மாறிலி { திரும்பு ((m_rtnType == "") ? "void" : m_rtnType); }
    வெறுமை rtnType(மாறிலி string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() மாறிலி { திரும்பு m_dontCombine || isTrace() || entryPoint(); }
    வெறுமை dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() மாறிலி { திரும்பு m_declPrivate; }
    வெறுமை declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() மாறிலி VL_MT_SAFE { திரும்பு m_keepIfEmpty; }
    வெறுமை keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() மாறிலி VL_MT_SAFE { திரும்பு m_slow; }
    வெறுமை slow(bool flag) { m_slow = flag; }
    bool funcPublic() மாறிலி { திரும்பு m_funcPublic; }
    வெறுமை funcPublic(bool flag) { m_funcPublic = flag; }
    வெறுமை argTypes(மாறிலி string& str) { m_argTypes = str; }
    string argTypes() மாறிலி { திரும்பு m_argTypes; }
    வெறுமை ifdef(மாறிலி string& str) { m_ifdef = str; }
    string ifdef() மாறிலி { திரும்பு m_ifdef; }
    bool isConstructor() மாறிலி { திரும்பு m_isConstructor; }
    வெறுமை isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() மாறிலி { திரும்பு m_isDestructor; }
    வெறுமை isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() மாறிலி { திரும்பு m_isMethod; }
    வெறுமை isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() மாறிலி { திரும்பு m_isLoose; }
    வெறுமை isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() மாறிலி { திரும்பு isMethod() && !isLoose(); }
    bool isVirtual() மாறிலி { திரும்பு m_isVirtual; }
    வெறுமை isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() மாறிலி { திரும்பு m_needProcess; }
    வெறுமை setNeedProcess() { m_needProcess = true; }
    bool entryPoint() மாறிலி { திரும்பு m_entryPoint; }
    வெறுமை entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() மாறிலி { திரும்பு m_dpiPure; }
    வெறுமை dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() மாறிலி { திரும்பு m_dpiContext; }
    வெறுமை dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() மாறிலி VL_MT_SAFE { திரும்பு m_dpiExportDispatcher; }
    வெறுமை dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() மாறிலி { திரும்பு m_dpiExportImpl; }
    வெறுமை dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() மாறிலி VL_MT_SAFE { திரும்பு m_dpiImportPrototype; }
    வெறுமை dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() மாறிலி { திரும்பு m_dpiImportWrapper; }
    வெறுமை dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() மாறிலி { திரும்பு !m_dpiCDecl.empty(); }
    மாறிலி string& dpiCDecl() மாறிலி { திரும்பு m_dpiCDecl; }
    வெறுமை dpiCDecl(மாறிலி string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() மாறிலி { திரும்பு m_rtnType == "VlCoroutine"; }
    வெறுமை recursive(bool flag) { m_recursive = flag; }
    bool recursive() மாறிலி { திரும்பு m_recursive; }
    வெறுமை unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() மாறிலி override { திரும்பு m_unlikely; }  // Note virtual override
    வெறுமை noLife(bool flag) { m_noLife = flag; }
    bool noLife() மாறிலி { திரும்பு m_noLife; }
    bool isCovergroupSample() மாறிலி { திரும்பு m_isCovergroupSample; }
    வெறுமை isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() மாறிலி { திரும்பு m_voidSelf; }
    வெறுமை voidSelf(bool flag) { m_voidSelf = flag; }
    வெறுமை cost(முழு cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() மாறிலி {
        திரும்பு !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : பொது Ast_முனை {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    மாறிலி string m_name;
    மாறிலி VUseType m_useType;  // What sort of use this is

பொது:
    AstCUse(File_வரி* fl, VUseType useType, மாறிலி string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    VUseType useType() மாறிலி { திரும்பு m_useType; }
};
class AstCell final : பொது Ast_முனை {
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
    File_வரி* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    முழு m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
பொது:
    AstCell(File_வரி* fl, File_வரி* mfl, மாறிலி string& instName, மாறிலி string& modName,
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
    வெறுமை cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    // ACCESSORS
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Cell name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string origName() மாறிலி override { திரும்பு m_origName; }  // * = Original name
    வெறுமை origName(மாறிலி string& name) { m_origName = name; }
    string modName() மாறிலி { திரும்பு m_modName; }  // * = Instance name
    வெறுமை modName(மாறிலி string& name) { m_modName = name; }
    File_வரி* modNameFileline() மாறிலி { திரும்பு m_modNameFileline; }
    Ast_முனை_Module* modp() மாறிலி { திரும்பு m_modp; }  // [AfterLink] = Pointer to module instantiated
    வெறுமை modp(Ast_முனை_Module* nodep) { m_modp = nodep; }
    முழு arrayIdx() மாறிலி { திரும்பு m_arrayIdx; }
    வெறுமை arrayIdx(முழு idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() மாறிலி { திரும்பு m_hasIfaceVar; }
    வெறுமை hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    வெறுமை trace(bool flag) { m_trace = flag; }
    bool isTrace() மாறிலி { திரும்பு m_trace; }
    வெறுமை recursive(bool flag) { m_recursive = flag; }
    bool recursive() மாறிலி { திரும்பு m_recursive; }
};
class AstCellInline final : பொது Ast_முனை {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    மாறிலி std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    மாறிலி std::string m_origModName;  // __BEGIN__ or __FORK__
பொது:
    AstCellInline(File_வரி* fl, மாறிலி std::string& name, மாறிலி std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    // ACCESSORS
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    std::string origModName() மாறிலி { திரும்பு m_origModName; }
};
class AstCellInlineScope final : பொது Ast_முனை {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    மாறிலி std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
பொது:
    AstCellInlineScope(File_வரி* fl, மாறிலி std::string& name, மாறிலி std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    // ACCESSORS
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி std::string& name) override { m_name = name; }
    std::string origModName() மாறிலி { திரும்பு m_origModName; }
};
class AstCgOptionAssign final : பொது Ast_முனை {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    மாறிலி VCoverOptionType m_optType;  // Option type
    மாறிலி string m_name;  // Original option name (for diagnostics on unknown options)
    மாறிலி bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
பொது:
    AstCgOptionAssign(File_வரி* fl, bool typeOption, VCoverOptionType optType,
                      மாறிலி string& rawName, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    VCoverOptionType optType() மாறிலி { திரும்பு m_optType; }
    bool typeOption() மாறிலி { திரும்பு m_typeOption; }
};
class AstClassExtends final : பொது Ast_முனை {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    மாறிலி bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

பொது:
    AstClassExtends(File_வரி* fl, Ast_முனை* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    string verilogKwd() மாறிலி override { திரும்பு isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() மாறிலி;
    AstClass* classp() மாறிலி;  // Like above, but throws error if nulll
    bool isImplements() மாறிலி { திரும்பு m_isImplements; }
    வெறுமை parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() மாறிலி { திரும்பு m_parameterized; }
};
class AstClocking final : பொது Ast_முனை {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    மாறிலி bool m_isGlobal;  // True if global clocking

பொது:
    AstClocking(File_வரி* fl, மாறிலி std::string& name, Ast_Sen_மரம்* sentreep, Ast_முனை* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool isDefault() மாறிலி { திரும்பு m_isDefault; }
    bool isGlobal() மாறிலி { திரும்பு m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    வெறுமை makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : பொது Ast_முனை {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

பொது:
    AstClockingItem(File_வரி* fl, VDirection direction, Ast_முனை_கோவை* skewp, Ast_முனை* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        எனில் (AstAssign* மாறிலி clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } இல்லையெனில் {
            exprp(VN_AS(clockingDeclp, முனை_கோவை));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    VDirection direction() மாறிலி { திரும்பு m_direction; }
    AstClockingItem* outputp() மாறிலி { திரும்பு m_outputp; }
    வெறுமை outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
};
class AstConfig final : பொது Ast_முனை {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

பொது:
    AstConfig(File_வரி* fl, மாறிலி std::string& libname, மாறிலி std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_libname + "." + m_configname; }
    std::string libname() மாறிலி VL_MT_STABLE { திரும்பு m_libname; }
    std::string configname() மாறிலி VL_MT_STABLE { திரும்பு m_configname; }
};
class AstConfigCell final : பொது Ast_முனை {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

பொது:
    AstConfigCell(File_வரி* fl, மாறிலி std::string& libname, மாறிலி std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_libname + "." + m_cellname; }
    std::string libname() மாறிலி VL_MT_STABLE { திரும்பு m_libname; }
    std::string cellname() மாறிலி VL_MT_STABLE { திரும்பு m_cellname; }
};
class AstConfigRule final : பொது Ast_முனை {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    மாறிலி bool m_isCell;  // Declared as "cell" versus "instance"

பொது:
    AstConfigRule(File_வரி* fl, Ast_முனை* cellp, Ast_முனை* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() மாறிலி VL_MT_STABLE { திரும்பு m_isCell; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstConfigUse final : பொது Ast_முனை {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    மாறிலி bool m_isConfig;  // ":config"; Config, not module/primitive name

பொது:
    AstConfigUse(File_வரி* fl, மாறிலி std::string& libname, மாறிலி std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_libname + "." + m_cellname; }
    std::string libname() மாறிலி VL_MT_STABLE { திரும்பு m_libname; }
    std::string cellname() மாறிலி VL_MT_STABLE { திரும்பு m_cellname; }
    bool isConfig() மாறிலி VL_MT_STABLE { திரும்பு m_isConfig; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstConstraint final : பொது Ast_முனை {
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
பொது:
    AstConstraint(File_வரி* fl, மாறிலி string& name, Ast_முனை* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Scope name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    வெறுமை baseOverride(மாறிலி VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() மாறிலி { திரும்பு m_baseOverride; }
    bool isExternDef() மாறிலி { திரும்பு m_isExternDef; }
    வெறுமை isExternDef(bool flag) { m_isExternDef = flag; }
    வெறுமை isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() மாறிலி { திரும்பு m_isExternExplicit; }
    வெறுமை isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() மாறிலி { திரும்பு m_isExternProto; }
    வெறுமை isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() மாறிலி { திரும்பு m_isKwdPure; }
    வெறுமை isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() மாறிலி { திரும்பு m_isStatic; }
};
class AstCoverBin final : பொது Ast_முனை {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    மாறிலி string m_name;  // Base name of the bin
    மாறிலி VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

பொது:
    AstCoverBin(File_வரி* fl, மாறிலி string& name, Ast_முனை* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_முனை_கோவை* iffp = nullptr)
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
    AstCoverBin(File_வரி* fl, மாறிலி string& name, Ast_முனை_கோவை* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_வரி* fl, மாறிலி string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_வரி* fl, மாறிலி string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    VCoverBinsType binsType() மாறிலி { திரும்பு m_binsType; }
    bool isWildcard() மாறிலி { திரும்பு m_isWildcard; }
    bool isArray() மாறிலி { திரும்பு m_isArray; }
    வெறுமை isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : பொது Ast_முனை {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    மாறிலி bool m_isNegated;  // Complement the selection within the cross product

பொது:
    AstCoverBinsof(File_வரி* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_முனை* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    bool isNegated() மாறிலி { திரும்பு m_isNegated; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {  // LCOV_EXCL_START
        மாறிலி AstCoverBinsof* மாறிலி asamep = VN_DBG_AS(samep, CoverBinsof);
        திரும்பு m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : பொது Ast_முனை {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    மாறிலி string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    மாறிலி VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

பொது:
    AstCoverCrossBin(File_வரி* fl, மாறிலி string& name, Ast_முனை* selectp, Ast_முனை_கோவை* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    string verilogKwd() மாறிலி override;
    VCoverBinsType binsType() மாறிலி { திரும்பு m_binsType; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {  // LCOV_EXCL_START
        மாறிலி AstCoverCrossBin* மாறிலி asamep = VN_DBG_AS(samep, CoverCrossBin);
        திரும்பு m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : பொது Ast_முனை {
    // The complete bin-tuple space of the enclosing cross
    மாறிலி string m_name;  // Referenced cross name

பொது:
    AstCoverCrossRef(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {  // LCOV_EXCL_START
        திரும்பு m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : பொது Ast_முனை {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    மாறிலி bool m_isOr;  // Union (||), rather than intersection (&&)

பொது:
    AstCoverCrossSelect(File_வரி* fl, Ast_முனை* lhsp, Ast_முனை* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool isOr() மாறிலி { திரும்பு m_isOr; }
    string verilogKwd() மாறிலி override { திரும்பு isOr() ? "||" : "&&"; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {  // LCOV_EXCL_START
        திரும்பு m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : பொது Ast_முனை {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
பொது:
    AstCoverCrossWith(File_வரி* fl, Ast_முனை* selectp, Ast_முனை_கோவை* filterp, Ast_முனை_கோவை* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : பொது Ast_முனை {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    மாறிலி VCoverOptionType m_optType;  // Option being assigned
    மாறிலி bool m_typeOption;  // type_option (vs option)

பொது:
    AstCoverOption(File_வரி* fl, bool typeOption, VCoverOptionType optType, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    VCoverOptionType optType() மாறிலி { திரும்பு m_optType; }
    bool typeOption() மாறிலி { திரும்பு m_typeOption; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {  // LCOV_EXCL_START
        மாறிலி AstCoverOption* மாறிலி asamep = VN_DBG_AS(samep, CoverOption);
        திரும்பு m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : பொது Ast_முனை {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
பொது:
    AstCoverToggle(File_வரி* fl, AstCoverInc* incp, Ast_முனை_கோவை* origp, Ast_முனை_கோவை* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு 3 + INSTR_மொத்தம்_BRANCH + INSTR_மொத்தம்_LD; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு true; }
    bool isOutputter() override {
        திரும்பு false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : பொது Ast_முனை {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    மாறிலி VTransRepType m_repType;

பொது:
    AstCoverTransItem(File_வரி* fl, Ast_முனை* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstCoverTransSet final : பொது Ast_முனை {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
பொது:
    AstCoverTransSet(File_வரி* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstCoverWith final : பொது Ast_முனை {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
பொது:
    AstCoverWith(File_வரி* fl, Ast_முனை* subp, Ast_முனை_கோவை* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        addSubp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : பொது Ast_முனை {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

பொது:
    AstCovergroup(File_வரி* fl, மாறிலி string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_முனை* membersp, Ast_Sen_மரம்* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    bool maybePointedTo() மாறிலி override { திரும்பு true; }
};
class AstCoverpointRef final : பொது Ast_முனை {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    மாறிலி string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

பொது:
    AstCoverpointRef(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_வரி* fl, Ast_முனை_கோவை* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
};
class AstDefParam final : பொது Ast_முனை {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
பொது:
    AstDefParam(File_வரி* fl, Ast_முனை_கோவை* pathp, மாறிலி string& name, Ast_முனை_கோவை* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு true; }
};
class AstDefaultClocking final : பொது Ast_முனை {
    std::string m_name;  // Clocking block name

பொது:
    AstDefaultClocking(File_வரி* fl, மாறிலி std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
};
class AstDefaultDisable final : பொது Ast_முனை {
    // @astgen op1 := condp : AstNodeExpr

பொது:
    AstDefaultDisable(File_வரி* fl, Ast_முனை_கோவை* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : பொது Ast_முனை {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
பொது:
    AstDpiExport(File_வரி* fl, மாறிலி string& vname, மாறிலி string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string cname() மாறிலி { திரும்பு m_cname; }
    வெறுமை cname(மாறிலி string& cname) { m_cname = cname; }
};
class AstElabDisplay final : பொது Ast_முனை {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

பொது:
    உள்வரி AstElabDisplay(File_வரி* fl, VDisplayType dispType, Ast_முனை_கோவை* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!fmtp());
        திரும்பு nullptr;
    }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string verilogKwd() மாறிலி override { திரும்பு "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() மாறிலி override { திரும்பு false; }
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { திரும்பு true; }  // SPECIAL: $display makes output
    bool isUnlikely() மாறிலி override { திரும்பு true; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    முழு instr_மொத்தம்() மாறிலி override { திரும்பு INSTR_மொத்தம்_PLI; }
    VDisplayType displayType() மாறிலி { திரும்பு m_displayType; }
    வெறுமை displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : பொது Ast_முனை {
    // Represents something missing, e.g. a missing argument in FOREACH
பொது:
    explicit AstEmpty(File_வரி* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstGenCaseItem final : பொது Ast_முனை {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
பொது:
    AstGenCaseItem(File_வரி* fl, Ast_முனை_கோவை* condsp, Ast_முனை* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() மாறிலி { திரும்பு condsp() == nullptr; }
};
class AstImplicit final : பொது Ast_முனை {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
பொது:
    AstImplicit(File_வரி* fl, Ast_முனை* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : பொது Ast_முனை {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
பொது:
    AstIntfRef(File_வரி* fl, மாறிலி string& name, மாறிலி string& parentName, மாறிலி string& baseName,
               மாறிலி string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    string parentName() மாறிலி { திரும்பு m_parentName; }
    string baseName() மாறிலி { திரும்பு m_baseName; }
    string modportName() மாறிலி { திரும்பு m_modportName; }
};
class AstLibrary final : பொது Ast_முனை {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

பொது:
    AstLibrary(File_வரி* fl, மாறிலி std::string& name, Ast_முனை_கோவை* filesp, Ast_முனை_கோவை* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
};
class AstModport final : பொது Ast_முனை {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
பொது:
    AstModport(File_வரி* fl, மாறிலி string& name, Ast_முனை* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() மாறிலி override { திரும்பு "modport"; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : பொது Ast_முனை {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
பொது:
    AstModportClockingRef(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    AstClocking* clockingp() மாறிலி VL_MT_STABLE {
        திரும்பு m_clockingp;
    }  // [After Link] Pointer to clocking block
    வெறுமை clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : பொது Ast_முனை {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
பொது:
    AstModportFTaskRef(File_வரி* fl, மாறிலி string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool isImport() மாறிலி { திரும்பு !m_export; }
    bool isExport() மாறிலி { திரும்பு m_export; }
    Ast_முனை_FTask* ftaskp() மாறிலி { திரும்பு m_ftaskp; }  // [After Link] Pointer to variable
    வெறுமை ftaskp(Ast_முனை_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : பொது Ast_முனை {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    மாறிலி VDirection m_direction;  // Direction of the variable (in/out)
பொது:
    AstModportVarRef(File_வரி* fl, மாறிலி string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_வரி* fl, மாறிலி string& name, Ast_முனை_கோவை* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    VDirection direction() மாறிலி { திரும்பு m_direction; }
    AstVar* varp() மாறிலி VL_MT_STABLE { திரும்பு m_varp; }  // [After Link] Pointer to variable
    வெறுமை varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : பொது Ast_முனை {
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
    std::string m_resolved_உச்சி_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::set<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<மாறிலி AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<மாறிலி Ast_முனை*, மாறிலி Ast_முனை_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

பொது:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    மாறிலி எழுத்து* broken() மாறிலி override;
    வெறுமை தள்ளு_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.செருகு(varp); }
    மாறிலி std::set<AstVar*>& deferredParamVarps() மாறிலி { திரும்பு m_deferredParamVarps; }
    வெறுமை clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    வெறுமை deleteContents();
    வெறுமை cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() மாறிலி override VL_MT_STABLE { திரும்பு "$root"; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    Ast_முனை_Module* உச்சி_Modulep() மாறிலி VL_MT_STABLE {  // Top module in hierarchy
        திரும்பு modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { திரும்பு m_typeTablep; }
    string astConstOrigParamName(மாறிலி AstConst* nodep) மாறிலி;
    வெறுமை astConstOrigParamName(மாறிலி AstConst* nodep, மாறிலி string& name);
    வெறுமை astConstOrigParamNameErase(மாறிலி AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    மாறிலி Ast_முனை_Module* containingModule(மாறிலி Ast_முனை* nodep);
    // Forget remembered modules, as the tree has moved.
    வெறுமை clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() மாறிலி { திரும்பு m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() மாறிலி { திரும்பு m_dollarUnitPkgp; }
    வெறுமை dollarUnitPkgp(AstPackage* மாறிலி packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() மாறிலி { திரும்பு m_dpiExportTriggerp; }
    வெறுமை dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() மாறிலி { திரும்பு m_delaySchedulerp; }
    வெறுமை delaySchedulerp(AstVar* மாறிலி varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() மாறிலி { திரும்பு m_nbaEventp; }
    வெறுமை nbaEventp(AstVarScope* மாறிலி varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() மாறிலி { திரும்பு m_nbaEventTriggerp; }
    வெறுமை nbaEventTriggerp(AstVarScope* மாறிலி varScopep) { m_nbaEventTriggerp = varScopep; }
    வெறுமை stdPackagep(AstPackage* மாறிலி packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() மாறிலி { திரும்பு m_stdPackagep; }
    வெறுமை stdPackageProcessp(AstClass* மாறிலி classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() மாறிலி { திரும்பு m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_வரி*) மாறிலி;
    Ast_உச்சி_Scope* உச்சி_Scopep() மாறிலி { திரும்பு m_உச்சி_Scopep; }
    வெறுமை create_உச்சி_Scope(AstScope* scopep);
    VTimescale timeunit() மாறிலி { திரும்பு m_timeunit; }
    வெறுமை timeunit(மாறிலி VTimescale& மதிப்பு) { m_timeunit = மதிப்பு; }
    VTimescale timeprecision() மாறிலி { திரும்பு m_timeprecision; }
    வெறுமை timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    வெறுமை timeprecisionMerge(File_வரி*, மாறிலி VTimescale& மதிப்பு);
    வெறுமை timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() மாறிலி { திரும்பு m_timescaleSpecified; }
    மாறிலி std::string& resolved_உச்சி_Module_Name() மாறிலி { திரும்பு m_resolved_உச்சி_Module_Name; }
    வெறுமை resolved_உச்சி_Module_Name(மாறிலி std::string& மதிப்பு) { m_resolved_உச்சி_Module_Name = மதிப்பு; }
    uint32_t nTraceCodes() மாறிலி { திரும்பு m_nTraceCodes; }
    வெறுமை nTraceCodes(uint32_t மதிப்பு) { m_nTraceCodes = மதிப்பு; }
    AstVarScope* stlFirstIterationp();
    வெறுமை clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    மாறிலி std::string trace_Lib_உச்சி_Name() மாறிலி {
        மாறிலி std::string& name = resolved_உச்சி_Module_Name();
        திரும்பு prettyName(name.empty() ? v3Global.rootp()->உச்சி_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    வெறுமை addEvalStats(மாறிலி std::string& phase);
};
class AstPackageExport final : பொது Ast_முனை {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

பொது:
    AstPackageExport(File_வரி* fl, மாறிலி string& pkgName, மாறிலி string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    string pkgName() மாறிலி VL_MT_STABLE { திரும்பு m_pkgName; }
    string prettyPkgNameQ() மாறிலி { திரும்பு "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() மாறிலி { திரும்பு m_packagep; }
    வெறுமை packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : பொது Ast_முனை {
    // A package export *::* declaration
பொது:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_வரி* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : பொது Ast_முனை {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

பொது:
    AstPackageImport(File_வரி* fl, AstPackage* packagep, மாறிலி string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_வரி* fl, மாறிலி string& pkgName, மாறிலி string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    string pkgName() மாறிலி VL_MT_STABLE { திரும்பு m_pkgName; }
    string prettyPkgNameQ() மாறிலி { திரும்பு "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() மாறிலி { திரும்பு m_packagep; }
    வெறுமை packagep(AstPackage* nodep) { m_packagep = nodep; }

தனிப்பட்ட:
    வெறுமை pkgNameFrom();
};
class AstPin final : பொது Ast_முனை {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    முழு m_pin_இலக்கம்;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
பொது:
    AstPin(File_வரி* fl, முழு pin_இலக்கம், மாறிலி string& name, Ast_முனை* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_இலக்கம்{pin_இலக்கம்}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    வெறுமை cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Pin name, ""=go by number
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string prettyOperatorName() மாறிலி override;
    bool dotStar() மாறிலி { திரும்பு name() == ".*"; }  // Fake name for .* connections until linked
    முழு pin_இலக்கம்() மாறிலி { திரும்பு m_pin_இலக்கம்; }
    AstVar* modVarp() மாறிலி { திரும்பு m_modVarp; }  // [After Link] Pointer to variable
    வெறுமை modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() மாறிலி { திரும்பு m_modPTypep; }
    வெறுமை modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() மாறிலி { திரும்பு m_param; }
    வெறுமை param(bool flag) { m_param = flag; }
    மாறிலி string& paramPath() மாறிலி { திரும்பு m_paramPath; }
    வெறுமை paramPath(மாறிலி string& path) { m_paramPath = path; }
    bool svDotName() மாறிலி { திரும்பு m_svDotName; }
    வெறுமை svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() மாறிலி { திரும்பு m_svImplicit; }
    வெறுமை svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : பொது Ast_முனை {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    மாறிலி முழு m_pin_இலக்கம்;  // Pin number
    மாறிலி string m_name;  // Name of pin
பொது:
    AstPort(File_வரி* fl, முழு pinnum, மாறிலி string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_இலக்கம்{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Port name
    முழு pin_இலக்கம்() மாறிலி { திரும்பு m_pin_இலக்கம்; }  // * = Pin number, for order based instantiation
};
class AstPragma final : பொது Ast_முனை {
    மாறிலி VPragmaType m_pragType;  // Type of pragma
    மாறிலி VTimescale m_timescale;  // For TIMEUNIT_SET
பொது:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_வரி* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_வரி* fl, மாறிலி VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    VPragmaType pragType() மாறிலி { திரும்பு m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() மாறிலி override { திரும்பு false; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() மாறிலி { திரும்பு m_timescale; }
};
class AstPropSpec final : பொது Ast_முனை {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

பொது:
    AstPropSpec(File_வரி* fl, AstSenItem* sensesp, Ast_முனை_கோவை* disablep, Ast_முனை* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool hasDType() மாறிலி override VL_MT_SAFE {
        திரும்பு true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() மாறிலி { திரும்பு m_propStrength; }
};
class AstPull final : பொது Ast_முனை {
    // @astgen op1 := lhsp : AstNodeExpr

    மாறிலி bool m_direction;

பொது:
    AstPull(File_வரி* fl, Ast_முனை_கோவை* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() மாறிலி { திரும்பு m_direction; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstScope final : பொது Ast_முனை {
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
பொது:
    AstScope(File_வரி* fl, Ast_முனை_Module* modp, மாறிலி string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!m_modp);
        திரும்பு nullptr;
    }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Scope name
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string pretty_Name_செய்தி() மாறிலி override;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    string nameDotless() மாறிலி;
    Ast_முனை_Module* modp() மாறிலி { திரும்பு m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() மாறிலி VL_MT_SAFE { திரும்பு m_aboveScopep; }
    வெறுமை aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() மாறிலி { திரும்பு m_aboveCellp; }
    வெறுமை aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_உச்சி() மாறிலி VL_MT_SAFE { திரும்பு aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : பொது Ast_முனை {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_edgeType;  // Edge type
பொது:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    AstSenItem(File_வரி* fl, VEdgeType edgeType, Ast_முனை_கோவை* senp, Ast_முனை_கோவை* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_வரி* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_வரி* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_வரி* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_வரி* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_வரி* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_வரி* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() மாறிலி { திரும்பு m_edgeType; }
    வெறுமை edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_மொத்தம்_Inc();
    }
    Ast_முனை_Var_Ref* varrefp() மாறிலி { திரும்பு VN_CAST(sensp(), முனை_Var_Ref); }
    //
    bool isClocked() மாறிலி { திரும்பு edgeType().clockedStmt(); }
    bool isComboOrStar() மாறிலி {
        திரும்பு edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() மாறிலி { திரும்பு edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() மாறிலி { திரும்பு edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() மாறிலி { திரும்பு edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() மாறிலி { திரும்பு edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() மாறிலி { திரும்பு edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() மாறிலி { திரும்பு edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_மரம் final : பொது Ast_முனை {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
பொது:
    Ast_Sen_மரம்(File_வரி* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_மரம்(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_மரம்;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    // METHODS
    bool hasClocked() மாறிலி;  // Includes a clocked statement
    bool hasEdge() மாறிலி;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() மாறிலி;  // Includes a STATIC SenItem
    bool hasInitial() மாறிலி;  // Includes a INITIAL SenItem
    bool hasFinal() மாறிலி;  // Includes a FINAL SenItem
    bool hasCombo() மாறிலி;  // Includes a COMBO SenItem
    bool hasHybrid() மாறிலி;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : பொது Ast_முனை {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

பொது:
    AstStrengthSpec(File_வரி* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { திரும்பு m_s0; }
    VStrength strength1() { திரும்பு m_s1; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstSystemCSection final : பொது Ast_முனை {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    மாறிலி VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

பொது:
    AstSystemCSection(File_வரி* fl, VSystemCSectionType sectionType, மாறிலி std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() மாறிலி { திரும்பு m_sectionType; }
    மாறிலி std::string& text() மாறிலி { திரும்பு m_text; }
    வெறுமை text(மாறிலி std::string& மதிப்பு) { m_text = மதிப்பு; }
    வெறுமை dump(std::ostream&) மாறிலி override;
    வெறுமை dumpJson(std::ostream&) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை*) மாறிலி override { திரும்பு false; }
    bool isPure() override { திரும்பு false; }
    bool isOutputter() override { திரும்பு true; }
};
class AstText final : பொது Ast_முனை {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    மாறிலி std::string m_text;  // The text to emit
பொது:
    AstText(File_வரி* fl, மாறிலி std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு text() == VN_DBG_AS(samep, Text)->text();
    }
    மாறிலி std::string& text() மாறிலி VL_MT_SAFE { திரும்பு m_text; }
};
class AstTextBlock final : பொது Ast_முனை {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    மாறிலி std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    மாறிலி std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    மாறிலி std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
பொது:
    explicit AstTextBlock(File_வரி* fl,  //
                          மாறிலி std::string& prefix = "",  //
                          மாறிலி std::string& separator = "",  //
                          மாறிலி std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    மாறிலி std::string& prefix() மாறிலி { திரும்பு m_prefix; }
    மாறிலி std::string& separator() மாறிலி { திரும்பு m_separator; }
    மாறிலி std::string& suffix() மாறிலி { திரும்பு m_suffix; }
    // Add some text, or a node to this block
    வெறுமை add(மாறிலி string& text) { addNodesp(new AstText{fileline(), text}); }
    வெறுமை add(Ast_முனை* nodep) { addNodesp(nodep); }
};
class Ast_உச்சி_Scope final : பொது Ast_முனை {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    friend class AstNetlist;  // Only the AstNetlist can create one
    Ast_உச்சி_Scope(File_வரி* fl, AstScope* ascopep)
        : ASTGEN_SUPER_உச்சி_Scope(fl) {
        this->scopep(ascopep);
    }

பொது:
    ASTGEN_MEMBERS_Ast_உச்சி_Scope;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* createTemp(மாறிலி string& name, unsigned width);
    AstVarScope* createTemp(மாறிலி string& name, Ast_முனை_DType* dtypep);
};
class AstTypeTable final : பொது Ast_முனை {
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
    using DetailedMap = std::map<VBasic_Type_சாவி, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

பொது:
    explicit AstTypeTable(File_வரி* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    வெறுமை cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_வரி* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_வரி* fl, VBasicDTypeKwd kwd, முழு width, முழு widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_வரி* fl, VBasicDTypeKwd kwd, மாறிலி VNumRange& range,
                                     முழு widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& முனை);
    AstBasicDType* find_செருகு_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_வரி* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_வரி* fl);
    AstQueueDType* findQueueIndexDType(File_வரி* fl);
    AstStreamDType* findStreamDType(File_வரி* fl);
    AstVoidDType* findVoidDType(File_வரி* fl);
    வெறுமை clearCache();
    வெறுமை repairCache();
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
};
class AstTypedef final : பொது Ast_முனை {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_இலக்கம்;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

பொது:
    AstTypedef(File_வரி* fl, மாறிலி string& name, Ast_முனை* attrsp, VFlagChildDType,
               Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_இலக்கம்{fl->token_இலக்கம்()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_வரி* fl, மாறிலி string& name, Ast_முனை_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_இலக்கம்{fl->token_இலக்கம்()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() மாறிலி;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    மெய்நிகர் Ast_முனை_DType* subDTypep() மாறிலி VL_MT_STABLE {
        திரும்பு dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_இலக்கம்() மாறிலி override { திரும்பு m_decl_Token_இலக்கம்; }
    வெறுமை decl_Token_இலக்கம்_Set_Min(uint32_t token_இலக்கம்) override {
        m_decl_Token_இலக்கம் = std::min(m_decl_Token_இலக்கம், token_இலக்கம்);
    }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    வெறுமை name(மாறிலி string& flag) override { m_name = flag; }
    bool attrPublic() மாறிலி { திரும்பு m_attrPublic; }
    வெறுமை attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() மாறிலி { திரும்பு m_isHideLocal; }
    வெறுமை isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() மாறிலி { திரும்பு m_isHideProtected; }
    வெறுமை isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() மாறிலி { திரும்பு m_isUnderClass; }
    வெறுமை isUnderClass(bool flag) { m_isUnderClass = flag; }
    வெறுமை tag(மாறிலி string& text) override { m_tag = text; }
    string tag() மாறிலி override { திரும்பு m_tag; }
};
class AstTypedefFwd final : பொது Ast_முனை {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    மாறிலி VFwdType m_fwdType;  // Forward type for lint check

பொது:
    AstTypedefFwd(File_வரி* fl, மாறிலி string& name, மாறிலி VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    VFwdType fwdType() மாறிலி { திரும்பு m_fwdType; }
};
class AstUdpTable final : பொது Ast_முனை {
    // @astgen op1 := linesp : List[AstUdpTableLine]
பொது:
    AstUdpTable(File_வரி* fl, Ast_Udp_Table_வரி* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        எனில் (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_வரி final : பொது Ast_முனை {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
தனிப்பட்ட:
    மாறிலி bool m_udpIsCombo;  // Combinational or sequential UDP

பொது:
    class UdpCombo {};
    Ast_Udp_Table_வரி(UdpCombo, File_வரி* fl, Ast_Udp_Table_வரி_Val* iFieldsp,
                    Ast_Udp_Table_வரி_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_வரி(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_வரி(UdpSequential, File_வரி* fl, Ast_Udp_Table_வரி_Val* iFieldsp,
                    Ast_Udp_Table_வரி_Val* oFieldsp1, Ast_Udp_Table_வரி_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_வரி(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_வரி;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு udpIsCombo() மாறிலி { திரும்பு m_udpIsCombo; }
};
class Ast_Udp_Table_வரி_Val final : பொது Ast_முனை {
    string m_text;  // Value character

பொது:
    Ast_Udp_Table_வரி_Val(File_வரி* fl, மாறிலி string& text)
        : ASTGEN_SUPER_Udp_Table_வரி_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_வரி_Val;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_text; }
    வெறுமை name(std::string மாறிலி& text) override VL_MT_STABLE { m_text = text; }
    string text() மாறிலி VL_MT_SAFE { திரும்பு m_text; }
};
class AstVar final : பொது Ast_முனை {
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
    முழு m_pin_இலக்கம் = 0;  // For JSON, if non-zero the connection pin number
    முழு m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
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
    bool m_mtask_Cache_வரி_Align : 1;  // Start MTask affinity group on a cache line
    வெறுமை init() {
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
        m_mtask_Cache_வரி_Align = false;
    }

பொது:
    AstVar(File_வரி* fl, VVarType type, மாறிலி string& name, VFlagChildDType, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_வரி* fl, VVarType type, மாறிலி string& name, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_வரி* fl, VVarType type, மாறிலி string& name, VFlagLogicPacked, முழு wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_வரி* fl, VVarType type, மாறிலி string& name, VFlagBitPacked, முழு wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_வரி* fl, VVarType type, மாறிலி string& name, மாறிலி AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        எனில் (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_மரம்(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    உள்வரி bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    string origName() மாறிலி override { திரும்பு m_origName; }  // * = Original name
    வெறுமை origName(மாறிலி string& name) { m_origName = name; }
    VVarType varType() மாறிலி VL_MT_SAFE { திரும்பு m_varType; }  // * = Type of variable
    வெறுமை direction(மாறிலி VDirection& flag) {
        m_direction = flag;
        எனில் (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() மாறிலி VL_MT_SAFE { திரும்பு m_direction; }
    bool isIO() மாறிலி VL_MT_SAFE { திரும்பு m_direction != VDirection::NONE; }
    bool isVLIO() மாறிலி {
        மாறிலி AstBasicDType* மாறிலி bdtypep = basicp();
        திரும்பு isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    வெறுமை declDirection(மாறிலி VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() மாறிலி { திரும்பு m_declDirection; }
    முழு libInoutId() மாறிலி { திரும்பு m_libInoutId; }
    வெறுமை libInoutId(முழு id) { m_libInoutId = id; }
    bool libInoutEnable() மாறிலி { திரும்பு m_libInoutEnable; }
    வெறுமை libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    வெறுமை varType(VVarType type) { m_varType = type; }
    வெறுமை varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    வெறுமை varType2In() {
        m_tristate = false;
        m_direction = VDirection::உள்ளீடு;
    }
    string scType() மாறிலி;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) மாறிலி;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) மாறிலி;
    string dpiTmpVarType(மாறிலி string& varName) மாறிலி;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) மாறிலி;
    string vlEnumType() மாறிலி;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) மாறிலி;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(மாறிலி string& propName) மாறிலி;  // Return VerilatorVarProps declaration
    வெறுமை combineType(VVarType type);
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* dtypeSkipRefp() மாறிலி VL_MT_STABLE { திரும்பு subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() மாறிலி VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    மெய்நிகர் Ast_முனை_DType* subDTypep() மாறிலி VL_MT_STABLE {
        திரும்பு dtypep() ? dtypep() : childDTypep();
    }
    வெறுமை ansi(bool flag) { m_ansi = flag; }
    வெறுமை declTyped(bool flag) { m_declTyped = flag; }
    வெறுமை sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    வெறுமை attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    வெறுமை attrScBv(bool flag) { m_attrScBv = flag; }
    வெறுமை attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    வெறுமை attrSFormat(bool flag) { m_attrSFormat = flag; }
    வெறுமை attrSplitVar(bool flag) { m_attrSplitVar = flag; }
    வெறுமை attrFsmState(bool flag) { m_attrFsmState = flag; }
    வெறுமை attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    வெறுமை attrFsmStateExpand(மாறிலி VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() மாறிலி { திரும்பு m_constPoolEntry; }
    வெறுமை setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() மாறிலி { திரும்பு m_covergroupRefMember; }
    வெறுமை covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() மாறிலி { திரும்பு m_embeddedCovergroup; }
    வெறுமை embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    வெறுமை rand(மாறிலி VRandAttr flag) { m_rand = flag; }
    வெறுமை usedParam(bool flag) { m_usedParam = flag; }
    வெறுமை usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    வெறுமை sigPublic(bool flag) { m_sigPublic = flag; }
    வெறுமை sigModPublic(bool flag) { m_sigModPublic = flag; }
    வெறுமை sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        எனில் (flag) sigPublic(true);
    }
    வெறுமை sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        எனில் (flag) sigUserRdPublic(true);
    }
    வெறுமை sc(bool flag) { m_sc = flag; }
    வெறுமை scSensitive(bool flag) { m_scSensitive = flag; }
    வெறுமை primaryIO(bool flag) { m_primaryIO = flag; }
    வெறுமை isConst(bool flag) { m_isConst = flag; }
    வெறுமை isContinuously(bool flag) { m_isContinuously = flag; }
    வெறுமை isIfaceParent(bool flag) { m_isIfaceParent = flag; }
    வெறுமை isIfaceArraySplit(bool flag) { m_isIfaceArraySplit = flag; }
    வெறுமை isInternal(bool flag) { m_isInternal = flag; }
    வெறுமை isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    வெறுமை funcLocal(bool flag) {
        m_funcLocal = flag;
        எனில் (flag) m_funcLocalSticky = true;
    }
    வெறுமை funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() மாறிலி { திரும்பு m_gotNansiType; }
    வெறுமை gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() மாறிலி { திரும்பு m_hasStrengthAssignment; }
    வெறுமை hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() மாறிலி { திரும்பு m_hasUserInit; }
    வெறுமை hasUserInit(bool flag) { m_hasUserInit = flag; }
    வெறுமை icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() மாறிலி { திரும்பு m_icoMaybeWritten; }
    வெறுமை isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() மாறிலி { திரும்பு m_isClockingDriven; }
    bool isDpiOpenArray() மாறிலி VL_MT_SAFE { திரும்பு m_isDpiOpenArray; }
    வெறுமை isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() மாறிலி { திரும்பு m_isHideLocal; }
    வெறுமை isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() மாறிலி { திரும்பு m_isHideProtected; }
    வெறுமை isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() மாறிலி { திரும்பு m_noCReset; }
    வெறுமை noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() மாறிலி { திரும்பு m_noReset; }
    வெறுமை noReset(bool flag) { m_noReset = flag; }
    bool noSubst() மாறிலி { திரும்பு m_noSubst; }
    வெறுமை noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() மாறிலி { திரும்பு m_processQueue; }
    வெறுமை processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() மாறிலி { திரும்பு m_sampled; }
    வெறுமை sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() மாறிலி { திரும்பு m_substConstOnly; }
    வெறுமை substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() மாறிலி { திரும்பு m_overriddenParam; }
    வெறுமை overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() மாறிலி { திரும்பு m_untypedParam; }
    வெறுமை untypedParam(bool flag) { m_untypedParam = flag; }
    வெறுமை trace(bool flag) { m_trace = flag; }
    வெறுமை isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() மாறிலி { திரும்பு m_isForceable; }
    வெறுமை setForceable() { m_isForceable = true; }
    வெறுமை setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() மாறிலி { திரும்பு m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() மாறிலி { திரும்பு m_isReadByDpi; }
    வெறுமை setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() மாறிலி { திரும்பு m_isWrittenByDpi; }
    வெறுமை setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() மாறிலி { திரும்பு m_isWrittenBySuspendable; }
    வெறுமை setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() மாறிலி { திரும்பு m_ignorePostRead; }
    வெறுமை setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() மாறிலி { திரும்பு m_ignorePostWrite; }
    வெறுமை setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() மாறிலி { திரும்பு m_ignoreSchedWrite; }
    வெறுமை setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() மாறிலி { திரும்பு m_dfgTriLowered; }
    வெறுமை setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() மாறிலி { திரும்பு m_dfgAllowMultidriveTri; }
    வெறுமை setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    வெறுமை globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() மாறிலி { திரும்பு m_globalConstrained; }
    bool isStdRandomizeArg() மாறிலி { திரும்பு m_isStdRandomizeArg; }
    வெறுமை setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    வெறுமை tag(மாறிலி string& text) override { m_tag = text; }
    string tag() மாறிலி override { திரும்பு m_tag; }
    bool isAnsi() மாறிலி { திரும்பு m_ansi; }
    bool isContinuously() மாறிலி { திரும்பு m_isContinuously; }
    bool isDeclTyped() மாறிலி { திரும்பு m_declTyped; }
    bool isInout() மாறிலி { திரும்பு m_direction.isInout(); }
    bool isInoutOrRef() மாறிலி { திரும்பு m_direction.isInoutOrRef(); }
    bool is_உள்ளீடு() மாறிலி { திரும்பு m_direction.is_உள்ளீடு(); }
    bool isNonOutput() மாறிலி { திரும்பு m_direction.isNonOutput(); }
    bool isReadOnly() மாறிலி VL_MT_SAFE { திரும்பு m_direction.isReadOnly(); }
    bool isConstRef() மாறிலி VL_MT_SAFE { திரும்பு m_direction.isConstRef(); }
    bool isRef() மாறிலி VL_MT_SAFE { திரும்பு m_direction.isRef(); }
    bool isWritable() மாறிலி VL_MT_SAFE { திரும்பு m_direction.isWritable(); }
    bool isTristate() மாறிலி { திரும்பு m_tristate; }
    bool isPrimaryIO() மாறிலி VL_MT_SAFE { திரும்பு m_primaryIO; }
    bool isPrimaryInish() மாறிலி { திரும்பு isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() மாறிலி VL_MT_SAFE { திரும்பு m_primaryClock; }
    வெறுமை setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() மாறிலி { திரும்பு varType() == VVarType::IFACEREF; }
    வெறுமை setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() மாறிலி { திரும்பு m_isIfaceParent; }
    bool isIfaceArraySplit() மாறிலி { திரும்பு m_isIfaceArraySplit; }
    bool isInternal() மாறிலி { திரும்பு m_isInternal; }
    bool isIfaceParam() மாறிலி { திரும்பு m_isIfaceParam; }
    bool isSignal() மாறிலி { திரும்பு varType().isSignal(); }
    bool isNet() மாறிலி { திரும்பு varType().isNet(); }
    bool isWor() மாறிலி { திரும்பு varType().isWor(); }
    bool isWiredNet() மாறிலி { திரும்பு varType().isWiredNet(); }
    bool isTemp() மாறிலி { திரும்பு varType().isTemp(); }
    bool isClassMember() மாறிலி { திரும்பு varType() == VVarType::MEMBER; }
    bool isVirtIface() மாறிலி {
        எனில் (AstIfaceRefDType* மாறிலி dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            திரும்பு dtp->isVirtual();
        }
        திரும்பு false;
    }
    bool isStatementTemp() மாறிலி { திரும்பு varType() == VVarType::STMTTEMP; }
    bool isXTemp() மாறிலி { திரும்பு varType() == VVarType::XTEMP; }
    bool isParam() மாறிலி { திரும்பு varType().isParam(); }
    bool isGParam() மாறிலி { திரும்பு varType() == VVarType::GPARAM; }
    bool isGenVar() மாறிலி { திரும்பு varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() மாறிலி {
        திரும்பு isIO() || isSignal() || isClassMember() || isTemp() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() மாறிலி {
        திரும்பு !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() மாறிலி {
        மாறிலி AstBasicDType* மாறிலி bdtypep = basicp();
        திரும்பு bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() மாறிலி { திரும்பு m_usedParam; }
    bool isUsedLoopIdx() மாறிலி { திரும்பு m_usedLoopIdx; }
    bool mtask_Cache_வரி_Align() மாறிலி { திரும்பு m_mtask_Cache_வரி_Align; }
    வெறுமை mtask_Cache_வரி_Align(bool flag) { m_mtask_Cache_வரி_Align = flag; }
    bool isSc() மாறிலி VL_MT_SAFE { திரும்பு m_sc; }
    bool isScQuad() மாறிலி;
    bool isScBv() மாறிலி VL_MT_STABLE;
    bool isScUint() மாறிலி;
    bool isScUintBool() மாறிலி;
    bool isScBigUint() மாறிலி VL_MT_STABLE;
    bool isScSensitive() மாறிலி { திரும்பு m_scSensitive; }
    bool isSigPublic() மாறிலி;
    bool isSigModPublic() மாறிலி { திரும்பு m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() மாறிலி { திரும்பு m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() மாறிலி { திரும்பு m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() மாறிலி { திரும்பு m_trace; }
    bool isRand() மாறிலி { திரும்பு m_rand.isRand(); }
    bool isRandC() மாறிலி { திரும்பு m_rand.isRandC(); }
    bool isConst() மாறிலி VL_MT_SAFE { திரும்பு m_isConst; }
    bool isLatched() மாறிலி { திரும்பு m_isLatched; }
    bool isFuncLocal() மாறிலி { திரும்பு m_funcLocal; }
    bool isFuncLocalSticky() மாறிலி { திரும்பு m_funcLocalSticky; }
    bool isFuncReturn() மாறிலி { திரும்பு m_funcReturn; }
    bool isPullup() மாறிலி { திரும்பு m_isPullup; }
    bool isPulldown() மாறிலி { திரும்பு m_isPulldown; }
    bool attrScBv() மாறிலி { திரும்பு m_attrScBv; }
    bool attrScBigUint() மாறிலி { திரும்பு m_attrScBigUint; }
    bool attrFileDescr() மாறிலி { திரும்பு m_attrFileDescr; }
    bool attrSFormat() மாறிலி { திரும்பு m_attrSFormat; }
    bool attrSplitVar() மாறிலி { திரும்பு m_attrSplitVar; }
    bool attrFsmState() மாறிலி { திரும்பு m_attrFsmState; }
    bool attrFsmArcInclCond() மாறிலி { திரும்பு m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() மாறிலி { திரும்பு m_attrFsmStateExpand; }
    AstIface* sensIfacep() மாறிலி { திரும்பு m_sensIfacep; }
    VRandAttr rand() மாறிலி { திரும்பு m_rand; }
    string verilogKwd() மாறிலி override;
    வெறுமை lifetime(மாறிலி VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() மாறிலி { திரும்பு m_lifetime; }
    வெறுமை pin_இலக்கம்(முழு id) { m_pin_இலக்கம் = id; }
    முழு pin_இலக்கம்() மாறிலி { திரும்பு m_pin_இலக்கம்; }
    வெறுமை propagateAttrFrom(மாறிலி AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        எனில் (fromp->attrFileDescr()) attrFileDescr(true);
        எனில் (fromp->isContinuously()) isContinuously(true);
    }
    வெறுமை propagateSplitAttrFrom(மாறிலி AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    மாறிலி எழுத்து* cannotSplitKindReason() மாறிலி;
    வெறுமை propagateWrapAttrFrom(மாறிலி AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    வெறுமை combineType(மாறிலி AstVar* otherp);
    bool needsCReset() மாறிலி {
        திரும்பு !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !isStatementTemp()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    நிலையான AstVar* scVarRecurse(Ast_முனை* nodep);
};
class AstVarScope final : பொது Ast_முனை {
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
    bool m_fork_By_மதிப்பு : 1;  // Set before a fork, whose processes get it by value
பொது:
    AstVarScope(File_வரி* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_மதிப்பு = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    வெறுமை cloneRelink() override {
        எனில் (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு scopep()->name() + "->" + varp()->name(); }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    AstVar* varp() மாறிலி VL_MT_STABLE { திரும்பு m_varp; }  // [After Link] Pointer to variable
    வெறுமை varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() மாறிலி VL_MT_STABLE { திரும்பு m_scopep; }  // Pointer to scope it's under
    வெறுமை scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() மாறிலி { திரும்பு m_trace; }
    வெறுமை trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() மாறிலி { திரும்பு m_optimizeLifePost; }
    வெறுமை optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_மதிப்பு() மாறிலி { திரும்பு m_fork_By_மதிப்பு; }
    வெறுமை fork_By_மதிப்பு(bool flag) { m_fork_By_மதிப்பு = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : பொது Ast_முனை_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    மாறிலி uint64_t m_index;  // Index of the element, with the array's lo() at 0
பொது:
    AstConsArrayItem(File_வரி* fl, uint64_t index, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_index{index} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    மாறிலி எழுத்து* broken() மாறிலி override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        திரும்பு nullptr;
    }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        திரும்பு m_index == VN_DBG_AS(samep, ConsArrayItem)->m_index;
    }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }  // By AstConsArray's map
    uint64_t index() மாறிலி { திரும்பு m_index; }
};
class AstConsAssocItem final : பொது Ast_முனை_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
பொது:
    AstConsAssocItem(File_வரி* fl, Ast_முனை_கோவை* keyp, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    மாறிலி எழுத்து* broken() மாறிலி override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        திரும்பு nullptr;
    }
};
class AstConsDynArrayItem final : பொது Ast_முனை_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
பொது:
    AstConsDynArrayItem(File_வரி* fl, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_வரி* fl, AstConst* keyp, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    மாறிலி எழுத்து* broken() மாறிலி override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        திரும்பு nullptr;
    }
};
class AstConsQueueItem final : பொது Ast_முனை_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
பொது:
    AstConsQueueItem(File_வரி* fl, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    மாறிலி எழுத்து* broken() மாறிலி override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        திரும்பு nullptr;
    }
};
class AstConsStructItem final : பொது Ast_முனை_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
பொது:
    AstConsStructItem(File_வரி* fl, AstMemberDType* memberp, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    மாறிலி எழுத்து* broken() மாறிலி override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        திரும்பு nullptr;
    }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    AstMemberDType* memberp() மாறிலி { திரும்பு m_memberp; }
};
class AstConsWildcardItem final : பொது Ast_முனை_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
பொது:
    AstConsWildcardItem(File_வரி* fl, AstConst* keyp, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    மாறிலி எழுத்து* broken() மாறிலி override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        திரும்பு nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : பொது Ast_முனை_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    முழு m_offset;  // Offset column numbers to uniq-ify IFs
பொது:
    AstCoverOtherDecl(File_வரி* fl, மாறிலி string& page, மாறிலி string& comment,
                      மாறிலி string& linescov, முழு offset, மாறிலி string& fsmVar = "",
                      மாறிலி string& fsmFrom = "", மாறிலி string& fsmTo = "",
                      மாறிலி string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு offset() மாறிலி { திரும்பு m_offset; }
    முழு size() மாறிலி override { திரும்பு 1; }
    மாறிலி string& linescov() மாறிலி { திரும்பு m_linescov; }
    மாறிலி string& fsmVar() மாறிலி { திரும்பு m_fsmVar; }
    மாறிலி string& fsmFrom() மாறிலி { திரும்பு m_fsmFrom; }
    மாறிலி string& fsmTo() மாறிலி { திரும்பு m_fsmTo; }
    மாறிலி string& fsmTag() மாறிலி { திரும்பு m_fsmTag; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstCoverOtherDecl* மாறிலி asamep = VN_DBG_AS(samep, CoverOtherDecl);
        திரும்பு Super::same_முனை(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : பொது Ast_முனை_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    மாறிலி VNumRange m_range;  // Packed array range covering each toggle bit
பொது:
    AstCoverToggleDecl(File_வரி* fl, மாறிலி string& page, மாறிலி string& comment,
                       மாறிலி VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    முழு size() மாறிலி override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        திரும்பு 2 * m_range.elements();
    }
    மாறிலி VNumRange& range() மாறிலி { திரும்பு m_range; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstCoverToggleDecl* மாறிலி asamep = VN_DBG_AS(samep, CoverToggleDecl);
        திரும்பு Super::same_முனை(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : பொது Ast_முனை_FTask {
    // A function inside a module
பொது:
    AstFunc(File_வரி* fl, மாறிலி string& name, Ast_முனை* stmtp, Ast_முனை* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    Ast_முனை_FTask* cloneType(மாறிலி string& name) override {
        திரும்பு new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() மாறிலி override { திரும்பு "function"; }
};
class AstLet final : பொது Ast_முனை_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
பொது:
    AstLet(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_கோவை));
        திரும்பு nullptr;
    }
    Ast_முனை_FTask* cloneType(மாறிலி string& name) override { திரும்பு new AstLet{fileline(), name}; }
    string verilogKwd() மாறிலி override { திரும்பு "let"; }
};
class AstProperty final : பொது Ast_முனை_FTask {
    // A property inside a module
பொது:
    AstProperty(File_வரி* fl, மாறிலி string& name, Ast_முனை* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    Ast_முனை_FTask* cloneType(மாறிலி string& name) override {
        திரும்பு new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() மாறிலி override { திரும்பு "property"; }
};
class AstSequence final : பொது Ast_முனை_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
பொது:
    AstSequence(File_வரி* fl, மாறிலி string& name, Ast_முனை* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    Ast_முனை_FTask* cloneType(மாறிலி string& name) override {
        திரும்பு new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() மாறிலி override { திரும்பு "sequence"; }
    bool isReferenced() மாறிலி { திரும்பு m_referenced; }
    வெறுமை isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : பொது Ast_முனை_FTask {
    // A task inside a module
பொது:
    AstTask(File_வரி* fl, மாறிலி string& name, Ast_முனை* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_முனை_FTask* cloneType(மாறிலி string& name) override {
        திரும்பு new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() மாறிலி override { திரும்பு "task"; }
};

// === AstNodeFile ===
class AstCFile final : பொது Ast_முனை_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
பொது:
    AstCFile(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    uint64_t complexityScore() மாறிலி { திரும்பு m_complexityScore; }
    வெறுமை complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() மாறிலி { திரும்பு m_slow; }
    வெறுமை slow(bool flag) { m_slow = flag; }
    bool source() மாறிலி { திரும்பு m_source; }
    வெறுமை source(bool flag) { m_source = flag; }
    bool support() மாறிலி { திரும்பு m_support; }
    வெறுமை support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : பொது Ast_முனை_File {
    // Verilog output file
    // Parents:  NETLIST
பொது:
    AstVFile(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
};
class AstCoverCross final : பொது Ast_முனை_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
பொது:
    AstCoverCross(File_வரி* fl, மாறிலி string& name, AstCoverpointRef* itemsp,
                  Ast_முனை_கோவை* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstCoverpoint final : பொது Ast_முனை_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
பொது:
    AstCoverpoint(File_வரி* fl, மாறிலி string& name, Ast_முனை_கோவை* exprp,
                  Ast_முனை_கோவை* iffp = nullptr, Ast_முனை* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};

// === AstNodeGen ===
class AstGenBlock final : பொது Ast_முனை_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    மாறிலி bool m_unnamed;  // Originally unnamed (name change does not affect this)
    மாறிலி bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

பொது:
    AstGenBlock(File_வரி* fl, மாறிலி string& name, Ast_முனை* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி std::string& name) override { m_name = name; }
    bool unnamed() மாறிலி { திரும்பு m_unnamed; }
    bool implied() மாறிலி { திரும்பு m_implied; }
    AstDefaultDisable* defaultDisablep() மாறிலி { திரும்பு m_defaultDisablep; }
    வெறுமை defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(மாறிலி string& outer) மாறிலி;
};
class AstGenCase final : பொது Ast_முனை_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
பொது:
    AstGenCase(File_வரி* fl, Ast_முனை_கோவை* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : பொது Ast_முனை_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
பொது:
    AstGenFor(File_வரி* fl, Ast_முனை* initsp, Ast_முனை_கோவை* condp, Ast_முனை* incsp, Ast_முனை* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : பொது Ast_முனை_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
பொது:
    AstGenIf(File_வரி* fl, Ast_முனை_கோவை* condp, Ast_முனை* thensp, Ast_முனை* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : பொது Ast_முனை_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_இலக்கம்;  // Declaration token number
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
    முழு m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool full, மாறிலி string& scope) மாறிலி;
    string dtypeNameScope() மாறிலி;  // Prefix of dtypeName(true) for the scope declaring the class

பொது:
    AstClass(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_இலக்கம்{fl->token_இலக்கம்()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() மாறிலி override { திரும்பு isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    bool timescaleMatters() மாறிலி override { திரும்பு false; }
    AstClassPackage* classOrPackagep() மாறிலி VL_MT_STABLE { திரும்பு m_classOrPackagep; }
    வெறுமை classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() மாறிலி VL_MT_STABLE {
        திரும்பு m_covergroupEnclosingClassp;
    }
    வெறுமை covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_முனை* membersp() மாறிலி VL_MT_STABLE { திரும்பு stmtsp(); }
    வெறுமை addMembersp(Ast_முனை* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() மாறிலி { திரும்பு m_hasRandVarsUpdate; }
    வெறுமை hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() மாறிலி { திரும்பு m_covergroup; }
    வெறுமை isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() மாறிலி { திரும்பு m_extended; }
    வெறுமை isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() மாறிலி { திரும்பு m_interfaceClass; }
    வெறுமை isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() மாறிலி { திரும்பு m_virtual; }
    வெறுமை isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() மாறிலி { திரும்பு m_needRNG; }
    வெறுமை needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() மாறிலி { திரும்பு m_useVirtualPublic; }
    வெறுமை useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    வெறுமை markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() மாறிலி { திரும்பு m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) மாறிலி;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(மாறிலி string& scope) மாறிலி;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    வெறுமை dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() மாறிலி { திரும்பு !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_செய்தி() மாறிலி override { திரும்பு hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    நிலையான string embeddedCovergroupTypeName(மாறிலி string& name) { திரும்பு "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() மாறிலி;
    // Covergroup options accessors
    முழு cgAutoBinMax() மாறிலி { திரும்பு m_cgAutoBinMax; }
    வெறுமை cgAutoBinMax(முழு மதிப்பு) { m_cgAutoBinMax = மதிப்பு; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    நிலையான bool isClassExtendedFrom(மாறிலி AstClass* refClassp, மாறிலி AstClass* baseClassp);
    uint32_t decl_Token_இலக்கம்() மாறிலி override { திரும்பு m_decl_Token_இலக்கம்; }
    வெறுமை decl_Token_இலக்கம்_Set_Min(uint32_t token_இலக்கம்) override {
        m_decl_Token_இலக்கம் = std::min(m_decl_Token_இலக்கம், token_இலக்கம்);
    }
    வெறுமை baseOverride(மாறிலி VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() மாறிலி { திரும்பு m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    நிலையான bool isCacheableChild(மாறிலி Ast_முனை* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    template <typename T_Callable>
    வெறுமை foreachMember(மாறிலி T_Callable& f) {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_முனை*>::மதிப்பு
                && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        எனில் (மாறிலி AstClassExtends* மாறிலி cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        க்கு (Ast_முனை* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (Ast_முனை::privateTypeTest<T_முனை>(stmtp)) f(this, static_cast<T_முனை*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    template <typename T_Callable>
    bool existsMember(மாறிலி T_Callable& p) மாறிலி {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, மாறிலி AstClass*, மாறிலி T_முனை*>::மதிப்பு
                && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        எனில் (மாறிலி AstClassExtends* மாறிலி cextendsp = this->extendsp()) {
            எனில் (cextendsp->classp()->existsMember(p)) திரும்பு true;
        }
        க்கு (Ast_முனை* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (Ast_முனை::privateTypeTest<T_முனை>(stmtp)) {
                எனில் (p(this, static_cast<T_முனை*>(stmtp))) திரும்பு true;
            }
        }
        திரும்பு false;
    }
};
class AstClassPackage final : பொது Ast_முனை_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
பொது:
    AstClassPackage(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() மாறிலி override { திரும்பு "classpackage"; }
    bool timescaleMatters() மாறிலி override { திரும்பு false; }
    AstClass* classp() மாறிலி VL_MT_SAFE { திரும்பு m_classp; }
    வெறுமை classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : பொது Ast_முனை_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
பொது:
    AstIface(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string verilogKwd() மாறிலி override { திரும்பு "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() மாறிலி override { திரும்பு false; }
    bool hasVirtualRef() மாறிலி { திரும்பு m_hasVirtualRef; }
    வெறுமை setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) மாறிலி;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    வெறுமை dtypeNameFreeze();
};
class AstModule final : பொது Ast_முனை_Module {
    // A module declaration
    மாறிலி bool m_isChecker;  // Module represents a checker
    மாறிலி bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

பொது:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() மாறிலி override {
        திரும்பு m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() மாறிலி override { திரும்பு true; }
    bool isChecker() மாறிலி { திரும்பு m_isChecker; }
    bool isProgram() மாறிலி { திரும்பு m_isProgram; }
    bool hasGenericIface() மாறிலி { திரும்பு m_hasGenericIface; }
    வெறுமை hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};
class AstNotFoundModule final : பொது Ast_முனை_Module {
    // A missing module declaration
பொது:
    AstNotFoundModule(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() மாறிலி override { திரும்பு "/*not-found-*/ module"; }
    bool timescaleMatters() மாறிலி override { திரும்பு false; }
};
class AstPackage final : பொது Ast_முனை_Module {
    // A package declaration
பொது:
    AstPackage(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() மாறிலி override { திரும்பு "package"; }
    bool timescaleMatters() மாறிலி override { திரும்பு !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : பொது Ast_முனை_Module {
    // A primitive declaration
பொது:
    AstPrimitive(File_வரி* fl, மாறிலி string& name, மாறிலி string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() மாறிலி override { திரும்பு "primitive"; }
    bool timescaleMatters() மாறிலி override { திரும்பு false; }
};

// === AstNodeProcedure ===
class AstAlways final : பொது Ast_முனை_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    மாறிலி VAlwaysKwd m_keyword;

பொது:
    AstAlways(File_வரி* fl, VAlwaysKwd keyword, Ast_Sen_மரம்* sentreep, Ast_முனை* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit உள்வரி AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    VAlwaysKwd keyword() மாறிலி { திரும்பு m_keyword; }
};
class AstAlwaysObserved final : பொது Ast_முனை_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

பொது:
    AstAlwaysObserved(File_வரி* fl, Ast_Sen_மரம்* sentreep, Ast_முனை* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : பொது Ast_முனை_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

பொது:
    explicit AstAlwaysPost(File_வரி* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : பொது Ast_முனை_Procedure {
    // Like always but Postponed scheduling region

பொது:
    AstAlwaysPostponed(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : பொது Ast_முனை_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

பொது:
    explicit AstAlwaysPre(File_வரி* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : பொது Ast_முனை_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

பொது:
    AstAlwaysReactive(File_வரி* fl, Ast_Sen_மரம்* sentreep, Ast_முனை* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : பொது Ast_முனை_Procedure {
பொது:
    AstFinal(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : பொது Ast_முனை_Procedure {
பொது:
    AstInitial(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : பொது Ast_முனை_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
பொது:
    AstInitialAutomatic(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : பொது Ast_முனை_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
பொது:
    AstInitialStatic(File_வரி* fl, Ast_முனை* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : பொது Ast_முனை_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
பொது:
    AstBracketRange(File_வரி* fl, Ast_முனை* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    மெய்நிகர் string emitC() { V3ERROR_NA_RETURN(""); }
    மெய்நிகர் string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு false; }
};
class AstRange final : பொது Ast_முனை_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    மாறிலி bool m_fromBracket = false;  // From C-style '[x]' declaration
பொது:
    AstRange(File_வரி* fl, Ast_முனை_கோவை* leftp, Ast_முனை_கோவை* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    உள்வரி AstRange(File_வரி* fl, முழு இடது, முழு வலது);
    உள்வரி AstRange(File_வரி* fl, மாறிலி VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    உள்வரி முழு இடது_Const() மாறிலி VL_MT_STABLE;
    உள்வரி முழு வலது_Const() மாறிலி VL_MT_STABLE;
    முழு hiConst() மாறிலி VL_MT_STABLE {
        மாறிலி முழு l = இடது_Const();
        மாறிலி முழு r = வலது_Const();
        திரும்பு l > r ? l : r;
    }
    முழு loConst() மாறிலி VL_MT_STABLE {
        மாறிலி முழு l = இடது_Const();
        மாறிலி முழு r = வலது_Const();
        திரும்பு l > r ? r : l;
    }
    முழு elementsConst() மாறிலி VL_MT_STABLE { திரும்பு hiConst() - loConst() + 1; }
    bool ascending() மாறிலி { திரும்பு இடது_Const() < வலது_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_இடது_To_வலது() மாறிலி {
        திரும்பு VNumRange{இடது_Const(), வலது_Const()}.seq_இடது_To_வலது();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_வலது_To_இடது() மாறிலி {
        திரும்பு VNumRange{இடது_Const(), வலது_Const()}.seq_வலது_To_இடது();
    }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    மெய்நிகர் string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstRange* மாறிலி asamep = VN_DBG_AS(samep, Range);
        திரும்பு fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() மாறிலி { திரும்பு m_fromBracket; }
};
class AstUnsizedRange final : பொது Ast_முனை_Range {
    // Unsized range specification, for open arrays
பொது:
    explicit AstUnsizedRange(File_வரி* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    மெய்நிகர் string emitC() { V3ERROR_NA_RETURN(""); }
    மெய்நிகர் string emitVerilog() { திரும்பு "[]"; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};
class AstWildcardRange final : பொது Ast_முனை_Range {
    // Wildcard range specification, for wildcard index type associative arrays
பொது:
    explicit AstWildcardRange(File_வரி* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    மெய்நிகர் string emitC() { V3ERROR_NA_RETURN(""); }
    மெய்நிகர் string emitVerilog() { திரும்பு "[*]"; }
    bool same_முனை(மாறிலி Ast_முனை* /*samep*/) மாறிலி override { திரும்பு true; }
};

#endif  // Guard
