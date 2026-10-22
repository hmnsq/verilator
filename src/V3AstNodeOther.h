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
#अन्तर्भाव "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

#अन्तर्भाव <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_गाँठो_Cons_Item VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    मित्र class Ast_गाँठो_अभिव्यक्ति;  // For m_knownConstantTree
    mutable bool m_known_Constant_रूख = false;  // Known to be a tree of constants

रक्षित:
    Ast_गाँठो_Cons_Item(VNType t, File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* valuep)
        : Ast_गाँठो{t, fl} {
        this->valuep(valuep);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Cons_Item;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class Ast_गाँठो_Cover_Decl VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    पूर्णाङ्क m_bin_अङ्क = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    पूर्णाङ्क m_local_Bin_अङ्क = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
सार्वजनिक:
    Ast_गाँठो_Cover_Decl(VNType t, File_पङ्क्ति* fl, अचर string& page, अचर string& comment)
        : Ast_गाँठो(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_गाँठो_Cover_Decl;
    अचर वर्ण* broken() अचर override {
        यदि (m_dataDeclp
            && (m_dataDeclp == this || m_dataDeclp->m_dataDeclp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        फर्काऊ nullptr;
    }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 1 + 2 * INSTR_गन्ती_LD; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    पूर्णाङ्क bin_अङ्क() अचर { फर्काऊ m_bin_अङ्क; }
    शून्य bin_अङ्क(पूर्णाङ्क flag) { m_bin_अङ्क = flag; }
    पूर्णाङ्क local_Bin_अङ्क() अचर { फर्काऊ m_local_Bin_अङ्क; }
    शून्य local_Bin_अङ्क(पूर्णाङ्क flag) { m_local_Bin_अङ्क = flag; }
    bool perInstance() अचर { फर्काऊ m_perInstance; }
    शून्य perInstance(bool flag) { m_perInstance = flag; }
    आभास पूर्णाङ्क size() अचर = 0;
    अचर string& comment() अचर { फर्काऊ m_text; }  // text to insert in code
    अचर string& page() अचर { फर्काऊ m_page; }
    अचर string& hier() अचर { फर्काऊ m_hier; }
    शून्य hier(अचर string& flag) { m_hier = flag; }
    शून्य comment(अचर string& flag) { m_text = flag; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर Ast_गाँठो_Cover_Decl* अचर asamep = VN_DBG_AS(samep, गाँठो_Cover_Decl);
        फर्काऊ (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    शून्य dataDeclp(Ast_गाँठो_Cover_Decl* nodep) { m_dataDeclp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_गाँठो_Cover_Decl* dataDeclNullp() अचर { फर्काऊ m_dataDeclp; }
    Ast_गाँठो_Cover_Decl* dataDeclThisp() { फर्काऊ dataDeclNullp() ? dataDeclNullp() : this; }
};
class Ast_गाँठो_FTask VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
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
    Ast_गाँठो_FTask(VNType t, File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* stmtsp)
        : Ast_गाँठो{t, fl}
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

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_FTask;
    आभास Ast_गाँठो_FTask* cloneType(अचर string& name) = 0;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool isGateOptimizable() अचर override {
        फर्काऊ !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    शून्य name(अचर string& name) override { m_name = name; }
    string cname() अचर { फर्काऊ m_cname; }
    शून्य cname(अचर string& cname) { m_cname = cname; }
    bool isFunction() अचर { फर्काऊ fvarp() != nullptr; }
    // MORE ACCESSORS
    शून्य dpiOpenParentInc() { ++m_dpiOpenParent; }
    शून्य dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() अचर { फर्काऊ m_dpiOpenParent; }
    bool taskPublic() अचर { फर्काऊ m_taskPublic; }
    शून्य taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() अचर { फर्काऊ m_classMethod; }
    शून्य classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() अचर { फर्काऊ m_didProto; }
    शून्य didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() अचर { फर्काऊ m_isExternProto; }
    शून्य isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() अचर { फर्काऊ m_isExternDef; }
    शून्य isExternDef(bool flag) { m_isExternDef = flag; }
    अचर string& ifacePortName() अचर { फर्काऊ m_ifacePortName; }
    शून्य ifacePortName(अचर string& name) { m_ifacePortName = name; }
    bool prototype() अचर { फर्काऊ m_prototype; }
    शून्य prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() अचर { फर्काऊ m_dpiExport; }
    शून्य dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() अचर { फर्काऊ m_dpiImport; }
    शून्य dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() अचर { फर्काऊ m_dpiContext; }
    शून्य dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() अचर { फर्काऊ m_dpiOpenChild; }
    शून्य dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() अचर { फर्काऊ m_dpiTask; }
    शून्य dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() अचर { फर्काऊ !m_dpiCDecl.empty(); }
    अचर string& dpiCDecl() अचर { फर्काऊ m_dpiCDecl; }
    शून्य dpiCDecl(अचर string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() अचर { फर्काऊ m_isConstructor; }
    शून्य isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() अचर { फर्काऊ m_isHideLocal; }
    शून्य isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() अचर { फर्काऊ m_isHideProtected; }
    शून्य isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() अचर { फर्काऊ m_dpiPure; }
    शून्य dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() अचर { फर्काऊ m_keepAlive; }
    शून्य keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() अचर { फर्काऊ m_needsSyms; }
    शून्य needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() अचर { फर्काऊ m_pureVirtual; }
    शून्य pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() अचर { फर्काऊ m_recursive; }
    शून्य recursive(bool flag) { m_recursive = flag; }
    bool isStatic() अचर { फर्काऊ m_static; }
    शून्य isStatic(bool flag) { m_static = flag; }
    bool underGenerate() अचर { फर्काऊ m_underGenerate; }
    शून्य underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() अचर { फर्काऊ m_verilogFunction; }
    शून्य verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() अचर { फर्काऊ m_verilogTask; }
    शून्य verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() अचर { फर्काऊ m_virtual; }
    शून्य isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() अचर { फर्काऊ m_needProcess; }
    शून्य setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() अचर { फर्काऊ m_isCovergroupSample; }
    शून्य isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    शून्य baseOverride(अचर VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() अचर { फर्काऊ m_baseOverride; }
    शून्य lifetime(अचर VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() अचर { फर्काऊ m_lifetime; }
    bool isPure() override;
    अचर वर्ण* broken() अचर override;
    शून्य propagateAttrFrom(अचर Ast_गाँठो_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        classMethod(fromp->classMethod());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

गुप्त:
    bool getPurityRecurse() अचर;
};
class Ast_गाँठो_File VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
सार्वजनिक:
    Ast_गाँठो_File(VNType t, File_पङ्क्ति* fl, अचर string& name)
        : Ast_गाँठो{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_गाँठो_File;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class Ast_गाँठो_Func_Cov_Item VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // Base class for functional coverage items (coverpoints, crosses)
रक्षित:
    string m_name;  // Item name

सार्वजनिक:
    Ast_गाँठो_Func_Cov_Item(VNType t, File_पङ्क्ति* fl, अचर string& name)
        : Ast_गाँठो{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_गाँठो_Func_Cov_Item;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& name) override { m_name = name; }
    bool maybePointedTo() अचर override { फर्काऊ true; }
};
class Ast_गाँठो_Gen VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // Generate construct
सार्वजनिक:
    Ast_गाँठो_Gen(VNType t, File_पङ्क्ति* fl)
        : Ast_गाँठो{t, fl} {}
    ASTGEN_MEMBERS_Ast_गाँठो_Gen;
};
class Ast_गाँठो_Module VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    अचर string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    पूर्णाङ्क m_depth = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    पूर्णाङ्क m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
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
    Ast_गाँठो_Module(VNType t, File_पङ्क्ति* fl, अचर string& name, अचर string& libname)
        : Ast_गाँठो{t, fl}
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

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Module;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    आभास bool timescaleMatters() अचर = 0;
    अन्तःस्थ bool isDollarUnit() अचर;  // Is the $unit package
    अन्तःस्थ bool isConstPool() अचर;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(अचर string& outer) अचर;
    // ACCESSORS
    शून्य name(अचर string& name) override { m_name = name; }
    string origName() अचर override { फर्काऊ m_origName; }
    string someInstanceName() अचर VL_MT_SAFE { फर्काऊ m_someInstanceName; }
    शून्य someInstanceName(अचर string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() अचर { फर्काऊ m_defaultDisablep; }
    शून्य defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() अचर { फर्काऊ m_inLibrary; }
    शून्य inLibrary(bool flag) { m_inLibrary = flag; }
    शून्य depth(पूर्णाङ्क मान) { m_depth = मान; }
    पूर्णाङ्क depth() अचर VL_MT_SAFE { फर्काऊ m_depth; }
    शून्य level(पूर्णाङ्क मान) { m_level = मान; }
    पूर्णाङ्क level() अचर VL_MT_SAFE { फर्काऊ m_level; }
    string libname() अचर { फर्काऊ m_libname; }
    bool is_शीर्ष() अचर VL_MT_SAFE { फर्काऊ level() == 1; }
    bool modPublic() अचर { फर्काऊ m_modPublic; }
    शून्य modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() अचर { फर्काऊ m_modTrace; }
    शून्य modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() अचर { फर्काऊ m_ctorVarReset; }
    शून्य ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() अचर { फर्काऊ m_dead; }
    शून्य dead(bool flag) { m_dead = flag; }
    bool hasGParam() अचर { फर्काऊ m_hasGParam; }
    शून्य hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() अचर { फर्काऊ m_hasParameterList; }
    शून्य hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() अचर { फर्काऊ m_hierBlock; }
    शून्य hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() अचर { फर्काऊ m_hierParams; }
    शून्य hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() अचर { फर्काऊ m_recursive; }
    शून्य recursive(bool flag) { m_recursive = flag; }
    शून्य recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() अचर { फर्काऊ m_recursiveClone; }
    bool parameterizedTemplate() अचर { फर्काऊ m_parameterizedTemplate; }
    शून्य parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    शून्य verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() अचर { फर्काऊ m_verilatorLib; }
    VLifetime lifetime() अचर { फर्काऊ m_lifetime; }
    शून्य lifetime(अचर VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() अचर { फर्काऊ m_timeunit; }
    शून्य timeunit(अचर VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() अचर { फर्काऊ m_unconnectedDrive; }
    शून्य unconnectedDrive(अचर VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_गाँठो_Procedure VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
रक्षित:
    Ast_गाँठो_Procedure(VNType t, File_पङ्क्ति* fl, Ast_गाँठो* stmtsp)
        : Ast_गाँठो{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Procedure;
    // METHODS
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool isJustOneBodyStmt() अचर { फर्काऊ stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() अचर { फर्काऊ m_suspendable; }
    शून्य setSuspendable() { m_suspendable = true; }
    bool needProcess() अचर { फर्काऊ m_needProcess; }
    शून्य setNeedProcess() { m_needProcess = true; }
};
class Ast_गाँठो_Range VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // A range, sized or unsized
रक्षित:
    Ast_गाँठो_Range(VNType t, File_पङ्क्ति* fl)
        : Ast_गाँठो{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Range;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : सार्वजनिक Ast_गाँठो {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

सार्वजनिक:
    AstActive(File_पङ्क्ति* fl, अचर string& name, Ast_Sen_रूख* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    // Statements are broken into pieces, as some must come before others.
    शून्य sentreep(Ast_Sen_रूख* nodep) { m_sentreep = nodep; }
    Ast_Sen_रूख* sentreep() अचर { फर्काऊ m_sentreep; }
    // METHODS
    अन्तःस्थ bool hasClocked() अचर;
    अन्तःस्थ bool hasCombo() अचर;
    अन्तःस्थ bool hasInitial() अचर;
    अन्तःस्थ bool hasStatic() अचर;
};
class AstAlias final : सार्वजनिक Ast_गाँठो {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
सार्वजनिक:
    AstAlias(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : सार्वजनिक Ast_गाँठो {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
सार्वजनिक:
    AstAliasScope(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : सार्वजनिक Ast_गाँठो {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
सार्वजनिक:
    AstBind(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Bind Target name
    शून्य name(अचर string& name) override { m_name = name; }
};
class AstCFunc final : सार्वजनिक Ast_गाँठो {
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
    पूर्णाङ्क m_cost;  // Function call cost
सार्वजनिक:
    AstCFunc(File_पङ्क्ति* fl, अचर string& name, AstScope* scopep, अचर string& rtnType = "")
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
        m_cost = v3Global.opt.instr_गन्ती_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstCFunc* अचर asamep = VN_DBG_AS(samep, CFunc);
        फर्काऊ ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    शून्य name(अचर string& name) override { m_name = name; }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ m_cost; }
    VBoolOrUnknown isConst() अचर { फर्काऊ m_isConst; }
    शून्य isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    शून्य isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() अचर { फर्काऊ m_isStatic; }
    शून्य isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() अचर VL_MT_SAFE { फर्काऊ m_isTrace; }
    शून्य isTrace(bool flag) { m_isTrace = flag; }
    शून्य cname(अचर string& name) { m_cname = name; }
    string cname() अचर { फर्काऊ m_cname; }
    AstScope* scopep() अचर { फर्काऊ m_scopep; }
    शून्य scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() अचर { फर्काऊ ((m_rtnType == "") ? "void" : m_rtnType); }
    शून्य rtnType(अचर string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() अचर { फर्काऊ m_dontCombine || isTrace() || entryPoint(); }
    शून्य dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() अचर { फर्काऊ m_declPrivate; }
    शून्य declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() अचर VL_MT_SAFE { फर्काऊ m_keepIfEmpty; }
    शून्य keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() अचर VL_MT_SAFE { फर्काऊ m_slow; }
    शून्य slow(bool flag) { m_slow = flag; }
    bool funcPublic() अचर { फर्काऊ m_funcPublic; }
    शून्य funcPublic(bool flag) { m_funcPublic = flag; }
    शून्य argTypes(अचर string& str) { m_argTypes = str; }
    string argTypes() अचर { फर्काऊ m_argTypes; }
    शून्य ifdef(अचर string& str) { m_ifdef = str; }
    string ifdef() अचर { फर्काऊ m_ifdef; }
    bool isConstructor() अचर { फर्काऊ m_isConstructor; }
    शून्य isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() अचर { फर्काऊ m_isDestructor; }
    शून्य isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() अचर { फर्काऊ m_isMethod; }
    शून्य isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() अचर { फर्काऊ m_isLoose; }
    शून्य isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() अचर { फर्काऊ isMethod() && !isLoose(); }
    bool isVirtual() अचर { फर्काऊ m_isVirtual; }
    शून्य isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() अचर { फर्काऊ m_needProcess; }
    शून्य setNeedProcess() { m_needProcess = true; }
    bool entryPoint() अचर { फर्काऊ m_entryPoint; }
    शून्य entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() अचर { फर्काऊ m_dpiPure; }
    शून्य dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() अचर { फर्काऊ m_dpiContext; }
    शून्य dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() अचर VL_MT_SAFE { फर्काऊ m_dpiExportDispatcher; }
    शून्य dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() अचर { फर्काऊ m_dpiExportImpl; }
    शून्य dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() अचर VL_MT_SAFE { फर्काऊ m_dpiImportPrototype; }
    शून्य dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() अचर { फर्काऊ m_dpiImportWrapper; }
    शून्य dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() अचर { फर्काऊ !m_dpiCDecl.empty(); }
    अचर string& dpiCDecl() अचर { फर्काऊ m_dpiCDecl; }
    शून्य dpiCDecl(अचर string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() अचर { फर्काऊ m_rtnType == "VlCoroutine"; }
    शून्य recursive(bool flag) { m_recursive = flag; }
    bool recursive() अचर { फर्काऊ m_recursive; }
    शून्य unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() अचर override { फर्काऊ m_unlikely; }  // Note virtual override
    शून्य noLife(bool flag) { m_noLife = flag; }
    bool noLife() अचर { फर्काऊ m_noLife; }
    bool isCovergroupSample() अचर { फर्काऊ m_isCovergroupSample; }
    शून्य isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() अचर { फर्काऊ m_voidSelf; }
    शून्य voidSelf(bool flag) { m_voidSelf = flag; }
    शून्य cost(पूर्णाङ्क cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() अचर {
        फर्काऊ !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : सार्वजनिक Ast_गाँठो {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    अचर string m_name;
    अचर VUseType m_useType;  // What sort of use this is

सार्वजनिक:
    AstCUse(File_पङ्क्ति* fl, VUseType useType, अचर string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    VUseType useType() अचर { फर्काऊ m_useType; }
};
class AstCell final : सार्वजनिक Ast_गाँठो {
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
    File_पङ्क्ति* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    पूर्णाङ्क m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
सार्वजनिक:
    AstCell(File_पङ्क्ति* fl, File_पङ्क्ति* mfl, अचर string& instName, अचर string& modName,
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
    शून्य cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    // ACCESSORS
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Cell name
    शून्य name(अचर string& name) override { m_name = name; }
    string origName() अचर override { फर्काऊ m_origName; }  // * = Original name
    शून्य origName(अचर string& name) { m_origName = name; }
    string modName() अचर { फर्काऊ m_modName; }  // * = Instance name
    शून्य modName(अचर string& name) { m_modName = name; }
    File_पङ्क्ति* modNameFileline() अचर { फर्काऊ m_modNameFileline; }
    Ast_गाँठो_Module* modp() अचर { फर्काऊ m_modp; }  // [AfterLink] = Pointer to module instantiated
    शून्य modp(Ast_गाँठो_Module* nodep) { m_modp = nodep; }
    पूर्णाङ्क arrayIdx() अचर { फर्काऊ m_arrayIdx; }
    शून्य arrayIdx(पूर्णाङ्क idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() अचर { फर्काऊ m_hasIfaceVar; }
    शून्य hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    शून्य trace(bool flag) { m_trace = flag; }
    bool isTrace() अचर { फर्काऊ m_trace; }
    शून्य recursive(bool flag) { m_recursive = flag; }
    bool recursive() अचर { फर्काऊ m_recursive; }
};
class AstCellInline final : सार्वजनिक Ast_गाँठो {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    अचर std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    अचर std::string m_origModName;  // __BEGIN__ or __FORK__
सार्वजनिक:
    AstCellInline(File_पङ्क्ति* fl, अचर std::string& name, अचर std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    // ACCESSORS
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    std::string origModName() अचर { फर्काऊ m_origModName; }
};
class AstCellInlineScope final : सार्वजनिक Ast_गाँठो {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    अचर std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
सार्वजनिक:
    AstCellInlineScope(File_पङ्क्ति* fl, अचर std::string& name, अचर std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    // ACCESSORS
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर std::string& name) override { m_name = name; }
    std::string origModName() अचर { फर्काऊ m_origModName; }
};
class AstCgOptionAssign final : सार्वजनिक Ast_गाँठो {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    अचर VCoverOptionType m_optType;  // Option type
    अचर string m_name;  // Original option name (for diagnostics on unknown options)
    अचर bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
सार्वजनिक:
    AstCgOptionAssign(File_पङ्क्ति* fl, bool typeOption, VCoverOptionType optType,
                      अचर string& rawName, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    VCoverOptionType optType() अचर { फर्काऊ m_optType; }
    bool typeOption() अचर { फर्काऊ m_typeOption; }
};
class AstClassExtends final : सार्वजनिक Ast_गाँठो {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    अचर bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

सार्वजनिक:
    AstClassExtends(File_पङ्क्ति* fl, Ast_गाँठो* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    string verilogKwd() अचर override { फर्काऊ isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() अचर;
    AstClass* classp() अचर;  // Like above, but throws error if nulll
    bool isImplements() अचर { फर्काऊ m_isImplements; }
    शून्य parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() अचर { फर्काऊ m_parameterized; }
};
class AstClocking final : सार्वजनिक Ast_गाँठो {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    अचर bool m_isGlobal;  // True if global clocking

सार्वजनिक:
    AstClocking(File_पङ्क्ति* fl, अचर std::string& name, Ast_Sen_रूख* sentreep, Ast_गाँठो* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool isDefault() अचर { फर्काऊ m_isDefault; }
    bool isGlobal() अचर { फर्काऊ m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    शून्य makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : सार्वजनिक Ast_गाँठो {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

सार्वजनिक:
    AstClockingItem(File_पङ्क्ति* fl, VDirection direction, Ast_गाँठो_अभिव्यक्ति* skewp, Ast_गाँठो* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        यदि (AstAssign* अचर clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } अन्यथा {
            exprp(VN_AS(clockingDeclp, गाँठो_अभिव्यक्ति));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    VDirection direction() अचर { फर्काऊ m_direction; }
    AstClockingItem* outputp() अचर { फर्काऊ m_outputp; }
    शून्य outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
};
class AstConfig final : सार्वजनिक Ast_गाँठो {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

सार्वजनिक:
    AstConfig(File_पङ्क्ति* fl, अचर std::string& libname, अचर std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_libname + "." + m_configname; }
    std::string libname() अचर VL_MT_STABLE { फर्काऊ m_libname; }
    std::string configname() अचर VL_MT_STABLE { फर्काऊ m_configname; }
};
class AstConfigCell final : सार्वजनिक Ast_गाँठो {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

सार्वजनिक:
    AstConfigCell(File_पङ्क्ति* fl, अचर std::string& libname, अचर std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_libname + "." + m_cellname; }
    std::string libname() अचर VL_MT_STABLE { फर्काऊ m_libname; }
    std::string cellname() अचर VL_MT_STABLE { फर्काऊ m_cellname; }
};
class AstConfigRule final : सार्वजनिक Ast_गाँठो {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    अचर bool m_isCell;  // Declared as "cell" versus "instance"

सार्वजनिक:
    AstConfigRule(File_पङ्क्ति* fl, Ast_गाँठो* cellp, Ast_गाँठो* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() अचर VL_MT_STABLE { फर्काऊ m_isCell; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstConfigUse final : सार्वजनिक Ast_गाँठो {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    अचर bool m_isConfig;  // ":config"; Config, not module/primitive name

सार्वजनिक:
    AstConfigUse(File_पङ्क्ति* fl, अचर std::string& libname, अचर std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_libname + "." + m_cellname; }
    std::string libname() अचर VL_MT_STABLE { फर्काऊ m_libname; }
    std::string cellname() अचर VL_MT_STABLE { फर्काऊ m_cellname; }
    bool isConfig() अचर VL_MT_STABLE { फर्काऊ m_isConfig; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstConstraint final : सार्वजनिक Ast_गाँठो {
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
सार्वजनिक:
    AstConstraint(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Scope name
    शून्य name(अचर string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    शून्य baseOverride(अचर VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() अचर { फर्काऊ m_baseOverride; }
    bool isExternDef() अचर { फर्काऊ m_isExternDef; }
    शून्य isExternDef(bool flag) { m_isExternDef = flag; }
    शून्य isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() अचर { फर्काऊ m_isExternExplicit; }
    शून्य isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() अचर { फर्काऊ m_isExternProto; }
    शून्य isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() अचर { फर्काऊ m_isKwdPure; }
    शून्य isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() अचर { फर्काऊ m_isStatic; }
};
class AstCoverBin final : सार्वजनिक Ast_गाँठो {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    अचर string m_name;  // Base name of the bin
    अचर VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

सार्वजनिक:
    AstCoverBin(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_गाँठो_अभिव्यक्ति* iffp = nullptr)
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
    AstCoverBin(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_अभिव्यक्ति* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_पङ्क्ति* fl, अचर string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_पङ्क्ति* fl, अचर string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    VCoverBinsType binsType() अचर { फर्काऊ m_binsType; }
    bool isWildcard() अचर { फर्काऊ m_isWildcard; }
    bool isArray() अचर { फर्काऊ m_isArray; }
    शून्य isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : सार्वजनिक Ast_गाँठो {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    अचर bool m_isNegated;  // Complement the selection within the cross product

सार्वजनिक:
    AstCoverBinsof(File_पङ्क्ति* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_गाँठो* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& name) override { m_name = name; }
    bool isNegated() अचर { फर्काऊ m_isNegated; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {  // LCOV_EXCL_START
        अचर AstCoverBinsof* अचर asamep = VN_DBG_AS(samep, CoverBinsof);
        फर्काऊ m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : सार्वजनिक Ast_गाँठो {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    अचर string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    अचर VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

सार्वजनिक:
    AstCoverCrossBin(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* selectp, Ast_गाँठो_अभिव्यक्ति* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    string verilogKwd() अचर override;
    VCoverBinsType binsType() अचर { फर्काऊ m_binsType; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {  // LCOV_EXCL_START
        अचर AstCoverCrossBin* अचर asamep = VN_DBG_AS(samep, CoverCrossBin);
        फर्काऊ m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : सार्वजनिक Ast_गाँठो {
    // The complete bin-tuple space of the enclosing cross
    अचर string m_name;  // Referenced cross name

सार्वजनिक:
    AstCoverCrossRef(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {  // LCOV_EXCL_START
        फर्काऊ m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : सार्वजनिक Ast_गाँठो {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    अचर bool m_isOr;  // Union (||), rather than intersection (&&)

सार्वजनिक:
    AstCoverCrossSelect(File_पङ्क्ति* fl, Ast_गाँठो* lhsp, Ast_गाँठो* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool isOr() अचर { फर्काऊ m_isOr; }
    string verilogKwd() अचर override { फर्काऊ isOr() ? "||" : "&&"; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {  // LCOV_EXCL_START
        फर्काऊ m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : सार्वजनिक Ast_गाँठो {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
सार्वजनिक:
    AstCoverCrossWith(File_पङ्क्ति* fl, Ast_गाँठो* selectp, Ast_गाँठो_अभिव्यक्ति* filterp, Ast_गाँठो_अभिव्यक्ति* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : सार्वजनिक Ast_गाँठो {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    अचर VCoverOptionType m_optType;  // Option being assigned
    अचर bool m_typeOption;  // type_option (vs option)

सार्वजनिक:
    AstCoverOption(File_पङ्क्ति* fl, bool typeOption, VCoverOptionType optType, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    VCoverOptionType optType() अचर { फर्काऊ m_optType; }
    bool typeOption() अचर { फर्काऊ m_typeOption; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {  // LCOV_EXCL_START
        अचर AstCoverOption* अचर asamep = VN_DBG_AS(samep, CoverOption);
        फर्काऊ m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : सार्वजनिक Ast_गाँठो {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
सार्वजनिक:
    AstCoverToggle(File_पङ्क्ति* fl, AstCoverInc* incp, Ast_गाँठो_अभिव्यक्ति* origp, Ast_गाँठो_अभिव्यक्ति* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ 3 + INSTR_गन्ती_BRANCH + INSTR_गन्ती_LD; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ true; }
    bool isOutputter() override {
        फर्काऊ false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : सार्वजनिक Ast_गाँठो {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    अचर VTransRepType m_repType;

सार्वजनिक:
    AstCoverTransItem(File_पङ्क्ति* fl, Ast_गाँठो* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstCoverTransSet final : सार्वजनिक Ast_गाँठो {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
सार्वजनिक:
    AstCoverTransSet(File_पङ्क्ति* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstCoverWith final : सार्वजनिक Ast_गाँठो {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
सार्वजनिक:
    AstCoverWith(File_पङ्क्ति* fl, Ast_गाँठो* subp, Ast_गाँठो_अभिव्यक्ति* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        addSubp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : सार्वजनिक Ast_गाँठो {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

सार्वजनिक:
    AstCovergroup(File_पङ्क्ति* fl, अचर string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_गाँठो* membersp, Ast_Sen_रूख* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& name) override { m_name = name; }
    bool maybePointedTo() अचर override { फर्काऊ true; }
};
class AstCoverpointRef final : सार्वजनिक Ast_गाँठो {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    अचर string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

सार्वजनिक:
    AstCoverpointRef(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
};
class AstDefParam final : सार्वजनिक Ast_गाँठो {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
सार्वजनिक:
    AstDefParam(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* pathp, अचर string& name, Ast_गाँठो_अभिव्यक्ति* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ true; }
};
class AstDefaultClocking final : सार्वजनिक Ast_गाँठो {
    std::string m_name;  // Clocking block name

सार्वजनिक:
    AstDefaultClocking(File_पङ्क्ति* fl, अचर std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
};
class AstDefaultDisable final : सार्वजनिक Ast_गाँठो {
    // @astgen op1 := condp : AstNodeExpr

सार्वजनिक:
    AstDefaultDisable(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : सार्वजनिक Ast_गाँठो {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
सार्वजनिक:
    AstDpiExport(File_पङ्क्ति* fl, अचर string& vname, अचर string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& name) override { m_name = name; }
    string cname() अचर { फर्काऊ m_cname; }
    शून्य cname(अचर string& cname) { m_cname = cname; }
};
class AstElabDisplay final : सार्वजनिक Ast_गाँठो {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

सार्वजनिक:
    अन्तःस्थ AstElabDisplay(File_पङ्क्ति* fl, VDisplayType dispType, Ast_गाँठो_अभिव्यक्ति* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!fmtp());
        फर्काऊ nullptr;
    }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string verilogKwd() अचर override { फर्काऊ "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() अचर override { फर्काऊ false; }
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { फर्काऊ true; }  // SPECIAL: $display makes output
    bool isUnlikely() अचर override { फर्काऊ true; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    पूर्णाङ्क instr_गन्ती() अचर override { फर्काऊ INSTR_गन्ती_PLI; }
    VDisplayType displayType() अचर { फर्काऊ m_displayType; }
    शून्य displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : सार्वजनिक Ast_गाँठो {
    // Represents something missing, e.g. a missing argument in FOREACH
सार्वजनिक:
    explicit AstEmpty(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstGenCaseItem final : सार्वजनिक Ast_गाँठो {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
सार्वजनिक:
    AstGenCaseItem(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* condsp, Ast_गाँठो* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() अचर { फर्काऊ condsp() == nullptr; }
};
class AstImplicit final : सार्वजनिक Ast_गाँठो {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
सार्वजनिक:
    AstImplicit(File_पङ्क्ति* fl, Ast_गाँठो* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : सार्वजनिक Ast_गाँठो {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
सार्वजनिक:
    AstIntfRef(File_पङ्क्ति* fl, अचर string& name, अचर string& parentName, अचर string& baseName,
               अचर string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    string parentName() अचर { फर्काऊ m_parentName; }
    string baseName() अचर { फर्काऊ m_baseName; }
    string modportName() अचर { फर्काऊ m_modportName; }
};
class AstLibrary final : सार्वजनिक Ast_गाँठो {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

सार्वजनिक:
    AstLibrary(File_पङ्क्ति* fl, अचर std::string& name, Ast_गाँठो_अभिव्यक्ति* filesp, Ast_गाँठो_अभिव्यक्ति* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
};
class AstModport final : सार्वजनिक Ast_गाँठो {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
सार्वजनिक:
    AstModport(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() अचर override { फर्काऊ "modport"; }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : सार्वजनिक Ast_गाँठो {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
सार्वजनिक:
    AstModportClockingRef(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    शून्य dump(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    AstClocking* clockingp() अचर VL_MT_STABLE {
        फर्काऊ m_clockingp;
    }  // [After Link] Pointer to clocking block
    शून्य clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : सार्वजनिक Ast_गाँठो {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
सार्वजनिक:
    AstModportFTaskRef(File_पङ्क्ति* fl, अचर string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool isImport() अचर { फर्काऊ !m_export; }
    bool isExport() अचर { फर्काऊ m_export; }
    Ast_गाँठो_FTask* ftaskp() अचर { फर्काऊ m_ftaskp; }  // [After Link] Pointer to variable
    शून्य ftaskp(Ast_गाँठो_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : सार्वजनिक Ast_गाँठो {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    अचर VDirection m_direction;  // Direction of the variable (in/out)
सार्वजनिक:
    AstModportVarRef(File_पङ्क्ति* fl, अचर string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_अभिव्यक्ति* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    VDirection direction() अचर { फर्काऊ m_direction; }
    AstVar* varp() अचर VL_MT_STABLE { फर्काऊ m_varp; }  // [After Link] Pointer to variable
    शून्य varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : सार्वजनिक Ast_गाँठो {
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
    std::unordered_map<अचर AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<अचर Ast_गाँठो*, अचर Ast_गाँठो_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

सार्वजनिक:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    अचर वर्ण* broken() अचर override;
    शून्य थप_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.घुसाउ(varp); }
    अचर std::set<AstVar*>& deferredParamVarps() अचर { फर्काऊ m_deferredParamVarps; }
    शून्य clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    शून्य deleteContents();
    शून्य cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() अचर override VL_MT_STABLE { फर्काऊ "$root"; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    Ast_गाँठो_Module* शीर्ष_Modulep() अचर VL_MT_STABLE {  // Top module in hierarchy
        फर्काऊ modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { फर्काऊ m_typeTablep; }
    string astConstOrigParamName(अचर AstConst* nodep) अचर;
    शून्य astConstOrigParamName(अचर AstConst* nodep, अचर string& name);
    शून्य astConstOrigParamNameErase(अचर AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    अचर Ast_गाँठो_Module* containingModule(अचर Ast_गाँठो* nodep);
    // Forget remembered modules, as the tree has moved.
    शून्य clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() अचर { फर्काऊ m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() अचर { फर्काऊ m_dollarUnitPkgp; }
    शून्य dollarUnitPkgp(AstPackage* अचर packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() अचर { फर्काऊ m_dpiExportTriggerp; }
    शून्य dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() अचर { फर्काऊ m_delaySchedulerp; }
    शून्य delaySchedulerp(AstVar* अचर varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() अचर { फर्काऊ m_nbaEventp; }
    शून्य nbaEventp(AstVarScope* अचर varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() अचर { फर्काऊ m_nbaEventTriggerp; }
    शून्य nbaEventTriggerp(AstVarScope* अचर varScopep) { m_nbaEventTriggerp = varScopep; }
    शून्य stdPackagep(AstPackage* अचर packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() अचर { फर्काऊ m_stdPackagep; }
    शून्य stdPackageProcessp(AstClass* अचर classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() अचर { फर्काऊ m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_पङ्क्ति*) अचर;
    Ast_शीर्ष_Scope* शीर्ष_Scopep() अचर { फर्काऊ m_शीर्ष_Scopep; }
    शून्य create_शीर्ष_Scope(AstScope* scopep);
    VTimescale timeunit() अचर { फर्काऊ m_timeunit; }
    शून्य timeunit(अचर VTimescale& मान) { m_timeunit = मान; }
    VTimescale timeprecision() अचर { फर्काऊ m_timeprecision; }
    शून्य timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    शून्य timeprecisionMerge(File_पङ्क्ति*, अचर VTimescale& मान);
    शून्य timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() अचर { फर्काऊ m_timescaleSpecified; }
    अचर std::string& resolved_शीर्ष_Module_Name() अचर { फर्काऊ m_resolved_शीर्ष_Module_Name; }
    शून्य resolved_शीर्ष_Module_Name(अचर std::string& मान) { m_resolved_शीर्ष_Module_Name = मान; }
    uint32_t nTraceCodes() अचर { फर्काऊ m_nTraceCodes; }
    शून्य nTraceCodes(uint32_t मान) { m_nTraceCodes = मान; }
    AstVarScope* stlFirstIterationp();
    शून्य clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    अचर std::string trace_Lib_शीर्ष_Name() अचर {
        अचर std::string& name = resolved_शीर्ष_Module_Name();
        फर्काऊ prettyName(name.empty() ? v3Global.rootp()->शीर्ष_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    शून्य addEvalStats(अचर std::string& phase);
};
class AstPackageExport final : सार्वजनिक Ast_गाँठो {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

सार्वजनिक:
    AstPackageExport(File_पङ्क्ति* fl, अचर string& pkgName, अचर string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    string pkgName() अचर VL_MT_STABLE { फर्काऊ m_pkgName; }
    string prettyPkgNameQ() अचर { फर्काऊ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() अचर { फर्काऊ m_packagep; }
    शून्य packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : सार्वजनिक Ast_गाँठो {
    // A package export *::* declaration
सार्वजनिक:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : सार्वजनिक Ast_गाँठो {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

सार्वजनिक:
    AstPackageImport(File_पङ्क्ति* fl, AstPackage* packagep, अचर string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_पङ्क्ति* fl, अचर string& pkgName, अचर string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    string pkgName() अचर VL_MT_STABLE { फर्काऊ m_pkgName; }
    string prettyPkgNameQ() अचर { फर्काऊ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() अचर { फर्काऊ m_packagep; }
    शून्य packagep(AstPackage* nodep) { m_packagep = nodep; }

गुप्त:
    शून्य pkgNameFrom();
};
class AstPin final : सार्वजनिक Ast_गाँठो {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    पूर्णाङ्क m_pin_अङ्क;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
सार्वजनिक:
    AstPin(File_पङ्क्ति* fl, पूर्णाङ्क pin_अङ्क, अचर string& name, Ast_गाँठो* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_अङ्क{pin_अङ्क}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    शून्य cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Pin name, ""=go by number
    शून्य name(अचर string& name) override { m_name = name; }
    string prettyOperatorName() अचर override;
    bool dotStar() अचर { फर्काऊ name() == ".*"; }  // Fake name for .* connections until linked
    पूर्णाङ्क pin_अङ्क() अचर { फर्काऊ m_pin_अङ्क; }
    AstVar* modVarp() अचर { फर्काऊ m_modVarp; }  // [After Link] Pointer to variable
    शून्य modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() अचर { फर्काऊ m_modPTypep; }
    शून्य modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() अचर { फर्काऊ m_param; }
    शून्य param(bool flag) { m_param = flag; }
    अचर string& paramPath() अचर { फर्काऊ m_paramPath; }
    शून्य paramPath(अचर string& path) { m_paramPath = path; }
    bool svDotName() अचर { फर्काऊ m_svDotName; }
    शून्य svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() अचर { फर्काऊ m_svImplicit; }
    शून्य svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : सार्वजनिक Ast_गाँठो {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    अचर पूर्णाङ्क m_pin_अङ्क;  // Pin number
    अचर string m_name;  // Name of pin
सार्वजनिक:
    AstPort(File_पङ्क्ति* fl, पूर्णाङ्क pinnum, अचर string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_अङ्क{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Port name
    पूर्णाङ्क pin_अङ्क() अचर { फर्काऊ m_pin_अङ्क; }  // * = Pin number, for order based instantiation
};
class AstPragma final : सार्वजनिक Ast_गाँठो {
    अचर VPragmaType m_pragType;  // Type of pragma
    अचर VTimescale m_timescale;  // For TIMEUNIT_SET
सार्वजनिक:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_पङ्क्ति* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_पङ्क्ति* fl, अचर VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    VPragmaType pragType() अचर { फर्काऊ m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() अचर override { फर्काऊ false; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() अचर { फर्काऊ m_timescale; }
};
class AstPropSpec final : सार्वजनिक Ast_गाँठो {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

सार्वजनिक:
    AstPropSpec(File_पङ्क्ति* fl, AstSenItem* sensesp, Ast_गाँठो_अभिव्यक्ति* disablep, Ast_गाँठो* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool hasDType() अचर override VL_MT_SAFE {
        फर्काऊ true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() अचर { फर्काऊ m_propStrength; }
};
class AstPull final : सार्वजनिक Ast_गाँठो {
    // @astgen op1 := lhsp : AstNodeExpr

    अचर bool m_direction;

सार्वजनिक:
    AstPull(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() अचर { फर्काऊ m_direction; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstScope final : सार्वजनिक Ast_गाँठो {
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
सार्वजनिक:
    AstScope(File_पङ्क्ति* fl, Ast_गाँठो_Module* modp, अचर string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!m_modp);
        फर्काऊ nullptr;
    }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Scope name
    शून्य name(अचर string& name) override { m_name = name; }
    string pretty_Name_सन्देश() अचर override;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    string nameDotless() अचर;
    Ast_गाँठो_Module* modp() अचर { फर्काऊ m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() अचर VL_MT_SAFE { फर्काऊ m_aboveScopep; }
    शून्य aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() अचर { फर्काऊ m_aboveCellp; }
    शून्य aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_शीर्ष() अचर VL_MT_SAFE { फर्काऊ aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : सार्वजनिक Ast_गाँठो {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_edgeType;  // Edge type
सार्वजनिक:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    AstSenItem(File_पङ्क्ति* fl, VEdgeType edgeType, Ast_गाँठो_अभिव्यक्ति* senp, Ast_गाँठो_अभिव्यक्ति* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_पङ्क्ति* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_पङ्क्ति* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_पङ्क्ति* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_पङ्क्ति* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_पङ्क्ति* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_पङ्क्ति* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() अचर { फर्काऊ m_edgeType; }
    शून्य edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_गन्ती_Inc();
    }
    Ast_गाँठो_Var_Ref* varrefp() अचर { फर्काऊ VN_CAST(sensp(), गाँठो_Var_Ref); }
    //
    bool isClocked() अचर { फर्काऊ edgeType().clockedStmt(); }
    bool isComboOrStar() अचर {
        फर्काऊ edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() अचर { फर्काऊ edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() अचर { फर्काऊ edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() अचर { फर्काऊ edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() अचर { फर्काऊ edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() अचर { फर्काऊ edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() अचर { फर्काऊ edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_रूख final : सार्वजनिक Ast_गाँठो {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
सार्वजनिक:
    Ast_Sen_रूख(File_पङ्क्ति* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_रूख(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_रूख;
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    // METHODS
    bool hasClocked() अचर;  // Includes a clocked statement
    bool hasEdge() अचर;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() अचर;  // Includes a STATIC SenItem
    bool hasInitial() अचर;  // Includes a INITIAL SenItem
    bool hasFinal() अचर;  // Includes a FINAL SenItem
    bool hasCombo() अचर;  // Includes a COMBO SenItem
    bool hasHybrid() अचर;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : सार्वजनिक Ast_गाँठो {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

सार्वजनिक:
    AstStrengthSpec(File_पङ्क्ति* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { फर्काऊ m_s0; }
    VStrength strength1() { फर्काऊ m_s1; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstSystemCSection final : सार्वजनिक Ast_गाँठो {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    अचर VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

सार्वजनिक:
    AstSystemCSection(File_पङ्क्ति* fl, VSystemCSectionType sectionType, अचर std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() अचर { फर्काऊ m_sectionType; }
    अचर std::string& text() अचर { फर्काऊ m_text; }
    शून्य text(अचर std::string& मान) { m_text = मान; }
    शून्य dump(std::ostream&) अचर override;
    शून्य dumpJson(std::ostream&) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो*) अचर override { फर्काऊ false; }
    bool isPure() override { फर्काऊ false; }
    bool isOutputter() override { फर्काऊ true; }
};
class AstText final : सार्वजनिक Ast_गाँठो {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    अचर std::string m_text;  // The text to emit
सार्वजनिक:
    AstText(File_पङ्क्ति* fl, अचर std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ text() == VN_DBG_AS(samep, Text)->text();
    }
    अचर std::string& text() अचर VL_MT_SAFE { फर्काऊ m_text; }
};
class AstTextBlock final : सार्वजनिक Ast_गाँठो {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    अचर std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    अचर std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    अचर std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
सार्वजनिक:
    explicit AstTextBlock(File_पङ्क्ति* fl,  //
                          अचर std::string& prefix = "",  //
                          अचर std::string& separator = "",  //
                          अचर std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    अचर std::string& prefix() अचर { फर्काऊ m_prefix; }
    अचर std::string& separator() अचर { फर्काऊ m_separator; }
    अचर std::string& suffix() अचर { फर्काऊ m_suffix; }
    // Add some text, or a node to this block
    शून्य add(अचर string& text) { addNodesp(new AstText{fileline(), text}); }
    शून्य add(Ast_गाँठो* nodep) { addNodesp(nodep); }
};
class Ast_शीर्ष_Scope final : सार्वजनिक Ast_गाँठो {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    मित्र class AstNetlist;  // Only the AstNetlist can create one
    Ast_शीर्ष_Scope(File_पङ्क्ति* fl, AstScope* ascopep)
        : ASTGEN_SUPER_शीर्ष_Scope(fl) {
        this->scopep(ascopep);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_शीर्ष_Scope;
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* createTemp(अचर string& name, अचिन्हित width);
    AstVarScope* createTemp(अचर string& name, Ast_गाँठो_DType* dtypep);
};
class AstTypeTable final : सार्वजनिक Ast_गाँठो {
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
    using DetailedMap = std::map<VBasic_Type_साँचो, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

सार्वजनिक:
    explicit AstTypeTable(File_पङ्क्ति* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    शून्य cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_पङ्क्ति* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_पङ्क्ति* fl, VBasicDTypeKwd kwd, पूर्णाङ्क width, पूर्णाङ्क widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_पङ्क्ति* fl, VBasicDTypeKwd kwd, अचर VNumRange& range,
                                     पूर्णाङ्क widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& गाँठो);
    AstBasicDType* find_घुसाउ_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_पङ्क्ति* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_पङ्क्ति* fl);
    AstQueueDType* findQueueIndexDType(File_पङ्क्ति* fl);
    AstStreamDType* findStreamDType(File_पङ्क्ति* fl);
    AstVoidDType* findVoidDType(File_पङ्क्ति* fl);
    शून्य clearCache();
    शून्य repairCache();
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
};
class AstTypedef final : सार्वजनिक Ast_गाँठो {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_अङ्क;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

सार्वजनिक:
    AstTypedef(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* attrsp, VFlagChildDType,
               Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_अङ्क{fl->token_अङ्क()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_अङ्क{fl->token_अङ्क()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() अचर;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    आभास Ast_गाँठो_DType* subDTypep() अचर VL_MT_STABLE {
        फर्काऊ dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_अङ्क() अचर override { फर्काऊ m_decl_Token_अङ्क; }
    शून्य decl_Token_अङ्क_Set_Min(uint32_t token_अङ्क) override {
        m_decl_Token_अङ्क = std::min(m_decl_Token_अङ्क, token_अङ्क);
    }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    शून्य name(अचर string& flag) override { m_name = flag; }
    bool attrPublic() अचर { फर्काऊ m_attrPublic; }
    शून्य attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() अचर { फर्काऊ m_isHideLocal; }
    शून्य isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() अचर { फर्काऊ m_isHideProtected; }
    शून्य isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() अचर { फर्काऊ m_isUnderClass; }
    शून्य isUnderClass(bool flag) { m_isUnderClass = flag; }
    शून्य tag(अचर string& text) override { m_tag = text; }
    string tag() अचर override { फर्काऊ m_tag; }
};
class AstTypedefFwd final : सार्वजनिक Ast_गाँठो {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    अचर VFwdType m_fwdType;  // Forward type for lint check

सार्वजनिक:
    AstTypedefFwd(File_पङ्क्ति* fl, अचर string& name, अचर VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    VFwdType fwdType() अचर { फर्काऊ m_fwdType; }
};
class AstUdpTable final : सार्वजनिक Ast_गाँठो {
    // @astgen op1 := linesp : List[AstUdpTableLine]
सार्वजनिक:
    AstUdpTable(File_पङ्क्ति* fl, Ast_Udp_Table_पङ्क्ति* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        यदि (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_पङ्क्ति final : सार्वजनिक Ast_गाँठो {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
गुप्त:
    अचर bool m_udpIsCombo;  // Combinational or sequential UDP

सार्वजनिक:
    class UdpCombo {};
    Ast_Udp_Table_पङ्क्ति(UdpCombo, File_पङ्क्ति* fl, Ast_Udp_Table_पङ्क्ति_Val* iFieldsp,
                    Ast_Udp_Table_पङ्क्ति_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_पङ्क्ति(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_पङ्क्ति(UdpSequential, File_पङ्क्ति* fl, Ast_Udp_Table_पङ्क्ति_Val* iFieldsp,
                    Ast_Udp_Table_पङ्क्ति_Val* oFieldsp1, Ast_Udp_Table_पङ्क्ति_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_पङ्क्ति(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_पङ्क्ति;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    पूर्णाङ्क udpIsCombo() अचर { फर्काऊ m_udpIsCombo; }
};
class Ast_Udp_Table_पङ्क्ति_Val final : सार्वजनिक Ast_गाँठो {
    string m_text;  // Value character

सार्वजनिक:
    Ast_Udp_Table_पङ्क्ति_Val(File_पङ्क्ति* fl, अचर string& text)
        : ASTGEN_SUPER_Udp_Table_पङ्क्ति_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_पङ्क्ति_Val;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_text; }
    शून्य name(std::string अचर& text) override VL_MT_STABLE { m_text = text; }
    string text() अचर VL_MT_SAFE { फर्काऊ m_text; }
};
class AstVar final : सार्वजनिक Ast_गाँठो {
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
    पूर्णाङ्क m_pin_अङ्क = 0;  // For JSON, if non-zero the connection pin number
    पूर्णाङ्क m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
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
    bool m_mtask_Cache_पङ्क्ति_Align : 1;  // Start MTask affinity group on a cache line
    शून्य init() {
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
        m_mtask_Cache_पङ्क्ति_Align = false;
    }

सार्वजनिक:
    AstVar(File_पङ्क्ति* fl, VVarType type, अचर string& name, VFlagChildDType, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_पङ्क्ति* fl, VVarType type, अचर string& name, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_पङ्क्ति* fl, VVarType type, अचर string& name, VFlagLogicPacked, पूर्णाङ्क wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_पङ्क्ति* fl, VVarType type, अचर string& name, VFlagBitPacked, पूर्णाङ्क wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_पङ्क्ति* fl, VVarType type, अचर string& name, अचर AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        यदि (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_रूख(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    अन्तःस्थ bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    string origName() अचर override { फर्काऊ m_origName; }  // * = Original name
    शून्य origName(अचर string& name) { m_origName = name; }
    VVarType varType() अचर VL_MT_SAFE { फर्काऊ m_varType; }  // * = Type of variable
    शून्य direction(अचर VDirection& flag) {
        m_direction = flag;
        यदि (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() अचर VL_MT_SAFE { फर्काऊ m_direction; }
    bool isIO() अचर VL_MT_SAFE { फर्काऊ m_direction != VDirection::NONE; }
    bool isVLIO() अचर {
        अचर AstBasicDType* अचर bdtypep = basicp();
        फर्काऊ isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    शून्य declDirection(अचर VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() अचर { फर्काऊ m_declDirection; }
    पूर्णाङ्क libInoutId() अचर { फर्काऊ m_libInoutId; }
    शून्य libInoutId(पूर्णाङ्क id) { m_libInoutId = id; }
    bool libInoutEnable() अचर { फर्काऊ m_libInoutEnable; }
    शून्य libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    शून्य varType(VVarType type) { m_varType = type; }
    शून्य varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    शून्य varType2In() {
        m_tristate = false;
        m_direction = VDirection::आगत;
    }
    string scType() अचर;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) अचर;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) अचर;
    string dpiTmpVarType(अचर string& varName) अचर;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) अचर;
    string vlEnumType() अचर;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) अचर;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(अचर string& propName) अचर;  // Return VerilatorVarProps declaration
    शून्य combineType(VVarType type);
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* dtypeSkipRefp() अचर VL_MT_STABLE { फर्काऊ subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() अचर VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    आभास Ast_गाँठो_DType* subDTypep() अचर VL_MT_STABLE {
        फर्काऊ dtypep() ? dtypep() : childDTypep();
    }
    शून्य ansi(bool flag) { m_ansi = flag; }
    शून्य declTyped(bool flag) { m_declTyped = flag; }
    शून्य sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    शून्य attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    शून्य attrScBv(bool flag) { m_attrScBv = flag; }
    शून्य attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    शून्य attrSFormat(bool flag) { m_attrSFormat = flag; }
    शून्य attrSplitVar(bool flag) { m_attrSplitVar = flag; }
    शून्य attrFsmState(bool flag) { m_attrFsmState = flag; }
    शून्य attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    शून्य attrFsmStateExpand(अचर VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() अचर { फर्काऊ m_constPoolEntry; }
    शून्य setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() अचर { फर्काऊ m_covergroupRefMember; }
    शून्य covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() अचर { फर्काऊ m_embeddedCovergroup; }
    शून्य embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    शून्य अक्रम(अचर VRandAttr flag) { m_rand = flag; }
    शून्य usedParam(bool flag) { m_usedParam = flag; }
    शून्य usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    शून्य sigPublic(bool flag) { m_sigPublic = flag; }
    शून्य sigModPublic(bool flag) { m_sigModPublic = flag; }
    शून्य sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        यदि (flag) sigPublic(true);
    }
    शून्य sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        यदि (flag) sigUserRdPublic(true);
    }
    शून्य sc(bool flag) { m_sc = flag; }
    शून्य scSensitive(bool flag) { m_scSensitive = flag; }
    शून्य primaryIO(bool flag) { m_primaryIO = flag; }
    शून्य isConst(bool flag) { m_isConst = flag; }
    शून्य isContinuously(bool flag) { m_isContinuously = flag; }
    शून्य isIfaceParent(bool flag) { m_isIfaceParent = flag; }
    शून्य isIfaceArraySplit(bool flag) { m_isIfaceArraySplit = flag; }
    शून्य isInternal(bool flag) { m_isInternal = flag; }
    शून्य isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    शून्य funcLocal(bool flag) {
        m_funcLocal = flag;
        यदि (flag) m_funcLocalSticky = true;
    }
    शून्य funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() अचर { फर्काऊ m_gotNansiType; }
    शून्य gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() अचर { फर्काऊ m_hasStrengthAssignment; }
    शून्य hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() अचर { फर्काऊ m_hasUserInit; }
    शून्य hasUserInit(bool flag) { m_hasUserInit = flag; }
    शून्य icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() अचर { फर्काऊ m_icoMaybeWritten; }
    शून्य isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() अचर { फर्काऊ m_isClockingDriven; }
    bool isDpiOpenArray() अचर VL_MT_SAFE { फर्काऊ m_isDpiOpenArray; }
    शून्य isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() अचर { फर्काऊ m_isHideLocal; }
    शून्य isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() अचर { फर्काऊ m_isHideProtected; }
    शून्य isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() अचर { फर्काऊ m_noCReset; }
    शून्य noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() अचर { फर्काऊ m_noReset; }
    शून्य noReset(bool flag) { m_noReset = flag; }
    bool noSubst() अचर { फर्काऊ m_noSubst; }
    शून्य noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() अचर { फर्काऊ m_processQueue; }
    शून्य processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() अचर { फर्काऊ m_sampled; }
    शून्य sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() अचर { फर्काऊ m_substConstOnly; }
    शून्य substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() अचर { फर्काऊ m_overriddenParam; }
    शून्य overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() अचर { फर्काऊ m_untypedParam; }
    शून्य untypedParam(bool flag) { m_untypedParam = flag; }
    शून्य trace(bool flag) { m_trace = flag; }
    शून्य isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() अचर { फर्काऊ m_isForceable; }
    शून्य setForceable() { m_isForceable = true; }
    शून्य setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() अचर { फर्काऊ m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() अचर { फर्काऊ m_isReadByDpi; }
    शून्य setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() अचर { फर्काऊ m_isWrittenByDpi; }
    शून्य setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() अचर { फर्काऊ m_isWrittenBySuspendable; }
    शून्य setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() अचर { फर्काऊ m_ignorePostRead; }
    शून्य setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() अचर { फर्काऊ m_ignorePostWrite; }
    शून्य setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() अचर { फर्काऊ m_ignoreSchedWrite; }
    शून्य setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() अचर { फर्काऊ m_dfgTriLowered; }
    शून्य setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() अचर { फर्काऊ m_dfgAllowMultidriveTri; }
    शून्य setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    शून्य globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() अचर { फर्काऊ m_globalConstrained; }
    bool isStdRandomizeArg() अचर { फर्काऊ m_isStdRandomizeArg; }
    शून्य setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    शून्य name(अचर string& name) override { m_name = name; }
    शून्य tag(अचर string& text) override { m_tag = text; }
    string tag() अचर override { फर्काऊ m_tag; }
    bool isAnsi() अचर { फर्काऊ m_ansi; }
    bool isContinuously() अचर { फर्काऊ m_isContinuously; }
    bool isDeclTyped() अचर { फर्काऊ m_declTyped; }
    bool isInout() अचर { फर्काऊ m_direction.isInout(); }
    bool isInoutOrRef() अचर { फर्काऊ m_direction.isInoutOrRef(); }
    bool is_आगत() अचर { फर्काऊ m_direction.is_आगत(); }
    bool isNonOutput() अचर { फर्काऊ m_direction.isNonOutput(); }
    bool isReadOnly() अचर VL_MT_SAFE { फर्काऊ m_direction.isReadOnly(); }
    bool isConstRef() अचर VL_MT_SAFE { फर्काऊ m_direction.isConstRef(); }
    bool isRef() अचर VL_MT_SAFE { फर्काऊ m_direction.isRef(); }
    bool isWritable() अचर VL_MT_SAFE { फर्काऊ m_direction.isWritable(); }
    bool isTristate() अचर { फर्काऊ m_tristate; }
    bool isPrimaryIO() अचर VL_MT_SAFE { फर्काऊ m_primaryIO; }
    bool isPrimaryInish() अचर { फर्काऊ isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() अचर VL_MT_SAFE { फर्काऊ m_primaryClock; }
    शून्य setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() अचर { फर्काऊ varType() == VVarType::IFACEREF; }
    शून्य setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() अचर { फर्काऊ m_isIfaceParent; }
    bool isIfaceArraySplit() अचर { फर्काऊ m_isIfaceArraySplit; }
    bool isInternal() अचर { फर्काऊ m_isInternal; }
    bool isIfaceParam() अचर { फर्काऊ m_isIfaceParam; }
    bool isSignal() अचर { फर्काऊ varType().isSignal(); }
    bool isNet() अचर { फर्काऊ varType().isNet(); }
    bool isWor() अचर { फर्काऊ varType().isWor(); }
    bool isWiredNet() अचर { फर्काऊ varType().isWiredNet(); }
    bool isTemp() अचर { फर्काऊ varType().isTemp(); }
    bool isClassMember() अचर { फर्काऊ varType() == VVarType::MEMBER; }
    bool isVirtIface() अचर {
        यदि (AstIfaceRefDType* अचर dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            फर्काऊ dtp->isVirtual();
        }
        फर्काऊ false;
    }
    bool isStatementTemp() अचर { फर्काऊ varType() == VVarType::STMTTEMP; }
    bool isXTemp() अचर { फर्काऊ varType() == VVarType::XTEMP; }
    bool isParam() अचर { फर्काऊ varType().isParam(); }
    bool isGParam() अचर { फर्काऊ varType() == VVarType::GPARAM; }
    bool isGenVar() अचर { फर्काऊ varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() अचर {
        फर्काऊ isIO() || isSignal() || isClassMember() || isTemp() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() अचर {
        फर्काऊ !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() अचर {
        अचर AstBasicDType* अचर bdtypep = basicp();
        फर्काऊ bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() अचर { फर्काऊ m_usedParam; }
    bool isUsedLoopIdx() अचर { फर्काऊ m_usedLoopIdx; }
    bool mtask_Cache_पङ्क्ति_Align() अचर { फर्काऊ m_mtask_Cache_पङ्क्ति_Align; }
    शून्य mtask_Cache_पङ्क्ति_Align(bool flag) { m_mtask_Cache_पङ्क्ति_Align = flag; }
    bool isSc() अचर VL_MT_SAFE { फर्काऊ m_sc; }
    bool isScQuad() अचर;
    bool isScBv() अचर VL_MT_STABLE;
    bool isScUint() अचर;
    bool isScUintBool() अचर;
    bool isScBigUint() अचर VL_MT_STABLE;
    bool isScSensitive() अचर { फर्काऊ m_scSensitive; }
    bool isSigPublic() अचर;
    bool isSigModPublic() अचर { फर्काऊ m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() अचर { फर्काऊ m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() अचर { फर्काऊ m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() अचर { फर्काऊ m_trace; }
    bool isRand() अचर { फर्काऊ m_rand.isRand(); }
    bool isRandC() अचर { फर्काऊ m_rand.isRandC(); }
    bool isConst() अचर VL_MT_SAFE { फर्काऊ m_isConst; }
    bool isLatched() अचर { फर्काऊ m_isLatched; }
    bool isFuncLocal() अचर { फर्काऊ m_funcLocal; }
    bool isFuncLocalSticky() अचर { फर्काऊ m_funcLocalSticky; }
    bool isFuncReturn() अचर { फर्काऊ m_funcReturn; }
    bool isPullup() अचर { फर्काऊ m_isPullup; }
    bool isPulldown() अचर { फर्काऊ m_isPulldown; }
    bool attrScBv() अचर { फर्काऊ m_attrScBv; }
    bool attrScBigUint() अचर { फर्काऊ m_attrScBigUint; }
    bool attrFileDescr() अचर { फर्काऊ m_attrFileDescr; }
    bool attrSFormat() अचर { फर्काऊ m_attrSFormat; }
    bool attrSplitVar() अचर { फर्काऊ m_attrSplitVar; }
    bool attrFsmState() अचर { फर्काऊ m_attrFsmState; }
    bool attrFsmArcInclCond() अचर { फर्काऊ m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() अचर { फर्काऊ m_attrFsmStateExpand; }
    AstIface* sensIfacep() अचर { फर्काऊ m_sensIfacep; }
    VRandAttr अक्रम() अचर { फर्काऊ m_rand; }
    string verilogKwd() अचर override;
    शून्य lifetime(अचर VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() अचर { फर्काऊ m_lifetime; }
    शून्य pin_अङ्क(पूर्णाङ्क id) { m_pin_अङ्क = id; }
    पूर्णाङ्क pin_अङ्क() अचर { फर्काऊ m_pin_अङ्क; }
    शून्य propagateAttrFrom(अचर AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        यदि (fromp->attrFileDescr()) attrFileDescr(true);
        यदि (fromp->isContinuously()) isContinuously(true);
    }
    शून्य propagateSplitAttrFrom(अचर AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    अचर वर्ण* cannotSplitKindReason() अचर;
    शून्य propagateWrapAttrFrom(अचर AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    शून्य combineType(अचर AstVar* otherp);
    bool needsCReset() अचर {
        फर्काऊ !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !isStatementTemp()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    स्थिर AstVar* scVarRecurse(Ast_गाँठो* nodep);
};
class AstVarScope final : सार्वजनिक Ast_गाँठो {
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
    bool m_fork_By_मान : 1;  // Set before a fork, whose processes get it by value
सार्वजनिक:
    AstVarScope(File_पङ्क्ति* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_मान = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    शून्य cloneRelink() override {
        यदि (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    string name() अचर override VL_MT_STABLE { फर्काऊ scopep()->name() + "->" + varp()->name(); }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    AstVar* varp() अचर VL_MT_STABLE { फर्काऊ m_varp; }  // [After Link] Pointer to variable
    शून्य varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() अचर VL_MT_STABLE { फर्काऊ m_scopep; }  // Pointer to scope it's under
    शून्य scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() अचर { फर्काऊ m_trace; }
    शून्य trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() अचर { फर्काऊ m_optimizeLifePost; }
    शून्य optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_मान() अचर { फर्काऊ m_fork_By_मान; }
    शून्य fork_By_मान(bool flag) { m_fork_By_मान = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : सार्वजनिक Ast_गाँठो_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    अचर uint64_t m_index;  // Index of the element, with the array's lo() at 0
सार्वजनिक:
    AstConsArrayItem(File_पङ्क्ति* fl, uint64_t index, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_index{index} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    अचर वर्ण* broken() अचर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        फर्काऊ nullptr;
    }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        फर्काऊ m_index == VN_DBG_AS(samep, ConsArrayItem)->m_index;
    }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }  // By AstConsArray's map
    uint64_t index() अचर { फर्काऊ m_index; }
};
class AstConsAssocItem final : सार्वजनिक Ast_गाँठो_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
सार्वजनिक:
    AstConsAssocItem(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* keyp, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    अचर वर्ण* broken() अचर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        फर्काऊ nullptr;
    }
};
class AstConsDynArrayItem final : सार्वजनिक Ast_गाँठो_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
सार्वजनिक:
    AstConsDynArrayItem(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_पङ्क्ति* fl, AstConst* keyp, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    अचर वर्ण* broken() अचर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        फर्काऊ nullptr;
    }
};
class AstConsQueueItem final : सार्वजनिक Ast_गाँठो_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
सार्वजनिक:
    AstConsQueueItem(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    अचर वर्ण* broken() अचर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        फर्काऊ nullptr;
    }
};
class AstConsStructItem final : सार्वजनिक Ast_गाँठो_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
सार्वजनिक:
    AstConsStructItem(File_पङ्क्ति* fl, AstMemberDType* memberp, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    अचर वर्ण* broken() अचर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        फर्काऊ nullptr;
    }
    शून्य dump(std::ostream& str) अचर override;
    AstMemberDType* memberp() अचर { फर्काऊ m_memberp; }
};
class AstConsWildcardItem final : सार्वजनिक Ast_गाँठो_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
सार्वजनिक:
    AstConsWildcardItem(File_पङ्क्ति* fl, AstConst* keyp, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    अचर वर्ण* broken() अचर override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        फर्काऊ nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : सार्वजनिक Ast_गाँठो_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    पूर्णाङ्क m_offset;  // Offset column numbers to uniq-ify IFs
सार्वजनिक:
    AstCoverOtherDecl(File_पङ्क्ति* fl, अचर string& page, अचर string& comment,
                      अचर string& linescov, पूर्णाङ्क offset, अचर string& fsmVar = "",
                      अचर string& fsmFrom = "", अचर string& fsmTo = "",
                      अचर string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    पूर्णाङ्क offset() अचर { फर्काऊ m_offset; }
    पूर्णाङ्क size() अचर override { फर्काऊ 1; }
    अचर string& linescov() अचर { फर्काऊ m_linescov; }
    अचर string& fsmVar() अचर { फर्काऊ m_fsmVar; }
    अचर string& fsmFrom() अचर { फर्काऊ m_fsmFrom; }
    अचर string& fsmTo() अचर { फर्काऊ m_fsmTo; }
    अचर string& fsmTag() अचर { फर्काऊ m_fsmTag; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstCoverOtherDecl* अचर asamep = VN_DBG_AS(samep, CoverOtherDecl);
        फर्काऊ Super::same_गाँठो(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : सार्वजनिक Ast_गाँठो_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    अचर VNumRange m_range;  // Packed array range covering each toggle bit
सार्वजनिक:
    AstCoverToggleDecl(File_पङ्क्ति* fl, अचर string& page, अचर string& comment,
                       अचर VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    पूर्णाङ्क size() अचर override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        फर्काऊ 2 * m_range.elements();
    }
    अचर VNumRange& range() अचर { फर्काऊ m_range; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstCoverToggleDecl* अचर asamep = VN_DBG_AS(samep, CoverToggleDecl);
        फर्काऊ Super::same_गाँठो(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : सार्वजनिक Ast_गाँठो_FTask {
    // A function inside a module
सार्वजनिक:
    AstFunc(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* stmtp, Ast_गाँठो* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    Ast_गाँठो_FTask* cloneType(अचर string& name) override {
        फर्काऊ new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() अचर override { फर्काऊ "function"; }
};
class AstLet final : सार्वजनिक Ast_गाँठो_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
सार्वजनिक:
    AstLet(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_अभिव्यक्ति));
        फर्काऊ nullptr;
    }
    Ast_गाँठो_FTask* cloneType(अचर string& name) override { फर्काऊ new AstLet{fileline(), name}; }
    string verilogKwd() अचर override { फर्काऊ "let"; }
};
class AstProperty final : सार्वजनिक Ast_गाँठो_FTask {
    // A property inside a module
सार्वजनिक:
    AstProperty(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    Ast_गाँठो_FTask* cloneType(अचर string& name) override {
        फर्काऊ new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() अचर override { फर्काऊ "property"; }
};
class AstSequence final : सार्वजनिक Ast_गाँठो_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
सार्वजनिक:
    AstSequence(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    Ast_गाँठो_FTask* cloneType(अचर string& name) override {
        फर्काऊ new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() अचर override { फर्काऊ "sequence"; }
    bool isReferenced() अचर { फर्काऊ m_referenced; }
    शून्य isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : सार्वजनिक Ast_गाँठो_FTask {
    // A task inside a module
सार्वजनिक:
    AstTask(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_गाँठो_FTask* cloneType(अचर string& name) override {
        फर्काऊ new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() अचर override { फर्काऊ "task"; }
};

// === AstNodeFile ===
class AstCFile final : सार्वजनिक Ast_गाँठो_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
सार्वजनिक:
    AstCFile(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    uint64_t complexityScore() अचर { फर्काऊ m_complexityScore; }
    शून्य complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() अचर { फर्काऊ m_slow; }
    शून्य slow(bool flag) { m_slow = flag; }
    bool source() अचर { फर्काऊ m_source; }
    शून्य source(bool flag) { m_source = flag; }
    bool support() अचर { फर्काऊ m_support; }
    शून्य support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : सार्वजनिक Ast_गाँठो_File {
    // Verilog output file
    // Parents:  NETLIST
सार्वजनिक:
    AstVFile(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
};
class AstCoverCross final : सार्वजनिक Ast_गाँठो_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
सार्वजनिक:
    AstCoverCross(File_पङ्क्ति* fl, अचर string& name, AstCoverpointRef* itemsp,
                  Ast_गाँठो_अभिव्यक्ति* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstCoverpoint final : सार्वजनिक Ast_गाँठो_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
सार्वजनिक:
    AstCoverpoint(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_अभिव्यक्ति* exprp,
                  Ast_गाँठो_अभिव्यक्ति* iffp = nullptr, Ast_गाँठो* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};

// === AstNodeGen ===
class AstGenBlock final : सार्वजनिक Ast_गाँठो_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    अचर bool m_unnamed;  // Originally unnamed (name change does not affect this)
    अचर bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

सार्वजनिक:
    AstGenBlock(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    std::string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर std::string& name) override { m_name = name; }
    bool unnamed() अचर { फर्काऊ m_unnamed; }
    bool implied() अचर { फर्काऊ m_implied; }
    AstDefaultDisable* defaultDisablep() अचर { फर्काऊ m_defaultDisablep; }
    शून्य defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(अचर string& outer) अचर;
};
class AstGenCase final : सार्वजनिक Ast_गाँठो_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
सार्वजनिक:
    AstGenCase(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : सार्वजनिक Ast_गाँठो_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
सार्वजनिक:
    AstGenFor(File_पङ्क्ति* fl, Ast_गाँठो* initsp, Ast_गाँठो_अभिव्यक्ति* condp, Ast_गाँठो* incsp, Ast_गाँठो* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : सार्वजनिक Ast_गाँठो_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
सार्वजनिक:
    AstGenIf(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* condp, Ast_गाँठो* thensp, Ast_गाँठो* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : सार्वजनिक Ast_गाँठो_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_अङ्क;  // Declaration token number
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
    पूर्णाङ्क m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool full, अचर string& scope) अचर;
    string dtypeNameScope() अचर;  // Prefix of dtypeName(true) for the scope declaring the class

सार्वजनिक:
    AstClass(File_पङ्क्ति* fl, अचर string& name, अचर string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_अङ्क{fl->token_अङ्क()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() अचर override { फर्काऊ isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    bool timescaleMatters() अचर override { फर्काऊ false; }
    AstClassPackage* classOrPackagep() अचर VL_MT_STABLE { फर्काऊ m_classOrPackagep; }
    शून्य classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() अचर VL_MT_STABLE {
        फर्काऊ m_covergroupEnclosingClassp;
    }
    शून्य covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_गाँठो* membersp() अचर VL_MT_STABLE { फर्काऊ stmtsp(); }
    शून्य addMembersp(Ast_गाँठो* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() अचर { फर्काऊ m_hasRandVarsUpdate; }
    शून्य hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() अचर { फर्काऊ m_covergroup; }
    शून्य isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() अचर { फर्काऊ m_extended; }
    शून्य isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() अचर { फर्काऊ m_interfaceClass; }
    शून्य isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() अचर { फर्काऊ m_virtual; }
    शून्य isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() अचर { फर्काऊ m_needRNG; }
    शून्य needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() अचर { फर्काऊ m_useVirtualPublic; }
    शून्य useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    शून्य markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() अचर { फर्काऊ m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) अचर;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(अचर string& scope) अचर;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    शून्य dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() अचर { फर्काऊ !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_सन्देश() अचर override { फर्काऊ hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    स्थिर string embeddedCovergroupTypeName(अचर string& name) { फर्काऊ "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() अचर;
    // Covergroup options accessors
    पूर्णाङ्क cgAutoBinMax() अचर { फर्काऊ m_cgAutoBinMax; }
    शून्य cgAutoBinMax(पूर्णाङ्क मान) { m_cgAutoBinMax = मान; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    स्थिर bool isClassExtendedFrom(अचर AstClass* refClassp, अचर AstClass* baseClassp);
    uint32_t decl_Token_अङ्क() अचर override { फर्काऊ m_decl_Token_अङ्क; }
    शून्य decl_Token_अङ्क_Set_Min(uint32_t token_अङ्क) override {
        m_decl_Token_अङ्क = std::min(m_decl_Token_अङ्क, token_अङ्क);
    }
    शून्य baseOverride(अचर VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() अचर { फर्काऊ m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    स्थिर bool isCacheableChild(अचर Ast_गाँठो* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    ढाँचा <typename T_Callable>
    शून्य foreachMember(अचर T_Callable& f) {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_गाँठो*>::मान
                && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        यदि (अचर AstClassExtends* अचर cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        लागि (Ast_गाँठो* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (Ast_गाँठो::privateTypeTest<T_गाँठो>(stmtp)) f(this, static_cast<T_गाँठो*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    ढाँचा <typename T_Callable>
    bool existsMember(अचर T_Callable& p) अचर {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, अचर AstClass*, अचर T_गाँठो*>::मान
                && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        यदि (अचर AstClassExtends* अचर cextendsp = this->extendsp()) {
            यदि (cextendsp->classp()->existsMember(p)) फर्काऊ true;
        }
        लागि (Ast_गाँठो* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (Ast_गाँठो::privateTypeTest<T_गाँठो>(stmtp)) {
                यदि (p(this, static_cast<T_गाँठो*>(stmtp))) फर्काऊ true;
            }
        }
        फर्काऊ false;
    }
};
class AstClassPackage final : सार्वजनिक Ast_गाँठो_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
सार्वजनिक:
    AstClassPackage(File_पङ्क्ति* fl, अचर string& name, अचर string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() अचर override { फर्काऊ "classpackage"; }
    bool timescaleMatters() अचर override { फर्काऊ false; }
    AstClass* classp() अचर VL_MT_SAFE { फर्काऊ m_classp; }
    शून्य classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : सार्वजनिक Ast_गाँठो_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
सार्वजनिक:
    AstIface(File_पङ्क्ति* fl, अचर string& name, अचर string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string verilogKwd() अचर override { फर्काऊ "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() अचर override { फर्काऊ false; }
    bool hasVirtualRef() अचर { फर्काऊ m_hasVirtualRef; }
    शून्य setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) अचर;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    शून्य dtypeNameFreeze();
};
class AstModule final : सार्वजनिक Ast_गाँठो_Module {
    // A module declaration
    अचर bool m_isChecker;  // Module represents a checker
    अचर bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

सार्वजनिक:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_पङ्क्ति* fl, अचर string& name, अचर string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_पङ्क्ति* fl, अचर string& name, अचर string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_पङ्क्ति* fl, अचर string& name, अचर string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() अचर override {
        फर्काऊ m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() अचर override { फर्काऊ true; }
    bool isChecker() अचर { फर्काऊ m_isChecker; }
    bool isProgram() अचर { फर्काऊ m_isProgram; }
    bool hasGenericIface() अचर { फर्काऊ m_hasGenericIface; }
    शून्य hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};
class AstNotFoundModule final : सार्वजनिक Ast_गाँठो_Module {
    // A missing module declaration
सार्वजनिक:
    AstNotFoundModule(File_पङ्क्ति* fl, अचर string& name, अचर string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() अचर override { फर्काऊ "/*not-found-*/ module"; }
    bool timescaleMatters() अचर override { फर्काऊ false; }
};
class AstPackage final : सार्वजनिक Ast_गाँठो_Module {
    // A package declaration
सार्वजनिक:
    AstPackage(File_पङ्क्ति* fl, अचर string& name, अचर string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() अचर override { फर्काऊ "package"; }
    bool timescaleMatters() अचर override { फर्काऊ !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : सार्वजनिक Ast_गाँठो_Module {
    // A primitive declaration
सार्वजनिक:
    AstPrimitive(File_पङ्क्ति* fl, अचर string& name, अचर string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() अचर override { फर्काऊ "primitive"; }
    bool timescaleMatters() अचर override { फर्काऊ false; }
};

// === AstNodeProcedure ===
class AstAlways final : सार्वजनिक Ast_गाँठो_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    अचर VAlwaysKwd m_keyword;

सार्वजनिक:
    AstAlways(File_पङ्क्ति* fl, VAlwaysKwd keyword, Ast_Sen_रूख* sentreep, Ast_गाँठो* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit अन्तःस्थ AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    VAlwaysKwd keyword() अचर { फर्काऊ m_keyword; }
};
class AstAlwaysObserved final : सार्वजनिक Ast_गाँठो_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

सार्वजनिक:
    AstAlwaysObserved(File_पङ्क्ति* fl, Ast_Sen_रूख* sentreep, Ast_गाँठो* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : सार्वजनिक Ast_गाँठो_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

सार्वजनिक:
    explicit AstAlwaysPost(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : सार्वजनिक Ast_गाँठो_Procedure {
    // Like always but Postponed scheduling region

सार्वजनिक:
    AstAlwaysPostponed(File_पङ्क्ति* fl, Ast_गाँठो* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : सार्वजनिक Ast_गाँठो_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

सार्वजनिक:
    explicit AstAlwaysPre(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : सार्वजनिक Ast_गाँठो_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

सार्वजनिक:
    AstAlwaysReactive(File_पङ्क्ति* fl, Ast_Sen_रूख* sentreep, Ast_गाँठो* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : सार्वजनिक Ast_गाँठो_Procedure {
सार्वजनिक:
    AstFinal(File_पङ्क्ति* fl, Ast_गाँठो* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : सार्वजनिक Ast_गाँठो_Procedure {
सार्वजनिक:
    AstInitial(File_पङ्क्ति* fl, Ast_गाँठो* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : सार्वजनिक Ast_गाँठो_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
सार्वजनिक:
    AstInitialAutomatic(File_पङ्क्ति* fl, Ast_गाँठो* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : सार्वजनिक Ast_गाँठो_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
सार्वजनिक:
    AstInitialStatic(File_पङ्क्ति* fl, Ast_गाँठो* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : सार्वजनिक Ast_गाँठो_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
सार्वजनिक:
    AstBracketRange(File_पङ्क्ति* fl, Ast_गाँठो* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    आभास string emitC() { V3ERROR_NA_RETURN(""); }
    आभास string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ false; }
};
class AstRange final : सार्वजनिक Ast_गाँठो_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    अचर bool m_fromBracket = false;  // From C-style '[x]' declaration
सार्वजनिक:
    AstRange(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* leftp, Ast_गाँठो_अभिव्यक्ति* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    अन्तःस्थ AstRange(File_पङ्क्ति* fl, पूर्णाङ्क बायाँ, पूर्णाङ्क दायाँ);
    अन्तःस्थ AstRange(File_पङ्क्ति* fl, अचर VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    अन्तःस्थ पूर्णाङ्क बायाँ_Const() अचर VL_MT_STABLE;
    अन्तःस्थ पूर्णाङ्क दायाँ_Const() अचर VL_MT_STABLE;
    पूर्णाङ्क hiConst() अचर VL_MT_STABLE {
        अचर पूर्णाङ्क l = बायाँ_Const();
        अचर पूर्णाङ्क r = दायाँ_Const();
        फर्काऊ l > r ? l : r;
    }
    पूर्णाङ्क loConst() अचर VL_MT_STABLE {
        अचर पूर्णाङ्क l = बायाँ_Const();
        अचर पूर्णाङ्क r = दायाँ_Const();
        फर्काऊ l > r ? r : l;
    }
    पूर्णाङ्क elementsConst() अचर VL_MT_STABLE { फर्काऊ hiConst() - loConst() + 1; }
    bool ascending() अचर { फर्काऊ बायाँ_Const() < दायाँ_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_बायाँ_To_दायाँ() अचर {
        फर्काऊ VNumRange{बायाँ_Const(), दायाँ_Const()}.seq_बायाँ_To_दायाँ();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_दायाँ_To_बायाँ() अचर {
        फर्काऊ VNumRange{बायाँ_Const(), दायाँ_Const()}.seq_दायाँ_To_बायाँ();
    }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    आभास string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstRange* अचर asamep = VN_DBG_AS(samep, Range);
        फर्काऊ fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() अचर { फर्काऊ m_fromBracket; }
};
class AstUnsizedRange final : सार्वजनिक Ast_गाँठो_Range {
    // Unsized range specification, for open arrays
सार्वजनिक:
    explicit AstUnsizedRange(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    आभास string emitC() { V3ERROR_NA_RETURN(""); }
    आभास string emitVerilog() { फर्काऊ "[]"; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};
class AstWildcardRange final : सार्वजनिक Ast_गाँठो_Range {
    // Wildcard range specification, for wildcard index type associative arrays
सार्वजनिक:
    explicit AstWildcardRange(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    आभास string emitC() { V3ERROR_NA_RETURN(""); }
    आभास string emitVerilog() { फर्काऊ "[*]"; }
    bool same_गाँठो(अचर Ast_गाँठो* /*samep*/) अचर override { फर्काऊ true; }
};

#endif  // Guard
