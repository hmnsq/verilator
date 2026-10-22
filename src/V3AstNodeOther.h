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

#अगर_अघोषित VERILATOR_V3ASTNODES_H_
#घोषणा VERILATOR_V3ASTNODES_H_

#अगर_अघोषित VERILATOR_V3AST_H_
#त्रुटि "Use V3Ast.h as the include"
#समावेश "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#घोषणा VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#पूर्ण_अगर

#समावेश <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_गाँठ_Cons_Item VL_NOT_FINAL : खुला Ast_गाँठ {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    मित्र class Ast_गाँठ_व्यंजक;  // For m_knownConstantTree
    mutable bool m_known_Constant_वृक्ष = false;  // Known to be a tree of constants

रक्षित:
    Ast_गाँठ_Cons_Item(VNType t, File_पंक्ति* fl, Ast_गाँठ_व्यंजक* valuep)
        : Ast_गाँठ{t, fl} {
        this->valuep(valuep);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Cons_Item;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class Ast_गाँठ_Cover_Decl VL_NOT_FINAL : खुला Ast_गाँठ {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    पूर्णांक m_bin_अंक = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    पूर्णांक m_local_Bin_अंक = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
खुला:
    Ast_गाँठ_Cover_Decl(VNType t, File_पंक्ति* fl, स्थिर string& page, स्थिर string& comment)
        : Ast_गाँठ(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_गाँठ_Cover_Decl;
    स्थिर अक्षर* broken() स्थिर override {
        अगर (m_तथ्य_Declp
            && (m_तथ्य_Declp == this || m_तथ्य_Declp->m_तथ्य_Declp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        वापस nullptr;
    }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक instr_गिनती() स्थिर override { वापस 1 + 2 * INSTR_गिनती_LD; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    पूर्णांक bin_अंक() स्थिर { वापस m_bin_अंक; }
    व्योम bin_अंक(पूर्णांक flag) { m_bin_अंक = flag; }
    पूर्णांक local_Bin_अंक() स्थिर { वापस m_local_Bin_अंक; }
    व्योम local_Bin_अंक(पूर्णांक flag) { m_local_Bin_अंक = flag; }
    bool perInstance() स्थिर { वापस m_perInstance; }
    व्योम perInstance(bool flag) { m_perInstance = flag; }
    भव पूर्णांक आकार() स्थिर = 0;
    स्थिर string& comment() स्थिर { वापस m_text; }  // text to insert in code
    स्थिर string& page() स्थिर { वापस m_page; }
    स्थिर string& hier() स्थिर { वापस m_hier; }
    व्योम hier(स्थिर string& flag) { m_hier = flag; }
    व्योम comment(स्थिर string& flag) { m_text = flag; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर Ast_गाँठ_Cover_Decl* स्थिर asamep = VN_DBG_AS(samep, गाँठ_Cover_Decl);
        वापस (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    व्योम तथ्य_Declp(Ast_गाँठ_Cover_Decl* nodep) { m_तथ्य_Declp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_गाँठ_Cover_Decl* तथ्य_Decl_Nullp() स्थिर { वापस m_तथ्य_Declp; }
    Ast_गाँठ_Cover_Decl* तथ्य_Decl_Thisp() { वापस तथ्य_Decl_Nullp() ? तथ्य_Decl_Nullp() : this; }
};
class Ast_गाँठ_FTask VL_NOT_FINAL : खुला Ast_गाँठ {
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

रक्षित:
    Ast_गाँठ_FTask(VNType t, File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* stmtsp)
        : Ast_गाँठ{t, fl}
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

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_FTask;
    भव Ast_गाँठ_FTask* cloneType(स्थिर string& name) = 0;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool isGateOptimizable() स्थिर override {
        वापस !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    व्योम name(स्थिर string& name) override { m_name = name; }
    string cname() स्थिर { वापस m_cname; }
    व्योम cname(स्थिर string& cname) { m_cname = cname; }
    bool isFunction() स्थिर { वापस fvarp() != nullptr; }
    // MORE ACCESSORS
    व्योम dpiOpenParentInc() { ++m_dpiOpenParent; }
    व्योम dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() स्थिर { वापस m_dpiOpenParent; }
    bool taskPublic() स्थिर { वापस m_taskPublic; }
    व्योम taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() स्थिर { वापस m_classMethod; }
    व्योम classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() स्थिर { वापस m_didProto; }
    व्योम didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() स्थिर { वापस m_isExternProto; }
    व्योम isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() स्थिर { वापस m_isExternDef; }
    व्योम isExternDef(bool flag) { m_isExternDef = flag; }
    स्थिर string& ifacePortName() स्थिर { वापस m_ifacePortName; }
    व्योम ifacePortName(स्थिर string& name) { m_ifacePortName = name; }
    bool prototype() स्थिर { वापस m_prototype; }
    व्योम prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() स्थिर { वापस m_dpiExport; }
    व्योम dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() स्थिर { वापस m_dpiImport; }
    व्योम dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() स्थिर { वापस m_dpiContext; }
    व्योम dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() स्थिर { वापस m_dpiOpenChild; }
    व्योम dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() स्थिर { वापस m_dpiTask; }
    व्योम dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() स्थिर { वापस !m_dpiCDecl.empty(); }
    स्थिर string& dpiCDecl() स्थिर { वापस m_dpiCDecl; }
    व्योम dpiCDecl(स्थिर string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() स्थिर { वापस m_isConstructor; }
    व्योम isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() स्थिर { वापस m_isHideLocal; }
    व्योम isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() स्थिर { वापस m_isHideProtected; }
    व्योम isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() स्थिर { वापस m_dpiPure; }
    व्योम dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() स्थिर { वापस m_keepAlive; }
    व्योम keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() स्थिर { वापस m_needsSyms; }
    व्योम needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() स्थिर { वापस m_pureVirtual; }
    व्योम pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() स्थिर { वापस m_recursive; }
    व्योम recursive(bool flag) { m_recursive = flag; }
    bool isStatic() स्थिर { वापस m_static; }
    व्योम isStatic(bool flag) { m_static = flag; }
    bool underGenerate() स्थिर { वापस m_underGenerate; }
    व्योम underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() स्थिर { वापस m_verilogFunction; }
    व्योम verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() स्थिर { वापस m_verilogTask; }
    व्योम verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() स्थिर { वापस m_virtual; }
    व्योम isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() स्थिर { वापस m_needProcess; }
    व्योम setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() स्थिर { वापस m_isCovergroupSample; }
    व्योम isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    व्योम baseOverride(स्थिर VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() स्थिर { वापस m_baseOverride; }
    व्योम lifetime(स्थिर VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() स्थिर { वापस m_lifetime; }
    bool isPure() override;
    स्थिर अक्षर* broken() स्थिर override;
    व्योम propagateAttrFrom(स्थिर Ast_गाँठ_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        classMethod(fromp->classMethod());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

निजी:
    bool getPurityRecurse() स्थिर;
};
class Ast_गाँठ_File VL_NOT_FINAL : खुला Ast_गाँठ {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
खुला:
    Ast_गाँठ_File(VNType t, File_पंक्ति* fl, स्थिर string& name)
        : Ast_गाँठ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_गाँठ_File;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class Ast_गाँठ_Func_Cov_Item VL_NOT_FINAL : खुला Ast_गाँठ {
    // Base class for functional coverage items (coverpoints, crosses)
रक्षित:
    string m_name;  // Item name

खुला:
    Ast_गाँठ_Func_Cov_Item(VNType t, File_पंक्ति* fl, स्थिर string& name)
        : Ast_गाँठ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_गाँठ_Func_Cov_Item;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& name) override { m_name = name; }
    bool maybePointedTo() स्थिर override { वापस true; }
};
class Ast_गाँठ_Gen VL_NOT_FINAL : खुला Ast_गाँठ {
    // Generate construct
खुला:
    Ast_गाँठ_Gen(VNType t, File_पंक्ति* fl)
        : Ast_गाँठ{t, fl} {}
    ASTGEN_MEMBERS_Ast_गाँठ_Gen;
};
class Ast_गाँठ_Module VL_NOT_FINAL : खुला Ast_गाँठ {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    स्थिर string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    पूर्णांक m_depth = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    पूर्णांक m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
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
रक्षित:
    Ast_गाँठ_Module(VNType t, File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname)
        : Ast_गाँठ{t, fl}
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

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Module;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    भव bool timescaleMatters() स्थिर = 0;
    अंतरभूत bool isDollarUnit() स्थिर;  // Is the $unit package
    अंतरभूत bool isConstPool() स्थिर;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(स्थिर string& outer) स्थिर;
    // ACCESSORS
    व्योम name(स्थिर string& name) override { m_name = name; }
    string origName() स्थिर override { वापस m_origName; }
    string someInstanceName() स्थिर VL_MT_SAFE { वापस m_someInstanceName; }
    व्योम someInstanceName(स्थिर string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() स्थिर { वापस m_defaultDisablep; }
    व्योम defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() स्थिर { वापस m_inLibrary; }
    व्योम inLibrary(bool flag) { m_inLibrary = flag; }
    व्योम depth(पूर्णांक मूल्य) { m_depth = मूल्य; }
    पूर्णांक depth() स्थिर VL_MT_SAFE { वापस m_depth; }
    व्योम level(पूर्णांक मूल्य) { m_level = मूल्य; }
    पूर्णांक level() स्थिर VL_MT_SAFE { वापस m_level; }
    string libname() स्थिर { वापस m_libname; }
    bool is_शीर्ष() स्थिर VL_MT_SAFE { वापस level() == 1; }
    bool modPublic() स्थिर { वापस m_modPublic; }
    व्योम modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() स्थिर { वापस m_modTrace; }
    व्योम modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() स्थिर { वापस m_ctorVarReset; }
    व्योम ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() स्थिर { वापस m_dead; }
    व्योम dead(bool flag) { m_dead = flag; }
    bool hasGParam() स्थिर { वापस m_hasGParam; }
    व्योम hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() स्थिर { वापस m_hasParameterList; }
    व्योम hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() स्थिर { वापस m_hierBlock; }
    व्योम hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() स्थिर { वापस m_hierParams; }
    व्योम hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() स्थिर { वापस m_recursive; }
    व्योम recursive(bool flag) { m_recursive = flag; }
    व्योम recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() स्थिर { वापस m_recursiveClone; }
    bool parameterizedTemplate() स्थिर { वापस m_parameterizedTemplate; }
    व्योम parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    व्योम verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() स्थिर { वापस m_verilatorLib; }
    VLifetime lifetime() स्थिर { वापस m_lifetime; }
    व्योम lifetime(स्थिर VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    व्योम timeunit(स्थिर VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() स्थिर { वापस m_unconnectedDrive; }
    व्योम unconnectedDrive(स्थिर VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_गाँठ_Procedure VL_NOT_FINAL : खुला Ast_गाँठ {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
रक्षित:
    Ast_गाँठ_Procedure(VNType t, File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : Ast_गाँठ{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Procedure;
    // METHODS
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isJustOneBodyStmt() स्थिर { वापस stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() स्थिर { वापस m_suspendable; }
    व्योम setSuspendable() { m_suspendable = true; }
    bool needProcess() स्थिर { वापस m_needProcess; }
    व्योम setNeedProcess() { m_needProcess = true; }
};
class Ast_गाँठ_Range VL_NOT_FINAL : खुला Ast_गाँठ {
    // A range, sized or unsized
रक्षित:
    Ast_गाँठ_Range(VNType t, File_पंक्ति* fl)
        : Ast_गाँठ{t, fl} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Range;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : खुला Ast_गाँठ {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

खुला:
    AstActive(File_पंक्ति* fl, स्थिर string& name, Ast_Sen_वृक्ष* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    // Statements are broken into pieces, as some must come before others.
    व्योम sentreep(Ast_Sen_वृक्ष* nodep) { m_sentreep = nodep; }
    Ast_Sen_वृक्ष* sentreep() स्थिर { वापस m_sentreep; }
    // METHODS
    अंतरभूत bool hasClocked() स्थिर;
    अंतरभूत bool hasCombo() स्थिर;
    अंतरभूत bool hasInitial() स्थिर;
    अंतरभूत bool hasStatic() स्थिर;
};
class AstAlias final : खुला Ast_गाँठ {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
खुला:
    AstAlias(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : खुला Ast_गाँठ {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
खुला:
    AstAliasScope(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : खुला Ast_गाँठ {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
खुला:
    AstBind(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Bind Target name
    व्योम name(स्थिर string& name) override { m_name = name; }
};
class AstCFunc final : खुला Ast_गाँठ {
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
    पूर्णांक m_cost;  // Function call cost
खुला:
    AstCFunc(File_पंक्ति* fl, स्थिर string& name, AstScope* scopep, स्थिर string& rtnType = "")
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
        m_cost = v3Global.opt.instr_गिनती_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstCFunc* स्थिर asamep = VN_DBG_AS(samep, CFunc);
        वापस ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    व्योम name(स्थिर string& name) override { m_name = name; }
    पूर्णांक instr_गिनती() स्थिर override { वापस m_cost; }
    VBoolOrUnknown isConst() स्थिर { वापस m_isConst; }
    व्योम isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    व्योम isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() स्थिर { वापस m_isStatic; }
    व्योम isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() स्थिर VL_MT_SAFE { वापस m_isTrace; }
    व्योम isTrace(bool flag) { m_isTrace = flag; }
    व्योम cname(स्थिर string& name) { m_cname = name; }
    string cname() स्थिर { वापस m_cname; }
    AstScope* scopep() स्थिर { वापस m_scopep; }
    व्योम scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() स्थिर { वापस ((m_rtnType == "") ? "void" : m_rtnType); }
    व्योम rtnType(स्थिर string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() स्थिर { वापस m_dontCombine || isTrace() || entryPoint(); }
    व्योम dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() स्थिर { वापस m_declPrivate; }
    व्योम declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() स्थिर VL_MT_SAFE { वापस m_keepIfEmpty; }
    व्योम keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() स्थिर VL_MT_SAFE { वापस m_slow; }
    व्योम slow(bool flag) { m_slow = flag; }
    bool funcPublic() स्थिर { वापस m_funcPublic; }
    व्योम funcPublic(bool flag) { m_funcPublic = flag; }
    व्योम argTypes(स्थिर string& str) { m_argTypes = str; }
    string argTypes() स्थिर { वापस m_argTypes; }
    व्योम ifdef(स्थिर string& str) { m_ifdef = str; }
    string ifdef() स्थिर { वापस m_ifdef; }
    bool isConstructor() स्थिर { वापस m_isConstructor; }
    व्योम isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() स्थिर { वापस m_isDestructor; }
    व्योम isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() स्थिर { वापस m_isMethod; }
    व्योम isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() स्थिर { वापस m_isLoose; }
    व्योम isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() स्थिर { वापस isMethod() && !isLoose(); }
    bool isVirtual() स्थिर { वापस m_isVirtual; }
    व्योम isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() स्थिर { वापस m_needProcess; }
    व्योम setNeedProcess() { m_needProcess = true; }
    bool entryPoint() स्थिर { वापस m_entryPoint; }
    व्योम entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() स्थिर { वापस m_dpiPure; }
    व्योम dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() स्थिर { वापस m_dpiContext; }
    व्योम dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() स्थिर VL_MT_SAFE { वापस m_dpiExportDispatcher; }
    व्योम dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() स्थिर { वापस m_dpiExportImpl; }
    व्योम dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() स्थिर VL_MT_SAFE { वापस m_dpiImportPrototype; }
    व्योम dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() स्थिर { वापस m_dpiImportWrapper; }
    व्योम dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() स्थिर { वापस !m_dpiCDecl.empty(); }
    स्थिर string& dpiCDecl() स्थिर { वापस m_dpiCDecl; }
    व्योम dpiCDecl(स्थिर string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() स्थिर { वापस m_rtnType == "VlCoroutine"; }
    व्योम recursive(bool flag) { m_recursive = flag; }
    bool recursive() स्थिर { वापस m_recursive; }
    व्योम unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() स्थिर override { वापस m_unlikely; }  // Note virtual override
    व्योम noLife(bool flag) { m_noLife = flag; }
    bool noLife() स्थिर { वापस m_noLife; }
    bool isCovergroupSample() स्थिर { वापस m_isCovergroupSample; }
    व्योम isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() स्थिर { वापस m_voidSelf; }
    व्योम voidSelf(bool flag) { m_voidSelf = flag; }
    व्योम cost(पूर्णांक cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() स्थिर {
        वापस !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : खुला Ast_गाँठ {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    स्थिर string m_name;
    स्थिर VUseType m_useType;  // What sort of use this is

खुला:
    AstCUse(File_पंक्ति* fl, VUseType useType, स्थिर string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    VUseType useType() स्थिर { वापस m_useType; }
};
class AstCell final : खुला Ast_गाँठ {
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
    File_पंक्ति* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    पूर्णांक m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
खुला:
    AstCell(File_पंक्ति* fl, File_पंक्ति* mfl, स्थिर string& instName, स्थिर string& modName,
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
    व्योम cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    // ACCESSORS
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Cell name
    व्योम name(स्थिर string& name) override { m_name = name; }
    string origName() स्थिर override { वापस m_origName; }  // * = Original name
    व्योम origName(स्थिर string& name) { m_origName = name; }
    string modName() स्थिर { वापस m_modName; }  // * = Instance name
    व्योम modName(स्थिर string& name) { m_modName = name; }
    File_पंक्ति* modNameFileline() स्थिर { वापस m_modNameFileline; }
    Ast_गाँठ_Module* modp() स्थिर { वापस m_modp; }  // [AfterLink] = Pointer to module instantiated
    व्योम modp(Ast_गाँठ_Module* nodep) { m_modp = nodep; }
    पूर्णांक arrayIdx() स्थिर { वापस m_arrayIdx; }
    व्योम arrayIdx(पूर्णांक idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() स्थिर { वापस m_hasIfaceVar; }
    व्योम hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    व्योम trace(bool flag) { m_trace = flag; }
    bool isTrace() स्थिर { वापस m_trace; }
    व्योम recursive(bool flag) { m_recursive = flag; }
    bool recursive() स्थिर { वापस m_recursive; }
};
class AstCellInline final : खुला Ast_गाँठ {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    स्थिर std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    स्थिर std::string m_origModName;  // __BEGIN__ or __FORK__
खुला:
    AstCellInline(File_पंक्ति* fl, स्थिर std::string& name, स्थिर std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    // ACCESSORS
    std::string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    std::string origModName() स्थिर { वापस m_origModName; }
};
class AstCellInlineScope final : खुला Ast_गाँठ {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    स्थिर std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
खुला:
    AstCellInlineScope(File_पंक्ति* fl, स्थिर std::string& name, स्थिर std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    // ACCESSORS
    std::string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर std::string& name) override { m_name = name; }
    std::string origModName() स्थिर { वापस m_origModName; }
};
class AstCgOptionAssign final : खुला Ast_गाँठ {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    स्थिर VCoverOptionType m_optType;  // Option type
    स्थिर string m_name;  // Original option name (for diagnostics on unknown options)
    स्थिर bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
खुला:
    AstCgOptionAssign(File_पंक्ति* fl, bool typeOption, VCoverOptionType optType,
                      स्थिर string& rawName, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    VCoverOptionType optType() स्थिर { वापस m_optType; }
    bool typeOption() स्थिर { वापस m_typeOption; }
};
class AstClassExtends final : खुला Ast_गाँठ {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    स्थिर bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

खुला:
    AstClassExtends(File_पंक्ति* fl, Ast_गाँठ* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    string verilogKwd() स्थिर override { वापस isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() स्थिर;
    AstClass* classp() स्थिर;  // Like above, but throws error if nulll
    bool isImplements() स्थिर { वापस m_isImplements; }
    व्योम parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() स्थिर { वापस m_parameterized; }
};
class AstClocking final : खुला Ast_गाँठ {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    स्थिर bool m_isGlobal;  // True if global clocking

खुला:
    AstClocking(File_पंक्ति* fl, स्थिर std::string& name, Ast_Sen_वृक्ष* sentreep, Ast_गाँठ* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    std::string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool isDefault() स्थिर { वापस m_isDefault; }
    bool isGlobal() स्थिर { वापस m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    व्योम makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : खुला Ast_गाँठ {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

खुला:
    AstClockingItem(File_पंक्ति* fl, VDirection direction, Ast_गाँठ_व्यंजक* skewp, Ast_गाँठ* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        अगर (AstAssign* स्थिर clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } अन्यथा {
            exprp(VN_AS(clockingDeclp, गाँठ_व्यंजक));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    VDirection direction() स्थिर { वापस m_direction; }
    AstClockingItem* outputp() स्थिर { वापस m_outputp; }
    व्योम outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
};
class AstConfig final : खुला Ast_गाँठ {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

खुला:
    AstConfig(File_पंक्ति* fl, स्थिर std::string& libname, स्थिर std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    std::string name() स्थिर override VL_MT_STABLE { वापस m_libname + "." + m_configname; }
    std::string libname() स्थिर VL_MT_STABLE { वापस m_libname; }
    std::string configname() स्थिर VL_MT_STABLE { वापस m_configname; }
};
class AstConfigCell final : खुला Ast_गाँठ {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

खुला:
    AstConfigCell(File_पंक्ति* fl, स्थिर std::string& libname, स्थिर std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() स्थिर override VL_MT_STABLE { वापस m_libname + "." + m_cellname; }
    std::string libname() स्थिर VL_MT_STABLE { वापस m_libname; }
    std::string cellname() स्थिर VL_MT_STABLE { वापस m_cellname; }
};
class AstConfigRule final : खुला Ast_गाँठ {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    स्थिर bool m_isCell;  // Declared as "cell" versus "instance"

खुला:
    AstConfigRule(File_पंक्ति* fl, Ast_गाँठ* cellp, Ast_गाँठ* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() स्थिर VL_MT_STABLE { वापस m_isCell; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstConfigUse final : खुला Ast_गाँठ {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    स्थिर bool m_isConfig;  // ":config"; Config, not module/primitive name

खुला:
    AstConfigUse(File_पंक्ति* fl, स्थिर std::string& libname, स्थिर std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() स्थिर override VL_MT_STABLE { वापस m_libname + "." + m_cellname; }
    std::string libname() स्थिर VL_MT_STABLE { वापस m_libname; }
    std::string cellname() स्थिर VL_MT_STABLE { वापस m_cellname; }
    bool isConfig() स्थिर VL_MT_STABLE { वापस m_isConfig; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstConstraint final : खुला Ast_गाँठ {
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
खुला:
    AstConstraint(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Scope name
    व्योम name(स्थिर string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    व्योम baseOverride(स्थिर VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() स्थिर { वापस m_baseOverride; }
    bool isExternDef() स्थिर { वापस m_isExternDef; }
    व्योम isExternDef(bool flag) { m_isExternDef = flag; }
    व्योम isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() स्थिर { वापस m_isExternExplicit; }
    व्योम isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() स्थिर { वापस m_isExternProto; }
    व्योम isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() स्थिर { वापस m_isKwdPure; }
    व्योम isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() स्थिर { वापस m_isStatic; }
};
class AstCoverBin final : खुला Ast_गाँठ {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    स्थिर string m_name;  // Base name of the bin
    स्थिर VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

खुला:
    AstCoverBin(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_गाँठ_व्यंजक* iffp = nullptr)
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
    AstCoverBin(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_व्यंजक* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_पंक्ति* fl, स्थिर string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_पंक्ति* fl, स्थिर string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    VCoverBinsType binsType() स्थिर { वापस m_binsType; }
    bool isWildcard() स्थिर { वापस m_isWildcard; }
    bool isArray() स्थिर { वापस m_isArray; }
    व्योम isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : खुला Ast_गाँठ {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    स्थिर bool m_isNegated;  // Complement the selection within the cross product

खुला:
    AstCoverBinsof(File_पंक्ति* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_गाँठ* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& name) override { m_name = name; }
    bool isNegated() स्थिर { वापस m_isNegated; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {  // LCOV_EXCL_START
        स्थिर AstCoverBinsof* स्थिर asamep = VN_DBG_AS(samep, CoverBinsof);
        वापस m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : खुला Ast_गाँठ {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    स्थिर string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    स्थिर VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

खुला:
    AstCoverCrossBin(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* selectp, Ast_गाँठ_व्यंजक* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    string verilogKwd() स्थिर override;
    VCoverBinsType binsType() स्थिर { वापस m_binsType; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {  // LCOV_EXCL_START
        स्थिर AstCoverCrossBin* स्थिर asamep = VN_DBG_AS(samep, CoverCrossBin);
        वापस m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : खुला Ast_गाँठ {
    // The complete bin-tuple space of the enclosing cross
    स्थिर string m_name;  // Referenced cross name

खुला:
    AstCoverCrossRef(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {  // LCOV_EXCL_START
        वापस m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : खुला Ast_गाँठ {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    स्थिर bool m_isOr;  // Union (||), rather than intersection (&&)

खुला:
    AstCoverCrossSelect(File_पंक्ति* fl, Ast_गाँठ* lhsp, Ast_गाँठ* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool isOr() स्थिर { वापस m_isOr; }
    string verilogKwd() स्थिर override { वापस isOr() ? "||" : "&&"; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {  // LCOV_EXCL_START
        वापस m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : खुला Ast_गाँठ {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
खुला:
    AstCoverCrossWith(File_पंक्ति* fl, Ast_गाँठ* selectp, Ast_गाँठ_व्यंजक* filterp, Ast_गाँठ_व्यंजक* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : खुला Ast_गाँठ {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    स्थिर VCoverOptionType m_optType;  // Option being assigned
    स्थिर bool m_typeOption;  // type_option (vs option)

खुला:
    AstCoverOption(File_पंक्ति* fl, bool typeOption, VCoverOptionType optType, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    VCoverOptionType optType() स्थिर { वापस m_optType; }
    bool typeOption() स्थिर { वापस m_typeOption; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {  // LCOV_EXCL_START
        स्थिर AstCoverOption* स्थिर asamep = VN_DBG_AS(samep, CoverOption);
        वापस m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : खुला Ast_गाँठ {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
खुला:
    AstCoverToggle(File_पंक्ति* fl, AstCoverInc* incp, Ast_गाँठ_व्यंजक* origp, Ast_गाँठ_व्यंजक* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    पूर्णांक instr_गिनती() स्थिर override { वापस 3 + INSTR_गिनती_BRANCH + INSTR_गिनती_LD; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस true; }
    bool isOutputter() override {
        वापस false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : खुला Ast_गाँठ {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    स्थिर VTransRepType m_repType;

खुला:
    AstCoverTransItem(File_पंक्ति* fl, Ast_गाँठ* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstCoverTransSet final : खुला Ast_गाँठ {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
खुला:
    AstCoverTransSet(File_पंक्ति* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstCoverWith final : खुला Ast_गाँठ {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
खुला:
    AstCoverWith(File_पंक्ति* fl, Ast_गाँठ* subp, Ast_गाँठ_व्यंजक* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        addSubp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : खुला Ast_गाँठ {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

खुला:
    AstCovergroup(File_पंक्ति* fl, स्थिर string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_गाँठ* membersp, Ast_Sen_वृक्ष* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& name) override { m_name = name; }
    bool maybePointedTo() स्थिर override { वापस true; }
};
class AstCoverpointRef final : खुला Ast_गाँठ {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    स्थिर string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

खुला:
    AstCoverpointRef(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
};
class AstDefParam final : खुला Ast_गाँठ {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
खुला:
    AstDefParam(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* pathp, स्थिर string& name, Ast_गाँठ_व्यंजक* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस true; }
};
class AstDefaultClocking final : खुला Ast_गाँठ {
    std::string m_name;  // Clocking block name

खुला:
    AstDefaultClocking(File_पंक्ति* fl, स्थिर std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() स्थिर override VL_MT_STABLE { वापस m_name; }
};
class AstDefaultDisable final : खुला Ast_गाँठ {
    // @astgen op1 := condp : AstNodeExpr

खुला:
    AstDefaultDisable(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : खुला Ast_गाँठ {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
खुला:
    AstDpiExport(File_पंक्ति* fl, स्थिर string& vname, स्थिर string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& name) override { m_name = name; }
    string cname() स्थिर { वापस m_cname; }
    व्योम cname(स्थिर string& cname) { m_cname = cname; }
};
class AstElabDisplay final : खुला Ast_गाँठ {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

खुला:
    अंतरभूत AstElabDisplay(File_पंक्ति* fl, VDisplayType dispType, Ast_गाँठ_व्यंजक* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!fmtp());
        वापस nullptr;
    }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string verilogKwd() स्थिर override { वापस "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() स्थिर override { वापस false; }
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool isPure() override { वापस false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { वापस true; }  // SPECIAL: $display makes output
    bool isUnlikely() स्थिर override { वापस true; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    पूर्णांक instr_गिनती() स्थिर override { वापस INSTR_गिनती_PLI; }
    VDisplayType displayType() स्थिर { वापस m_displayType; }
    व्योम displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : खुला Ast_गाँठ {
    // Represents something missing, e.g. a missing argument in FOREACH
खुला:
    explicit AstEmpty(File_पंक्ति* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstGenCaseItem final : खुला Ast_गाँठ {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
खुला:
    AstGenCaseItem(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condsp, Ast_गाँठ* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() स्थिर { वापस condsp() == nullptr; }
};
class AstImplicit final : खुला Ast_गाँठ {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
खुला:
    AstImplicit(File_पंक्ति* fl, Ast_गाँठ* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : खुला Ast_गाँठ {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
खुला:
    AstIntfRef(File_पंक्ति* fl, स्थिर string& name, स्थिर string& parentName, स्थिर string& baseName,
               स्थिर string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    string parentName() स्थिर { वापस m_parentName; }
    string baseName() स्थिर { वापस m_baseName; }
    string modportName() स्थिर { वापस m_modportName; }
};
class AstLibrary final : खुला Ast_गाँठ {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

खुला:
    AstLibrary(File_पंक्ति* fl, स्थिर std::string& name, Ast_गाँठ_व्यंजक* filesp, Ast_गाँठ_व्यंजक* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() स्थिर override VL_MT_STABLE { वापस m_name; }
};
class AstModport final : खुला Ast_गाँठ {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
खुला:
    AstModport(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() स्थिर override { वापस "modport"; }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : खुला Ast_गाँठ {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
खुला:
    AstModportClockingRef(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    व्योम dump(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    AstClocking* clockingp() स्थिर VL_MT_STABLE {
        वापस m_clockingp;
    }  // [After Link] Pointer to clocking block
    व्योम clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : खुला Ast_गाँठ {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
खुला:
    AstModportFTaskRef(File_पंक्ति* fl, स्थिर string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool isImport() स्थिर { वापस !m_export; }
    bool isExport() स्थिर { वापस m_export; }
    Ast_गाँठ_FTask* ftaskp() स्थिर { वापस m_ftaskp; }  // [After Link] Pointer to variable
    व्योम ftaskp(Ast_गाँठ_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : खुला Ast_गाँठ {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    स्थिर VDirection m_direction;  // Direction of the variable (in/out)
खुला:
    AstModportVarRef(File_पंक्ति* fl, स्थिर string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_व्यंजक* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    VDirection direction() स्थिर { वापस m_direction; }
    AstVar* varp() स्थिर VL_MT_STABLE { वापस m_varp; }  // [After Link] Pointer to variable
    व्योम varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : खुला Ast_गाँठ {
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
    std::string m_resolved_शीर्ष_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::set<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<स्थिर AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<स्थिर Ast_गाँठ*, स्थिर Ast_गाँठ_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

खुला:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    स्थिर अक्षर* broken() स्थिर override;
    व्योम धकेलो_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.जोड़ो(varp); }
    स्थिर std::set<AstVar*>& deferredParamVarps() स्थिर { वापस m_deferredParamVarps; }
    व्योम clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    व्योम deleteContents();
    व्योम cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() स्थिर override VL_MT_STABLE { वापस "$root"; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    Ast_गाँठ_Module* शीर्ष_Modulep() स्थिर VL_MT_STABLE {  // Top module in hierarchy
        वापस modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { वापस m_typeTablep; }
    string astConstOrigParamName(स्थिर AstConst* nodep) स्थिर;
    व्योम astConstOrigParamName(स्थिर AstConst* nodep, स्थिर string& name);
    व्योम astConstOrigParamNameErase(स्थिर AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    स्थिर Ast_गाँठ_Module* containingModule(स्थिर Ast_गाँठ* nodep);
    // Forget remembered modules, as the tree has moved.
    व्योम clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() स्थिर { वापस m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() स्थिर { वापस m_dollarUnitPkgp; }
    व्योम dollarUnitPkgp(AstPackage* स्थिर packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() स्थिर { वापस m_dpiExportTriggerp; }
    व्योम dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() स्थिर { वापस m_delaySchedulerp; }
    व्योम delaySchedulerp(AstVar* स्थिर varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() स्थिर { वापस m_nbaEventp; }
    व्योम nbaEventp(AstVarScope* स्थिर varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() स्थिर { वापस m_nbaEventTriggerp; }
    व्योम nbaEventTriggerp(AstVarScope* स्थिर varScopep) { m_nbaEventTriggerp = varScopep; }
    व्योम stdPackagep(AstPackage* स्थिर packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() स्थिर { वापस m_stdPackagep; }
    व्योम stdPackageProcessp(AstClass* स्थिर classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() स्थिर { वापस m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_पंक्ति*) स्थिर;
    Ast_शीर्ष_Scope* शीर्ष_Scopep() स्थिर { वापस m_शीर्ष_Scopep; }
    व्योम create_शीर्ष_Scope(AstScope* scopep);
    VTimescale timeunit() स्थिर { वापस m_timeunit; }
    व्योम timeunit(स्थिर VTimescale& मूल्य) { m_timeunit = मूल्य; }
    VTimescale timeprecision() स्थिर { वापस m_timeprecision; }
    व्योम timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    व्योम timeprecisionMerge(File_पंक्ति*, स्थिर VTimescale& मूल्य);
    व्योम timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() स्थिर { वापस m_timescaleSpecified; }
    स्थिर std::string& resolved_शीर्ष_Module_Name() स्थिर { वापस m_resolved_शीर्ष_Module_Name; }
    व्योम resolved_शीर्ष_Module_Name(स्थिर std::string& मूल्य) { m_resolved_शीर्ष_Module_Name = मूल्य; }
    uint32_t nTraceCodes() स्थिर { वापस m_nTraceCodes; }
    व्योम nTraceCodes(uint32_t मूल्य) { m_nTraceCodes = मूल्य; }
    AstVarScope* stlFirstIterationp();
    व्योम clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    स्थिर std::string trace_Lib_शीर्ष_Name() स्थिर {
        स्थिर std::string& name = resolved_शीर्ष_Module_Name();
        वापस prettyName(name.empty() ? v3Global.rootp()->शीर्ष_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    व्योम addEvalStats(स्थिर std::string& phase);
};
class AstPackageExport final : खुला Ast_गाँठ {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

खुला:
    AstPackageExport(File_पंक्ति* fl, स्थिर string& pkgName, स्थिर string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    string pkgName() स्थिर VL_MT_STABLE { वापस m_pkgName; }
    string prettyPkgNameQ() स्थिर { वापस "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() स्थिर { वापस m_packagep; }
    व्योम packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : खुला Ast_गाँठ {
    // A package export *::* declaration
खुला:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_पंक्ति* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : खुला Ast_गाँठ {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

खुला:
    AstPackageImport(File_पंक्ति* fl, AstPackage* packagep, स्थिर string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_पंक्ति* fl, स्थिर string& pkgName, स्थिर string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    string pkgName() स्थिर VL_MT_STABLE { वापस m_pkgName; }
    string prettyPkgNameQ() स्थिर { वापस "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() स्थिर { वापस m_packagep; }
    व्योम packagep(AstPackage* nodep) { m_packagep = nodep; }

निजी:
    व्योम pkgNameFrom();
};
class AstPin final : खुला Ast_गाँठ {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    पूर्णांक m_pin_अंक;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
खुला:
    AstPin(File_पंक्ति* fl, पूर्णांक pin_अंक, स्थिर string& name, Ast_गाँठ* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_अंक{pin_अंक}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    व्योम cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Pin name, ""=go by number
    व्योम name(स्थिर string& name) override { m_name = name; }
    string prettyOperatorName() स्थिर override;
    bool dotStar() स्थिर { वापस name() == ".*"; }  // Fake name for .* connections until linked
    पूर्णांक pin_अंक() स्थिर { वापस m_pin_अंक; }
    AstVar* modVarp() स्थिर { वापस m_modVarp; }  // [After Link] Pointer to variable
    व्योम modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() स्थिर { वापस m_modPTypep; }
    व्योम modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() स्थिर { वापस m_param; }
    व्योम param(bool flag) { m_param = flag; }
    स्थिर string& paramPath() स्थिर { वापस m_paramPath; }
    व्योम paramPath(स्थिर string& path) { m_paramPath = path; }
    bool svDotName() स्थिर { वापस m_svDotName; }
    व्योम svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() स्थिर { वापस m_svImplicit; }
    व्योम svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : खुला Ast_गाँठ {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    स्थिर पूर्णांक m_pin_अंक;  // Pin number
    स्थिर string m_name;  // Name of pin
खुला:
    AstPort(File_पंक्ति* fl, पूर्णांक pinnum, स्थिर string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_अंक{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Port name
    पूर्णांक pin_अंक() स्थिर { वापस m_pin_अंक; }  // * = Pin number, for order based instantiation
};
class AstPragma final : खुला Ast_गाँठ {
    स्थिर VPragmaType m_pragType;  // Type of pragma
    स्थिर VTimescale m_timescale;  // For TIMEUNIT_SET
खुला:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_पंक्ति* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_पंक्ति* fl, स्थिर VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    VPragmaType pragType() स्थिर { वापस m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() स्थिर override { वापस false; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() स्थिर { वापस m_timescale; }
};
class AstPropSpec final : खुला Ast_गाँठ {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

खुला:
    AstPropSpec(File_पंक्ति* fl, AstSenItem* sensesp, Ast_गाँठ_व्यंजक* disablep, Ast_गाँठ* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool hasDType() स्थिर override VL_MT_SAFE {
        वापस true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() स्थिर { वापस m_propStrength; }
};
class AstPull final : खुला Ast_गाँठ {
    // @astgen op1 := lhsp : AstNodeExpr

    स्थिर bool m_direction;

खुला:
    AstPull(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() स्थिर { वापस m_direction; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstScope final : खुला Ast_गाँठ {
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
खुला:
    AstScope(File_पंक्ति* fl, Ast_गाँठ_Module* modp, स्थिर string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!m_modp);
        वापस nullptr;
    }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Scope name
    व्योम name(स्थिर string& name) override { m_name = name; }
    string pretty_Name_संदेश() स्थिर override;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    string nameDotless() स्थिर;
    Ast_गाँठ_Module* modp() स्थिर { वापस m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() स्थिर VL_MT_SAFE { वापस m_aboveScopep; }
    व्योम aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() स्थिर { वापस m_aboveCellp; }
    व्योम aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_शीर्ष() स्थिर VL_MT_SAFE { वापस aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : खुला Ast_गाँठ {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_edgeType;  // Edge type
खुला:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    AstSenItem(File_पंक्ति* fl, VEdgeType edgeType, Ast_गाँठ_व्यंजक* senp, Ast_गाँठ_व्यंजक* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_पंक्ति* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_पंक्ति* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_पंक्ति* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_पंक्ति* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_पंक्ति* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_पंक्ति* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() स्थिर { वापस m_edgeType; }
    व्योम edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_गिनती_Inc();
    }
    Ast_गाँठ_Var_Ref* varrefp() स्थिर { वापस VN_CAST(sensp(), गाँठ_Var_Ref); }
    //
    bool isClocked() स्थिर { वापस edgeType().clockedStmt(); }
    bool isComboOrStar() स्थिर {
        वापस edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() स्थिर { वापस edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() स्थिर { वापस edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() स्थिर { वापस edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() स्थिर { वापस edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() स्थिर { वापस edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() स्थिर { वापस edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_वृक्ष final : खुला Ast_गाँठ {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
खुला:
    Ast_Sen_वृक्ष(File_पंक्ति* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_वृक्ष(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_वृक्ष;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    // METHODS
    bool hasClocked() स्थिर;  // Includes a clocked statement
    bool hasEdge() स्थिर;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() स्थिर;  // Includes a STATIC SenItem
    bool hasInitial() स्थिर;  // Includes a INITIAL SenItem
    bool hasFinal() स्थिर;  // Includes a FINAL SenItem
    bool hasCombo() स्थिर;  // Includes a COMBO SenItem
    bool hasHybrid() स्थिर;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : खुला Ast_गाँठ {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

खुला:
    AstStrengthSpec(File_पंक्ति* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { वापस m_s0; }
    VStrength strength1() { वापस m_s1; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstSystemCSection final : खुला Ast_गाँठ {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    स्थिर VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

खुला:
    AstSystemCSection(File_पंक्ति* fl, VSystemCSectionType sectionType, स्थिर std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() स्थिर { वापस m_sectionType; }
    स्थिर std::string& text() स्थिर { वापस m_text; }
    व्योम text(स्थिर std::string& मूल्य) { m_text = मूल्य; }
    व्योम dump(std::ostream&) स्थिर override;
    व्योम dumpJson(std::ostream&) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर override { वापस false; }
    bool isPure() override { वापस false; }
    bool isOutputter() override { वापस true; }
};
class AstText final : खुला Ast_गाँठ {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    स्थिर std::string m_text;  // The text to emit
खुला:
    AstText(File_पंक्ति* fl, स्थिर std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस text() == VN_DBG_AS(samep, Text)->text();
    }
    स्थिर std::string& text() स्थिर VL_MT_SAFE { वापस m_text; }
};
class AstTextBlock final : खुला Ast_गाँठ {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    स्थिर std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    स्थिर std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    स्थिर std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
खुला:
    explicit AstTextBlock(File_पंक्ति* fl,  //
                          स्थिर std::string& prefix = "",  //
                          स्थिर std::string& separator = "",  //
                          स्थिर std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    स्थिर std::string& prefix() स्थिर { वापस m_prefix; }
    स्थिर std::string& separator() स्थिर { वापस m_separator; }
    स्थिर std::string& suffix() स्थिर { वापस m_suffix; }
    // Add some text, or a node to this block
    व्योम add(स्थिर string& text) { addNodesp(new AstText{fileline(), text}); }
    व्योम add(Ast_गाँठ* nodep) { addNodesp(nodep); }
};
class Ast_शीर्ष_Scope final : खुला Ast_गाँठ {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    मित्र class AstNetlist;  // Only the AstNetlist can create one
    Ast_शीर्ष_Scope(File_पंक्ति* fl, AstScope* ascopep)
        : ASTGEN_SUPER_शीर्ष_Scope(fl) {
        this->scopep(ascopep);
    }

खुला:
    ASTGEN_MEMBERS_Ast_शीर्ष_Scope;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* create_अस्थायी(स्थिर string& name, अचिन्हित width);
    AstVarScope* create_अस्थायी(स्थिर string& name, Ast_गाँठ_DType* dtypep);
};
class AstTypeTable final : खुला Ast_गाँठ {
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
    using DetailedMap = std::map<VBasic_Type_कुंजी, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

खुला:
    explicit AstTypeTable(File_पंक्ति* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    व्योम cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_पंक्ति* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_पंक्ति* fl, VBasicDTypeKwd kwd, पूर्णांक width, पूर्णांक widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_पंक्ति* fl, VBasicDTypeKwd kwd, स्थिर VNumRange& range,
                                     पूर्णांक widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& गाँठ);
    AstBasicDType* find_जोड़ो_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_पंक्ति* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_पंक्ति* fl);
    AstQueueDType* find_Queue_सूचकांक_DType(File_पंक्ति* fl);
    AstStreamDType* findStreamDType(File_पंक्ति* fl);
    AstVoidDType* findVoidDType(File_पंक्ति* fl);
    व्योम clearCache();
    व्योम repairCache();
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
};
class AstTypedef final : खुला Ast_गाँठ {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_अंक;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

खुला:
    AstTypedef(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* attrsp, VFlagChildDType,
               Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_अंक{fl->token_अंक()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_अंक{fl->token_अंक()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() स्थिर;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    भव Ast_गाँठ_DType* subDTypep() स्थिर VL_MT_STABLE {
        वापस dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_अंक() स्थिर override { वापस m_decl_Token_अंक; }
    व्योम decl_Token_अंक_Set_Min(uint32_t token_अंक) override {
        m_decl_Token_अंक = std::min(m_decl_Token_अंक, token_अंक);
    }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    व्योम name(स्थिर string& flag) override { m_name = flag; }
    bool attrPublic() स्थिर { वापस m_attrPublic; }
    व्योम attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() स्थिर { वापस m_isHideLocal; }
    व्योम isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() स्थिर { वापस m_isHideProtected; }
    व्योम isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() स्थिर { वापस m_isUnderClass; }
    व्योम isUnderClass(bool flag) { m_isUnderClass = flag; }
    व्योम tag(स्थिर string& text) override { m_tag = text; }
    string tag() स्थिर override { वापस m_tag; }
};
class AstTypedefFwd final : खुला Ast_गाँठ {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    स्थिर VFwdType m_fwdType;  // Forward type for lint check

खुला:
    AstTypedefFwd(File_पंक्ति* fl, स्थिर string& name, स्थिर VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    VFwdType fwdType() स्थिर { वापस m_fwdType; }
};
class AstUdpTable final : खुला Ast_गाँठ {
    // @astgen op1 := linesp : List[AstUdpTableLine]
खुला:
    AstUdpTable(File_पंक्ति* fl, Ast_Udp_Table_पंक्ति* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        अगर (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_पंक्ति final : खुला Ast_गाँठ {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
निजी:
    स्थिर bool m_udpIsCombo;  // Combinational or sequential UDP

खुला:
    class UdpCombo {};
    Ast_Udp_Table_पंक्ति(UdpCombo, File_पंक्ति* fl, Ast_Udp_Table_पंक्ति_Val* iFieldsp,
                    Ast_Udp_Table_पंक्ति_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_पंक्ति(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_पंक्ति(UdpSequential, File_पंक्ति* fl, Ast_Udp_Table_पंक्ति_Val* iFieldsp,
                    Ast_Udp_Table_पंक्ति_Val* oFieldsp1, Ast_Udp_Table_पंक्ति_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_पंक्ति(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_पंक्ति;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक udpIsCombo() स्थिर { वापस m_udpIsCombo; }
};
class Ast_Udp_Table_पंक्ति_Val final : खुला Ast_गाँठ {
    string m_text;  // Value character

खुला:
    Ast_Udp_Table_पंक्ति_Val(File_पंक्ति* fl, स्थिर string& text)
        : ASTGEN_SUPER_Udp_Table_पंक्ति_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_पंक्ति_Val;
    string name() स्थिर override VL_MT_STABLE { वापस m_text; }
    व्योम name(std::string स्थिर& text) override VL_MT_STABLE { m_text = text; }
    string text() स्थिर VL_MT_SAFE { वापस m_text; }
};
class AstVar final : खुला Ast_गाँठ {
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
    पूर्णांक m_pin_अंक = 0;  // For JSON, if non-zero the connection pin number
    पूर्णांक m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
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
    bool m_mtask_Cache_पंक्ति_Align : 1;  // Start MTask affinity group on a cache line
    व्योम init() {
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
        m_mtask_Cache_पंक्ति_Align = false;
    }

खुला:
    AstVar(File_पंक्ति* fl, VVarType type, स्थिर string& name, VFlagChildDType, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_पंक्ति* fl, VVarType type, स्थिर string& name, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_पंक्ति* fl, VVarType type, स्थिर string& name, VFlagLogicPacked, पूर्णांक wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_पंक्ति* fl, VVarType type, स्थिर string& name, VFlagBitPacked, पूर्णांक wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_पंक्ति* fl, VVarType type, स्थिर string& name, स्थिर AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        अगर (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_वृक्ष(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    अंतरभूत bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    string origName() स्थिर override { वापस m_origName; }  // * = Original name
    व्योम origName(स्थिर string& name) { m_origName = name; }
    VVarType varType() स्थिर VL_MT_SAFE { वापस m_varType; }  // * = Type of variable
    व्योम direction(स्थिर VDirection& flag) {
        m_direction = flag;
        अगर (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() स्थिर VL_MT_SAFE { वापस m_direction; }
    bool isIO() स्थिर VL_MT_SAFE { वापस m_direction != VDirection::NONE; }
    bool isVLIO() स्थिर {
        स्थिर AstBasicDType* स्थिर bdtypep = basicp();
        वापस isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    व्योम declDirection(स्थिर VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() स्थिर { वापस m_declDirection; }
    पूर्णांक libInoutId() स्थिर { वापस m_libInoutId; }
    व्योम libInoutId(पूर्णांक id) { m_libInoutId = id; }
    bool libInoutEnable() स्थिर { वापस m_libInoutEnable; }
    व्योम libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    व्योम varType(VVarType type) { m_varType = type; }
    व्योम varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    व्योम varType2In() {
        m_tristate = false;
        m_direction = VDirection::निवेश;
    }
    string scType() स्थिर;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) स्थिर;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) स्थिर;
    string dpiTmpVarType(स्थिर string& varName) स्थिर;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) स्थिर;
    string vlEnumType() स्थिर;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) स्थिर;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(स्थिर string& propName) स्थिर;  // Return VerilatorVarProps declaration
    व्योम combineType(VVarType type);
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* dtypeSkipRefp() स्थिर VL_MT_STABLE { वापस subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() स्थिर VL_MT_STABLE { वापस subDTypep()->basicp(); }
    भव Ast_गाँठ_DType* subDTypep() स्थिर VL_MT_STABLE {
        वापस dtypep() ? dtypep() : childDTypep();
    }
    व्योम ansi(bool flag) { m_ansi = flag; }
    व्योम declTyped(bool flag) { m_declTyped = flag; }
    व्योम sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    व्योम attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    व्योम attrScBv(bool flag) { m_attrScBv = flag; }
    व्योम attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    व्योम attrSFormat(bool flag) { m_attrSFormat = flag; }
    व्योम attrSplitVar(bool flag) { m_attrSplitVar = flag; }
    व्योम attrFsmState(bool flag) { m_attrFsmState = flag; }
    व्योम attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    व्योम attrFsmStateExpand(स्थिर VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() स्थिर { वापस m_constPoolEntry; }
    व्योम setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() स्थिर { वापस m_covergroupRefMember; }
    व्योम covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() स्थिर { वापस m_embeddedCovergroup; }
    व्योम embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    व्योम अक्रम(स्थिर VRandAttr flag) { m_rand = flag; }
    व्योम usedParam(bool flag) { m_usedParam = flag; }
    व्योम usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    व्योम sigPublic(bool flag) { m_sigPublic = flag; }
    व्योम sigModPublic(bool flag) { m_sigModPublic = flag; }
    व्योम sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        अगर (flag) sigPublic(true);
    }
    व्योम sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        अगर (flag) sigUserRdPublic(true);
    }
    व्योम sc(bool flag) { m_sc = flag; }
    व्योम scSensitive(bool flag) { m_scSensitive = flag; }
    व्योम primaryIO(bool flag) { m_primaryIO = flag; }
    व्योम isConst(bool flag) { m_isConst = flag; }
    व्योम isContinuously(bool flag) { m_isContinuously = flag; }
    व्योम isIfaceParent(bool flag) { m_isIfaceParent = flag; }
    व्योम isIfaceArraySplit(bool flag) { m_isIfaceArraySplit = flag; }
    व्योम isInternal(bool flag) { m_isInternal = flag; }
    व्योम isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    व्योम funcLocal(bool flag) {
        m_funcLocal = flag;
        अगर (flag) m_funcLocalSticky = true;
    }
    व्योम funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() स्थिर { वापस m_gotNansiType; }
    व्योम gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() स्थिर { वापस m_hasStrengthAssignment; }
    व्योम hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() स्थिर { वापस m_hasUserInit; }
    व्योम hasUserInit(bool flag) { m_hasUserInit = flag; }
    व्योम icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() स्थिर { वापस m_icoMaybeWritten; }
    व्योम isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() स्थिर { वापस m_isClockingDriven; }
    bool isDpiOpenArray() स्थिर VL_MT_SAFE { वापस m_isDpiOpenArray; }
    व्योम isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() स्थिर { वापस m_isHideLocal; }
    व्योम isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() स्थिर { वापस m_isHideProtected; }
    व्योम isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() स्थिर { वापस m_noCReset; }
    व्योम noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() स्थिर { वापस m_noReset; }
    व्योम noReset(bool flag) { m_noReset = flag; }
    bool noSubst() स्थिर { वापस m_noSubst; }
    व्योम noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() स्थिर { वापस m_processQueue; }
    व्योम processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() स्थिर { वापस m_sampled; }
    व्योम sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() स्थिर { वापस m_substConstOnly; }
    व्योम substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() स्थिर { वापस m_overriddenParam; }
    व्योम overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() स्थिर { वापस m_untypedParam; }
    व्योम untypedParam(bool flag) { m_untypedParam = flag; }
    व्योम trace(bool flag) { m_trace = flag; }
    व्योम isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() स्थिर { वापस m_isForceable; }
    व्योम setForceable() { m_isForceable = true; }
    व्योम setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() स्थिर { वापस m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() स्थिर { वापस m_isReadByDpi; }
    व्योम setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() स्थिर { वापस m_isWrittenByDpi; }
    व्योम setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() स्थिर { वापस m_isWrittenBySuspendable; }
    व्योम setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() स्थिर { वापस m_ignorePostRead; }
    व्योम setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() स्थिर { वापस m_ignorePostWrite; }
    व्योम setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() स्थिर { वापस m_ignoreSchedWrite; }
    व्योम setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() स्थिर { वापस m_dfgTriLowered; }
    व्योम setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() स्थिर { वापस m_dfgAllowMultidriveTri; }
    व्योम setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    व्योम globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() स्थिर { वापस m_globalConstrained; }
    bool isStdRandomizeArg() स्थिर { वापस m_isStdRandomizeArg; }
    व्योम setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    व्योम name(स्थिर string& name) override { m_name = name; }
    व्योम tag(स्थिर string& text) override { m_tag = text; }
    string tag() स्थिर override { वापस m_tag; }
    bool isAnsi() स्थिर { वापस m_ansi; }
    bool isContinuously() स्थिर { वापस m_isContinuously; }
    bool isDeclTyped() स्थिर { वापस m_declTyped; }
    bool isInout() स्थिर { वापस m_direction.isInout(); }
    bool isInoutOrRef() स्थिर { वापस m_direction.isInoutOrRef(); }
    bool is_निवेश() स्थिर { वापस m_direction.is_निवेश(); }
    bool isNonOutput() स्थिर { वापस m_direction.isNonOutput(); }
    bool isReadOnly() स्थिर VL_MT_SAFE { वापस m_direction.isReadOnly(); }
    bool isConstRef() स्थिर VL_MT_SAFE { वापस m_direction.isConstRef(); }
    bool isRef() स्थिर VL_MT_SAFE { वापस m_direction.isRef(); }
    bool isWritable() स्थिर VL_MT_SAFE { वापस m_direction.isWritable(); }
    bool isTristate() स्थिर { वापस m_tristate; }
    bool isPrimaryIO() स्थिर VL_MT_SAFE { वापस m_primaryIO; }
    bool isPrimaryInish() स्थिर { वापस isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() स्थिर VL_MT_SAFE { वापस m_primaryClock; }
    व्योम setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() स्थिर { वापस varType() == VVarType::IFACEREF; }
    व्योम setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() स्थिर { वापस m_isIfaceParent; }
    bool isIfaceArraySplit() स्थिर { वापस m_isIfaceArraySplit; }
    bool isInternal() स्थिर { वापस m_isInternal; }
    bool isIfaceParam() स्थिर { वापस m_isIfaceParam; }
    bool isSignal() स्थिर { वापस varType().isSignal(); }
    bool isNet() स्थिर { वापस varType().isNet(); }
    bool isWor() स्थिर { वापस varType().isWor(); }
    bool isWiredNet() स्थिर { वापस varType().isWiredNet(); }
    bool is_अस्थायी() स्थिर { वापस varType().is_अस्थायी(); }
    bool isClassMember() स्थिर { वापस varType() == VVarType::MEMBER; }
    bool isVirtIface() स्थिर {
        अगर (AstIfaceRefDType* स्थिर dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            वापस dtp->isVirtual();
        }
        वापस false;
    }
    bool is_Statement_अस्थायी() स्थिर { वापस varType() == VVarType::STMTTEMP; }
    bool isXTemp() स्थिर { वापस varType() == VVarType::XTEMP; }
    bool isParam() स्थिर { वापस varType().isParam(); }
    bool isGParam() स्थिर { वापस varType() == VVarType::GPARAM; }
    bool isGenVar() स्थिर { वापस varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() स्थिर {
        वापस isIO() || isSignal() || isClassMember() || is_अस्थायी() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() स्थिर {
        वापस !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() स्थिर {
        स्थिर AstBasicDType* स्थिर bdtypep = basicp();
        वापस bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() स्थिर { वापस m_usedParam; }
    bool isUsedLoopIdx() स्थिर { वापस m_usedLoopIdx; }
    bool mtask_Cache_पंक्ति_Align() स्थिर { वापस m_mtask_Cache_पंक्ति_Align; }
    व्योम mtask_Cache_पंक्ति_Align(bool flag) { m_mtask_Cache_पंक्ति_Align = flag; }
    bool isSc() स्थिर VL_MT_SAFE { वापस m_sc; }
    bool isScQuad() स्थिर;
    bool isScBv() स्थिर VL_MT_STABLE;
    bool isScUint() स्थिर;
    bool isScUintBool() स्थिर;
    bool isScBigUint() स्थिर VL_MT_STABLE;
    bool isScSensitive() स्थिर { वापस m_scSensitive; }
    bool isSigPublic() स्थिर;
    bool isSigModPublic() स्थिर { वापस m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() स्थिर { वापस m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() स्थिर { वापस m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() स्थिर { वापस m_trace; }
    bool isRand() स्थिर { वापस m_rand.isRand(); }
    bool isRandC() स्थिर { वापस m_rand.isRandC(); }
    bool isConst() स्थिर VL_MT_SAFE { वापस m_isConst; }
    bool isLatched() स्थिर { वापस m_isLatched; }
    bool isFuncLocal() स्थिर { वापस m_funcLocal; }
    bool isFuncLocalSticky() स्थिर { वापस m_funcLocalSticky; }
    bool isFuncReturn() स्थिर { वापस m_funcReturn; }
    bool isPullup() स्थिर { वापस m_isPullup; }
    bool isPulldown() स्थिर { वापस m_isPulldown; }
    bool attrScBv() स्थिर { वापस m_attrScBv; }
    bool attrScBigUint() स्थिर { वापस m_attrScBigUint; }
    bool attrFileDescr() स्थिर { वापस m_attrFileDescr; }
    bool attrSFormat() स्थिर { वापस m_attrSFormat; }
    bool attrSplitVar() स्थिर { वापस m_attrSplitVar; }
    bool attrFsmState() स्थिर { वापस m_attrFsmState; }
    bool attrFsmArcInclCond() स्थिर { वापस m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() स्थिर { वापस m_attrFsmStateExpand; }
    AstIface* sensIfacep() स्थिर { वापस m_sensIfacep; }
    VRandAttr अक्रम() स्थिर { वापस m_rand; }
    string verilogKwd() स्थिर override;
    व्योम lifetime(स्थिर VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() स्थिर { वापस m_lifetime; }
    व्योम pin_अंक(पूर्णांक id) { m_pin_अंक = id; }
    पूर्णांक pin_अंक() स्थिर { वापस m_pin_अंक; }
    व्योम propagateAttrFrom(स्थिर AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        अगर (fromp->attrFileDescr()) attrFileDescr(true);
        अगर (fromp->isContinuously()) isContinuously(true);
    }
    व्योम propagateSplitAttrFrom(स्थिर AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    स्थिर अक्षर* cannotSplitKindReason() स्थिर;
    व्योम propagateWrapAttrFrom(स्थिर AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    व्योम combineType(स्थिर AstVar* otherp);
    bool needsCReset() स्थिर {
        वापस !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !is_Statement_अस्थायी()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    अटल AstVar* scVarRecurse(Ast_गाँठ* nodep);
};
class AstVarScope final : खुला Ast_गाँठ {
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
    bool m_fork_By_मूल्य : 1;  // Set before a fork, whose processes get it by value
खुला:
    AstVarScope(File_पंक्ति* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_मूल्य = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    व्योम cloneRelink() override {
        अगर (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    string name() स्थिर override VL_MT_STABLE { वापस scopep()->name() + "->" + varp()->name(); }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    AstVar* varp() स्थिर VL_MT_STABLE { वापस m_varp; }  // [After Link] Pointer to variable
    व्योम varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() स्थिर VL_MT_STABLE { वापस m_scopep; }  // Pointer to scope it's under
    व्योम scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() स्थिर { वापस m_trace; }
    व्योम trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() स्थिर { वापस m_optimizeLifePost; }
    व्योम optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_मूल्य() स्थिर { वापस m_fork_By_मूल्य; }
    व्योम fork_By_मूल्य(bool flag) { m_fork_By_मूल्य = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : खुला Ast_गाँठ_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    स्थिर uint64_t m_सूचकांक;  // Index of the element, with the array's lo() at 0
खुला:
    AstConsArrayItem(File_पंक्ति* fl, uint64_t सूचकांक_2, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_सूचकांक{सूचकांक_2} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    स्थिर अक्षर* broken() स्थिर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        वापस nullptr;
    }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        वापस m_सूचकांक == VN_DBG_AS(samep, ConsArrayItem)->m_सूचकांक;
    }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }  // By AstConsArray's map
    uint64_t सूचकांक_2() स्थिर { वापस m_सूचकांक; }
};
class AstConsAssocItem final : खुला Ast_गाँठ_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
खुला:
    AstConsAssocItem(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* keyp, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    स्थिर अक्षर* broken() स्थिर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        वापस nullptr;
    }
};
class AstConsDynArrayItem final : खुला Ast_गाँठ_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
खुला:
    AstConsDynArrayItem(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_पंक्ति* fl, AstConst* keyp, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    स्थिर अक्षर* broken() स्थिर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        वापस nullptr;
    }
};
class AstConsQueueItem final : खुला Ast_गाँठ_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
खुला:
    AstConsQueueItem(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    स्थिर अक्षर* broken() स्थिर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        वापस nullptr;
    }
};
class AstConsStructItem final : खुला Ast_गाँठ_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
खुला:
    AstConsStructItem(File_पंक्ति* fl, AstMemberDType* memberp, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    स्थिर अक्षर* broken() स्थिर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        वापस nullptr;
    }
    व्योम dump(std::ostream& str) स्थिर override;
    AstMemberDType* memberp() स्थिर { वापस m_memberp; }
};
class AstConsWildcardItem final : खुला Ast_गाँठ_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
खुला:
    AstConsWildcardItem(File_पंक्ति* fl, AstConst* keyp, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    स्थिर अक्षर* broken() स्थिर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        वापस nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : खुला Ast_गाँठ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    पूर्णांक m_offset;  // Offset column numbers to uniq-ify IFs
खुला:
    AstCoverOtherDecl(File_पंक्ति* fl, स्थिर string& page, स्थिर string& comment,
                      स्थिर string& linescov, पूर्णांक offset, स्थिर string& fsmVar = "",
                      स्थिर string& fsmFrom = "", स्थिर string& fsmTo = "",
                      स्थिर string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक offset() स्थिर { वापस m_offset; }
    पूर्णांक आकार() स्थिर override { वापस 1; }
    स्थिर string& linescov() स्थिर { वापस m_linescov; }
    स्थिर string& fsmVar() स्थिर { वापस m_fsmVar; }
    स्थिर string& fsmFrom() स्थिर { वापस m_fsmFrom; }
    स्थिर string& fsmTo() स्थिर { वापस m_fsmTo; }
    स्थिर string& fsmTag() स्थिर { वापस m_fsmTag; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstCoverOtherDecl* स्थिर asamep = VN_DBG_AS(samep, CoverOtherDecl);
        वापस Super::same_गाँठ(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : खुला Ast_गाँठ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    स्थिर VNumRange m_range;  // Packed array range covering each toggle bit
खुला:
    AstCoverToggleDecl(File_पंक्ति* fl, स्थिर string& page, स्थिर string& comment,
                       स्थिर VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    पूर्णांक आकार() स्थिर override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        वापस 2 * m_range.elements();
    }
    स्थिर VNumRange& range() स्थिर { वापस m_range; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstCoverToggleDecl* स्थिर asamep = VN_DBG_AS(samep, CoverToggleDecl);
        वापस Super::same_गाँठ(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : खुला Ast_गाँठ_FTask {
    // A function inside a module
खुला:
    AstFunc(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* stmtp, Ast_गाँठ* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    Ast_गाँठ_FTask* cloneType(स्थिर string& name) override {
        वापस new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() स्थिर override { वापस "function"; }
};
class AstLet final : खुला Ast_गाँठ_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
खुला:
    AstLet(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_व्यंजक));
        वापस nullptr;
    }
    Ast_गाँठ_FTask* cloneType(स्थिर string& name) override { वापस new AstLet{fileline(), name}; }
    string verilogKwd() स्थिर override { वापस "let"; }
};
class AstProperty final : खुला Ast_गाँठ_FTask {
    // A property inside a module
खुला:
    AstProperty(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    Ast_गाँठ_FTask* cloneType(स्थिर string& name) override {
        वापस new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() स्थिर override { वापस "property"; }
};
class AstSequence final : खुला Ast_गाँठ_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
खुला:
    AstSequence(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    Ast_गाँठ_FTask* cloneType(स्थिर string& name) override {
        वापस new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() स्थिर override { वापस "sequence"; }
    bool isReferenced() स्थिर { वापस m_referenced; }
    व्योम isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : खुला Ast_गाँठ_FTask {
    // A task inside a module
खुला:
    AstTask(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_गाँठ_FTask* cloneType(स्थिर string& name) override {
        वापस new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() स्थिर override { वापस "task"; }
};

// === AstNodeFile ===
class AstCFile final : खुला Ast_गाँठ_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
खुला:
    AstCFile(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    uint64_t complexityScore() स्थिर { वापस m_complexityScore; }
    व्योम complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() स्थिर { वापस m_slow; }
    व्योम slow(bool flag) { m_slow = flag; }
    bool source() स्थिर { वापस m_source; }
    व्योम source(bool flag) { m_source = flag; }
    bool support() स्थिर { वापस m_support; }
    व्योम support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : खुला Ast_गाँठ_File {
    // Verilog output file
    // Parents:  NETLIST
खुला:
    AstVFile(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
};
class AstCoverCross final : खुला Ast_गाँठ_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
खुला:
    AstCoverCross(File_पंक्ति* fl, स्थिर string& name, AstCoverpointRef* itemsp,
                  Ast_गाँठ_व्यंजक* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstCoverpoint final : खुला Ast_गाँठ_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
खुला:
    AstCoverpoint(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_व्यंजक* exprp,
                  Ast_गाँठ_व्यंजक* iffp = nullptr, Ast_गाँठ* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};

// === AstNodeGen ===
class AstGenBlock final : खुला Ast_गाँठ_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    स्थिर bool m_unnamed;  // Originally unnamed (name change does not affect this)
    स्थिर bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

खुला:
    AstGenBlock(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    std::string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर std::string& name) override { m_name = name; }
    bool unnamed() स्थिर { वापस m_unnamed; }
    bool implied() स्थिर { वापस m_implied; }
    AstDefaultDisable* defaultDisablep() स्थिर { वापस m_defaultDisablep; }
    व्योम defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(स्थिर string& outer) स्थिर;
};
class AstGenCase final : खुला Ast_गाँठ_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
खुला:
    AstGenCase(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : खुला Ast_गाँठ_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
खुला:
    AstGenFor(File_पंक्ति* fl, Ast_गाँठ* initsp, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ* incsp, Ast_गाँठ* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : खुला Ast_गाँठ_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
खुला:
    AstGenIf(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* condp, Ast_गाँठ* thensp, Ast_गाँठ* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : खुला Ast_गाँठ_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_अंक;  // Declaration token number
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
    पूर्णांक m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool full, स्थिर string& scope) स्थिर;
    string dtypeNameScope() स्थिर;  // Prefix of dtypeName(true) for the scope declaring the class

खुला:
    AstClass(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_अंक{fl->token_अंक()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() स्थिर override { वापस isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    bool timescaleMatters() स्थिर override { वापस false; }
    AstClassPackage* classOrPackagep() स्थिर VL_MT_STABLE { वापस m_classOrPackagep; }
    व्योम classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() स्थिर VL_MT_STABLE {
        वापस m_covergroupEnclosingClassp;
    }
    व्योम covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_गाँठ* membersp() स्थिर VL_MT_STABLE { वापस stmtsp(); }
    व्योम addMembersp(Ast_गाँठ* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() स्थिर { वापस m_hasRandVarsUpdate; }
    व्योम hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() स्थिर { वापस m_covergroup; }
    व्योम isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() स्थिर { वापस m_extended; }
    व्योम isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() स्थिर { वापस m_interfaceClass; }
    व्योम isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() स्थिर { वापस m_virtual; }
    व्योम isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() स्थिर { वापस m_needRNG; }
    व्योम needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() स्थिर { वापस m_useVirtualPublic; }
    व्योम useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    व्योम markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() स्थिर { वापस m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) स्थिर;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(स्थिर string& scope) स्थिर;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    व्योम dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() स्थिर { वापस !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_संदेश() स्थिर override { वापस hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    अटल string embeddedCovergroupTypeName(स्थिर string& name) { वापस "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() स्थिर;
    // Covergroup options accessors
    पूर्णांक cgAutoBinMax() स्थिर { वापस m_cgAutoBinMax; }
    व्योम cgAutoBinMax(पूर्णांक मूल्य) { m_cgAutoBinMax = मूल्य; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    अटल bool isClassExtendedFrom(स्थिर AstClass* refClassp, स्थिर AstClass* baseClassp);
    uint32_t decl_Token_अंक() स्थिर override { वापस m_decl_Token_अंक; }
    व्योम decl_Token_अंक_Set_Min(uint32_t token_अंक) override {
        m_decl_Token_अंक = std::min(m_decl_Token_अंक, token_अंक);
    }
    व्योम baseOverride(स्थिर VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() स्थिर { वापस m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    अटल bool isCacheableChild(स्थिर Ast_गाँठ* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    ढाँचा <typename T_Callable>
    व्योम foreachMember(स्थिर T_Callable& f) {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_गाँठ*>::मूल्य
                && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        अगर (स्थिर AstClassExtends* स्थिर cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        क्रम (Ast_गाँठ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (Ast_गाँठ::privateTypeTest<T_गाँठ>(stmtp)) f(this, static_cast<T_गाँठ*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    ढाँचा <typename T_Callable>
    bool existsMember(स्थिर T_Callable& p) स्थिर {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, स्थिर AstClass*, स्थिर T_गाँठ*>::मूल्य
                && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        अगर (स्थिर AstClassExtends* स्थिर cextendsp = this->extendsp()) {
            अगर (cextendsp->classp()->existsMember(p)) वापस true;
        }
        क्रम (Ast_गाँठ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (Ast_गाँठ::privateTypeTest<T_गाँठ>(stmtp)) {
                अगर (p(this, static_cast<T_गाँठ*>(stmtp))) वापस true;
            }
        }
        वापस false;
    }
};
class AstClassPackage final : खुला Ast_गाँठ_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
खुला:
    AstClassPackage(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() स्थिर override { वापस "classpackage"; }
    bool timescaleMatters() स्थिर override { वापस false; }
    AstClass* classp() स्थिर VL_MT_SAFE { वापस m_classp; }
    व्योम classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : खुला Ast_गाँठ_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
खुला:
    AstIface(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string verilogKwd() स्थिर override { वापस "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() स्थिर override { वापस false; }
    bool hasVirtualRef() स्थिर { वापस m_hasVirtualRef; }
    व्योम setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) स्थिर;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    व्योम dtypeNameFreeze();
};
class AstModule final : खुला Ast_गाँठ_Module {
    // A module declaration
    स्थिर bool m_isChecker;  // Module represents a checker
    स्थिर bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

खुला:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() स्थिर override {
        वापस m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() स्थिर override { वापस true; }
    bool isChecker() स्थिर { वापस m_isChecker; }
    bool isProgram() स्थिर { वापस m_isProgram; }
    bool hasGenericIface() स्थिर { वापस m_hasGenericIface; }
    व्योम hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};
class AstNotFoundModule final : खुला Ast_गाँठ_Module {
    // A missing module declaration
खुला:
    AstNotFoundModule(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() स्थिर override { वापस "/*not-found-*/ module"; }
    bool timescaleMatters() स्थिर override { वापस false; }
};
class AstPackage final : खुला Ast_गाँठ_Module {
    // A package declaration
खुला:
    AstPackage(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() स्थिर override { वापस "package"; }
    bool timescaleMatters() स्थिर override { वापस !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : खुला Ast_गाँठ_Module {
    // A primitive declaration
खुला:
    AstPrimitive(File_पंक्ति* fl, स्थिर string& name, स्थिर string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() स्थिर override { वापस "primitive"; }
    bool timescaleMatters() स्थिर override { वापस false; }
};

// === AstNodeProcedure ===
class AstAlways final : खुला Ast_गाँठ_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    स्थिर VAlwaysKwd m_keyword;

खुला:
    AstAlways(File_पंक्ति* fl, VAlwaysKwd keyword, Ast_Sen_वृक्ष* sentreep, Ast_गाँठ* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit अंतरभूत AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    VAlwaysKwd keyword() स्थिर { वापस m_keyword; }
};
class AstAlwaysObserved final : खुला Ast_गाँठ_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

खुला:
    AstAlwaysObserved(File_पंक्ति* fl, Ast_Sen_वृक्ष* sentreep, Ast_गाँठ* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : खुला Ast_गाँठ_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

खुला:
    explicit AstAlwaysPost(File_पंक्ति* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : खुला Ast_गाँठ_Procedure {
    // Like always but Postponed scheduling region

खुला:
    AstAlwaysPostponed(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : खुला Ast_गाँठ_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

खुला:
    explicit AstAlwaysPre(File_पंक्ति* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : खुला Ast_गाँठ_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

खुला:
    AstAlwaysReactive(File_पंक्ति* fl, Ast_Sen_वृक्ष* sentreep, Ast_गाँठ* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : खुला Ast_गाँठ_Procedure {
खुला:
    AstFinal(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : खुला Ast_गाँठ_Procedure {
खुला:
    AstInitial(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : खुला Ast_गाँठ_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
खुला:
    AstInitialAutomatic(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : खुला Ast_गाँठ_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
खुला:
    AstInitialStatic(File_पंक्ति* fl, Ast_गाँठ* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : खुला Ast_गाँठ_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
खुला:
    AstBracketRange(File_पंक्ति* fl, Ast_गाँठ* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    भव string emitC() { V3ERROR_NA_RETURN(""); }
    भव string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस false; }
};
class AstRange final : खुला Ast_गाँठ_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    स्थिर bool m_fromBracket = false;  // From C-style '[x]' declaration
खुला:
    AstRange(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* leftp, Ast_गाँठ_व्यंजक* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    अंतरभूत AstRange(File_पंक्ति* fl, पूर्णांक बायाँ, पूर्णांक दायाँ);
    अंतरभूत AstRange(File_पंक्ति* fl, स्थिर VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    अंतरभूत पूर्णांक बायाँ_Const() स्थिर VL_MT_STABLE;
    अंतरभूत पूर्णांक दायाँ_Const() स्थिर VL_MT_STABLE;
    पूर्णांक hiConst() स्थिर VL_MT_STABLE {
        स्थिर पूर्णांक l = बायाँ_Const();
        स्थिर पूर्णांक r = दायाँ_Const();
        वापस l > r ? l : r;
    }
    पूर्णांक loConst() स्थिर VL_MT_STABLE {
        स्थिर पूर्णांक l = बायाँ_Const();
        स्थिर पूर्णांक r = दायाँ_Const();
        वापस l > r ? r : l;
    }
    पूर्णांक elementsConst() स्थिर VL_MT_STABLE { वापस hiConst() - loConst() + 1; }
    bool ascending() स्थिर { वापस बायाँ_Const() < दायाँ_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_बायाँ_To_दायाँ() स्थिर {
        वापस VNumRange{बायाँ_Const(), दायाँ_Const()}.seq_बायाँ_To_दायाँ();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_दायाँ_To_बायाँ() स्थिर {
        वापस VNumRange{बायाँ_Const(), दायाँ_Const()}.seq_दायाँ_To_बायाँ();
    }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    भव string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstRange* स्थिर asamep = VN_DBG_AS(samep, Range);
        वापस fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() स्थिर { वापस m_fromBracket; }
};
class AstUnsizedRange final : खुला Ast_गाँठ_Range {
    // Unsized range specification, for open arrays
खुला:
    explicit AstUnsizedRange(File_पंक्ति* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    भव string emitC() { V3ERROR_NA_RETURN(""); }
    भव string emitVerilog() { वापस "[]"; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};
class AstWildcardRange final : खुला Ast_गाँठ_Range {
    // Wildcard range specification, for wildcard index type associative arrays
खुला:
    explicit AstWildcardRange(File_पंक्ति* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    भव string emitC() { V3ERROR_NA_RETURN(""); }
    भव string emitVerilog() { वापस "[*]"; }
    bool same_गाँठ(स्थिर Ast_गाँठ* /*samep*/) स्थिर override { वापस true; }
};

#पूर्ण_अगर  // Guard
